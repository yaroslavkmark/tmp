#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>


int main(int argc, char* argv[]) {
    int code;
    int input;
    input = open(argv[1], O_RDONLY);

    if (input < 0)
        write(STDERR_FILENO, &input, sizeof(int));

    int pipefd[2];
    pipe(pipefd);

    pid_t child;
    child = fork();

    if (child == 0) {
        dup2(input, STDIN_FILENO);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);

        if ((code = execve("child.out", argv, NULL)) != 0)
            write(STDERR_FILENO, &code, sizeof(int));

    }
    else {
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[1]);
        wait(NULL);

        char c;
        while (read(STDIN_FILENO, &c, sizeof(char)) != 0) {
            write(STDOUT_FILENO, &c, sizeof(char));
        }
    }

    close(STDOUT_FILENO);
    close(STDIN_FILENO);

    return 0;
}