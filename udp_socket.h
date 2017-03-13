#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

struct addr_info_node {
    int id;                 // id of host
    struct addrinfo data;   // addrinfo of the host
    struct addr_info_node *next;    // next info node
}