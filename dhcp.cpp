#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <map>
#include <sstream>
#include "dhcp_errors.h"

class Network {
public:
    explicit Network(const std::string &name, const std::string &pass, const std::string &vip_base = "177.177.0.")
            : name(name), password(pass), vip_base(vip_base) {
    }
    std::string add_peer(const std::string &pass, const std::string &ip) {
        if (pass != password) {
            return "";
        }
        std::string vip = generate_vip();
        vip_table[vip] = ip;

        //open_connection(ip);         // вызов функции/еще чего из части с неблокирующими сокетами

        return vip;
    }
    void remove_peer(const std::string &ip) {
        //close_connection(ip);

        // напишу, когда согласуем архитектуру
    }

private:
    const std::string name;
    const std::string password;
    unsigned short int MAX_NUM = 254;
    unsigned short int ip_number = 2;
    unsigned short int ip_count = 0;
    const std::string vip_base;
    std::map<std::string, std::string> vip_table;   // после подключения multiclient будет во втором поле хранить дескрипторы
    std::string generate_vip() {
        if (ip_count == MAX_NUM) {
            return "NET_FULL";
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

  std::map<std::string, Network*> network_list;

  listen(s, 2);

  unsigned short int net_number = 170;  // чтобы ip создаваемых сетей различались
                                        // пока что выход за 255 не обрабатывается

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
                if (vip == "NET_FULL") {
                    std::cout << "Connection error: network is full." << std::endl;
                    send(ss, (void *)NET_FULL, sizeof(NET_FULL), 0);
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
                send(ss, (void *)PASSWD_INC, sizeof(PASSWD_INC), 0);
                close(ss);
            }
        } else {
            std::cout << "Connection error: network doesn't exist." << std::endl;
            send(ss, (void *)NET_NAME_INC, sizeof(NET_NAME_INC), 0);
            close(ss);
        }
    } else {
        std::cout << "Peer requests network creation." << std::endl;
        if(network_list.find(network) != network_list.end()) {
            std::cout << "Network creation error: network already exists." << std::endl;
            send(ss, (void *)NET_NAME_TAKEN, sizeof(NET_NAME_TAKEN), 0);
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


}

