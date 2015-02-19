#ifndef DEFENITIONS_H_
#define DEFENITIONS_H_

#include <cstdint>
#include <cstdbool>
#include <cstddef>

constexpr unsigned BASE_FOLDER_MAX = 32;
#ifdef FILENAME_MAX
    #if FILENAME_MAX > (128 + BASE_FOLDER_MAX)
        constexpr unsigned REL_PATH_MAX = 128;
    #else
        constexpr unsigned REL_PATH_MAX = (FILENAME_MAX - BASE_FOLDER_MAX);
        static_assert(REL_PATH_MAX > 16, "FILENAME_MAX is too small");
    #endif
#else
    constexpr unsigned REL_PATH_MAX = 128;
#endif

constexpr uint16_t DEFAULT_PORT = 7777;

#ifndef WIN32
typedef int SOCKET;
#define SOCKET_ERROR (-1)
#endif

#ifdef WIN32
typedef int socklen_t;
#define DEFAULT_SERVER_BASE_FOLDER "C:\\server\\"
constexpr char PATH_SEPERATOR_BAD  = '/';
constexpr char PATH_SEPERATOR_GOOD = '\\';
#else
#define DEFAULT_SERVER_BASE_FOLDER "/opt/server/"
constexpr char PATH_SEPERATOR_BAD  = '\\';
constexpr char PATH_SEPERATOR_GOOD ='/';
#endif

#ifdef WIN32
#define SLEEP(seconds) Sleep(seconds * 1000) // Milliseconds
#else
#define SLEEP(seconds) sleep(seconds);       // Seconds
#endif

constexpr unsigned AES_KEY_LENGTH = 128;
constexpr unsigned BUFFER_SERVER_SIZE = 0x400;
constexpr unsigned RSA_NSIZE = 2048;

typedef uint16_t msgCode_t;

#endif
