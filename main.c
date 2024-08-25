#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define DEFAULT_PORT 8080

void handle_client(int connfd);
void send_404(int connfd);
void send_file(int connfd, const char *file);

int main(int argc, char *argv[]) {
    int port = (argc > 1) ? atoi(argv[1]) : DEFAULT_PORT;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket failed");
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        return 1;
    }

    if (listen(sockfd, 10) < 0) {
        perror("listen failed");
        close(sockfd);
        return 1;
    }

    while (1) {
        int connfd = accept(sockfd, NULL, NULL);
        if (connfd < 0) {
            perror("accept failed");
            continue;
        }
        handle_client(connfd);
        close(connfd);
    }

    close(sockfd);
    return 0;
}

void handle_client(int connfd) {
    char buffer[BUFFER_SIZE] = {0};
    recv(connfd, buffer, BUFFER_SIZE, 0);

    if (strncmp(buffer, "GET ", 4) == 0) {
        char *file = buffer + 5;
        char *end = strchr(file, ' ');
        if (end) {
            *end = '\0';
            send_file(connfd, file);
        } else {
            send_404(connfd);
        }
    } else if (strncmp(buffer, "POST ", 5) == 0) {
        send(connfd,
             "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>POST request</h1></body></html>",
             90, 0);
    } else {
        send_404(connfd);
    }
}

void send_404(int connfd) {
    send(connfd,
         "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n<html><body><h1>404 Not Found</h1></body></html>",
         90, 0);
}

void send_file(int connfd, const char *file) {
    int opened_fd = open(file, O_RDONLY);
    if (opened_fd == -1) {
        send_404(connfd);
    } else {
        off_t len = 256;
        send(connfd, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n", 44, 0);
	sendfile(opened_fd,connfd ,0, &len,0,0);
	close(opened_fd);
    }
}

