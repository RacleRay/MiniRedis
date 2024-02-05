#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <arpa/inet.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <unistd.h>


static void die(const char *msg);


static void die(const char *msg) {
    int err = errno;
    (void)fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}


int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);
    int rv = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (rv != 0) {
        die("connect()");
    }

    char msg[] = "Hello, server!";
    write(fd, msg, strlen(msg));

    char rbuf[64] = {};
    ssize_t n = read(fd, rbuf, sizeof(rbuf) - 1);
    if (n < 0) {
        die("read()");
    }

    printf("[server says]: %s\n", rbuf);
    close(fd);

    return 0;
}