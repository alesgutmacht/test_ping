#include "ping.h"

#include <stdio.h>

int main(int argc, char *argv[])
{
    char *addr_ip = "";

    if (argc > 1)
    {
        addr_ip = argv[1];
    }

    return run_ping(addr_ip);
}
