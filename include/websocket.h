#ifndef _WEBSOCKET_
#define _WEBSOCKET_
#include "common.h"

int handle_websocket_handshake(SOCKET, char*);
void send_websocket_close(SOCKET, uint16_t, char*);
int send_websocket_frame(SOCKET, int, void*, size_t);
int handle_ws_client(SOCKET);

#endif