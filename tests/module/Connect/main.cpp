#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>

#include <connect.hpp>

int main() {
    printf("Test connect starts\n");
    try {
        class connect connect_client;
        connect_client.run();
    } catch (const std::exception &) // Consider using a custom exception type for intentional
    { // throws. A good idea might be a `return_exception`.
        return EXIT_FAILURE;
    }
    return 0;
}
