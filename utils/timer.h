/**
 * @file timer.h
 * @brief One-shot timer
 */

#pragma once

/**
 * @class Timer
 * @brief Implements one-shot timer
 */
class Timer
{
public:
    /**
     * @brief Constructor
     */
    Timer();

    /**
     * @brief Destructor
     */
    ~Timer();

    // Copy is prohibited
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

    /**
     * @brief Move constructor
     * @param [in, out] other The source object
     */
    Timer(Timer&& other);

    /**
     * @brief Move assignment operator
     * @param [in, out] other The source object
     * @return A reference to the current object
     */
    Timer& operator=(Timer&& other);

    /**
     * @brief Get the timer descriptor
     * @return The timer file descriptor
     */
    int getTimerFd();

    /**
     * @brief Start a one-shot timer
     * @param [in] ms Interval in milliseconds
     * @return true - if started successfully
     */
    bool oneShot(int ms);

    /**
     * @brief Read the number of timer triggers
     * @return The number of triggers
     */
    int accept();

    /**
     * @brief Get the last error code
     * @return The value of errno
     */
    int getLastErrno() const;

private:
    int timerFd_ = -1;

    int errno_ = 0;
};
