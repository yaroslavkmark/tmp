#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#define MEMORY_SIZE (1024 * 1024)

typedef struct Allocator {
    void* memory_start;
    size_t memory_size;
    void* free_list;
} Allocator;

Allocator* allocator_create(void* const memory, const size_t size);
void allocator_destroy(Allocator* const allocator);
void* allocator_alloc(Allocator* const allocator, const size_t size);
void allocator_free(Allocator* const allocator, void* const memory);

void my_write(const char* message) {
    write(STDERR_FILENO, message, strlen(message));
}


void my_write_hex(void* ptr) {
    char buffer[64];
    unsigned long addr = (unsigned long)ptr;
    size_t i;
    for (i = 0; i < sizeof(buffer) - 1 && addr; ++i) {
        unsigned char byte = addr & 0xF;
        buffer[i] = (byte < 10) ? '0' + byte : 'a' + (byte - 10);
        addr >>= 4;
    }
    buffer[i] = '\0';
    my_write(buffer);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        my_write("Usage: <path_to_allocator_library>\n");
        return 1;
    }

    void* handle = dlopen(argv[1], RTLD_LAZY);
    if (!handle) {
        my_write("Failed to load library: ");
        my_write(dlerror());
        my_write("\n");
        return 1;
    }

    Allocator* (*allocator_create)(void*, size_t) = dlsym(handle, "allocator_create");
    void (*allocator_destroy)(Allocator*) = dlsym(handle, "allocator_destroy");
    void* (*allocator_alloc)(Allocator*, size_t) = dlsym(handle, "allocator_alloc");
    void (*allocator_free)(Allocator*, void*) = dlsym(handle, "allocator_free");

    char* error;
    if ((error = dlerror()) != NULL) {
        my_write("Error resolving symbols: ");
        my_write(error);
        my_write("\n");
        dlclose(handle);
        return 1;
    }

    void* memory = mmap(NULL, MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED) {
        my_write("mmap failed\n");
        dlclose(handle);
        return 1;
    }

    Allocator* allocator = allocator_create(memory, MEMORY_SIZE);
    if (!allocator) {
        my_write("Failed to create allocator\n");
        munmap(memory, MEMORY_SIZE);
        dlclose(handle);
        return 1;
    }

    void* block = allocator_alloc(allocator, 128);
    if (block) {
        my_write("Allocated block at ");
        my_write_hex(block);
        my_write("\n");
    } else {
        my_write("Failed to allocate block\n");
    }

    allocator_free(allocator, block);
    my_write("Freed block\n");

    allocator_destroy(allocator);
    munmap(memory, MEMORY_SIZE);
    dlclose(handle);

    return 0;
}