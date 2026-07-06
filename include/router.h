#ifndef _ROUTER_
#define _ROUTER_
#include "common.h"
#include "handler.h"

typedef struct {
    const char* method;
    const char* path;
    void (*handler_func)(SOCKET, char*, char*);
} Route;
void route_http_request(SOCKET, char*);
#endif