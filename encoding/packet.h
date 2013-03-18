#ifndef _PACKET_
#define _PACKET_
#define MAX_PAYLOAD_PRINT 20

#define TYPE_DATA 1
#define TYPE_CONTROL 2

typedef struct payload_t {
    int size;
    uint8_t* data;
} payload;

typedef struct coeffs_t {
    uint8_t n;
    uint32_t start1; // Start sequence number
    uint8_t* alpha;  // Coefficient for the n-th segment
    uint16_t* start; // Starting sequence number for the 1+ packets ; relative to previous packet
    uint16_t* size;  // Real size of the n-th segment (can be padded with zeros )
    uint8_t* hdrSize;  // headers (IP and TCP) size in the n-th segment
} coeffs;

typedef struct encodedpacket_t {
    coeffs* coeffs;
    payload* payload;
} encodedpacket;

typedef struct clearpacket_t {
    int indexStart;
    uint8_t hdrSize;
    payload* payload;
} clearpacket;

typedef struct clearpacketarray_t {
    int nPackets;
    clearpacket** packets;
} clearpacketarray;

typedef struct encodedpacketarray_t {
    int nPackets;
    encodedpacket** packets;
} encodedpacketarray;

void payloadPrint(payload p);

void encodedPacketPrint(encodedpacket packet);

void clearPacketPrint(clearpacket packet);

clearpacket* clearPacketCreate(uint32_t index, uint16_t size, uint8_t hdrSize, uint8_t* data);

void clearPacketFree(clearpacket* p);

payload* payloadCreate(int size, uint8_t* data);

void encodedPacketFree(encodedpacket* p);

void payloadFree(payload* p);

void encodedArrayFree(encodedpacketarray* a);

void clearArrayFree(clearpacketarray* a);

void encodedArrayAppend(encodedpacketarray* a, encodedpacket* p);

void clearArrayAppend(clearpacketarray* a, clearpacket* p);

void clearArrayRemove(clearpacketarray* a, int index);

encodedpacket* encodedPacketCopy(encodedpacket p);

#endif
