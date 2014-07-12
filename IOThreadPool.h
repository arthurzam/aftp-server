#ifndef IOTHREADPOOL_H_
#define IOTHREADPOOL_H_

#include <thread>
#include <atomic>

#include "defenitions.h"

class User;

#define IO_THREADS_COUNT 2
#define IO_DATA_SIZE 128

typedef struct {
    const User* user;
    union {
        char path[REL_PATH_MAX + BASE_FOLDER_MAX];
        struct {
            char src[REL_PATH_MAX + BASE_FOLDER_MAX];
            char dst[REL_PATH_MAX + BASE_FOLDER_MAX];
        } path2;
    } data;
} fsData;

class IOThreadPool {
    private:
        enum DATA_STATE {
            FREE = 0,
            WAITING,
            CONTROLED
        };
        std::thread threads[IO_THREADS_COUNT];

        typedef struct {
            fsData data;
            void(*function)(fsData*);
            std::atomic<enum DATA_STATE> state;
        } fsThreadData;

        fsThreadData data[IO_DATA_SIZE];

        int count;

        void slaveThread();
    public:
        IOThreadPool();
        ~IOThreadPool();
        bool add1pathFunction(char* buffer, const User* user, void(*function)(fsData*));
        bool add2pathFunction(char* buffer, const User* user, void(*function)(fsData*));
};

#endif /* IOTHREADPOOL_H_ */