SHELL = powershell.exe
CC = gcc
INCLUDES = -I"E:\Program Files(x86)\OpenSSL-Win64\include" -Iinclude
SRCS = src/main.c src/server.c src/router.c src/handler.c src/client_manager.c src/room.c src/websocket.c \
src/file_manager.c src/helper.c
OUT = build/chatroom

windows:
	$(CC) $(SRCS) -o $(OUT) $(INCLUDES) -L"E:\Program Files(x86)\OpenSSL-Win64\lib\VC\x64\MT" -lws2_32 -lssl -lcrypto
linux:
	$(CC) $(SRCS) -o $(OUT)  $(INCLUDES) -lssl -lcrypto -lpthread

all: linux