/**
 * @file timer.cpp
 * @brief One-shot timer
 */

#include "timer.h"

#include <errno.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <cstdint>

Timer::Timer()
{
    timerFd_ = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);

    if (timerFd_ == -1)
        errno_ = errno;
}

Timer::~Timer()
{
    if (timerFd_ != -1)
    {
        close(timerFd_);
        timerFd_ = -1;
    }
}

Timer::Timer(Timer&& other)
{
    timerFd_ = other.timerFd_;
    other.timerFd_ = -1;
}

Timer& Timer::operator=(Timer&& other)
{
    if (this != &other)
    {
        if (timerFd_ != -1)
            close(timerFd_);

        timerFd_ = other.timerFd_;
        other.timerFd_ = -1;
    }

    return *this;
}

int Timer::getTimerFd()
{
    return timerFd_;
}

bool Timer::oneShot(int ms)
{
    struct itimerspec its;
    its.it_value.tv_sec = ms / 1000;
    its.it_value.tv_nsec = (ms % 1000) * 1000000;
    its.it_interval = {0, 0};

    int res = timerfd_settime(timerFd_, 0, &its, nullptr);

    if (res != 0)
    {
        errno_ = errno;
        return false;
    }

    errno_ = 0;

    return true;
}

int Timer::accept()
{
    uint64_t value;

    int res = read(timerFd_, &value, sizeof(value));

    if (res != sizeof(value))
    {
        errno_ = errno;
        return false;
    }

    errno_ = 0;

    return value;
}

int Timer::getLastErrno() const
{
    return errno_;
}
