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

    if (argc != 2) {
        fprintf(stderr, "Использование: %s файл\n", argv[0]);
        return 1;
    }

    fflush(stdout);

    pid = fork();
    if (pid == -1) {
        perror("Ошибка fork");
        return 1;
    }

    if (pid == 0) {
        execlp("cat", "cat", argv[1], (char *)NULL);
        perror("Ошибка запуска cat");
        _exit(127);
    }

    printf("Родитель (pid %ld): запущен потомок %ld\n", (long)getpid(), (long)pid);
    printf("Родитель: жду завершения потомка...\n");
    fflush(stdout);

    do {
        w = waitpid(pid, &status, 0);
    } while (w == -1 && errno == EINTR);

    if (w == -1) {
        perror("Ошибка waitpid");
        return 1;
    }

    if (WIFEXITED(status))
        printf("Родитель: потомок завершился с кодом %d\n", WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        printf("Родитель: потомок убит сигналом %d\n", WTERMSIG(status));

    printf("Родитель: это последняя строка, она выведена после завершения потомка\n");

    return 0;
}
