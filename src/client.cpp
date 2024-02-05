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
}


static int32_t query(int fd, const char* text) {
    uint32_t len = (uint32_t)strlen(text);
    if (len > MAX_MSG_LEN) {
        return -1;
    }

    char wbuf[4 + MAX_MSG_LEN];
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], text, len);
    if (int32_t err = write_all(fd, wbuf, 4 + len)) {
        return err;
    }

    char rbuf[4 + MAX_MSG_LEN + 1];
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4);
    if (err) {
        if (errno == 0) {
            msg("EOF");
        } else {
            msg("read_full()");
        }
        return err;
    }

    memcpy(&len, rbuf, 4);
    if (len > MAX_MSG_LEN) {
        msg("message too long");
        return -1;
    }

    err = read_full(fd, &rbuf[4], len);
    if (err) {
        msg("read_full() error");
        return err;
    }

    rbuf[4 + len] = '\0';
    printf("[server says]: %s\n", &rbuf[4]);

    return 0;
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

    int32_t err = query(fd, "Hello, server!");
    if (err) {
        goto L_DONE;
    }

    err = query(fd, "How are you?");
    if (err) {
        goto L_DONE;
    }

    err = query(fd, "Bye!");
    if (err) {
        goto L_DONE;
    }

L_DONE:
    close(fd);
    return 0;

    // char msg[] = "Hello, server!";
    // write(fd, msg, strlen(msg));

    // char rbuf[64] = {};
    // ssize_t n = read(fd, rbuf, sizeof(rbuf) - 1);
    // if (n < 0) {
    //     die("read()");
    // }

    // printf("[server says]: %s\n", rbuf);
    // close(fd);

    // return 0;
}
