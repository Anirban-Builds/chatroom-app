#include "common.h"

void load_env(char *filename){
    FILE *f = fopen(filename, "r");
    if (!f) {
        // printf("No .env file found, skipping (%s)\n", filename);
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = '\0';

        if (line[0] == '\0' || line[0] == '#') continue;

        char *eq = strchr(line, '=');
        if (!eq) continue;

        *eq = '\0';
        char *key = line;
        char *value = eq + 1;

        SET_ENV(key, value);
    }
}

int is_allowed_origin(char* request)
{
    char* origin_start = STRCASESTR(request, "Origin: ");
    if (!origin_start) {
        return 0;
    }
    origin_start += 8;

    char origin[256] = {0};
    int i = 0;
    while (origin_start[i] != '\r' && origin_start[i] != '\n' && i < 255) {
        origin[i] = origin_start[i];
        i++;
    }
    origin[i] = '\0';

    if (strcmp(origin, getenv("CORS")) == 0){
        return 1;   // Allowed
    }

    return 0;   // Blocked
}

int is_edit_msg(char* buffer){
    if(strstr(buffer, "\"type\":\"edit\"")){
    return 1;
    }
return 0;
}

char* strcasestr_custom(char* str, char* substr) {
    if (!*substr) return (char*)str;
    for (; *str; str++) {
        const char *h = str, *n = substr;
        while (*h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n)) {
            h++; n++;
        }
        if (!*n) return (char*)str;
    }
    return NULL;
}

