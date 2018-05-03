#ifndef INCLUDE_NONBLOCK_HPP_
#define INCLUDE_NONBLOCK_HPP_

void serve(int sock_fd, sockaddr_in local, int debug, int port, sockaddr_in remote, socklen_t remotelen);

#endif // 
