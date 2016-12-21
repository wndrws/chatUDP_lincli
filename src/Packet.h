#pragma once

//#include <chrono>
#include <string>
#include <cstring>
#include <set>

using namespace std;

#define MAX_BUF_SIZE 1433

class Packet {
    //chrono::time_point<> timestamp;
    string str_TimeStamp;
    char data[MAX_BUF_SIZE];
public:
    Packet(string ts, char* buf) : str_TimeStamp(ts) { strcpy(data, buf); };
    string getTimeStamp() const { return str_TimeStamp; };
    const char* getDataPointer() const { return data; };

    bool operator<(const Packet& y) const {
        return this->getTimeStamp() < y.getTimeStamp();
    }
};