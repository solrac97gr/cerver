#include "cerver.h"
#include <cjson/cJSON.h>

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

// Extrac the body.message from the request body and set it as the response body
void PrintMsgFromBody(Request *request, Response *response) {
	if (request->body) {
		cJSON *root = cJSON_Parse(request->body);
		if (root) {
			cJSON *message = cJSON_GetObjectItem(root, "message");
			if (message) {
				add_header(response, "Content-Type", "application/json");
				response->body = strdup(message->valuestring);
			} else {
				response->body = strdup("{\"error\": \"No message in body\"}");
			}
			cJSON_Delete(root);
		} else {
			response->body = strdup("{\"error\": \"Invalid JSON\"}");
		}
	} else {
		response->body = strdup("{\"error\": \"No body received\"}");	
}
}

int main(int argc, char *argv[]) {
  Cerver *cerver = NewCerver(8080);
  cerver->route_register(cerver, "/", "GET", HelloWorld);
  cerver->route_register(cerver, "/echo", "POST",
                         EchoHandler); // Register EchoHandler for POST /echo
  cerver->route_register(cerver, "/print", "POST", PrintMsgFromBody);
  cerver->start(cerver);
  return 0;
}
