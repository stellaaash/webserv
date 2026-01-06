#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

int main() {
    struct addrinfo  hints;
    struct addrinfo* results;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int status;
    if ((status = getaddrinfo("tryhackme.org", NULL, &hints, &results)) != 0) {
        std::clog << "[!] - getaddrinfo failed with message " << gai_strerror(status) << std::endl;
        ;
        exit(1);
    }

    char ipstr[INET_ADDRSTRLEN];
    for (struct addrinfo* a = results; a != NULL; a = a->ai_next) {
        struct sockaddr_in* ipv4 = (struct sockaddr_in*)a->ai_addr;
        inet_ntop(results->ai_family, &ipv4->sin_addr, ipstr, sizeof(ipstr));
        std::clog << ipstr << std::endl;
    }
    freeaddrinfo(results);
    return 0;
}
