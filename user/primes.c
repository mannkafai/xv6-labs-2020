#include "kernel/types.h"
#include "user/user.h"

static void prime(int *fds_r);

void prime(int *fds_r)
{
    int fds_w[2];
    int pid;
    int num;
    int n;
    close(fds_r[1]);
    if ((n = read(fds_r[0], &num, sizeof(num))) != sizeof(num))
    {
        close(fds_r[0]);
        return;
    }
    printf("prime %d\n", num);

    if (pipe(fds_w) < 0)
    {
        printf("create pipe failed...\n");
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
        prime(fds_w);
    }
    else
    {
        close(fds_w[0]);
        int num1;
        while ((n = read(fds_r[0], &num1, sizeof(num1))) == sizeof(num1))
        {
            if (num1 % num != 0)
                write(fds_w[1], &num1, sizeof(num1));
        }
        close(fds_r[0]);
        close(fds_w[1]);
        wait(0);
        exit(0);
    }
}

int main(int argc, char const *argv[])
{
    int fd1[2];
    int pid;
    if (pipe(fd1) < 0)
    {
        printf("create pipe1 failed...\n");
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
        prime(fd1);
        exit(0);
        // close(fd1[1]);
        // int num;
        // int n;
        // while ((n = read(fd1[0], &num, sizeof(num))) == 4)
        // {
        //     printf("recv %d\n", num);
        // }
        // close(fd1[0]);
        // exit(0);
    }
    else
    {
        close(fd1[0]);
        for (int i = 2; i < 35; i++)
        {
            write(fd1[1], &i, sizeof(i));
        }
        close(fd1[1]);
        wait(0);
        exit(0);
    }
}