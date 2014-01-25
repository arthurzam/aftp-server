#ifndef DEFENITIONS_H_
#define DEFENITIONS_H_

#include <cstdlib>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE !FALSE
#endif

typedef unsigned char bool_t;
typedef unsigned char byte_t;

#ifdef WIN32
typedef int from_len_t;
#else
#include <sys/socket.h>
typedef socklen_t from_len_t;
#endif


#ifndef FILENAME_MAX
#define FILENAME_MAX 260
#endif

#ifndef USERNAME_LENGTH
#define USERNAME_LENGTH 0x40
#endif

#ifndef WIN32
#define SOCKET int
#define SOCKET_ERROR	(-1)
#endif

#ifdef WIN32
#define SERVER_BASE_FOLDER "C:\\server\\"
#define CLEAR_SCREEN "cls"
#define THREAD_RETURN_VALUE void
#else
#define SERVER_BASE_FOLDER "\\opt\\server\\"
#define CLEAR_SCREEN "clear"
#define THREAD_RETURN_VALUE void*
#endif

#endif
