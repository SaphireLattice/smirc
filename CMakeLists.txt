cmake_minimum_required(VERSION 3.6)
project(smirc)

set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu11 -Werror -Wall -Wno-long-long -pedantic")
set(CMAKE_C_STANDARD 11)

find_package(OpenSSL REQUIRED)

set(COMMON_SOURCES
        utils.c
        config.c
        config.h)
set(IRC_SOURCES
        irc/irc_server.c
        irc/irc_client.c
        irc/commands.c)
set(MUD_SOURCES
        mud/client.c
        mud/telnet.c
        mud/mcp.c
        ssl.c)
set(SMIRC
        main.c
        ${IRC_SOURCES}
        ${MUD_SOURCES}
        ${COMMON_SOURCES})

add_executable(smirc ${SMIRC})
target_link_libraries(smirc
        pthread
        ${OPENSSL_LIBRARIES})

add_executable(colortest
        test_progs/colortest.c
        mud/telnet.c
        ${COMMON_SOURCES})

add_executable(configtest
        test_progs/configtest.c
        ${COMMON_SOURCES})

#add_executable(mudtest
#        mud/main.c
#        ${MUD_SOURCES}
#        ${COMMON_SOURCES})

#target_compile_definitions(mudtest PRIVATE STANDALONE_MUD)
#target_link_libraries(mudtest
#        pthread
#        ${OPENSSL_LIBRARIES})