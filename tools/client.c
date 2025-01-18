#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "client.h"

int ctl_query(const char *socket_path, const char *cmd, sb_t *out)
{
    struct sockaddr_un addr;

    if (strlen(socket_path) >= sizeof addr.sun_path) {
        errno = ENAMETOOLONG;
        return -1;
    }
    memset(&addr, 0, sizeof addr);
    addr.sun_family = AF_UNIX;
    memcpy(addr.sun_path, socket_path, strlen(socket_path) + 1);

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;
    if (connect(fd, (struct sockaddr *)&addr, sizeof addr) < 0) {
        int e = errno;
        close(fd);
        errno = e;
        return -1;
    }

    size_t len = strlen(cmd);
    if (write(fd, cmd, len) != (ssize_t)len || write(fd, "\n", 1) != 1) {
        int e = errno;
        close(fd);
        errno = e;
        return -1;
    }

    char buf[4096];
    ssize_t n;
    while ((n = read(fd, buf, sizeof buf)) > 0)
        sb_append(out, buf, (size_t)n);
    close(fd);
    return 0;
}
