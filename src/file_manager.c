#include "file_manager.h"
#include "common.h"
#include "handler.h"

#define ARGS SOCKET fd, char* body, char* buffer

void file_upload(ARGS) {
    char boundary[128]={0};
    char *ct = strstr(buffer, "boundary=");
    if (!ct) {
        handle_response(fd, "failure", 400, "Bad Request", "");
        return;
    }
    sscanf(ct + 9, "%127[^;\r\n]", boundary);

    // find filename
    char filename[256] = {0};
    char *fn = strstr(buffer, "filename=\"");
    if (fn) sscanf(fn + 10, "%255[^\"]", filename);

    char *safe_fn = strrchr(filename, '/');
    if(!safe_fn) safe_fn = strrchr(filename, '\\');
    if(safe_fn) safe_fn++;
    else safe_fn = filename;

    // find actual file content (skip part headers)
    char *file_data = strstr(body, "\r\n\r\n");
    if (!file_data){
        handle_response(fd, "failure", 400, "Bad Request", "");
        return;
    }
    file_data += 4;

    // find end boundary
    char end_boundary[130] = {0};
    snprintf(end_boundary, sizeof(end_boundary), "--%s", boundary);
    char *file_end = NULL;
    int content_length = 0;
    char *cl = STRCASESTR(buffer, "Content-Length:");
    if (cl) sscanf(cl + 15, "%d", &content_length);
    int body_remaining_len = content_length - (file_data - body);
    #ifdef _WIN32
    for (int i = 0; i <= body_remaining_len - (int)strlen(end_boundary); i++) {
        if (memcmp(file_data + i, end_boundary, strlen(end_boundary)) == 0) {
            file_end = file_data + i;
            break;
        }
    }
    #else
    file_end = memmem(file_data, body_remaining_len, end_boundary, strlen(end_boundary));
    #endif
    if (!file_end){
        handle_response(fd, "failure", 400, "Bad Request", "");
        return;
    }

    int file_len = file_end - file_data;

    #ifdef _WIN32
    _mkdir("uploads");
    #else
    mkdir("uploads", 0777);
    #endif

    // save file
    char path[512] ={0};
    long timestamp = time(NULL);
    snprintf(path, sizeof(path), "uploads/%ld_%s", timestamp, safe_fn);

    FILE *fp = fopen(path, "wb");
    if (!fp) {
        handle_response(fd, "failure", 500, "Internal Server Error", "");
        return;
    }
    fwrite(file_data, 1, file_len, fp);
    fclose(fp);

    // send response
    char data[600]= {0};
    char *underscore = strchr(path, '_');
    if (underscore) {safe_fn = underscore + 1; }
    snprintf(data, sizeof(data), "{\"url\": \"https://anirban0011-chatroom-app.hf.space/uploads/%s/%ld\"}", safe_fn, timestamp);
    handle_response(fd, "success", 200, "OK", data);
}

void file_delete(ARGS){
    char* target_path = body;
    if(!remove(target_path)){
        handle_response(fd, "success", 200, "OK", "");
    }
    else{
        handle_response(fd, "failure", 404, "Not Found", "");
    }
}

void url_decode(char *dst, char *src) {
    char a, b;
    while (*src) {
        if (*src == '%' && (a = src[1]) && (b = src[2]) &&
            isxdigit((unsigned char)a) && isxdigit((unsigned char)b)) {
            a = tolower(a); b = tolower(b);
            a = (a >= 'a') ? (a - 'a' + 10) : (a - '0');
            b = (b >= 'a') ? (b - 'a' + 10) : (b - '0');
            *dst++ = 16 * a + b;
            src += 3;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

void file_read(ARGS){
    char* real_disk_path = body;
    FILE *fp = fopen(real_disk_path, "rb");
    if (!fp) {
        handle_response(fd, "ERROR", 404, "Path Not found", "");
        return;
    }

    // Get file size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *file_buffer = malloc(file_size);
    fread(file_buffer, 1, file_size, fp);
    fclose(fp);
    handle_file_download(fd, file_buffer, file_size);
    free(file_buffer);
}