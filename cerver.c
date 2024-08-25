#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

typedef struct Request {
    char *method;
    char *path;
} Request;

typedef struct Response {
    int socket;
} Response;

typedef struct Route {
    char *path;
    char *method;
    void (*handler) (struct Request *, struct Response *);
} Route;

typedef struct Cerver {
    int port;
    Route *routes;
    int route_count;
    int route_capacity;
    void (*start) (struct Cerver *);
    void (*route_register) (struct Cerver *, char *, char *, void (*handler) (struct Request *, struct Response *));
    void (*handle_request) (struct Cerver *, struct Request *, struct Response *);
    void (*send) (struct Response *, const char *);
} Cerver;

void cerver_send(Response *response, const char *data) {
    send(response->socket, data, strlen(data), 0);
}

void cerver_start(Cerver *cerver) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(cerver->port);

    // Binding the socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is running on port %d\n", cerver->port);

    while (1) {
        // Accept incoming connections
        int new_socket;
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        // Handle the connection
        printf("Connection accepted\n");
        char buffer[1024] = {0};
        ssize_t bytes_read = read(new_socket, buffer, 1024);
        if (bytes_read < 0) {
            perror("Read error");
            close(new_socket);
            continue;
        }
        printf("Received buffer: %s\n", buffer);

        char *method = strtok(buffer, " ");
        if (method == NULL) {
            fprintf(stderr, "Failed to parse method\n");
            close(new_socket);
            continue;
        }
        char *path = strtok(NULL, " ");
        if (path == NULL) {
            fprintf(stderr, "Failed to parse path\n");
            close(new_socket);
            continue;
        }

        // Create Request and Response objects
        Request request = { .method = method, .path = path };
        Response response = { .socket = new_socket };

        // Handle the request
        printf("Method: %s\n", method);
        printf("Path: %s\n", path);
        cerver->handle_request(cerver, &request, &response);

        close(new_socket);
    }

    close(server_fd);
}

void cerver_route_register(Cerver *cerver, char *path, char *method, void (*handler) (struct Request *, struct Response *)) {
    if (cerver->route_count >= cerver->route_capacity) {
        cerver->route_capacity *= 2;
        cerver->routes = (Route *) realloc(cerver->routes, sizeof(Route) * cerver->route_capacity);
        if (cerver->routes == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
    }
    Route *route = &cerver->routes[cerver->route_count++];
    route->path = path;
    route->method = method;
    route->handler = handler;
}

void handle_request(Cerver *cerver, Request *request, Response *response) {
    int route_found = 0;
    for (int i = 0; i < cerver->route_count; i++) {
        if (strcmp(cerver->routes[i].path, request->path) == 0 && strcmp(cerver->routes[i].method, request->method) == 0) {
            cerver->routes[i].handler(request, response);
            route_found = 1;
            break;
        }
    }
    if (!route_found) {
        // Send 404 response
        const char *not_found_response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        cerver_send(response, not_found_response);
    }
}

Cerver *NewCerver(int port) {
    Cerver *cerver = (Cerver *) malloc(sizeof(Cerver));
    if (cerver) {
        // Initialize cerver values
        cerver->port = port;
        cerver->start = cerver_start;
        cerver->route_register = cerver_route_register;
        cerver->handle_request = handle_request;
        cerver->send = cerver_send;
        cerver->route_count = 0;
        cerver->route_capacity = 10;
        cerver->routes = (Route *) malloc(sizeof(Route) * cerver->route_capacity);
        if (cerver->routes == NULL) {
            // Handle memory allocation failure
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
    }

    return cerver;
}

void HelloWorld(Request *request, Response *response) {
    const char *response_data = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Hello, World!</h1></body></html>";
    cerver_send(response, response_data);
}

int main() {
    Cerver *cerver = NewCerver(8080);
    cerver->route_register(cerver, "/", "GET", HelloWorld);
    cerver->route_register(cerver, "/hello", "GET", HelloWorld);
    cerver->start(cerver);
    return 0;
}