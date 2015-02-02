#include "IOThreadPool.h"
#include "User.h"
#include "messages.h"

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

extern bool needExit;

IOThreadPool::IOThreadPool()
{
    unsigned i;
    for(i = 0; i < IO_DATA_SIZE; ++i)
        this->data[i].state = DATA_STATE::FREE;
    this->count = 0;
    for(i = 0; i < IO_THREADS_COUNT; ++i)
        this->threads[i] = std::thread(&IOThreadPool::slaveThread, this);
}

IOThreadPool::~IOThreadPool()
{
    for(unsigned i = 0; i < IO_THREADS_COUNT; ++i)
        this->threads[i].join();
}

bool IOThreadPool::add1pathFunction(char* buffer, const User* user, void(*function)(fsData*))
{
    const fsThreadData* end = this->data + IO_DATA_SIZE;

    if (this->count != IO_DATA_SIZE)
    {
        for(fsThreadData* currAdd = this->data; currAdd != end; ++currAdd)
        {
            if(currAdd->state == DATA_STATE::FREE)
            {
                currAdd->function = function;
                currAdd->data.user = user;

                if(!user->getRealFile(buffer, currAdd->data.data.path))
                {
                    user->sendData(SERVER_MSG::SOURCE_BAD);
                    return (false);
                }

                currAdd->state = DATA_STATE::WAITING;
                ++this->count;
                return (true);
            }
        }
    }
    user->sendData(SERVER_MSG::SERVER_BUSY);
    return (false);
}

bool IOThreadPool::add2pathFunction(char* buffer, const User* user, void(*function)(fsData*))
{
    const fsThreadData* end = this->data + IO_DATA_SIZE;
    if (this->count != IO_DATA_SIZE)
    {
        for(fsThreadData* currAdd = this->data; currAdd != end; ++currAdd)
        {
            if(currAdd->state == DATA_STATE::FREE)
            {
                if(!user->getRealFile(buffer + 1, currAdd->data.data.path2.src) ||
                   !user->getRealFile(buffer + 4 + *(buffer), currAdd->data.data.path2.dst))
                {
                    user->sendData(SERVER_MSG::SOURCE_BAD);
                    return (false);
                }

                currAdd->function = function;
                currAdd->data.user = user;

                currAdd->state = DATA_STATE::WAITING;
                ++this->count;
                return (true);
            }
        }
    }
    user->sendData(SERVER_MSG::SERVER_BUSY);
    return (false);
}

void IOThreadPool::slaveThread()
{
    bool found = true;
    DATA_STATE t;
    const fsThreadData* end = this->data + IO_DATA_SIZE;
    while (!needExit)
    {
        if(!found || this->count == 0)
        {
            SLEEP(1);
        }

        found = false;
        for(fsThreadData* currManaged = this->data; currManaged != end; ++currManaged)
        {
            t = DATA_STATE::WAITING;
            if(std::atomic_compare_exchange_strong(&currManaged->state, &t, DATA_STATE::CONTROLED))
            {
                currManaged->function(&currManaged->data);
                currManaged->state = DATA_STATE::FREE;
                --this->count;
                found = true;
            }
        }
    }
}
