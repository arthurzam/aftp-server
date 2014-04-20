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


#ifndef FILENAME_MAX
#define FILENAME_MAX 260
#endif

#define USERNAME_LENGTH 0x40

#ifndef WIN32
#define SOCKET int
#define SOCKET_ERROR	(-1)
#endif

#ifdef WIN32
typedef int from_len_t;
#define SERVER_BASE_FOLDER "C:\\server\\"
#define PATH_SEPERATOR_BAD  '/'
#define PATH_SEPERATOR_GOOD '\\'
#define CLEAR_SCREEN "cls"
#define THREAD_RETURN_VALUE void
#else
#include <sys/socket.h>
typedef socklen_t from_len_t;
#define SERVER_BASE_FOLDER "/opt/server/"
#define PATH_SEPERATOR_BAD  '\\'
#define PATH_SEPERATOR_GOOD '/'
#define CLEAR_SCREEN "clear"
#define THREAD_RETURN_VALUE void*
#endif

#endif
