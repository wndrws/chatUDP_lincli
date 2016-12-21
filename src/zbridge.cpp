#include <unordered_map>
#include <chrono>
#include "Data.h"
#include "etcp.h"
#define RECLEN_TYPE uint16_t

extern bool blockingMode;
extern Data dataFromServer;

// Прочесть len байтов из дейтаграммы для aid и записать по указателю bp.
int ureadn(SOCKET s, char* bp, int len) {
    if(dataFromServer.empty()) {
        char buf[MAX_BUF_SIZE];
#ifdef USE_CONNECTED_CLIENT
        int rcvdb = recv(s, buf, sizeof(buf)-1, 0);
#else
        int rcvdb = recvfrom(s, buf, sizeof(buf)-1, 0, NULL, NULL); // no check for sender validity
#endif
        if(rcvdb < 0) {
            if(blockingMode) fprintf(stderr, "Error while trying to receive datagram in ureadn()!");
            return -1;
        }
        buf[rcvdb] = '\0';
        dataFromServer.addPacket(buf);
    }

    size_t size;
    char dgram[MAX_BUF_SIZE];
    char* ptr;

    size = dataFromServer.getDataArrayCopy(dgram);
    ptr = dataFromServer.getCurrentDataPointer();

    // Assuming all len bytes are in a single datagram
    int cnt = 0;
    while(cnt < len && cnt < size) {
        *bp++ = *ptr++;
        cnt++;
    }
    if(cnt == size) fprintf(stderr, "Datagram end reached in ureadn()!\n");

//    int cnt, rc;
//
//    cnt = len;
//    while(cnt > 0) {
//        //rc = recv(sock, bp, cnt, 0);
//        if(rc < 0) {                // Ошибка чтения?..
//            if(errno == EINTR)      // Или вызов был прерван?
//                continue;           // Повторить чтение
//            return -1;              // Вернуть код ошибки
//        }
//        if(rc == 0) {               // Если ничего не принято,
//            return len - cnt;       // вернуть неполный счетчик.
//        }
//        bp += rc;
//        cnt -= rc;
//    }
    dataFromServer.setCurrentDataPointer(ptr);
    return len;
}

// Прочесть из дейтаграммы для aid запись переменной длины и записать по
// по указателю bp. Максимальный размер буфера для записи равен len.
// Максимальный размер записи определяется типом RECLEN_TYPE.
int ureadvrec(SOCKET s, char* bp, int len) {
    RECLEN_TYPE reclen;
    int rc;

    // Прочитать длину записи:
    rc = ureadn(s, (char*) &reclen, sizeof(RECLEN_TYPE));
    // Проверка, что считалось верное число байт:
    if(rc != sizeof(RECLEN_TYPE)) return rc < 0 ? -1 : 0;
    switch (sizeof(RECLEN_TYPE)) {
        case 2: reclen = ntohs(reclen); break; //uint16_t
        case 4: reclen = ntohl(reclen); break; //uint32_t
        default: break;
    }
    if(reclen > len) {
        // В буфере не хватает места для размещения данных -
        // сохраняем, что влезает, а остальное отбрасываем.
        while(reclen > 0) {
            rc = ureadn(s, bp, len);
            if(rc != len) return rc < 0 ? -1 : 0;
            reclen -= len;
            if(reclen < len) len = reclen;
        }
        set_errno(EMSGSIZE);
        return -1;
    }
    // Если всё в порядке, читаем запись:
    rc = ureadn(s, bp, reclen);
    if(rc != reclen) return rc < 0 ? -1 : 0;
    return rc;
}

// Прочесть из дейтаграммы для aid одну строку (до '\n') по указателю bufptr.
// Максимальный размер буфера для записи равен len.
int ureadline(SOCKET s, char* bufptr, int len) {
    char* bufx = bufptr;
    //static char* bp;
    //static int cnt = 0;
    //static char b[65536];
    //char c;

    if(dataFromServer.empty()) {
        char buf[MAX_BUF_SIZE];
#ifdef USE_CONNECTED_CLIENT
        int rcvdb = recv(s, buf, sizeof(buf)-1, 0);
#else
        int rcvdb = recvfrom(s, buf, sizeof(buf)-1, 0, NULL, NULL); // no check for sender validity
#endif
        if(rcvdb < 0) {
            fprintf(stderr, "Error while trying to receive datagram in ureadn()!");
            return -1;
        }
        buf[rcvdb] = '\0';
        dataFromServer.addPacket(buf);
    }

    size_t size;
    char dgram[MAX_BUF_SIZE];
    char* ptr;

    size = dataFromServer.getDataArrayCopy(dgram);
    ptr = dataFromServer.getCurrentDataPointer();

    int cnt = 0;
    while(*ptr != '\n' && *ptr != '\0' && cnt < size && cnt < len) {
        *bufx++ = *ptr++;
        cnt++;
    }
    if(cnt == len && *ptr != '\n') fprintf(stderr, "Line is too big for buffer size provided to ureadline()!\n");
    if(*ptr == '\0')
        fprintf(stderr, "Symbol '\\0' met before '\\n' in ureadline()!\n");
    *bufx = '\n';
    if(cnt == size) fprintf(stderr, "Info: Datagram end reached in ureadline()!\n");
    if(cnt < len) *(++bufx) = '\0';
    dataFromServer.setCurrentDataPointer(++ptr);
    return (int) (bufx - bufptr);
//    while (--len > 0) {
//        cnt = recv(aid, &c, 1, 0);
//        if (cnt < 0) {
//            if (errno == EINTR) {
//                len++; /* Уменьшим на 1 в заголовке while. */
//                continue;
//            }
//            return -1;
//        }
//        if (cnt == 0) return 0;
//        *bufptr++ = c;
//        if (c == '\n') {
//            *bufptr = '\0';
//            return (int) (bufptr - bufx);
//        }
//    }fа

//    Snader:
//    while (--len > 0) {
//        if(--cnt <= 0) {
//            cnt = recv(sock, b, sizeof(b), 0);
//            if(cnt < 0) {
//                if(errno == EINTR) {
//                    len++; /* Уменьшим на 1 в заголовке while. */
//                    continue;
//                }
//                return -1;
//            }
//            if(cnt == 0) return 0;
//            bp = b;
//        }
//        c = *bp++;
//        *bufptr++ = c;
//        if(c == '\n') {
//            *bufptr = '\0';
//            return (int) (bufptr - bufx);
//        }
//    }

//    set_errno(EMSGSIZE);
//    return -1;
}

addr_id makeAddrID(const struct sockaddr_in *sap) {
    return ((unsigned long long) sap->sin_addr.s_addr << 8*sizeof(uint16_t) + sap->sin_port);
}

sockaddr_in unMakeAddrID(addr_id aid) {
    uint16_t port = (uint16_t) (aid % 0x10000);
    uint32_t addr = (uint32_t) (aid >> 8*sizeof(u_short));
    sockaddr_in peer;
    bzero(&peer, sizeof(peer));
    peer.sin_addr.s_addr = addr;
    peer.sin_port = port;
    peer.sin_family = AF_INET;
    return peer;
}

int usendto(SOCKET sock, struct sockaddr_in peer, const char* msg) {
    // Add timestamp to the message
    chrono::time_point<chrono::system_clock> now = chrono::system_clock::now();
    string fullmsg = to_string(chrono::duration_cast<chrono::microseconds>(now.time_since_epoch()).count());
    fullmsg.append("\n").append(msg);
    //sockaddr_in peer = unMakeAddrID(aid);
#ifdef USE_CONNECTED_CLIENT
    return send(sock, fullmsg.c_str(), (int) fullmsg.length(), 0);
#else
    return sendto(sock, fullmsg.c_str(), (int) fullmsg.length(), 0, (sockaddr*) &peer, sizeof(peer));
#endif
}