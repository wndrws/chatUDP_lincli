//
// Created by user on 16.10.16.
//
#include "etcp.h"

void error(int status, int err, const char* fmt, ... ) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "%s: ", program_name);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    if(err) {
        fprintf(stderr, ": %s (%d)\n", strerror(err), err);
    }
    if(status) {
        EXIT(status);
    }
}