#include "cerver.h"

void HelloWorld(Request *request, Response *response) {
  add_header(response, "Content-Type", "application/json");
  response->body = strdup("{\"message\": \"Hello, World!\"}");
}

void EchoHandler(Request *request, Response *response) {
  add_header(response, "Content-Type", "application/json");
  if (request->body) {
    // Directly set the response body to the request body
    response->body = strdup(request->body);
  } else {
    response->body = strdup("{\"body\":\"No body received\"}");
  }
}

int main(int argc, char *argv[]) {
  Cerver *cerver = NewCerver(8080);
  cerver->route_register(cerver, "/", "GET", HelloWorld);
  cerver->route_register(cerver, "/echo", "POST",
                         EchoHandler); // Register EchoHandler for POST /echo
  cerver->start(cerver);
  return 0;
}
