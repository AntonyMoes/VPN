//
// Created by antonymo on 03.05.18.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <map>
#include <sstream>
#include "../include/dhcp_errors.h"
#include "../include/Network.h"

#include <net/if.h>
#include "../include/rwr.hpp"
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <set>
#include <algorithm>
#include <thread>
//#include "../include/nonblock.hpp"

#define BUFSIZE 1500
#define PORT 55555

int debug = 1;

int shut = false;

int foo() {

    //DHCP_SETUP



    int dhcp_s = socket(AF_INET, SOCK_STREAM, 0);
    in_addr in;
    int res = inet_aton("0.0.0.0", &in);
    if (!res) {
        return -2;
    }

    sockaddr_in sockaddr_ = {
            .sin_family = AF_INET,
            .sin_port = htons(12345),
            .sin_addr = in
    };

    res = bind(dhcp_s, (sockaddr*) &sockaddr_, sizeof(sockaddr_));
    if (res) {
        std::cout << std::strerror(errno);
        return -1;
    }

    std::map<std::string, Network*> network_list;

    listen(dhcp_s, 5);

    unsigned short int net_number = 170;  // чтобы ip создаваемых сетей различались
    // пока что выход за 255 не обрабатывается



    while(true)  {
        sockaddr_in  peer_addr;
        socklen_t peer_addr_size;
        int ss = accept(dhcp_s, (sockaddr *)  &peer_addr, &peer_addr_size);


        char str[INET_ADDRSTRLEN];
        inet_ntop( AF_INET, &peer_addr, str, INET_ADDRSTRLEN );
        std::string ip = str;

        std::cout << "Recieved request from " << ip << std::endl;

        char buff[4 * 1024];
        int recv_bytes = recv(ss, buff, 4 * 1024, 0);
        std::stringstream input(buff);
        std::string request_type;
        std::string network;
        std::string password;

        input >> request_type >> network >> password;
        input.clear();

        std::string vip;
        if(request_type == "shutdown") {
            shut = true;
            break;
        } else if(request_type == "connect") {
            std::cout << "Peer requests connection." << std::endl;
            std::cout << "Trying to connect to \"" << network << "\" network..." << std::endl;
            if(network_list.find(network) != network_list.end()) {
                vip = network_list[network]->add_peer(password, ip);
                if(!vip.empty()) {
                    if (vip == "NET_FULL") {
                        std::cout << "Connection error: network is full." << std::endl;
                        send(ss, (void *)NET_FULL, strlen(NET_FULL), 0);
                        close(ss);
                    } else {
                        std::cout << "Peer successfully added." << std::endl;
                        std::cout << "Generated VIP is: " << vip << std::endl;
                        std::cout << "Returning vip to the peer." << std::endl;
                        send(ss, (void *) vip.c_str(), vip.size(), 0);
                        close(ss);
                    }
                } else {
                    std::cout << "Connection error: password incorrect." << std::endl;
                    send(ss, (void *)PASSWD_INC, strlen(PASSWD_INC), 0);
                    close(ss);
                }
            } else {
                std::cout << "Connection error: network doesn't exist." << std::endl;
                send(ss, (void *)NET_NAME_INC, strlen(NET_NAME_INC), 0);
                close(ss);
            }
        } else {
            std::cout << "Peer requests network creation." << std::endl;
            if(network_list.find(network) != network_list.end()) {
                std::cout << "Network creation error: network \"" << network << "\" already exists." << std::endl;
                send(ss, (void *)NET_NAME_TAKEN, strlen(NET_NAME_TAKEN), 0);
                close(ss);
            } else {
                network_list[network] = new Network(network, password, "170." + std::to_string(net_number++) + ".0.");
                std::cout << "Network \"" << network << "\" successfully created." << std::endl;
                vip = network_list[network]->add_peer(password, ip);
                std::cout << "Generated VIP is: " << vip << std::endl;
                std::cout << "Returning vip to the peer." << std::endl;
                send(ss, (void *) vip.c_str(), vip.size(), 0);
                close(ss);
            }
        }

        std::cout << std::endl;
    }



    for (auto iter: network_list) {
        delete iter.second;
    }







}

int main() {
    int block = 0;
    int tap_fd, option;
    int flags = IFF_TUN;
    char if_name[IFNAMSIZ] = "";
    char buffer[BUFSIZE];
    sockaddr_in local, remote;
    char remote_ip[16] = "";            /* dotted quad IP string */
    unsigned short int port = PORT;
    int sock_fd, optval = 1;
    socklen_t remotelen;
    unsigned long int tap2net = 0, net2tap = 0;

    if ( (sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(1);
    }


    if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) < 0) {
        perror("setsockopt()");
        exit(1);
    }

    fcntl(sock_fd, F_SETFL, O_NONBLOCK);
    int net_fd;
    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(port);
    if (bind(sock_fd, (struct sockaddr*) &local, sizeof(local)) < 0) {
        perror("bind()");
        exit(1);
    }
    my_err((char*)"SERVER OPENED\n");
    if (listen(sock_fd, 5) < 0) {
        perror("listen()");
        exit(1);
    }

    std::set<int> clients;
    clients.clear();


    std::thread th(foo);

    //CYCLE

    while(!shut){
        /* wait for connection request */
        remotelen = sizeof(remote);
        memset(&remote, 0, remotelen);
        fd_set readset;
        FD_ZERO(&readset);
        FD_SET(sock_fd, &readset);
        for(auto it: clients) {
            FD_SET(it, &readset);
        }







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
            do_debug(debug, (char*)"SERVER: Client connected from %s\n", inet_ntoa(remote.sin_addr));
            fcntl(net_fd, F_SETFL, O_NONBLOCK);
            clients.insert(net_fd);
        }
        do_debug(debug, (char*)"clients: %i\n", clients.size());
        for(auto it: clients) {
            if(FD_ISSET(it, &readset)) {
                //send smth
            }
        }
    }

    th.join();



    // END
    close(sock_fd);
    for(std::set<int>::iterator it = clients.begin(); it != clients.end(); it++) {
        close(*it);
    }



    return 0;
}