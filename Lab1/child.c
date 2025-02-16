#include <unistd.h>
#include <stdio.h>

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


int main(int argc, char* argv[]) {
    char c;
    char word[64];
    char *ptr = word;
    double num;
    double sum = 0;
    int code;

    while (read(STDIN_FILENO, &c, sizeof(char)) != 0) {
        if (c != ' ' && c != '\n') {
            *ptr = c;
            ++ptr;
        } else {
            *ptr = 0;
            ptr = word;

            if ((code = read_double(ptr, &num)) != 0)
                write(STDERR_FILENO, &code, sizeof(int));

            sum += num;
            ptr = word;

            sprintf(word, "%g", sum);
            if (c == '\n') {
                char* dig = word;
                while (*dig != 0) {
                    write(STDOUT_FILENO, dig, sizeof(char));
                    ++dig;
                }
                write(STDOUT_FILENO, "\n", sizeof(char));
                sum = 0;
            }
        }
    }

    return 0;
}