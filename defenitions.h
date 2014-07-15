#ifndef DEFENITIONS_H_
#define DEFENITIONS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define BASE_FOLDER_MAX 32
#ifdef FILENAME_MAX
#if FILENAME_MAX > (128 + BASE_FOLDER_MAX)
#define REL_PATH_MAX 128
#else
#define REL_PATH_MAX (FILENAME_MAX - BASE_FOLDER_MAX)
#endif
#else
#define REL_PATH_MAX 128
#endif

#define DEFAULT_PORT 7777

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
#else
#define DEFAULT_SERVER_BASE_FOLDER "/opt/server/"
#define PATH_SEPERATOR_BAD  '\\'
#define PATH_SEPERATOR_GOOD '/'
#define CLEAR_SCREEN "clear"
#endif

#endif
