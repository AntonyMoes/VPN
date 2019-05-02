cmake_minimum_required(VERSION 3.10)
project(naas)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

file(GLOB SERVER_SRCS
        "src/server/*.cpp"
        )

file(GLOB SERVER_HDRS
        "include/server/*.h"
        )

file(GLOB CLIENT_SRCS
        "src/client/*.cpp"
        )

file(GLOB CLIENT_HDRS
        "include/client/*.h"
        )

file(GLOB SHARED_SRCS
        "src/*.cpp"
        )

file(GLOB SHARED_HDRS
        "include/*.h"
        )

add_executable(server server.cpp ${SERVER_SRCS} ${SERVER_HDRS} ${SHARED_SRCS} ${SHARED_HDRS})
target_link_libraries(server pthread)

add_executable(client client.cpp ${CLIENT_SRCS} ${CLIENT_HDRS} ${SHARED_SRCS} ${SHARED_HDRS})
target_link_libraries(client pthread)
