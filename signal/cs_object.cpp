#include "cs_object.h"
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/prctl.h>

CSThread::CSThread()
{
    started_ = false;
    run_ = false;
    stop_ = false;
}

CSThread::~CSThread()
{}

void CSThread::msleep(uint32_t mseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(mseconds));
}

pid_t CSThread::getCurrentThreadId()
{
    return static_cast<pid_t>(::syscall(SYS_gettid));
}

void CSThread::setObjectName(std::string name)
{
    thread_name_ = name;
}

pid_t CSThread::getThreadId()
{
    return thread_id_;
}

void CSThread::start(int wait_time) //millisecond
{
    wait_time_ = wait_time;

    if (!started_) {
        started_ = true;
        stop_ = false;
        runner_.reset(new std::thread([this]{this->run_inner();}));
    }
}

void CSThread::join()
{
    runner_->join();
}

void CSThread::run_inner()
{
    thread_id_ = static_cast<pid_t>(::syscall(SYS_gettid));
    run_ = true;

    ::prctl(PR_SET_NAME, thread_name_.c_str());

    run();

    run_ = false;
}

void CSThread::run()
{
    int64_t t = wait_time_ * 1000;
    while (!stop_) {
        std::shared_ptr<CsSignal::PendingSlot> data;
        if (array_.wait_dequeue_timed(data, t)) {
            (*data)();
        }
    }
}

void CSThread::wait()
{}

void CSThread::quit()
{
    stop();
    started_ = false;
    runner_->join();
}

void CSThread::stop()
{
    stop_ = true;
}

bool CSThread::isRunning()
{
    return run_;
}


CSObject::CSObject(CSObject* parent)
    :parent_(parent), m_b_moveto_thread(false), thread_(nullptr)
{
    CSObject* tmp = parent_;
    CSObject* lastParent = nullptr;
    while (tmp) {
        lastParent = tmp;
        if (lastParent->isMovetoThread()) {
            break;
        }

        tmp = tmp->getParent();
    }

    if (lastParent) {
        moveToThread(lastParent->thread());
    }
}

CSObject::~CSObject()
{
    finished();
}

void CSObject::moveToThread(CSThread* pt)
{
    if (pt) {
        thread_ = pt;
        m_b_moveto_thread = true;
        array_ = &(pt->array_);
    }
}

CSObject* CSObject::getParent()
{
    return parent_;
}

CSThread* CSObject::thread()
{
    return thread_;
}

bool CSObject::isMovetoThread()
{
    return m_b_moveto_thread;
}


void CSObject::put_data(std::shared_ptr<CsSignal::PendingSlot> data)
{
    array_->enqueue(data);
}

void CSObject::queueSlot(std::shared_ptr<CsSignal::PendingSlot> data, CsSignal::ConnectionKind type)
{
   if (!m_b_moveto_thread) {
       CSObject* tmp = getParent();
       CSObject* lastParent = nullptr;
       while (tmp) {
            lastParent = tmp;
            if (lastParent->isMovetoThread()) {
               break;
            }
            tmp = tmp->getParent();
       }

       if (lastParent) {
            lastParent->put_data(data);
       } else {
           CsSignal::SlotBase::queueSlot(data, type);
       }
   } else {
       put_data(data);
   }
}

CSCoreApplication::CSCoreApplication()
{}

CSCoreApplication::~CSCoreApplication()
{}

void CSCoreApplication::exec()
{
    int64_t t = 5 * 1000;
    stop_ = false;
    while (!stop_) {
        std::shared_ptr<CsSignal::PendingSlot> data;
        if (core_array_.wait_dequeue_timed(data, t)) {
            (*data)();
        }
    }
}

void CSCoreApplication::exit()
{
    stop_ = true;
}

void CSCoreApplication::put_data(std::shared_ptr<CsSignal::PendingSlot> data)
{
    core_array_.enqueue(data);
}

CSTimer::CSTimer()
{}

CSTimer::~CSTimer()
{
    stop_ = true;
    notify_.enqueue(0);
    runner_->join();
}

void CSTimer::stop()
{}

void CSTimer::run()
{
      int64_t t = wait_time_ * 1000;
      while (!stop_) {
          int32_t tmp;
          if (!notify_.wait_dequeue_timed(tmp, t)) {
              timeout();
          }
      }
}

