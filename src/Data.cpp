#include "Data.h"

Data::Data(char *udp_data) {
    addPacket(udp_data);
    cur_byte = (char*) packets.cbegin()->getDataPointer();
}

char* Data::getCurrentDataPointer() {
    return cur_byte;
}

size_t Data::getDataArrayCopy(char bufToWrite[]) {
    strcpy(bufToWrite, packets.cbegin()->getDataPointer());
    size_t size = strlen(bufToWrite);
    return size;
}

void Data::addPacket(char *udp_data) {
    char* p = udp_data;
    while(*p != '\n') { ++p; }
    unsigned long long ts_length = p - udp_data;
    string ts(udp_data, (unsigned int) ts_length);
    bool wasEmpty = packets.empty();
    packets.insert(Packet(ts, p+1));
    if(wasEmpty) {
        cur_byte = (char*) packets.cbegin()->getDataPointer();
    }
}

void Data::setCurrentDataPointer(char *p) {
    cur_byte = p;
    if (*cur_byte == '\0') {
        packets.erase(packets.begin());
        if (!packets.empty()) cur_byte = (char *) packets.cbegin()->getDataPointer();
        else cur_byte = NULL;
    }
}

bool Data::empty() { return packets.empty(); }
