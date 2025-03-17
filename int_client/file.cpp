#include "file.h"

#include <cstring>

#include "packet.h"

File::File()
{
    _inBuf.resize(packetAnsLen * 2);
    _outBuf.resize(packetPutLen);

    PacketPut* packet = reinterpret_cast<PacketPut*>(_outBuf.data());

    packet->start = static_cast<uint32_t>(PacketStart::Put);
    packet->end = static_cast<uint32_t>(packetEnd);
}

unsigned char* File::inPtr()
{
    return _inBuf.data() + _inOffcet;
}

unsigned char* File::outPtr()
{
    return _outBuf.data();
}

bool File::prepare(const std::string& filePath)
{
    _msg.clear();

    _file = fopen64(filePath.c_str(), "r");

    if (_file == nullptr)
    {
        _msg = "File open error";

        return false;
    }

    std::string fileName = filePath.substr(filePath.find_last_of('/') + 1);

    fseeko64(_file, 0, SEEK_END);

    uint64_t fileLen = ftello64(_file);

    fseeko64(_file, 0, SEEK_SET);

    _seqTotal = fileLen / dataLen;

    if (fileLen % dataLen != 0)
        ++_seqTotal;

    PacketPut* packet = reinterpret_cast<PacketPut*>(_outBuf.data());

    packet->seqTotal = _seqTotal;
    packet->seqNum = 0;
    packet->payload = fileName.length();

    std::memcpy(
        packet->data, fileName.c_str(), fileName.length());

    ++_nextSeqNum;

    return true;
}

bool File::procPacket()
{
    _msg.clear();

    if (_nextSeqNum > _seqTotal)
        return false;

    PacketPut* packet = reinterpret_cast<PacketPut*>(_outBuf.data());

    auto res = fread(packet->data, 1, dataLen, _file);

    if (ferror(_file))
    {
        _isComlete = true;

        closeFile();

        _msg = "File read error";

        return false;
    }

    if (feof(_file))
        closeFile();

    packet->seqNum = _nextSeqNum;
    ++_nextSeqNum;

    packet->payload = res;

    return true;
}

#include <iostream> // -----------------------------------
void File::procAnswer(uint32_t len)
{
    _msg.clear();

    // Оставшаяся длина пакета
    auto remainLen = packetAnsLen - _inOffcet;

    // Если пришел неполный пакет (начало или продолжение, но не до конца)
    if (len < remainLen)
    {
        _inOffcet += len;

        return;
    }

    // Если пришло окончание пакета или пакет целиком
    _inOffcet = 0;

    // Разбираем поступивший ответ
    PacketAns* packet = reinterpret_cast<PacketAns*>(_inBuf.data());

    if (packet->start != static_cast<uint32_t>(PacketStart::Ans) ||
        packet->seqNum != _nextSeqConfirm ||
        packet->seqTotal != _seqTotal ||
        packet->end != packetEnd)
    {
        // --------------------------
        if (packet->start != static_cast<uint32_t>(PacketStart::Ans))
            std::cout << "start " << packet->start << std::endl;
        if (packet->seqNum != _nextSeqConfirm)
            std::cout << "seqNum " << packet->seqNum << std::endl;
        if (packet->seqTotal != _seqTotal)
            std::cout << "seqTotal " << packet->seqTotal << std::endl;
        if (packet->end != packetEnd)
            std::cout << "end " << packet->end << std::endl;
        // --------------------------

        _isComlete = true;

        closeFile();

        _msg = "Invalid answer received";

        return;
    }

    if (_nextSeqConfirm == 0)
    {
        _msg = "Transmission started";
    }
    else if (_nextSeqConfirm == _seqTotal / 5)
    {
        _msg = "Transmited 20%";
    }
    else if (_nextSeqConfirm == _seqTotal * 2 / 5)
    {
        _msg = "Transmited 40%";
    }
    else if (_nextSeqConfirm == _seqTotal * 3 / 5)
    {
        _msg = "Transmited 60%";
    }
    else if (_nextSeqConfirm == _seqTotal * 4 / 5)
    {
        _msg = "Transmited 80%";
    }
    else if (_nextSeqConfirm == _seqTotal)
    {
        _msg = "Transmited 100%. Transmission finished";

        _isComlete = true;
    }

    // Если кроме окончания пакета пришла еще часть следующего пакета
    if (len > remainLen)
    {
        std::memcpy(
            _inBuf.data(), _inBuf.data() + packetAnsLen, len - remainLen);

        _inOffcet = len - remainLen;
    }

    ++_nextSeqConfirm;
}

void File::startTimer()
{
    _timer.oneShot(_ansTime);
}

bool File::checkComplete()
{
    if (!_isComlete && _timer.accept() > 0)
    {
        _isComlete = true;

        closeFile();

        _msg = "Transmission error";
    }

    return _isComlete;
}

bool File::getMsg(std::string& msg)
{
    if (_msg.empty())
        return false;

    msg = std::move(_msg);

    return true;
}

void File::closeFile()
{
    if (_file != nullptr)
    {
        fclose(_file);
        _file = nullptr;
    }
}
