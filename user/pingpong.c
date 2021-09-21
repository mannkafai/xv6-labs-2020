#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char const *argv[])
{
    int fd1[2];
    int fd2[2];
    int pid;
    if (pipe(fd1) < 0)
    {
        printf("create pipe1 failed...\n");
        exit(1);
    }
    if (pipe(fd2) < 0)
    {
        printf("create pipe2 failed...\n");
        exit(1);
    }
    pid = fork();
    if (pid < 0)
    {
        printf("fork failed\n");
        exit(1);
    }
    if (pid == 0)
    {
        char buf[64];
        int n;
        do
        {
            n = read(fd1[0], buf, 64);
            if (n < 1)
                sleep(1);
        } while (n < 1);
        printf("%d: received ping\n", getpid());
        write(fd2[1], "a", 1);
        exit(0);
    }
    else
    {
        char buf[64];
        int n;
        n = write(fd1[1], "a", 1);
        do
        {
            n = read(fd2[0], buf, 64);
            if (n < 1)
                sleep(1);
        } while (n < 1);
        printf("%d: received pong\n", getpid());
        exit(0);
    }
}