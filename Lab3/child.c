#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <errno.h>

void write_error(const char *msg) {
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
}

int read_double(char* inp, double* number) {
    double int_part = 0;
    double frac_part = 0;
    double sign = 1;
    if (*inp == '-') {
        sign = -1;
        ++inp;
    }

    while (*inp != '.' && *inp != ',' && *inp != 0) {
        if (*inp < '0' || *inp > '9') {

            return 3;
        }
        int_part *= 10;
        int_part += (*inp - '0');
        ++inp;
    }
    if (*inp == 0) {
        *number = sign * int_part;
        return 0;
    }
    while (*inp != 0) {++inp;}
    --inp;
    while (*inp != '.' && *inp != ',') {
        if (*inp < '0' || *inp > '9' || *inp == 0) {
            return 3;
        }
        frac_part += (*inp - '0');
        frac_part /= 10;
        --inp;
    }
    *number = sign * (int_part + frac_part);
    return 0;
}

int read_until_space(int fd, char *targ, char *c) {
    char *ptr = targ;
    while (1) {
        if (read(fd, c, sizeof(char)) == 0) {
            *ptr = 0;
            *c = '\n';
            return 1;
        }
        if (*c == ' ' || *c == '\t' || *c == '\n') {
            *ptr = 0;
            return 0;
        }
        *ptr = *c;
        ++ptr;
    }
}

int generate_keys(int *key1, int *key2) {
    if ((*key1 = ftok("main.c", 0)) == -1) {
        write_error("ftok error\n");
        return -1;
    }
    if ((*key2 = ftok("child.c", 0)) == -1) {
        write_error("ftok error\n");
        return -1;
    }
    return 0;
}

int shmem_create(int *shmid, int key, size_t size) {
    if ((*shmid = shmget(key, size, IPC_CREAT|IPC_EXCL|0666)) == -1) {
        if (errno != EEXIST) {
            write_error("shmget error\n");
            return -1;
        }

        if ((*shmid = shmget(key, size, 0666)) == -1) {
            write_error("shmget error\n");
            return -1;
        }
        if (shmctl(*shmid, 0, NULL) < 0) {
            write_error("shmctl error\n");
            return -1;
        }
        if ((*shmid = shmget(key, size, IPC_CREAT|IPC_EXCL|0666)) == -1) {
            write_error("shmget error\n");
            return -1;
        }
    }
    return 0;
}


int main(int argc, char* argv[]) {

    int output_size_key;
    int output_key;
    if (generate_keys(&output_size_key, &output_key)) {
        return -1;
    }

    int output_size_shmid;
    if (shmem_create(&output_size_shmid, output_size_key, sizeof(size_t))) {
        return -1;
    }
    size_t *output_size;
    if ((output_size = (size_t *)shmat(output_size_shmid, NULL, 0)) == NULL) {
        write_error("shmat error\n");
        return -1;
    }
    *output_size = 0;

    char c = ' ';

    char word[64];
    char *ptr = word;

    char *output_buff;
    size_t len = 256;
    if ((output_buff = (char *)malloc(len * sizeof(char))) == NULL) {
        write_error("malloc error\n");
        return -1;
    }

    double num;
    double sum = 0;
    int f = 1;
    char *w = output_buff;

    while (f) {
        if (read_until_space(STDIN_FILENO, word, &c)) {
            f = 0;
        }
        if (strlen(word) == 0) {
            break;
        }
        if ((read_double(word, &num)) != 0) {
            write_error("error reading double");
            return -1;
        }

        sum += num;
        ptr = word;

        if (c == '\n') {

            sprintf(word, "%g", sum);
            sum = 0;

            strcpy(w, word);

            w[strlen(w)] = '\n';
            w[strlen(w)] = 0;
            w = &(w[strlen(w)]);

            (*output_size)++;

            if (len - strlen(output_buff) < 64) {
                len *= 2;
                output_buff = (char *)realloc(output_buff, len * sizeof(char));
                if (output_buff == NULL) {
                    write_error("realloc error\n");
                    return -1;
                }
            }
        }
    }
    int output_shmid;
    if (shmem_create(&output_shmid, output_key, *output_size * sizeof(char))) {
        return -1;
    }
    char *output;
    if ((output = (char *)shmat(output_shmid, NULL, 0)) == NULL) {
        write_error("shmgat error\n");
        return -1;
    }

    strcpy(output, output_buff);

    if (shmdt(output) != 0) {
        write_error("shmdt error\n");
        return -1;
    }
    if (shmdt(output_size) != 0) {
        write_error("shmdt error\n");
        return -1;
    }
    free(output_buff);
    close(STDIN_FILENO);

    return 0;
}