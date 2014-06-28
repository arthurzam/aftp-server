#ifndef DEFENITIONS_H_
#define DEFENITIONS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef FILENAME_MAX
#if FILENAME_MAX > (128 + 20)
#define REL_PATH_MAX 128
#else
#define REL_PATH_MAX (FILENAME_MAX - 20)
#endif
#else
#define REL_PATH_MAX 128
#endif

#ifndef WIN32
#define SOCKET int
#define SOCKET_ERROR	(-1)
#endif

#ifdef WIN32
typedef int socklen_t;
#define DEFAULT_SERVER_BASE_FOLDER "C:\\server\\"
#define PATH_SEPERATOR_BAD  '/'
#define PATH_SEPERATOR_GOOD '\\'
#define CLEAR_SCREEN "cls"
typedef void threadReturnValue;
#else
#define DEFAULT_SERVER_BASE_FOLDER "/opt/server/"
#define PATH_SEPERATOR_BAD  '\\'
#define PATH_SEPERATOR_GOOD '/'
#define CLEAR_SCREEN "clear"
typedef void* threadReturnValue;
#endif

#endif
