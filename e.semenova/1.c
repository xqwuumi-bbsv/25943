#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <ulimit.h>
#include <limits.h>

extern char **environ;

int main(int argc, char *argv[]) {
    int opt;

    if (argc == 1) {
        printf("Program started without options. Usage: %s [-i] [-s] [-p] [-u] [-U num] [-c] [-C size] [-d] [-v] [-V name=value]\n", argv[0]);
        return 0;
    }

    while ((opt = getopt(argc, argv, "ispucdvU:C:V:")) != -1) {
        switch (opt) {
            case 'i':
                printf("Real UID: %d, Effective UID: %d\n", getuid(), geteuid());
                printf("Real GID: %d, Effective GID: %d\n", getgid(), getegid());
                break;

            case 's':
                if (setpgid(0, 0) == 0) {
                    printf("Process successfully became group leader.\n");
                } else {
                    perror("setpgid error");
                }
                break;

            case 'p':
                printf("PID: %d\n", getpid());
                printf("PPID: %d\n", getppid());
                printf("PGID: %d\n", getpgid(0));
                break;

            case 'u': {
                long ulim = ulimit(UL_GETFSIZE, 0);
                if (ulim == -1) {
                    perror("ulimit get error");
                } else {
                    printf("Current ulimit (512-byte blocks): %ld\n", ulim);
                }
                break;
            }

            case 'U': {
                char *endptr;
                long new_ulim = strtol(optarg, &endptr, 10); 
                
                if (*endptr != '\0') {
                    fprintf(stderr, "Invalid value for U: must be a correct number\n");
                } else if (ulimit(UL_SETFSIZE, new_ulim) == -1) {
                    perror("ulimit set error (-U)");
                } else {
                    printf("New ulimit set successfully: %ld\n", new_ulim);
                }
                break;
            }

            case 'c': {
                struct rlimit rl;
                if (getrlimit(RLIMIT_CORE, &rl) == 0) {
                    printf("Core file size limit (bytes): %lu\n", (unsigned long)rl.rlim_cur);
                } else {
                    perror("getrlimit RLIMIT_CORE error");
                }
                break;
            }

            case 'C': {
                struct rlimit rl;
                char *endptr;
                long new_core_size = strtol(optarg, &endptr, 10);

                if (*endptr != '\0') {
                    fprintf(stderr, "Invalid value for C: must be a correct number\n");
                } else if (getrlimit(RLIMIT_CORE, &rl) == 0) {
                    rl.rlim_cur = new_core_size;
                    if (setrlimit(RLIMIT_CORE, &rl) == 0) {
                        printf("New core file size limit set: %ld\n", new_core_size);
                    } else {
                        perror("setrlimit RLIMIT_CORE error");
                    }
                } else {
                    perror("getrlimit error before setrlimit");
                }
                break;
            }

            case 'd': {
                char cwd[PATH_MAX];
                if (getcwd(cwd, sizeof(cwd)) != NULL) {
                    printf("Current working directory: %s\n", cwd);
                } else {
                    perror("getcwd error");
                }
                break;
            }

            case 'v':
                printf("--- Environment variables ---\n");
                for (char **env = environ; *env != NULL; env++) {
                    printf("%s\n", *env);
                }
                printf("-----------------------------\n");
                break;

            case 'V':
                if (putenv(optarg) == 0) {
                    printf("Environment variable set/modified: %s\n", optarg);
                } else {
                    perror("putenv error");
                }
                break;

            case '?':
                printf("Usage: %s [-i] [-s] [-p] [-u] [-U num] [-c] [-C size] [-d] [-v] [-V name=value]\n", argv[0]);
                break;

            default:
                break;
        }
    }

    return 0;
}
