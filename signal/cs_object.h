#pragma once

#include <signal/cs_signal.h>
#include <signal/cs_slot.h>
#include <iostream>
#include "blockingconcurrentqueue.h"
#include <pthread.h>
#include <stdint.h>

class CSThread
{
public:
    CSThread();
    ~CSThread();

public:
    static void msleep(uint32_t mseconds);
    static pid_t getCurrentThreadId();

public:
    void setObjectName(std::string name);
    pid_t getThreadId();

    virtual void start(int wait_time = 5);

    void run_inner();

    virtual void run();

    void wait();

    void quit();
    virtual void stop();

    bool isRunning();

    void join();

public:
    moodycamel::BlockingConcurrentQueue<std::shared_ptr<CsSignal::PendingSlot>> array_;
    std::shared_ptr<std::thread> runner_;
    std::atomic<bool> stop_;
    std::atomic<bool> run_;
    int32_t wait_time_;
    std::string thread_name_;
    std::atomic<bool>   started_;
    pid_t   thread_id_;
};

class CSObject: public CsSignal::SignalBase, public CsSignal::SlotBase
{
public:
    CSObject(CSObject* parent = nullptr);
    ~CSObject();

public:
      SIGNAL_1(void finished())
      SIGNAL_2(finished)

public:
    void moveToThread(CSThread* pt);
    CSObject* getParent();
    CSThread* thread();

    virtual bool isMovetoThread();

protected:
    virtual void put_data(std::shared_ptr<CsSignal::PendingSlot> data);

    virtual void queueSlot(std::shared_ptr<CsSignal::PendingSlot> data, CsSignal::ConnectionKind type);

private:
    moodycamel::BlockingConcurrentQueue<std::shared_ptr<CsSignal::PendingSlot>> *array_;
    std::atomic<bool>   m_b_moveto_thread;
    CSThread*           thread_;
    CSObject*           parent_;
};

class CSCoreApplication: public CSObject
{
public:
    CSCoreApplication();
    ~CSCoreApplication();

public:
    void exec();

    void exit();

protected:
    virtual void put_data(std::shared_ptr<CsSignal::PendingSlot> data);

private:
    moodycamel::BlockingConcurrentQueue<std::shared_ptr<CsSignal::PendingSlot>> core_array_;

    std::atomic<bool>  stop_;
};

class CSTimer : public CSThread, public CSObject
{
public:
    CSTimer();
    ~CSTimer();

    virtual void stop();

public:
      SIGNAL_1(void timeout())
      SIGNAL_2(timeout)

      virtual void run();
private:
      moodycamel::BlockingConcurrentQueue<int32_t> notify_;
};
