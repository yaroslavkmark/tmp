#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#define MAX_BLOCK_SIZES 4

typedef struct {
    size_t block_size;     // Размер одного блока
    size_t block_count;    // Количество блоков
    uint8_t *bitmap;       // Битовая карта для управления блоками
    uint8_t *memory_start; // Указатель на начало памяти для этого пула
} MemoryPool;

typedef struct {
    void *memory;                      // Указатель на переданную память
    size_t memory_size;                // Размер памяти
    MemoryPool pools[MAX_BLOCK_SIZES]; // Пулы для разных размеров блоков
} Allocator;

static void write_message(const char *message)
{
    write(STDERR_FILENO, message, strlen(message));
}

EXPORT Allocator *allocator_create(void *mem, size_t mem_size)
{
    Allocator *allocator = (Allocator *)malloc(sizeof(Allocator));
    if (!allocator)
        return NULL;

    allocator->memory = mem;
    allocator->memory_size = mem_size;

    size_t block_sizes[MAX_BLOCK_SIZES] = {16, 32, 64, 128};

    uint8_t *current_memory = mem;
    for (int i = 0; i < MAX_BLOCK_SIZES; i++)
    {
        size_t block_size = block_sizes[i];
        size_t block_count = mem_size / block_size / MAX_BLOCK_SIZES;

        allocator->pools[i].block_size = block_size;
        allocator->pools[i].block_count = block_count;
        allocator->pools[i].bitmap = current_memory;
        allocator->pools[i].memory_start = current_memory + block_count / 8;

        memset(allocator->pools[i].bitmap, 0, block_count / 8);

        current_memory += block_count / 8 + block_count * block_size;
    }

    return allocator;
}

EXPORT void *allocator_alloc(Allocator *allocator, size_t size)
{
    for (int i = 0; i < MAX_BLOCK_SIZES; i++)
    {
        MemoryPool *pool = &allocator->pools[i];

        if (size > pool->block_size)
            continue;

        for (size_t j = 0; j < pool->block_count; j++)
        {
            size_t byte_index = j / 8;
            size_t bit_index = j % 8;

            if (!(pool->bitmap[byte_index] & (1 << bit_index)))
            {
                pool->bitmap[byte_index] |= (1 << bit_index);
                return pool->memory_start + j * pool->block_size;
            }
        }
    }

    return NULL;
}

EXPORT void allocator_free(Allocator *allocator, void *ptr)
{
    for (int i = 0; i < MAX_BLOCK_SIZES; i++)
    {
        MemoryPool *pool = &allocator->pools[i];

        if (ptr >= (void *)pool->memory_start && ptr < (void *)(pool->memory_start + pool->block_count * pool->block_size))
        {
            size_t offset = (uint8_t *)ptr - pool->memory_start;
            size_t index = offset / pool->block_size;
            size_t byte_index = index / 8;
            size_t bit_index = index % 8;

            pool->bitmap[byte_index] &= ~(1 << bit_index);
            return;
        }
    }
}

EXPORT void allocator_destroy(Allocator *allocator)
{
    if (allocator)
    {
        if (munmap(allocator->memory, allocator->memory_size) == -1)
        {
            const char error_msg[] = "Error: munmap failed\n";
            write_message(error_msg);
            exit(EXIT_FAILURE);
        }
        free(allocator);
    }
}