#ifndef IOTHREADPOOL_H_
#define IOTHREADPOOL_H_

#include <thread>
#include <atomic>

#include "defenitions.h"

class User;

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
        /*
         * the amount of threads running under the Thread Pool.
         * When compiling the program to a server I suggest to try to change this number
         * and see the performance. From what I see a low number works great on my testing
         * machine.
         */
        static constexpr unsigned IO_THREADS_COUNT = 2;

        /*
         * slots number in queue.
         * as greater this number is, the more memory the object takes, but queue would run
         * out of space more seldom.
         */
        static constexpr unsigned IO_DATA_SIZE = 128;

        enum DATA_STATE {
            FREE = 0, // current place's data can be overwrited
            WAITING,  // waiting for a thread to take over and run
            CONTROLED // is being run
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
