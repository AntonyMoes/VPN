//
// Created by antonymo on 03.05.18.
//

#include "../include/Network.h"

std::string Network::generate_vip() {
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
std::string Network::add_peer(const std::string &pass, const std::string &ip) {
    if (pass != password) {
        return "";
    }
    std::string vip = generate_vip();
    vip_table[vip] = ip;

    //open_connection(ip);         // вызов функции/еще чего из части с неблокирующими сокетами

    return vip;
}
void Network::remove_peer(const std::string &ip) {
    //close_connection(ip);

    // напишу, когда согласуем архитектуру
}