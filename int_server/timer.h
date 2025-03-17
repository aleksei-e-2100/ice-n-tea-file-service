#pragma once

struct Timer
{
public:
    Timer();

    ~Timer();

    // Не допускаем копирование
    Timer(const Timer&) = delete;
    void operator=(const Timer&) = delete;

    // Допускаем перемещение
    Timer(Timer&&);
    Timer& operator=(Timer&& other);

public:
    bool oneShot(int msec);

    bool periodic(int msec);

    bool stop();

    int accept();

    int getErrCode() const;

private:
    bool changeTimer(int msec, bool periodic);

private:
    int _timerFd = -1;

    int _errno = 0;
};
