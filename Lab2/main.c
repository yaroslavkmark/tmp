#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#define MAX_THREADS 10
#define BUF_SIZE 1024
#define RES_SIZE 256

typedef struct {
    int mod;
    int N;
    int fd;
    char* pattern;
    int pattern_len;
    unsigned int** res;
    int* res_size;
    int* counter;
    pthread_mutex_t* mutex_res;
    pthread_mutex_t* mutex_file;
} Data;

int uint_cmp(const void* a1, const void* a2) {
    return (*(unsigned int*)a1 > *(int*)a2) - (*(unsigned int*)a1 < *(int*)a2);
}

int print_unsigned_int(const unsigned int num) {
    char buf[16];
    char res[32];
    int n = 0;
    int x = num;

    if (x == 0) {
        write(STDOUT_FILENO, "0\n", 2);
        return 0;
    }

    while (x) {
        buf[n] = (x % 10) + '0';
        x = x / 10;
        n++;
    }

    for (int i = 0; i < n; i++) {
        res[i] = buf[n - i - 1];
    }

    res[n] = '\n';
    write(STDOUT_FILENO, res, (n + 1));

    return 0;
}

int str_to_int(const char* string) {
    int num = 0;
    int i = 0;
    char c = string[i];
    while ((c >= '0') && (c <= '9')) {
        if ((num * 10 + (c - '0')) > 100)
            return -1;

        num = num * 10 + (c - '0');
        i++;
        c = string[i];
    }

    if ((c == 0) && (i != 0))
        return num;
    else
        return -1;

}

void* find_inclusions_by_mod(void* data) {
    Data* d = (Data*)data;
    int symb = d->mod;
    int i;
    int bytes_read = 0;
    char* buf = malloc(BUF_SIZE * sizeof(char));

    if (!buf) {
        write(STDOUT_FILENO, "fail to alloc\n", 15);
        return NULL;
    }

    int buf_offset = 0;

    pthread_mutex_lock(d->mutex_file);
    lseek(d->fd, symb, SEEK_SET);
    bytes_read = read(d->fd, buf, BUF_SIZE);
    pthread_mutex_unlock(d->mutex_file);

    int my_res_size = RES_SIZE / d->N;
    int* my_res = malloc(sizeof(int) * my_res_size);
    int my_counter = 0;

    while (1) {
        if ((bytes_read - buf_offset) < d->pattern_len) {
            if (bytes_read < BUF_SIZE) break;

            pthread_mutex_lock(d->mutex_file);
            lseek(d->fd, symb, SEEK_SET);
            bytes_read = read(d->fd, buf, BUF_SIZE);
            buf_offset = 0;
            pthread_mutex_unlock(d->mutex_file);

            if (bytes_read < d->pattern_len) break;
        }

        i = 0;
        while ((d->pattern)[i] && (buf[buf_offset + i] == (d->pattern)[i])) {
            i++;
        }

        if (!(d->pattern)[i]) {
            my_res[my_counter] = symb;
            my_counter ++;
            if (my_counter >= my_res_size) {
                my_res_size *= 2;
                my_res = (unsigned int*)realloc(my_res, *(d->res_size) * sizeof(unsigned int));
            }
        }
        symb += d->N;
        buf_offset += d->N;
    }

    pthread_mutex_lock(d->mutex_res);
    int index = *(d->counter);
    if (index + my_counter >= *(d->res_size)) {
        *(d->res_size) =  (*(d->res_size) + my_counter) * 2;
        *(d->res) = (unsigned int*)realloc(*(d->res), *(d->res_size) * sizeof(unsigned int));
    }
    (*(d->counter)) = (*(d->counter)) + my_counter;
    pthread_mutex_unlock(d->mutex_res);

    for (int i = 0; i < my_counter; i++) {
        (*(d->res))[index + i] = my_res[i];
    }

    free(buf);
    return NULL;
}

int main(int argc, char* argv[]) {

    if (argc != 4) {
        char* msg = "enter: ./main.out <path to file> <number of threads> <pattern>\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        return -1;
    }

    int N = str_to_int(argv[2]);
    if((N == -1) || (N > MAX_THREADS)) {
        char* msg = "invalid number of threads\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        return -1;
    }

    pthread_t* threads = (pthread_t*)malloc(N * sizeof(pthread_t));
    Data* threads_data = (Data*)malloc(N * sizeof(Data));
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        char* msg = "fail to open file\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        return 1;
    }
    int pattern_len = strlen(argv[3]);
    char* pattern = (char*)malloc((pattern_len + 1) * sizeof(char));
    strcpy(pattern, argv[3]);
    int res_size = RES_SIZE;
    unsigned int* results = (unsigned int*)malloc(res_size * sizeof(unsigned int));
    int counter = 0;

    pthread_mutex_t mutex_res;
    pthread_mutex_init(&mutex_res, NULL);
    pthread_mutex_t mutex_file;
    pthread_mutex_init(&mutex_file, NULL);

    for (int i = 0; i < N; i++) {
        threads_data[i].fd = fd;
        threads_data[i].mod = i;
        threads_data[i].N = N;
        threads_data[i].pattern = pattern;
        threads_data[i].pattern_len = pattern_len;
        threads_data[i].res = &results;
        threads_data[i].res_size = &res_size;
        threads_data[i].counter = &counter;
        threads_data[i].mutex_res = &mutex_res;
        threads_data[i].mutex_file = &mutex_file;
    }

    for (int i = 0; i < N; i++) {
        pthread_create(threads + i, NULL, &find_inclusions_by_mod, threads_data + i);
    }

    for (int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
    }

    qsort(results, counter, sizeof(int), &uint_cmp);

    for (int i = 0; i < counter; i++) {
        print_unsigned_int(results[i]);
    }

    if (counter < 1){
        write(STDOUT_FILENO, "-1\n", 3);
    }

    free(threads);
    free(threads_data);
    free(pattern);
    free(results);

    close(fd);

    pthread_mutex_destroy(&mutex_res);
    pthread_mutex_destroy(&mutex_file);
    return 0;
}