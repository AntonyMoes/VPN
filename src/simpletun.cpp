#include <net/if.h>
#include "rwr.hpp"
#include <string.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <set>
#include <algorithm>
#include "nonblock.hpp"

/* buffer for reading from tun/tap interface, must be >= 1500 */
#define BUFSIZE 1500
#define CLIENT 0
#define SERVER 1
#define PORT 55555

int debug;
char *progname;

int main(int argc, char *argv[]) {
    int block;
    int tap_fd, option;
    int flags = IFF_TUN;
    char if_name[IFNAMSIZ] = "";
    char buffer[BUFSIZE];
    struct sockaddr_in local, remote;
    char remote_ip[16] = "";            /* dotted quad IP string */
    unsigned short int port = PORT;
    int sock_fd, net_fd, optval = 1;
    socklen_t remotelen;
    int cliserv = -1;    /* must be specified on cmd line */
    unsigned long int tap2net = 0, net2tap = 0;

    progname = argv[0];

    /* Check command line options */
    while((option = getopt(argc, argv, "hd:bn")) > 0) {
        switch(option) {
            case 'd':
            debug = 1;
            break;
            case 'h':
            usage(progname);
            break;
            case 'b':
            block = 1;
            break;
            case 'n':
            block = 0;
            break;
            default:
            my_err("Unknown option %c\n", option);
            usage(progname);
        }
    }



    if ( (sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(1);
    }


    /* Server, wait for connections */

    /* avoid EADDRINUSE error on bind() */
    if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) < 0) {
        perror("setsockopt()");
        exit(1);
    }

    serve(sock_fd, local, debug, port, remote, remotelen);

    /* use select() to handle two descriptors at once */
    return(0);
}
