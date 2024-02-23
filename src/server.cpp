#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <arpa/inet.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <unistd.h>


const size_t MAX_MSG_LEN = 4096;


static void die(const char *msg);
static void msg(const char *msg);
static void do_something(int connfd);

static int32_t read_full(int fd, char *buf, size_t len);
static int32_t write_all(int fd, const char *buf, size_t len);

static int32_t one_request(int connfd);


static void die(const char *msg) {
    int err = errno;
    (void)fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

static void msg(const char *msg) {
    (void)fprintf(stderr, "%s\n", msg);
}


static int32_t read_full(int fd, char *buf, size_t len) {
    while (len > 0) {
        ssize_t rv = read(fd, buf, len);
        if (rv <= 0) {
            return -1;
        }
        assert((size_t)rv <= len);
        len -= (size_t)rv;
        buf += rv;
    }
    return 0;
}


static int32_t write_all(int fd, const char *buf, size_t len) {
    while (len > 0) {
        ssize_t rv = write(fd, buf, len);
        if (rv <= 0) {
            return -1;
        }
        assert((size_t)rv <= len);
        len -= (size_t)rv;
        buf += rv;
    }
    return 0;
}


static int32_t one_request(int connfd) {
    char rbuf[4 + MAX_MSG_LEN + 1] = {};
    errno = 0;

    int32_t err = read_full(connfd, rbuf, 4);
    if (err) {
        if (errno == 0) {
            msg("EOF");
        } else {
            msg("read() error");
        }
        return err;
    }

    // read length
    uint32_t len = 0;
    memcpy(&len, rbuf, 4);
    if (len > MAX_MSG_LEN) {
        msg("message too long");
        return -1;
    }

    // read message at length
    err = read_full(connfd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    rbuf[4 + len] = '\0';
    printf("[client says]: %s\n", &rbuf[4]);

    //=================================================================================
    // reply
    const char reply[] = "hello client";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);

    return write_all(connfd, wbuf, 4 + len);
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

        // do_something(connfd);
        while (true) {
            int32_t err = one_request(connfd);
            if (err) {
                break;
            }
        }

        close(connfd);
    }

    return 0;
}