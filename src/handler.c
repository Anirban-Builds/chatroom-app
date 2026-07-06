#include "common.h"
#include "client_manager.h"
#include "websocket.h"
#include "file_manager.h"
#include "room.h"

#define HANDLER_ARGS SOCKET fd, char* body, char* buffer

char res[2048];
char json_body[2048];

// sending multiple packets
int send_all(SOCKET fd, char *buf, long len) {
    long sent_total = 0;
    while (sent_total < len) {
        long n = send(fd, buf + sent_total, len - sent_total, 0);
        if (n <= 0) {
            return 1; // connection error / unexpected close
        }
        sent_total += n;
    }
    return 0;
}

void handle_preflight(SOCKET fd){
    char res[256];
    int res_len = snprintf(res, sizeof(res),
        "HTTP/1.1 204 No Content\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n\r\n"
    );
    if (res_len < 0) return;
    if ((size_t)res_len >= sizeof(res)) {
        res_len = sizeof(res) - 1;
    }
    send(fd, res, res_len, 0);
}

void handle_response(SOCKET fd, char *status, int code, char *message, char* data){
    if(!data || !strlen(data)) data = "null";
    snprintf(json_body, sizeof(json_body),
    "{\"status\": \"%s\", \"message\": \"%s\", \"code\": %d, \"data\": %s}",
    status, message, code, data);

   int res_len = snprintf(res, sizeof(res),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Content-Length: %d\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n\r\n%s",
        code, message, strlen(json_body), json_body
    );
    if (res_len < 0) return;
    if ((size_t)res_len >= sizeof(res)) {
        res_len = sizeof(res) - 1;
    }
    send(fd, res, res_len, 0);
}

void handle_index(HANDLER_ARGS){
    handle_response(fd, "success", 200, "Server is Live ✅", "");
}

void handle_room_is_full(HANDLER_ARGS){
    if(room_has_space()){
        handle_response(fd, "success", 200, "", "{\"full\" : \"0\"}");
    }
    else{
        handle_response(fd, "success", 200, "", "{\"full\" : \"1\"}");
    }
}

void handle_file_upload(HANDLER_ARGS){
    file_upload(fd, body, buffer);
}

void handle_file_download(SOCKET fd, char *file_bytes, long file_size){
      if (!file_bytes || file_size <= 0) {
        handle_response(fd, "failure", 404, "File Not Found", NULL);
        return;
    }
    char headers[256] = {0};
    int header_len = snprintf(headers, sizeof(headers),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Content-Length: %ld\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n\r\n", file_size);
    if (header_len < 0) return;
    if ((size_t)header_len >= sizeof(headers)) {
        header_len = sizeof(headers) - 1;
    }

    send(fd, headers, header_len, 0);
    send_all(fd, file_bytes, file_size);
}

void handle_file_delete(HANDLER_ARGS){
    char *url_start = strstr(body, "\"url\"");
    if (!url_start) {
        handle_response(fd, "failure", 400, "Bad Request", "");
        return;
    }
    url_start = strchr(url_start + 5, '"');
    if (!url_start) {
        handle_response(fd, "failure", 400, "Bad Request", "");
        return;
    }
    url_start++;

    char *path_start = strstr(url_start, "uploads/");
    if (!path_start) {
        handle_response(fd, "failure", 400, "Bad Request", "");
        return;
    }
    char *url_end = strchr(path_start, '"');
    if (!url_end) {
        handle_response(fd, "failure", 400, "Bad Request", "");
        return;
    }
    *url_end = '\0';

    char decoded_path[512] = {0};
    url_decode(decoded_path, path_start); // remove %20 by spaces

    char *last_slash = strrchr(decoded_path, '/');
    if (!last_slash){
    handle_response(fd, "ERROR", 400, "Bad Request", "");
    return;
    }
    *last_slash = '\0';
     char *timestamp = last_slash + 1;
     char *filename = decoded_path + 8;
     char real_disk_path[512] = {0};
     snprintf(real_disk_path, sizeof(real_disk_path), "uploads/%s_%s", timestamp, filename);
     if (strstr(real_disk_path, "..")) {
        handle_response(fd, "ERROR", 403, "Forbidden", "");
        return;
    }
    file_delete(fd, real_disk_path, buffer);
}

void handle_file_read(HANDLER_ARGS){
    char *file_path = strstr(buffer,"/uploads/");
    if (!file_path) {
        handle_response(fd, "ERROR", 404, "Not Found", "");
        return;
    }
    file_path += 1;

    if (*file_path == '\0') {
        handle_response(fd, "ERROR", 400, "Filename Missing", "");
        return;
    }
    char *end = strchr(file_path, ' '); // skips %20 like substring
    if (end) *end = '\0';

    char decoded_path[512] = {0};
    url_decode(decoded_path, file_path);

    char *physical_path = decoded_path;
    if (*physical_path == '/') physical_path++;

    char *last_slash = strrchr(physical_path, '/');
    if (!last_slash){
    handle_response(fd, "ERROR", 400, "Bad Request", "");
    return;
    }

    *last_slash = '\0';
    char *timestamp = last_slash + 1;
    char *filename = physical_path + 8;
    char real_disk_path[512] = {0};
    snprintf(real_disk_path, sizeof(real_disk_path), "uploads/%s_%s", timestamp, filename);

    if (strstr(real_disk_path, "..")) {
        handle_response(fd, "ERROR", 403, "Forbidden", "");
        return;
    }

    file_read(fd, real_disk_path, buffer);
}

void handle_chat(HANDLER_ARGS){
    char username[32] = {0};
    char* q = strstr(buffer, "?username=");
    if(!q){
        handle_response(fd, "ERROR", 400, "Username required", NULL);
        return;
    }
    q += 10;
    int i = 0;
    while(q[i] != ' ' && q[i] != '\r' && q[i] != '\n' && q[i] != '&' && i < 31){
        username[i] = q[i];
        i++;
    }
    username[i] = '\0';

    if(handle_websocket_handshake(fd, buffer)){
        handle_response(fd, "ERROR", 500, "Could not upgrade connection", "");
        return;
    }

    add_client(fd, username);
    handle_ws_client(fd);
    remove_client(fd);
}