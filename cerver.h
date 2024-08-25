#ifndef CERVER_H
#define CERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

typedef struct Header {
    char *key;
    char *value;
} Header;

typedef struct Request {
    char *method;
    char *path;
    Header *headers;
    int header_count;
    char *body;
} Request;

typedef struct Response {
    int socket;
    Header *headers;
    int header_count;
    char *body;
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
    void (*send) (struct Response *, const char *, const char *);
} Cerver;

void cerver_send(Response *response, const char *status_line, const char *body);
void parse_headers(char *buffer, Request *request);
void add_header(Response *response, const char *key, const char *value);
void cerver_start(Cerver *cerver);
void cerver_route_register(Cerver *cerver, char *path, char *method, void (*handler) (struct Request *, struct Response *));
void handle_request(Cerver *cerver, Request *request, Response *response);
Cerver *NewCerver(int port);

#endif // CERVER_H