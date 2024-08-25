#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "cerver.h"

void cerver_send(Response *response, const char *status_line, const char *body) {
    char header_buffer[1024] = {0};
    strcat(header_buffer, status_line);
    strcat(header_buffer, "\r\n");
    for (int i = 0; i < response->header_count; i++) {
        strcat(header_buffer, response->headers[i].key);
        strcat(header_buffer, ": ");
        strcat(header_buffer, response->headers[i].value);
        strcat(header_buffer, "\r\n");
    }
    strcat(header_buffer, "\r\n"); // End of headers
    send(response->socket, header_buffer, strlen(header_buffer), 0);
    send(response->socket, body, strlen(body), 0);
}

void parse_headers(char *buffer, Request *request) {
    char *line = strtok(buffer, "\r\n");
    while (line != NULL) {
        if (strstr(line, ": ") != NULL) {
            char *key = strtok(line, ": ");
            char *value = strtok(NULL, "\r\n");
            request->headers[request->header_count].key = strdup(key);
            request->headers[request->header_count].value = strdup(value);
            request->header_count++;
        }
        line = strtok(NULL, "\r\n");
    }
}

void add_header(Response *response, const char *key, const char *value) {
    response->headers[response->header_count].key = strdup(key);
    response->headers[response->header_count].value = strdup(value);
    response->header_count++;
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
        char buffer[2048] = {0}; // Increased buffer size to handle larger requests
        ssize_t bytes_read = read(new_socket, buffer, 2048);
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
        Request request = { .method = method, .path = path, .headers = malloc(sizeof(Header) * 20), .header_count = 0, .body = NULL };
        Response response = { .socket = new_socket, .headers = malloc(sizeof(Header) * 20), .header_count = 0, .body = NULL };

        // Parse headers
        parse_headers(buffer, &request);

        // Extract body if present
        char *body_start = NULL;
        for (size_t i = 0; i < bytes_read - 3; i++) {
            if (buffer[i] == '\r' && buffer[i + 1] == '\n' && buffer[i + 2] == '\r' && buffer[i + 3] == '\n') {
                body_start = buffer + i + 4;
                break;
            }
        }

        if (body_start != NULL) {
            size_t body_length = bytes_read - (body_start - buffer);
            request.body = (char *)malloc(body_length + 1); // Allocate memory for the body
            if (request.body) {
                strncpy(request.body, body_start, body_length);
                request.body[body_length] = '\0'; // Null-terminate the body
                printf("Extracted body: %s\n", request.body); // Debug statement
            } else {
                fprintf(stderr, "Failed to allocate memory for request body\n");
            }
        } else {
            printf("No body found in the request\n"); // Debug statement
            printf("Received buffer length: %ld\n", bytes_read); // Print the length of the buffer
            printf("Received buffer: %s\n", buffer); // Print the entire buffer for debugging

            // Print each character in the buffer with its ASCII value
            for (size_t i = 0; i < bytes_read; i++) {
                printf("buffer[%zu] = '%c' (ASCII: %d)\n", i, buffer[i], (unsigned char)buffer[i]);
            }
        }
        // Handle the request
        printf("Method: %s\n", method);
        printf("Path: %s\n", path);
        cerver->handle_request(cerver, &request, &response);

        // Send the response
        cerver_send(&response, "HTTP/1.1 200 OK", response.body ? response.body : "");

        // Free allocated memory
        for (int i = 0; i < request.header_count; i++) {
            free(request.headers[i].key);
            free(request.headers[i].value);
        }
        free(request.headers);
        if (request.body) {
            free(request.body);
        }

        for (int i = 0; i < response.header_count; i++) {
            free(response.headers[i].key);
            free(response.headers[i].value);
        }
        free(response.headers);
        if (response.body) {
            free(response.body);
        }

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
    printf("Handling request for path: %s, method: %s\n", request->path, request->method);
    for (int i = 0; i < cerver->route_count; i++) {
        printf("Checking route: %s %s\n", cerver->routes[i].method, cerver->routes[i].path);
        if (strcmp(cerver->routes[i].path, request->path) == 0 && strcmp(cerver->routes[i].method, request->method) == 0) {
            printf("Route found, calling handler\n");
            cerver->routes[i].handler(request, response);
            route_found = 1;
            break;
        }
    }
    if (!route_found) {
        // Send 404 response
        printf("Route not found, sending 404 response\n");
        add_header(response, "Content-Length", "0");
        const char *status_line = "HTTP/1.1 404 Not Found";
        const char *body = "";
        cerver_send(response, status_line, body);
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