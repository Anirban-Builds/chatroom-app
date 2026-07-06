#ifndef _HANDLER_
#define _HANDLER_
#include "common.h"

void handle_index(SOCKET, char*, char*);
void handle_response(SOCKET, char*, int, char*, char*);
void handle_chat(SOCKET, char*, char*);
void handle_file_upload(SOCKET, char*, char*);
void handle_file_delete(SOCKET, char*, char*);
void handle_file_read(SOCKET, char*, char*);
void handle_preflight(SOCKET);
void handle_file_download(SOCKET, char*, long);
void handle_room_is_full(SOCKET, char*, char*);
int send_all(SOCKET, char*, long);
#endif