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

    ~File() = default;

    // Не допускаем перемещение и копирование
    File(File&) = delete;
    File& operator=(File&) = delete;
    File(File&&) = delete;
    File& operator=(File&&) = delete;

public:
    unsigned char* inPtr();

    unsigned char* outPtr();

    bool prepare(const std::string& filePath);

    bool procPacket();

    void procAnswer(uint32_t len);

    void startTimer();

    bool checkComplete();

    bool getMsg(std::string& msg);

private:
    void closeFile();

private:
    FILE* _file = nullptr;

    uint32_t _nextSeqNum = 0;
    uint32_t _seqTotal = 0;

    uint32_t _nextSeqConfirm = 0;

    std::vector<unsigned char> _inBuf;
    uint32_t _inOffcet = 0;

    std::vector<unsigned char> _outBuf;

    Timer _timer;
    const int _ansTime = 3000;

    bool _isComlete = false;

    std::string _msg;
};
