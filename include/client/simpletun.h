#ifndef NAAS_SIMPLETUN_H
#define NAAS_SIMPLETUN_H

#include <string>

int connect_to_server(const std::string &ifname, const std::string &remote_ip, int sock_fd, int tap_fd);

#endif //NAAS_SIMPLETUN_H
