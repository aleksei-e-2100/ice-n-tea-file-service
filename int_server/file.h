#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "timer.h"

class File
{
public:
    File();

    ~File();

    // Не допускаем перемещение и копирование
    File(File&) = delete;
    File& operator=(File&) = delete;
    File(File&&) = delete;
    File& operator=(File&&) = delete;

public:
    unsigned char* inPtr();

    unsigned char* outPtr();

    bool procData(uint32_t len);

    void startTimer();

    bool checkComplete();

    void dropFile();

    bool getMsg(std::string& msg);

private:
    bool procFirst();

    bool procAnother();

private:
    FILE* _file = nullptr;
    std::string _fileName = "";

    uint32_t _nextSeqNum = 0;
    uint32_t _seqTotal = 0;

    std::vector<unsigned char> _inBuf;
    uint32_t _inOffcet = 0;

    std::vector<unsigned char> _outBuf;

    Timer _timer;
    const int _packetTime = 3000;

    bool _isComlete = false;

    std::string _msg;
};
