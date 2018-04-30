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

/* buffer for reading from tun/tap interface, must be >= 1500 */
#define BUFSIZE 1500
#define CLIENT 0
#define SERVER 1
#define PORT 55555

int debug;
char *progname;

/**************************************************************************
* tun_alloc: allocates or reconnects to a tun/tap device. The caller     *
*            must reserve enough space in *dev.                          *
**************************************************************************/
int tun_alloc(char *dev, int flags) {

    struct ifreq ifr;
    int fd, err;
    char *clonedev = "/dev/net/tun";

    if( (fd = open(clonedev , O_RDWR)) < 0 ) {
        perror("Opening /dev/net/tun");
        return fd;
    }

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = flags;

    if (*dev) {
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    if( (err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 ) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        return err;
    }

    strcpy(dev, ifr.ifr_name);

    return fd;
}

void cli_conn(int tap_fd, int net_fd, char* buffer) {
    int maxfd = (tap_fd > net_fd)?tap_fd:net_fd;
    int tap2net = 0;
    int net2tap = 0;
    uint16_t plength = 0;
    uint16_t nread = 0;
    uint16_t nwrite = 0;
    while(1) {
        int ret;
        fd_set rd_set;

        FD_ZERO(&rd_set);
        FD_SET(tap_fd, &rd_set); FD_SET(net_fd, &rd_set);

        ret = select(maxfd + 1, &rd_set, NULL, NULL, NULL);

        if (ret < 0 && errno == EAGAIN){
            continue;
        }

        if (ret < 0) {
            perror("select()");
            exit(1);
        }

        if(FD_ISSET(tap_fd, &rd_set)) {
            /* data from tun/tap: just read it and write it to the network */

            nread = cread(tap_fd, buffer, BUFSIZE);

            tap2net++;
            do_debug(debug, "TAP2NET %lu: Read %d bytes from the tap interface\n", tap2net, nread);

            /* write length + packet */
            plength = htons(nread);
            nwrite = cwrite(net_fd, (char *)&plength, sizeof(plength));

            nwrite = cwrite(net_fd, buffer, nread);

            do_debug(debug, "TAP2NET %lu: Written %d bytes to the network\n", tap2net, nwrite);
        }

        if(FD_ISSET(net_fd, &rd_set)) {
            /* data from the network: read it, and write it to the tun/tap interface.
            * We need to read the length first, and then the packet */

            /* Read length */
            nread = read_n(net_fd, (char *)&plength, sizeof(plength));
            if(nread == 0) {
                /* ctrl-c at the other end */
                continue;
            }

            net2tap++;

            /* read packet */
            nread = read_n(net_fd, buffer, ntohs(plength));
            do_debug(debug, "NET2TAP %lu: Read %d bytes from the network\n", net2tap, nread);

            /* now buffer[] contains a full packet or frame, write it into the tun/tap interface */
            nwrite = cwrite(tap_fd, buffer, nread);
            do_debug(debug, "NET2TAP %lu: Written %d bytes to the tap interface\n", net2tap, nwrite);
        }
    }
}

int ser_conn(int tap_fd, int net_fd, char* buffer, unsigned long &tap2net, unsigned long &net2tap) {
    int maxfd = (tap_fd > net_fd)?tap_fd:net_fd;
    uint16_t plength = 0;
    uint16_t nread = 0;
    uint16_t nwrite = 0;

    int ret;
    int errsv;
    fd_set rd_set;

    FD_ZERO(&rd_set);
    FD_SET(tap_fd, &rd_set); FD_SET(net_fd, &rd_set);

    ret = select(maxfd + 1, &rd_set, NULL, NULL, NULL);

    if (ret < 0 && (errno == EINTR || errno == EAGAIN)){
        return 1;
    }

    if (ret < 0) {
        perror("select()");
        exit(1);
    }

    if(FD_ISSET(tap_fd, &rd_set)) {
        /* data from tun/tap: just read it and write it to the network */

        nread = cread(tap_fd, buffer, BUFSIZE);
        errsv = errno;
        if (errsv == EAGAIN) {return 1;}
        tap2net++;
        do_debug(debug, "TAP2NET %lu: Read %d bytes from the tap interface\n", tap2net, nread);

        /* write length + packet */
        plength = htons(nread);
        nwrite = cwrite(net_fd, (char *)&plength, sizeof(plength));

        nwrite = cwrite(net_fd, buffer, nread);

        do_debug(debug, "TAP2NET %lu: Written %d bytes to the network\n", tap2net, nwrite);
    }

    if(FD_ISSET(net_fd, &rd_set)) {
        /* data from the network: read it, and write it to the tun/tap interface.
        * We need to read the length first, and then the packet */

        /* Read length */
        nread = read_n(net_fd, (char *)&plength, sizeof(plength));
        if(nread == 0) {
            /* ctrl-c at the other end */
            return 0;
        }

        net2tap++;

        /* read packet */
        nread = read_n(net_fd, buffer, ntohs(plength));
        do_debug(debug, "NET2TAP %lu: Read %d bytes from the network\n", net2tap, nread);
        if (errno == EAGAIN) {return 0;}
        /* now buffer[] contains a full packet or frame, write it into the tun/tap interface */
        nwrite = cwrite(tap_fd, buffer, nread);
        do_debug(debug, "NET2TAP %lu: Written %d bytes to the tap interface\n", net2tap, nwrite);
    }
    return 1;
}

int main(int argc, char *argv[]) {
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
    while((option = getopt(argc, argv, "i:sc:p:uahd")) > 0) {
        switch(option) {
            case 'd':
            debug = 1;
            break;
            case 'h':
            usage(progname);
            break;
            case 'i':
            strncpy(if_name,optarg, IFNAMSIZ-1);
            break;
            case 's':
            cliserv = SERVER;
            break;
            case 'c':
            cliserv = CLIENT;
            strncpy(remote_ip,optarg,15);
            break;
            case 'p':
            port = atoi(optarg);
            break;
            case 'u':
            flags = IFF_TUN;
            break;
            case 'a':
            flags = IFF_TAP;
            break;
            default:
            my_err("Unknown option %c\n", option);
            usage(progname);
        }
    }

    argv += optind;
    argc -= optind;

    if(argc > 0) {
        my_err("Too many options!\n");
        usage(progname);
    }

    if(*if_name == '\0') {
        my_err("Must specify interface name!\n");
        usage(progname);
    } else if(cliserv < 0) {
        my_err("Must specify client or server mode!\n");
        usage(progname);
    } else if((cliserv == CLIENT)&&(*remote_ip == '\0')) {
        my_err("Must specify server address!\n");
        usage(progname);
    }

    /* initialize tun/tap interface */
    if ( (tap_fd = tun_alloc(if_name, flags | IFF_NO_PI)) < 0 ) {
        my_err("Error connecting to tun/tap interface %s!\n", if_name);
        exit(1);
    }

    do_debug(debug, "Successfully connected to interface %s\n", if_name);

    if ( (sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(1);
    }

    if(cliserv == CLIENT) {
        /* Client, try to connect to server */

        /* assign the destination address */
        memset(&remote, 0, sizeof(remote));
        remote.sin_family = AF_INET;
        remote.sin_addr.s_addr = inet_addr(remote_ip);
        remote.sin_port = htons(port);

        /* connection request */
        if (connect(sock_fd, (struct sockaddr*) &remote, sizeof(remote)) < 0) {
            perror("connect()");
            exit(1);
        }
        //int bytesReceived = recv(sock_fd, buf, 5, 0);

        net_fd = sock_fd;
        do_debug(debug, "CLIENT: Connected to server %s\n", inet_ntoa(remote.sin_addr));
        cli_conn(tap_fd, net_fd,buffer);
    } else {
        /* Server, wait for connections */

        /* avoid EADDRINUSE error on bind() */
        if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) < 0) {
            perror("setsockopt()");
            exit(1);
        }
        fcntl(sock_fd, F_SETFL, O_NONBLOCK);
        fcntl(tap_fd, F_SETFL, O_NONBLOCK);
        memset(&local, 0, sizeof(local));
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = htonl(INADDR_ANY);
        local.sin_port = htons(port);
        if (bind(sock_fd, (struct sockaddr*) &local, sizeof(local)) < 0) {
            perror("bind()");
            exit(1);
        }

        if (listen(sock_fd, 5) < 0) {
            perror("listen()");
            exit(1);
        }

        std::set<int> clients;
        clients.clear();
        while(1){
            /* wait for connection request */
            remotelen = sizeof(remote);
            memset(&remote, 0, remotelen);
            fd_set readset;
            FD_ZERO(&readset);
            FD_SET(sock_fd, &readset);
            for(std::set<int>::iterator it = clients.begin(); it != clients.end(); it++)
            FD_SET(*it, &readset);
            timeval timeout;
            timeout.tv_sec = 15;
            timeout.tv_usec = 0;
            int mx = std::max(sock_fd, *max_element(clients.begin(), clients.end()));
            if(select(mx+1, &readset, NULL, NULL, NULL) <= 0)
            {
                perror("select");
                exit(3);
            }
            if(FD_ISSET(sock_fd, &readset)) {

                if ((net_fd = accept(sock_fd, (struct sockaddr*)&remote, &remotelen)) < 0) {
                    perror("accept()");
                    exit(1);
                }
                do_debug(debug, "SERVER: Client connected from %s\n", inet_ntoa(remote.sin_addr));
                fcntl(net_fd, F_SETFL, O_NONBLOCK);
                clients.insert(net_fd);
            }
            do_debug(debug, "clients: %i\n", clients.size());
            for(std::set<int>::iterator it = clients.begin(); it != clients.end(); it++) {
                if(FD_ISSET(*it, &readset)) {
                    if(ser_conn(tap_fd, *it, buffer, tap2net, net2tap) == 0) {
                        close(*it);
                        clients.erase(*it);
                        continue;
                    }
                }
            }
        }

        /* use select() to handle two descriptors at once */


    }
    return(0);
}
