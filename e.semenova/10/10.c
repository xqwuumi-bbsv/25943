#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    pid_t pid, w;
    int status;

    if (argc < 2) {
        fprintf(stderr, "Использование: %s команда [аргументы...]\n", argv[0]);
        return 1;
    }

    fflush(stdout);

    pid = fork();
    if (pid == -1) {
        perror("Ошибка fork");
        return 1;
    }

    if (pid == 0) {
        execvp(argv[1], &argv[1]);
        perror(argv[1]);
        _exit(127);
    }

    do {
        w = waitpid(pid, &status, 0);
    } while (w == -1 && errno == EINTR);

    if (w == -1) {
        perror("Ошибка waitpid");
        return 1;
    }

    if (WIFEXITED(status))
        printf("Код завершения: %d\n", WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        printf("Команда убита сигналом %d\n", WTERMSIG(status));

    return 0;
}
