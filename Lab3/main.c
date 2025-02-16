#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/ipc.h>



void write_error(const char *msg) {
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
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

int shmem_get(int *shmid, int key, size_t size) {
    if ((*shmid = shmget(key, size, SHM_RDONLY)) == -1) {
        write_error("shmget error\n");
        return -1;
    }
    return 0;
}


int main(int argc, char* argv[]) {
    int input;
    input = open(argv[1], O_RDONLY);
    if (input < 0) {
        write_error("file error\n");
        return -1;
    }

    int output_size_key;
    int output_key;
    if (generate_keys(&output_size_key, &output_key)) {
        return -1;
    }

    pid_t child;
    child = fork();

    switch (child)
    {
        case -1:
            write_error("fork error\n");
            return -1;

        case 0:
            dup2(input, STDIN_FILENO);

            if (execve("child.out", argv, NULL) != 0) {
                write_error("execve error\n");
                return -1;
            }
            break;

        default:
            waitpid(child, NULL, 0);
    }

    int output_size_shmid;
    if (shmem_get(&output_size_shmid, output_size_key, sizeof(size_t))) {
        return -1;
    }
    size_t *output_size;
    if ((output_size = (size_t *)shmat(output_size_shmid, NULL, 0)) == NULL) {
        write_error("shmat error\n");
        return -1;
    }

    int output_shmid;
    if (shmem_get(&output_shmid, output_key, *output_size * sizeof(char))) {
        return -1;
    }
    char *output;
    if ((output = (char *)shmat(output_shmid, NULL, 0)) == NULL) {
        write_error("shmat error\n");
        return -1;
    }

    write(STDOUT_FILENO, output, (strlen(output)) * sizeof(char));

    if (shmctl(output_size_shmid, 0, NULL) == -1) {
        write_error("shmctl error\n");
        return -1;
    }

    if (shmctl(output_shmid, 0, NULL) == -1) {
        write_error("shmctl error\n");
        return -1;
    }

    close(input);

    return 0;
}