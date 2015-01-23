#ifndef SERVER_LOOKUP_H_
#define SERVER_LOOKUP_H_

#include <cstdint>
#include <cstddef>
#include "fileControl.h"

struct table_msg_t {
    uint_fast8_t num;
    void(*function)(fsData*);
};

#define CLIENT_MSG_COUNT 28

static const struct table_msg_t table_msgs[] = {
    {0, NULL},
    {0, NULL},

    {0, NULL},
    {0, NULL},
    {0, NULL},
    {0, NULL},
    {0, NULL},

    {0, NULL},
    {0, NULL},
    {0, NULL},
    {0, NULL},

    {0, NULL},

    {0, NULL},
    {0, NULL},
    {0, NULL},
    {2, moveFile},
    {2, copyFile},
    {1, removeFile},
    {1, getFilesize},
    {1, getMD5OfFile},
#ifdef WIN32
    {2, copyFile},
#else
    {2, symbolicLink},
#endif

    {0, NULL},
    {1, createDirectory},
    {1, removeFolder},
    {2, moveDirectory},
    {2, copyFolder},
    {1, getContentDirectory},
    {0, NULL},
};

static_assert(sizeof(table_msgs) / sizeof(table_msg_t) == CLIENT_MSG_COUNT, "lookup table for messages isn't complete");

#endif /* SERVER_LOOKUP_H_ */
