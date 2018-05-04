#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <string.h>
#include <set>
#include <algorithm>
#include "../include/rwr.hpp"
#include "../include/nonblock.hpp"


void serve(int sock_fd, sockaddr_in local, int debug, int port, sockaddr_in remote, socklen_t remotelen) {
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
     my_err("SERVER OPENED\n");
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
             do_debug(debug, "SERVER: Client connected from %s\n", inet_ntoa(remote.sin_addr));
             fcntl(net_fd, F_SETFL, O_NONBLOCK);
             clients.insert(net_fd);
         }
         do_debug(debug, "clients: %i\n", clients.size());
         for(std::set<int>::iterator it = clients.begin(); it != clients.end(); it++) {
             if(FD_ISSET(*it, &readset)) {
                 //send smth
             }
         }
     }

     close(sock_fd);
     for(std::set<int>::iterator it = clients.begin(); it != clients.end(); it++) {
         close(*it);
     }

}
