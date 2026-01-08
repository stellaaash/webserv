#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

int main() {
    struct addrinfo  hints;
    struct addrinfo* results;

    // Prepare the struct "hints" that will give info about how what kind of address info we want
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // Get the address for listening on localhost on port 8080
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

    // Create the main socket (that will listen for incoming connections)
    int s = socket(results->ai_family, results->ai_socktype, results->ai_protocol);
    if (s == -1) {
        perror("[!] - socket failed");
        exit(1);
    }

    // Bind it to port 8080
    if (bind(s, results->ai_addr, results->ai_addrlen) != 0) {
        perror("[!] - bind failed");
        exit(1);
    }

    // Listen on port 80 using the newly-bound socket (10 connections at a time max)
    if (listen(s, 10) != 0) {
        perror("[!] - listen failed");
        close(s);
        exit(1);
    }

    // Accept an incoming connection
    struct sockaddr_storage their_addr;
    socklen_t               addr_size = sizeof(their_addr);
    int                     new_s = accept(s, (struct sockaddr*)&their_addr, &addr_size);
    if (new_s < 0) {
        perror("[!] - accept failed");
        close(s);
        exit(1);
    }

    // Receive and print all bytes until an EOF or an error occurs
    char    buf[10];
    ssize_t size_received;
    memset(buf, 0, sizeof(buf));
    while ((size_received = recv(new_s, buf, sizeof(buf), MSG_DONTWAIT)) > 0 ||
           errno == EWOULDBLOCK || errno == EAGAIN) {
        std::clog << "[!] - yay" << std::endl;
        write(2, buf, sizeof(buf));
        memset(buf, 0, sizeof(buf));
    }
    std::clog << "\n[!] - recv exited with " << size_received << std::endl;
    if (size_received < 0) perror("[!] - recv failed");
    freeaddrinfo(results);
    close(new_s);
    close(s);
    return 0;
}
