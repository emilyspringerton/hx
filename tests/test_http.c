#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET socket_t;
#define close_socket closesocket
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
typedef int socket_t;
#define close_socket close
#endif

static void usage(const char *prog) {
    fprintf(stderr, "usage: %s --host HOST --port PORT --path PATH --expect-status CODE\n", prog);
}

static socket_t connect_host(const char *host, const char *port) {
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    struct addrinfo *rp = NULL;
    socket_t sock = (socket_t)-1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) != 0) {
        return -1;
    }

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock == (socket_t)-1) {
            continue;
        }
        if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }
        close_socket(sock);
        sock = (socket_t)-1;
    }

    freeaddrinfo(res);
    return sock;
}

int main(int argc, char **argv) {
    const char *host = NULL;
    const char *port = NULL;
    const char *path = NULL;
    int expected_status = -1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--host") == 0 && i + 1 < argc) {
            host = argv[++i];
        } else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = argv[++i];
        } else if (strcmp(argv[i], "--path") == 0 && i + 1 < argc) {
            path = argv[++i];
        } else if (strcmp(argv[i], "--expect-status") == 0 && i + 1 < argc) {
            expected_status = atoi(argv[++i]);
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    if (!host || !port || !path || expected_status < 0) {
        usage(argv[0]);
        return 1;
    }

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "failed to initialize winsock\n");
        return 1;
    }
#endif

    socket_t sock = connect_host(host, port);
    if (sock == (socket_t)-1) {
        fprintf(stderr, "failed to connect to %s:%s\n", host, port);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    char request[1024];
    int req_len = snprintf(request, sizeof(request),
                           "GET %s HTTP/1.1\r\nHost: %s:%s\r\nConnection: close\r\n\r\n",
                           path, host, port);
    if (req_len <= 0 || req_len >= (int)sizeof(request)) {
        fprintf(stderr, "request too long\n");
#ifdef _WIN32
        WSACleanup();
#endif
        close_socket(sock);
        return 1;
    }

    if (send(sock, request, req_len, 0) != req_len) {
        perror("send");
#ifdef _WIN32
        WSACleanup();
#endif
        close_socket(sock);
        return 1;
    }

    char response[4096];
    int total = 0;
    while ((size_t)total < sizeof(response) - 1) {
        int n = recv(sock, response + total, (int)(sizeof(response) - 1 - (size_t)total), 0);
        if (n <= 0) {
            break;
        }
        total += n;
        if (strstr(response, "\r\n\r\n") != NULL) {
            break;
        }
    }

    response[total] = '\0';
    close_socket(sock);
#ifdef _WIN32
    WSACleanup();
#endif

    if (strncmp(response, "HTTP/1.1", 8) != 0) {
        fprintf(stderr, "invalid HTTP response: %s\n", response);
        return 1;
    }

    char *status_start = strchr(response, ' ');
    if (!status_start) {
        fprintf(stderr, "unable to parse status line\n");
        return 1;
    }

    int status = atoi(status_start + 1);
    if (status != expected_status) {
        fprintf(stderr, "expected status %d, got %d\n", expected_status, status);
        return 1;
    }

    return 0;
}
