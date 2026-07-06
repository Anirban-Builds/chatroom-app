#include "client_manager.h"
#include "handler.h"
#include "room.h"
#include "websocket.h"

Client clients[MAX_CLIENTS];
mutex_t clients_mutex;

void init_client_manager(){
    MUTEX_INIT(clients_mutex);
}

void add_client(SOCKET fd, char* username){
    MUTEX_LOCK(clients_mutex);

    for(int i = 0; i < MAX_CLIENTS; i++){
    if(clients[i].active && strcmp(clients[i].username, username) == 0){
        send_websocket_close(clients[i].fd, 4001, "Username already inside room");
        SHUTDOWN_BOTH(clients[i].fd);
        close_socket(clients[i].fd);
        clients[i].active = 0;
        break;
        }
    }

    if(!room_has_space()){
        send_websocket_close(fd, 4002, "Room is completely full");
        MUTEX_UNLOCK(clients_mutex);
        return;
    }
    for(int i=0; i< MAX_CLIENTS; i++){
        if(!clients[i].active ){
            clients[i].fd = fd;
            strncpy(clients[i].username, username, sizeof(clients[i].username) - 1);
            clients[i].username[sizeof(clients[i].username) - 1] = '\0';
            clients[i].active = 1;
            clients[i].typing = 0;
            MUTEX_UNLOCK(clients_mutex);
            return; // to avoid duplication
        }
    }
    // MUTEX_UNLOCK(clients_mutex); code should return from room block if no space
}

void remove_client(SOCKET fd){
    MUTEX_LOCK(clients_mutex);
    for(int i=0; i< MAX_CLIENTS; i++){
        if(clients[i].active && clients[i].fd == fd){
            clients[i].active = 0;
            MUTEX_UNLOCK(clients_mutex);
            return; // to avoid overwrite
        }
    }
    MUTEX_UNLOCK(clients_mutex);
    return;
}

int verify_client(SOCKET fd, char* username){
     MUTEX_LOCK(clients_mutex);
     for(int i=0; i<MAX_CLIENTS; i++){
        if(clients[i].fd == fd){
            if(!strcmp(clients[i].username, username)){
                MUTEX_UNLOCK(clients_mutex);
                return 1;
            }
        }
     }
     MUTEX_UNLOCK(clients_mutex);
     return 0;
}

void broadcast_websocket_message(unsigned char* msg, size_t len, SOCKET sender_fd){
    MUTEX_LOCK(clients_mutex);
    for(int i=0; i<MAX_CLIENTS; i++){
        if(clients[i].active && clients[i].fd != sender_fd){
            send_websocket_frame(clients[i].fd, 0x01, msg, len);
        }
    }
    MUTEX_UNLOCK(clients_mutex);
}