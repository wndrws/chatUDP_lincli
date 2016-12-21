#include "etcp.h"

// Прочесть len байтов из сокета sock и записать по указателю bp.
int readn(SOCKET sock, char* bp, int len) {
    return ureadn(sock, bp, len);
}

// Прочесть из сокета sock запись переменной длины и записать по
// по указателю bp. Максимальный размер буфера для записи равен len.
// Максимальный размер записи определяется типом RECLEN_TYPE.
int readvrec(SOCKET sock, char* bp, int len) {
    return ureadvrec(sock, bp, len);
}

// Прочесть из сокета sock одну строку (до '\n') по указателю bufptr.
// Максимальный размер буфера для записи равен len.
int readline(SOCKET sock, char* bufptr, int len) {
    return ureadline(sock, bufptr, len);
}