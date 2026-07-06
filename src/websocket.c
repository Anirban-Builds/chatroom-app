#include "websocket.h"
#include "client_manager.h"
#include "common.h"
#include "helper.h"

#define WS_MAGIC "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define MAX_FRAME_SIZE  (64 * 1024)
#define MAX_MESSAGE_SIZE  (1024 * 1024)

static void base64_encode(unsigned char* input, int len, char* output){
    BIO *bmem, *b64;
    BUF_MEM *bptr;

    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, input, len);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);
    memcpy(output, bptr->data, bptr->length);
    output[bptr->length] = '\0';
    BIO_free_all(b64);
}

int handle_websocket_handshake(SOCKET fd, char* buffer){
    char* key_start = strstr(buffer, "Sec-WebSocket-Key: ");
    if(!key_start) return 1;
    key_start += 19;

    char key[64] = {0};
    int i = 0;
    while(key_start[i] != '\r' && key_start[i] != '\n' && i < 63){
        key[i] = key_start[i];
        i++;
    }
    key[i] = '\0';

    char combined[128] = {0};
    snprintf(combined, sizeof(combined), "%s%s", key, WS_MAGIC);

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)combined, strlen(combined), hash);

    char accept_key[64] = {0};
    base64_encode(hash, SHA_DIGEST_LENGTH, accept_key);

    char response[256];
    snprintf(response, sizeof(response),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n\r\n",
        accept_key
    );
    send(fd, response, strlen(response), 0);
    return 0;
}

void send_websocket_close(SOCKET fd, uint16_t code, char* reason)
{
    unsigned char frame[128] = {0};
    size_t len = 2;
    frame[0] = 0x88;
    if (code) {
        frame[2] = (code >> 8) & 0xFF;
        frame[3] = code & 0xFF;
        len = 4;

        if (reason && *reason) {
            size_t rlen = strlen(reason);
            if (rlen > 123) rlen = 123;
            memcpy(frame + 4, reason, rlen);
            len += rlen;
        }
    }
    frame[1] = (unsigned char)(len - 2);
    send(fd, frame, len, 0);
}

static int read_websocket_frame(SOCKET fd,
                                unsigned char* buffer,
                                size_t* out_len,
                                int* opcode,
                                int* is_final)
{
    unsigned char header[2];
    if (recv(fd, header, 2, 0) != 2) return 1;
    *is_final = (header[0] & 0x80) ? 1 : 0;
    *opcode   = header[0] & 0x0F;
    uint64_t payload_len = header[1] & 0x7F;
    int masked = header[1] & 0x80;

    if (payload_len == 126) {
        unsigned char ext[2];
        if (recv(fd, ext, 2, 0) != 2) return 1;
        payload_len = (ext[0] << 8) | ext[1];
    }

    else if (payload_len == 127) {
        unsigned char ext[8];
        if (recv(fd, ext, 8, 0) != 8) return 1;
        payload_len = ((uint64_t)ext[4] << 24) | (ext[5] << 16) | (ext[6] << 8) | ext[7];
    }

    if (payload_len > MAX_FRAME_SIZE) {
        send_websocket_close(fd, 1009, "Frame too large");
        return 1;
    }

    unsigned char masking_key[4] = {0};

    if (masked) {
        if (recv(fd, masking_key, 4, 0) != 4) return 1;
    }

    if (recv(fd, buffer, payload_len, 0) != (ssize_t)payload_len) return 1;

    if (masked) {
        for (uint64_t i = 0; i < payload_len; i++) {
            buffer[i] ^= masking_key[i % 4];
        }
    }
    *out_len = payload_len;
    return 0;
}

int send_websocket_frame(SOCKET fd, int opcode, void* data, size_t len)
{
    unsigned char header[10];
    size_t header_len = 2;

    header[0] = 0x80 | (unsigned char)opcode;
    if (len < 126) {
        header[1] = (unsigned char)len;
    } else if (len < 65536) {
        header[1] = 126;
        header[2] = (len >> 8) & 0xFF;
        header[3] = len & 0xFF;
        header_len = 4;
    } else return -1;

    send(fd, header, header_len, 0);
    if (len > 0 && data) send(fd, data, len, 0);
    return 0;
}

int handle_ws_client(SOCKET fd)
{
    unsigned char buffer[MAX_FRAME_SIZE + 1];
    unsigned char *message_buffer = NULL;
    size_t message_len = 0;
    int first_opcode = 0;
    int opcode, is_final;

    while (1) {
        size_t payload_len = 0;
        if (read_websocket_frame(fd, buffer, &payload_len, &opcode, &is_final)) {
            break;
        }

        switch (opcode) {
            case 0x01:  // Text start
            case 0x02:  // Binary start
                if (message_buffer) {
                    free(message_buffer);   // cleanup previous incomplete message
                }
                message_buffer = malloc(MAX_MESSAGE_SIZE);
                if (!message_buffer) {
                    send_websocket_close(fd, 1011, "Out of memory");
                    return 1;
                }
                message_len = 0;
                first_opcode = opcode;
                // fallthrough

            case 0x00:  // Continuation frame
                if (!message_buffer) {
                    send_websocket_close(fd, 1002, "Unexpected continuation");
                    return 1;
                }

                if (message_len + payload_len > MAX_MESSAGE_SIZE) {
                    free(message_buffer);
                    send_websocket_close(fd, 1009, "Message too large");
                    return 1;
                }

                memcpy(message_buffer + message_len, buffer, payload_len);
                message_len += payload_len;

                if (is_final) {
                    message_buffer[message_len] = '\0';

                    if(is_edit_msg(message_buffer)){
                        char *u = strstr(message_buffer, "\"username\":\"");
                        if(!u){
                            send_websocket_close(fd, 4003, "User Not Found");
                            goto cleanup;
                        }
                        u += 12;
                        char msg_username[32] = {0};
                        int i = 0;
                        while(u[i] != '"' && u[i] != '\0' && i < 31){
                            msg_username[i] = u[i];
                            i++;
                        }
                        msg_username[i] = '\0'; // extract username
                        if(!verify_client(fd, msg_username)){
                            send_websocket_close(fd, 4003, "Unauthorized");
                            goto cleanup;
                        }
                    }
                    broadcast_websocket_message(message_buffer, message_len, fd);
                    free(message_buffer);
                    message_buffer = NULL;
                    message_len = 0;
                }
                break;

            case 0x08:  // Close
                send_websocket_close(fd, 1000, "Normal closure");
                goto cleanup;

            case 0x09:  // Ping
                send_websocket_frame(fd, 0x0A, buffer, payload_len);
                break;

            case 0x0A:  // Pong
                break;

            default:
                send_websocket_close(fd, 1003, "Unsupported opcode");
                goto cleanup;
        }
    }
cleanup:
    if (message_buffer) free(message_buffer);
    return 0;
}