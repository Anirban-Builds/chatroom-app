#ifndef _CLIENT_MANAGER_
#define _CLIENT_MANAGER_
#include "common.h"
#ifdef _WIN32
#else
#endif
typedef struct{
    SOCKET fd;
    char username[32];
    int active;
    int typing;
} Client;

extern Client clients[MAX_CLIENTS];
extern mutex_t clients_mutex;

void init_client_manager();
void add_client(SOCKET, char*);
void remove_client(SOCKET);
int verify_client(SOCKET, char*);
void broadcast_websocket_message(unsigned char*, size_t, SOCKET);
#endif