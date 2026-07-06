CC = gcc
SRCS = src/main.c src/server.c src/router.c src/handler.c src/client_manager.c src/room.c src/websocket.c \
src/file_manager.c src/helper.c
OUT = build/chatroom

ifeq ($(OS),Windows_NT)
    SHELL = powershell.exe
    INCLUDES = -I"E:\Program Files(x86)\OpenSSL-Win64\include" -Iinclude
    WIN_LIBS = -L"E:\Program Files(x86)\OpenSSL-Win64\lib\VC\x64\MT" -lws2_32 -lssl -lcrypto
else
    INCLUDES = -Iinclude
endif

windows:
	$(CC) $(SRCS) -o $(OUT) $(INCLUDES) $(WIN_LIBS)

linux:
	$(CC) $(SRCS) -o $(OUT) $(INCLUDES) -lssl -lcrypto -lpthread

all: linux