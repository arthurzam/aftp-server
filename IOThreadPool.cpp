#include "IOThreadPool.h"
#include "User.h"

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

extern bool needExit;

IOThreadPool::IOThreadPool()
{
    int i;
    for(i = 0; i < IO_DATA_SIZE; ++i)
        this->data[i].state = DATA_STATE::FREE;
    this->count = 0;
    for(i = 0; i < IO_THREADS_COUNT; ++i)
        this->threads[i] = std::thread(&IOThreadPool::slaveThread, this);
}

IOThreadPool::~IOThreadPool()
{
    for(int i = 0; i < IO_THREADS_COUNT; ++i)
        this->threads[i].join();
}

bool IOThreadPool::add1pathFunction(char* buffer, const User* user, void(*function)(fsData*))
{
    fsThreadData* currAdd = this->data;

    if (this->count == IO_DATA_SIZE)
    {
        user->sendData(302);
        return (false);
    }
    for(; currAdd != this->data + IO_DATA_SIZE; ++currAdd)
    {
        if(currAdd->state == DATA_STATE::FREE)
        {
            currAdd->function = function;
            currAdd->data.user = user;

            user->getRealFile(buffer, currAdd->data.data.path);

            currAdd->state = DATA_STATE::WAITING;
            ++this->count;
            return (true);
        }
    }
    return (false);
}

bool IOThreadPool::add2pathFunction(char* buffer, const User* user, void(*function)(fsData*))
{
    fsThreadData* currAdd = this->data;

    if (this->count == IO_DATA_SIZE)
    {
        user->sendData(302);
        return (false);
    }
    for(; currAdd != this->data + IO_DATA_SIZE; ++currAdd)
    {
        if(currAdd->state == DATA_STATE::FREE)
        {
            if(!user->getRealFile(buffer + 1, currAdd->data.data.path2.src) ||
               !user->getRealFile(buffer + 4 + *(buffer), currAdd->data.data.path2.dst))
            {
                user->sendData(300);
                return (false);
            }

            currAdd->function = function;
            currAdd->data.user = user;

            currAdd->state = DATA_STATE::WAITING;
            ++this->count;
            return (true);
        }
    }
    return (false);
}

void IOThreadPool::slaveThread()
{
    bool found;
    while (!needExit)
    {
        found = false;
        for(fsThreadData* currManaged = this->data; currManaged != this->data + IO_DATA_SIZE; ++currManaged)
        {
            DATA_STATE t = DATA_STATE::WAITING;
            if(std::atomic_compare_exchange_strong(&currManaged->state, &t, DATA_STATE::CONTROLED))
            {
                currManaged->function(&currManaged->data);
                currManaged->state = DATA_STATE::FREE;
                --this->count;
                found = true;
            }
        }
        if(!found)
        {
#ifdef WIN32
            Sleep(1000); //mili-seconds
#else
            sleep(1);    // seconds
#endif
        }
    }
}
