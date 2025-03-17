#pragma once

#include <cstdint>

enum class PacketStart : uint32_t
{
    Put = 0xFFFFFFFF,
    Ans = 0x1F1F1F1F
};

const uint32_t packetEnd = 0xAFAFAFAF;

const uint32_t packetPutLen = 1400;

constexpr uint32_t dataLen = packetPutLen - sizeof(uint32_t) * 5;

constexpr uint32_t packetAnsLen = sizeof(uint32_t) * 4;

#pragma pack(push, 1)
struct PacketPut
{
    uint32_t start;
    uint32_t seqNum;
    uint32_t seqTotal;
    uint32_t payload;
    unsigned char data[dataLen];
    uint32_t end;
};

struct PacketAns
{
    uint32_t start;
    uint32_t seqNum;
    uint32_t seqTotal;
    uint32_t end;
};
#pragma pack(pop)
