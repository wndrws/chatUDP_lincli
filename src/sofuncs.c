//
// Created by user on 16.10.16.
//

#include "etcp.h"

SOCKET udp_server(char *hostname, char *portname) {
    struct sockaddr_in local;
    SOCKET s;

    set_address(hostname, portname, &local, "udp");
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if(!isvalidsock(s))
        error(1, errno, "Error in socket() call", strerror(errno));
    if(bind(s, (struct sockaddr*) &local, sizeof(local)))
        error(1, errno, "Error in bind() call", strerror(errno));
    return s;
}

SOCKET udp_client(char *hostname, char *portname, struct sockaddr_in* peer) {
    SOCKET s;

    set_address(hostname, portname, peer, "udp");
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if(!isvalidsock(s))
        error(1, errno, "Error in socket() call", strerror(errno));
    return s;
}

SOCKET udp_connected_client(char *hostname, char *portname, struct sockaddr_in* peer) {
    SOCKET s;

    set_address(hostname, portname, peer, "udp");
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if(!isvalidsock(s))
        error(1, errno, "Error in socket() call", strerror(errno));
    if(connect(s, (struct sockaddr*) peer, sizeof(*peer)))
        error(1, errno, "Error in connect() call: %s", strerror(errno));
    return s;
}

static void set_address(char* hostname, char* portname,
                             struct sockaddr_in *sap, char* protocol) {
    struct servent *sp;
    struct hostent *hp;
    char* endptr;
    uint16_t port;

    bzero(sap, sizeof(*sap));
    sap->sin_family = AF_INET;
    if(hostname != NULL) {
        if(!inet_aton(hostname, &sap->sin_addr)) {
            hp = gethostbyname(hostname);
            if(hp == NULL)
                error(1, 0, "Unknown host: %s\n", hostname);
            else sap->sin_addr = *(struct in_addr*)hp->h_addr;
        }
    } else sap->sin_addr.s_addr = htonl(INADDR_ANY);
    port = strtol(portname, &endptr, 0);
    if(*endptr == '\0') sap->sin_port = htons(port);
    else {
        sp = getservbyname(portname, protocol);
        if(sp == NULL)
            error(1, 0, "Unknown port: %s\n", portname);
        else sap->sin_port = sp->s_port;
    }
}