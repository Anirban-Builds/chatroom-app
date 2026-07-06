#ifndef _FILEMANAGER_
#define _FILEMANAGER_
#include "common.h"

void file_upload(SOCKET, char*, char*);
void file_delete(SOCKET, char*, char*);
void file_read(SOCKET, char*, char*);
void url_decode(char*, char*);

#endif