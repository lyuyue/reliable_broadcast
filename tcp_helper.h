#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <time.h>

void set_timeout(int sockfd) {
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt() error");
    }

    return;
}