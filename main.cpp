#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

int main() {
    struct addrinfo  hints;
    struct addrinfo* results;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int status;
    if ((status = getaddrinfo(NULL, "8080", &hints, &results)) != 0) {
        std::clog << "[!] - getaddrinfo failed with message " << gai_strerror(status) << std::endl;
        ;
        exit(1);
    }

    // Print all addresses available
    char ipstr[INET_ADDRSTRLEN];
    for (struct addrinfo* a = results; a != NULL; a = a->ai_next) {
        struct sockaddr_in* ipv4 = (struct sockaddr_in*)a->ai_addr;
        inet_ntop(results->ai_family, &ipv4->sin_addr, ipstr, sizeof(ipstr));
        std::clog << ipstr << std::endl;
    }

    // Create the main socket
    int s = socket(results->ai_family, results->ai_socktype, results->ai_protocol);
    if (s == -1) {
        perror("[!] - socket failed");
        exit(1);
    }

    // Bind it to port 80
    if (bind(s, results->ai_addr, results->ai_addrlen) != 0) {
        perror("[!] - bind failed");
        exit(1);
    }

    // Listen on port 80 using the newly-bound socket
    if (listen(s, 10) != 0) {
        perror("[!] - listen failed");
        exit(1);
    }

    // Accept an incoming connection
    accept(s, results->ai_addr, &results->ai_addrlen);

    freeaddrinfo(results);
    return 0;
}
