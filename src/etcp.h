//
// Created by user on 15.10.16.
//

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "skel.h"

#ifdef __SVR4
#define bzero(b,n) memset( ( b ), 0, ( n ) )
#endif

void error(int, int, const char *, ...);
int readn(SOCKET, char *, int);
int readvrec(SOCKET, char *, int);
//int readcrlf(SOCKET, char*, size_t);
int readline(SOCKET, char*, int);
//int tcp_server(char *, char *);
//int tcp_client(char *, char *);
SOCKET udp_server(char*, char*);
SOCKET udp_client(char*, char*, struct sockaddr_in*);
SOCKET udp_connected_client(char*, char*, struct sockaddr_in*);
static void set_address(char *, char *, struct sockaddr_in *, char *);

#ifdef __cplusplus
}
#endif

int ureadn(SOCKET s, char* bp, int len);
int ureadvrec(SOCKET s, char* bp, int len);
int ureadline(SOCKET s, char* bufptr, int len);
int usendto(SOCKET sock, struct sockaddr_in peer, const char* msg);
addr_id makeAddrID(const struct sockaddr_in *sap);
//sockaddr_in unMakeAddrID(addr_id aid);

#define USE_CONNECTED_CLIENT