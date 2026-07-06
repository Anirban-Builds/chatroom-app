#include "room.h"
#include "client_manager.h"

int room_has_space(){
    int cnt =0;
    for(int i=0; i< MAX_CLIENTS; i++){
        if(clients[i].active) cnt ++;
    }
    return cnt < MAX_CLIENTS; // less then max means has space
}
