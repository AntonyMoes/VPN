#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring>
//#include <zconf.h>
#include <unistd.h>
#include <map>
#include <sstream>

class Network {
public:
    Network(const std::string &str)
            : password(str) {
    }
    std::string add_peer(const std::string &pass, const std::string &ip) {
        if (pass != password) {
            return "";
        }
        std::string vip = generate_vip();
        vip_table[vip] = ip;
        create_tun(vip, ip); // подумаем-с
        return vip;
    }
    void remove_peer(const std::string &ip) {
        //implement
    }

private:
    const std::string password;
    unsigned short int MAX_NUM = 254;
    unsigned short int ip_number = 2;
    unsigned short int ip_count = 0;
    const std::string vip_base = "177.177.0.";
    std::map<std::string, std::string> vip_table;
    std::string generate_vip() {
        if (ip_count == MAX_NUM) {
            return "";
        }

        while (!vip_table[vip_base  + std::to_string(ip_number)].empty()) {
            if (ip_number == MAX_NUM + 1) {
                ip_number = 2;
            } else {
                ip_number++;
            }

        }

        ip_count++;

        return vip_base + std::to_string(ip_number);
    }
    void create_tun(const std::string &vip,const std::string &ip) {
        //implement
    }
};

int main() {
  int s = socket(AF_INET, SOCK_STREAM, 0);

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

  res = bind(s, (sockaddr*) &sockaddr_, sizeof(sockaddr_));
  if (res) {
    std::cout << std::strerror(errno);
    return -1;
  }
  std::map<in_addr, in_addr> vip_table;

  std::map<std::string, Network*> network_list;

  listen(s, 2);

  while (true) {
    sockaddr_in  peer_addr;
    socklen_t peer_addr_size;
    int ss = accept(s, (struct sockaddr *)  &peer_addr, &peer_addr_size);


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
    if(request_type == "connect") {
        std::cout << "Peer requests connection." << std::endl;
        std::cout << "Trying to connect to \"" << network << "\" network..." << std::endl;
        if(network_list.find(network) != network_list.end()) {
            vip = network_list[network]->add_peer(password, ip);
            if(!vip.empty()) {
                std::cout << "Peer successfully added." << std::endl;
                std::cout << "Generated VIP is: " << vip << std::endl;
                std::cout << "Returning vip to the peer." << std::endl;
                send(ss, (void *)vip.c_str(), vip.size(), 0);
                close(ss);
            } else {
                std::cout << "Connection error: password incorrect." << std::endl;
                send(ss, (void *)"INC_PASSWD", sizeof("INC_PASSWD"), 0);
                close(ss);
            }
        } else {
            std::cout << "Connection error: network doesn't exist." << std::endl;
            send(ss, (void *)"INC_NETWORK", sizeof("INC_NETWORK"), 0);
            close(ss);
        }
    } else {
        std::cout << "Peer requests network creation." << std::endl;
        if(network_list.find(network) != network_list.end()) {
            std::cout << "Network creation error: network already exists." << std::endl;
            send(ss, (void *)"INC_NETWORK", sizeof("INC_NETWORK"), 0);
            close(ss);
        }
        network_list[network] = new Network(password);
        std::cout << "Network \"" << network << "\" successfully created." << std::endl;
        vip = network_list[network]->add_peer(password, ip);
        std::cout << "Generated VIP is: " << vip << std::endl;
        std::cout << "Returning vip to the peer." << std::endl;
        send(ss, (void *)vip.c_str(), vip.size(), 0);
        close(ss);
    }

    std::cout << std::endl;
  }
}
