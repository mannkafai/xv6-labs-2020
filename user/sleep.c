#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char const *argv[])
{
    int secondes = 0;
    if (argc < 2)
    {
        fprintf(2, "Usage: sleep secondes...\n");
        exit(1);
    }
    secondes = atoi(argv[1]);
    sleep(secondes);
    exit(0);
}
