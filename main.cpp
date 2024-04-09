#include <initializer_list>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <stdio.h>

#include "signal/cs_object.h"

void generateCore() {
    struct rlimit rlmt;
    if (getrlimit(RLIMIT_CORE, &rlmt) == -1) {
        printf("getrlimit fail\n");
        return;
    }

    rlmt.rlim_cur = rlmt.rlim_max = RLIM_INFINITY;

    if (setrlimit(RLIMIT_CORE, &rlmt) == -1) {
        printf("setrlimit fail\n");
        return;
    }
}

struct CsInfo
{
public:
    std::string strRequestId;
    
    double dvar;
    int    ivar;

    void dump() {
        std::cout << " str: " << strRequestId << " double: " << dvar << " int: " << ivar << std::endl;
    }
};

class CTestObj1 : public CSObject
{
public:
    explicit CTestObj1(CSObject* parent = nullptr) : CSObject(parent) {}
    ~CTestObj1() = default;

public:
    SIGNAL_1(void signal_string(const std::string& content))
    SIGNAL_2(signal_string, content)

    SIGNAL_1(void signal_info(CsInfo& info, int id))
    SIGNAL_2(signal_info, info, id)
};

class CTestObj2 : public CSObject
{
public:
    explicit CTestObj2(CSObject* parent = nullptr) : CSObject(parent) {}
    ~CTestObj2() = default;

public:
    void slot_string(const std::string& content) {
        std::cout << "slot_string cur thread id: " << CSThread::getCurrentThreadId() << " content: " << content << std::endl;
    }
    void slot_info(CsInfo& info, int id) {
        std::cout << "slot_info cur thread id: " << CSThread::getCurrentThreadId() << " id: " << id;
        info.dump();
    }
};

int main(int argc, char *argv[])
{
    generateCore();

    CSCoreApplication app;

    std::cout << "main thread id: " << CSThread::getCurrentThreadId() << std::endl;

    std::string str("123456");
    int id = 100;
    CsInfo csinfo;
    csinfo.dvar = 3.14;
    csinfo.ivar = 200;
    csinfo.strRequestId = "requestid";

    std::cout << "****************************************begin sync test " << std::endl;

    CTestObj1 sync_obj1;
    CTestObj2 sync_obj2;
    connect(&sync_obj1, &CTestObj1::signal_string, &sync_obj2, &CTestObj2::slot_string, CsSignal::ConnectionKind::QueuedConnection);
    connect(&sync_obj1, &CTestObj1::signal_info, &sync_obj2, &CTestObj2::slot_info, CsSignal::ConnectionKind::QueuedConnection);
    sync_obj1.signal_string(str);
    sync_obj1.signal_info(csinfo, id);
    
    std::cout << "****************************************begin async test " << std::endl;

    CTestObj1 async_obj1;
    CTestObj2 async_obj2;
    CSThread* t = new CSThread();
    connect(&async_obj1, &CTestObj1::signal_string, &async_obj2, &CTestObj2::slot_string, CsSignal::ConnectionKind::QueuedConnection);
    connect(&async_obj1, &CTestObj1::signal_info, &async_obj2, &CTestObj2::slot_info, CsSignal::ConnectionKind::QueuedConnection);
    async_obj2.moveToThread(t);
    t->start();
    CSThread::msleep(500);
    async_obj1.signal_string(str);
    async_obj1.signal_info(csinfo, id);

    app.exec();
    return 0;
}
