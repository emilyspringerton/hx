#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static const char base64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void usage(const char *prog) {
    fprintf(stderr, "usage: %s --host HOST --port PORT --path PATH\n", prog);
}

static int connect_host(const char *host, const char *port) {
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    struct addrinfo *rp = NULL;
    int sock = -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) != 0) {
        return -1;
    }

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock == -1) {
            continue;
        }
        if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }
        close(sock);
        sock = -1;
    }

    freeaddrinfo(res);
    return sock;
}

static void base64_encode(const unsigned char *in, size_t len, char *out, size_t out_len) {
    size_t i = 0;
    size_t o = 0;
    while (i < len && o + 4 < out_len) {
        unsigned int val = 0;
        int remaining = (int)(len - i);

        val |= (unsigned int)in[i++] << 16;
        if (remaining > 1) {
            val |= (unsigned int)in[i++] << 8;
        }
        if (remaining > 2) {
            val |= (unsigned int)in[i++];
        }

        out[o++] = base64_table[(val >> 18) & 0x3F];
        out[o++] = base64_table[(val >> 12) & 0x3F];
        if (remaining > 1) {
            out[o++] = base64_table[(val >> 6) & 0x3F];
        } else {
            out[o++] = '=';
        }
        if (remaining > 2) {
            out[o++] = base64_table[val & 0x3F];
        } else {
            out[o++] = '=';
        }
    }
    out[o] = '\0';
}

static void random_bytes(unsigned char *buf, size_t len) {
    FILE *f = fopen("/dev/urandom", "rb");
    if (f) {
        size_t read_len = fread(buf, 1, len, f);
        fclose(f);
        if (read_len == len) {
            return;
        }
    }
    for (size_t i = 0; i < len; i++) {
        buf[i] = (unsigned char)(rand() % 255);
    }
}

static void sha1_transform(unsigned int state[5], const unsigned char buffer[64]) {
    unsigned int a, b, c, d, e;
    unsigned int w[80];

    for (int i = 0; i < 16; i++) {
        w[i] = (unsigned int)buffer[i * 4 + 0] << 24;
        w[i] |= (unsigned int)buffer[i * 4 + 1] << 16;
        w[i] |= (unsigned int)buffer[i * 4 + 2] << 8;
        w[i] |= (unsigned int)buffer[i * 4 + 3];
    }

    for (int i = 16; i < 80; i++) {
        unsigned int val = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
        w[i] = (val << 1) | (val >> 31);
    }

    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];

    for (int i = 0; i < 80; i++) {
        unsigned int f, k;
        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }

        unsigned int temp = ((a << 5) | (a >> 27)) + f + e + k + w[i];
        e = d;
        d = c;
        c = (b << 30) | (b >> 2);
        b = a;
        a = temp;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

static void sha1(const unsigned char *data, size_t len, unsigned char out[20]) {
    unsigned int state[5] = {
        0x67452301,
        0xEFCDAB89,
        0x98BADCFE,
        0x10325476,
        0xC3D2E1F0
    };

    unsigned char block[64];
    size_t offset = 0;
    while (len - offset >= 64) {
        memcpy(block, data + offset, 64);
        sha1_transform(state, block);
        offset += 64;
    }

    size_t remaining = len - offset;
    memset(block, 0, sizeof(block));
    if (remaining > 0) {
        memcpy(block, data + offset, remaining);
    }
    block[remaining] = 0x80;

    if (remaining >= 56) {
        sha1_transform(state, block);
        memset(block, 0, sizeof(block));
    }

    unsigned long long bit_len = (unsigned long long)len * 8;
    block[56] = (unsigned char)(bit_len >> 56);
    block[57] = (unsigned char)(bit_len >> 48);
    block[58] = (unsigned char)(bit_len >> 40);
    block[59] = (unsigned char)(bit_len >> 32);
    block[60] = (unsigned char)(bit_len >> 24);
    block[61] = (unsigned char)(bit_len >> 16);
    block[62] = (unsigned char)(bit_len >> 8);
    block[63] = (unsigned char)(bit_len);

    sha1_transform(state, block);

    for (int i = 0; i < 5; i++) {
        out[i * 4 + 0] = (unsigned char)(state[i] >> 24);
        out[i * 4 + 1] = (unsigned char)(state[i] >> 16);
        out[i * 4 + 2] = (unsigned char)(state[i] >> 8);
        out[i * 4 + 3] = (unsigned char)(state[i]);
    }
}

static int header_value(const char *headers, const char *key, char *out, size_t out_len) {
    const char *cursor = headers;
    size_t key_len = strlen(key);
    while (*cursor) {
        const char *line_end = strstr(cursor, "\r\n");
        size_t line_len = line_end ? (size_t)(line_end - cursor) : strlen(cursor);
        if (line_len > key_len + 1) {
            int match = 1;
            for (size_t i = 0; i < key_len; i++) {
                char a = cursor[i];
                char b = key[i];
                if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                if (a != b) {
                    match = 0;
                    break;
                }
            }
            if (match && cursor[key_len] == ':') {
                const char *value = cursor + key_len + 1;
                while (*value == ' ' && *value != '\r' && *value != '\n') {
                    value++;
                }
                size_t i = 0;
                while (value[i] && value[i] != '\r' && value[i] != '\n' && i + 1 < out_len) {
                    out[i] = value[i];
                    i++;
                }
                out[i] = '\0';
                return 1;
            }
        }
        if (!line_end) {
            break;
        }
        cursor = line_end + 2;
    }
    return 0;
}

int main(int argc, char **argv) {
    const char *host = NULL;
    const char *port = NULL;
    const char *path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--host") == 0 && i + 1 < argc) {
            host = argv[++i];
        } else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = argv[++i];
        } else if (strcmp(argv[i], "--path") == 0 && i + 1 < argc) {
            path = argv[++i];
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    if (!host || !port || !path) {
        usage(argv[0]);
        return 1;
    }

    int sock = connect_host(host, port);
    if (sock < 0) {
        fprintf(stderr, "failed to connect to %s:%s\n", host, port);
        return 1;
    }

    unsigned char raw_key[16];
    char key[64];
    random_bytes(raw_key, sizeof(raw_key));
    base64_encode(raw_key, sizeof(raw_key), key, sizeof(key));

    char request[1024];
    int req_len = snprintf(request, sizeof(request),
                           "GET %s HTTP/1.1\r\n"
                           "Host: %s:%s\r\n"
                           "Upgrade: websocket\r\n"
                           "Connection: Upgrade\r\n"
                           "Sec-WebSocket-Key: %s\r\n"
                           "Sec-WebSocket-Version: 13\r\n\r\n",
                           path, host, port, key);
    if (req_len <= 0 || req_len >= (int)sizeof(request)) {
        fprintf(stderr, "request too long\n");
        close(sock);
        return 1;
    }

    if (send(sock, request, (size_t)req_len, 0) != req_len) {
        perror("send");
        close(sock);
        return 1;
    }

    char response[4096];
    ssize_t total = 0;
    while ((size_t)total < sizeof(response) - 1) {
        ssize_t n = recv(sock, response + total, sizeof(response) - 1 - (size_t)total, 0);
        if (n <= 0) {
            break;
        }
        total += n;
        if (strstr(response, "\r\n\r\n") != NULL) {
            break;
        }
    }
    response[total] = '\0';
    close(sock);

    if (strstr(response, "101") == NULL) {
        fprintf(stderr, "expected 101 response, got: %s\n", response);
        return 1;
    }

    char accept[128];
    if (!header_value(response, "Sec-WebSocket-Accept", accept, sizeof(accept))) {
        fprintf(stderr, "missing Sec-WebSocket-Accept header\n");
        return 1;
    }

    const char *magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    char concat[128];
    snprintf(concat, sizeof(concat), "%s%s", key, magic);

    unsigned char digest[20];
    sha1((unsigned char *)concat, strlen(concat), digest);

    char expected[128];
    base64_encode(digest, sizeof(digest), expected, sizeof(expected));

    if (strcmp(accept, expected) != 0) {
        fprintf(stderr, "invalid Sec-WebSocket-Accept: expected %s got %s\n", expected, accept);
        return 1;
    }

    return 0;
}
