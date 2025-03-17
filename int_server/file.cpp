#include "file.h"

#include <cstring>

#include "packet.h"

File::File()
{
    _inBuf.resize(packetPutLen * 2);
    _outBuf.resize(packetAnsLen);

    PacketAns* packet = reinterpret_cast<PacketAns*>(_outBuf.data());

    packet->start = static_cast<uint32_t>(PacketStart::Ans);
    packet->end = static_cast<uint32_t>(packetEnd);
}

File::~File()
{
    if (_file != nullptr)
    {
        fclose(_file);
        _file = nullptr;
    }
}

unsigned char* File::inPtr()
{
    return _inBuf.data() + _inOffcet;
}

unsigned char* File::outPtr()
{
    return _outBuf.data();
}

bool File::procData(uint32_t len)
{
    // Оставшаяся длина пакета
    auto remainLen = packetPutLen - _inOffcet;

    // Если пришел неполный пакет (начало или продолжение, но не до конца)
    if (len < remainLen)
    {
        _inOffcet += len;

        return false;
    }

    // Если пришло окончание пакета или пакет целиком
    _inOffcet = 0;

    PacketPut* packet = reinterpret_cast<PacketPut*>(_inBuf.data());

    if (_seqTotal == 0)
    {
        if (!procFirst())
            return false;
    }
    else if (!procAnother())
    {
        return false;
    }

    // Если кроме окончания пакета пришла еще часть следующего пакета
    if (len > remainLen)
    {
        std::memcpy(
            _inBuf.data(), _inBuf.data() + packetPutLen, len - remainLen);

        _inOffcet = len - remainLen;
    }

    return true;
}

void File::startTimer()
{
    _timer.oneShot(_packetTime);
}

bool File::checkComplete()
{
    if (_timer.accept() == 0)
        return false;

    if (!_isComlete)
    {
        dropFile();

        _isComlete = true;

        _msg = "Transmission error";
    }

    return _isComlete;
}

void File::dropFile()
{
    if (_file != nullptr)
    {
        fclose(_file);
        _file = nullptr;
    }

    remove(_fileName.c_str());
}

bool File::getMsg(std::string& msg)
{
    if (_msg.empty())
        return false;

    msg = std::move(_msg);

    return true;
}

bool File::procFirst()
{
    _msg.clear();

    PacketPut* packet = reinterpret_cast<PacketPut*>(_inBuf.data());

    if (packet->start != static_cast<uint32_t>(PacketStart::Put) ||
        packet->seqNum != 0 ||
        packet->seqTotal == 0 ||
        packet->payload == 0 ||
        packet->payload > 50 ||
        packet->end != packetEnd)
    {
        _isComlete = true;

        _msg = "Invalid first packet received";

        return false;
    }

    std::string fileName(
        reinterpret_cast<char*>(&packet->data), packet->payload);

    _file = fopen64(fileName.c_str(), "w");

    if (_file == nullptr)
    {
        _isComlete = true;

        _msg = "Error creating file to save received data";

        return false;
    }

    _seqTotal = packet->seqTotal;

    ++_nextSeqNum;

    reinterpret_cast<PacketAns*>(_outBuf.data())->seqTotal = packet->seqTotal;
    reinterpret_cast<PacketAns*>(_outBuf.data())->seqNum = packet->seqNum;

    _msg = "Receiving started";

    return true;
}

bool File::procAnother()
{
    _msg.clear();

    PacketPut* packet = reinterpret_cast<PacketPut*>(_inBuf.data());

    if (packet->start != static_cast<uint32_t>(PacketStart::Put) ||
        packet->seqNum != _nextSeqNum ||
        packet->seqTotal != _seqTotal ||
        packet->payload == 0 ||
        packet->payload > dataLen ||
        packet->end != packetEnd)
    {
        dropFile();

        _isComlete = true;

        _msg = "Invalid packet received";

        return false;
    }

    auto res = fwrite_unlocked(packet->data, packet->payload, 1, _file);

    if (res != 1)
    {
        dropFile();

        _isComlete = true;

        _msg = "Error saving received data";

        return false;
    }

    if (_nextSeqNum == _seqTotal / 5)
    {
        _msg = "Received 20%";
    }
    else if (_nextSeqNum == _seqTotal * 2 / 5)
    {
        _msg = "Received 40%";
    }
    else if (_nextSeqNum == _seqTotal * 3 / 5)
    {
        _msg = "Received 60%";
    }
    else if (_nextSeqNum == _seqTotal * 4 / 5)
    {
        _msg = "Received 80%";
    }
    else if (_nextSeqNum == _seqTotal)
    {
        _msg = "Received 100%. Receiving finished";

        _isComlete = true;

        fclose(_file);
        _file = nullptr;
    }

    ++_nextSeqNum;

    reinterpret_cast<PacketAns*>(_outBuf.data())->seqNum = packet->seqNum;

    return true;
}
