#include "timer.h"

#include <errno.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include <cstdint>

Timer::Timer()
{
    _timerFd =
        timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);

    if (_timerFd == -1)
        _errno = errno;
}

Timer::~Timer()
{
    if (_timerFd != -1)
    {
        close(_timerFd);
        _timerFd = -1;
    }
}

Timer::Timer(Timer&& other)
{
    _timerFd = other._timerFd;

    other._timerFd = -1;
}

Timer& Timer::operator=(Timer&& other)
{
    if (this != &other)
    {
        if (_timerFd != -1)
            close(_timerFd);

        _timerFd = other._timerFd;

        other._timerFd = -1;
    }

    return *this;
}

bool Timer::oneShot(int msec)
{
    return changeTimer(msec, false);
}

bool Timer::periodic(int msec)
{
    return changeTimer(msec, true);
}

bool Timer::stop()
{
    return changeTimer(0, false);
}

int Timer::accept()
{
    uint64_t buf;

    int res = read(_timerFd, &buf, sizeof(buf));

    if (res != sizeof(buf))
    {
        _errno = errno;

        return 0;
    }

    _errno = 0;

    return buf;
}

int Timer::getErrCode() const
{
    return _errno;
}

bool Timer::changeTimer(int msec, bool periodic)
{
    struct itimerspec its;

    its.it_value.tv_sec = msec / 1000;
    its.it_value.tv_nsec = (msec % 1000) * 1000000;

    its.it_interval = periodic ? its.it_value : (struct timespec){ 0 };

    int res = timerfd_settime(_timerFd, 0, &its, NULL);

    if (res != 0)
    {
        _errno = errno;

        return false;
    }

    _errno = 0;

    return true;
}
