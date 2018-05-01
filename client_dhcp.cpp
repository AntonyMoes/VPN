#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <map>
#include <sstream>

const char *SERVER_ADDR = "192.168.1.58";  // mb std::string
const int SERVER_PORT = 12345;
const int BUFF_SIZE = 4* 256;

int connect_to_server(const std::string &ifname, const std::string &remote_ip);

void create_tun(const std::string &vip) {
    std::string syscall = "./tun.sh ";
    syscall += "vpn_tun";
    syscall += " ";
    syscall += vip;
    system(syscall.c_str());
}

int main() {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    in_addr in;
    int res = inet_aton(SERVER_ADDR, &in);
    if (!res) {
        return -2;
    }

    sockaddr_in sockaddr_ = {
            .sin_family = AF_INET,
            .sin_port = htons(SERVER_PORT),
            .sin_addr = in
    };

    res = bind(s, (sockaddr*) &sockaddr_, sizeof(sockaddr_));
    if (res) {
        std::cout << std::strerror(errno);
        return -1;
    }

    // Формирование запроса
    // -----------------------------------------

    std::string request_type;
    std::string net_name;
    std::string net_pass;
    std::cin >> request_type >> net_name >> net_pass;  // пока пусть будет так

    std::string request = request_type + " " + net_name + " " + net_pass;

    // -----------------------------------------

    connect(s, (sockaddr*) &sockaddr_, sizeof(sockaddr_));
    send(s, (void *)request.c_str(), sizeof(request.c_str()), 0);

    char buff[BUFF_SIZE];
    int recv_bytes = recv(s, buff, BUFF_SIZE, 0);
    std::stringstream dhcp_input(buff);

    std::string vip;
    dhcp_input >> vip;
    if (vip == "ERR") {
        std::string error_type;
        dhcp_input >> error_type;
        std::cout << "Error occured: " << error_type << std::endl;
        return 55;
    }

    create_tun(vip);



    //вызов симплтана
    connect_to_server("vpn_tun", SERVER_ADDR);



    return 0;
}
