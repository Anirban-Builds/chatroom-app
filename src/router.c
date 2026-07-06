#include "router.h"
#include "helper.h"

Route routes[] = {
    {"GET",  "/", handle_index},
    {"GET", "/chat", handle_chat},
    {"POST", "/upload", handle_file_upload},
    {"DELETE", "/delete", handle_file_delete},
    {"GET", "/uploads", handle_file_read},
    {"GET", "/room", handle_room_is_full},
};

int route_count = sizeof(routes) / sizeof(routes[0]);

void route_http_request(SOCKET client_fd, char* buffer){

    // parse req string and find route
    char method[16], path[256]; // assumed array size
    sscanf(buffer, "%15s %255s", method, path);
    char *body = strstr(buffer, "\r\n\r\n");
    if (body) {
        body += 4;
        if (*body == '\0') body = NULL;
    }
    char* q = strchr(path, '?');
    if(q) *q = '\0';

    if(!strcmp(method, "OPTIONS")){
    if(!is_allowed_origin(buffer)){
        handle_response(client_fd, "ERROR", 403, "Origin not allowed", "");
        return;
    }
    handle_preflight(client_fd);
    return;
}
    char temp_path[256] = {0};
    if (strncmp(path, "/uploads/", 9) == 0) {
        // for reading files
        strcpy(temp_path, path);
        path[8] = '\0';
        body = temp_path; // GET request
    }

    for(int i=0; i<route_count; i++){

        if(!strcmp(routes[i].method, method) && !strcmp(routes[i].path, path)){
        routes[i].handler_func(client_fd, body, buffer);
        return;
       }
    }
    handle_response(client_fd, "ERROR", 404, "Path Not found", "");
}