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
    for(i = 0; i < IO_THREADS_COUNT; ++i)
        this->threads[i] = std::thread(&IOThreadPool::slaveThread, this);
    this->count = 0;
}

IOThreadPool::~IOThreadPool()
{
    for(unsigned i = 0; i < IO_THREADS_COUNT; ++i)
        this->threads[i].join();
}

bool IOThreadPool::addPathFunction(int type, char* buffer, const User* user, bool(*function)(fsData*))
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

                if(type == 1) // only 1 path
                {
                    if(!user->getRealFile(buffer, currAdd->data.data.path))
                    {
                        user->sendData(SERVER_MSG::ERROR_OCCURED);
                        return (false);
                    }
                }
                else // 2 path
                {
                    if(!user->getRealFile(buffer + 1, currAdd->data.data.path2.src) ||
                       !user->getRealFile(buffer + 4 + *(buffer), currAdd->data.data.path2.dst))
                    {
                        user->sendData(SERVER_MSG::ERROR_OCCURED);
                        return (false);
                    }
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

void IOThreadPool::slaveThread()
{
    bool found = true;
    DATA_STATE t;
    const fsThreadData* end = this->data + IO_DATA_SIZE;
    while (!needExit)
    {
        if((!found) | (this->count == 0))
        {
            SLEEP(1);
        }

        found = false;
        for(fsThreadData* currManaged = this->data; currManaged != end; ++currManaged)
        {
            t = DATA_STATE::WAITING;
            if(std::atomic_compare_exchange_strong(&currManaged->state, &t, DATA_STATE::CONTROLED))
            {
                currManaged->data.user->sendData(currManaged->function(&currManaged->data)
                                                ? SERVER_MSG::ACTION_COMPLETED
                                                : SERVER_MSG::ERROR_OCCURED);
                currManaged->state = DATA_STATE::FREE;
                --this->count;
                found = true;
            }
        }
    }
}
