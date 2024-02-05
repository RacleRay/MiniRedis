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
static void msg(const char *msg);
static void do_something(int connfd);


static void die(const char *msg) {
    int err = errno;
    (void)fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

static void msg(const char *msg) {
    (void)fprintf(stderr, "%s\n", msg);
}


static void do_something(int connfd) {
    char rbuf[64] = {};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
    if (n < 0) {
        msg("read() error");
        return;
    }
    printf("[client says]: %s\n", rbuf);

    char wbuf[] = "hello client";
    write(connfd, wbuf, sizeof(wbuf)); 
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");        
    }

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0);
    int rv = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (rv != 0) {
        die("bind()");
    }

    // listen
    rv = listen(fd, SOMAXCONN);
    if (rv) {
        die("listen()");
    }

    // connetions
    while (true) {
        // accept
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
        if (connfd < 0) {
            continue;   // error at connection
        }

        do_something(connfd);

        close(connfd);
    }

    return 0;
}