#ifndef THREADER_H_
#define THREADER_H_

#ifdef WIN32
#include <windows.h>
#include <process.h>
typedef HANDLE aftp_thread;
#else
#include <pthread.h>
typedef pthread_t aftp_thread;
#endif

aftp_thread createThread(void* function, void* argument);
void joinThread(aftp_thread thread);

#endif
