#ifndef SERVER_LOOKUP_H_
#define SERVER_LOOKUP_H_

#include <cstdint>
#include <cstddef>
#include "fileControl.h"

struct table_msg_t {
    uint_fast8_t num;
    bool(*function)(fsData*);
    uint_fast16_t min_ret;
};

constexpr unsigned CLIENT_MSG_COUNT = 27;

static const struct table_msg_t table_msgs[] = {
    {0, NULL, 19},
    {0, NULL, 0},

    {0, NULL, 0},
    {0, NULL, 10},
    {0, NULL, 10},
    {0, NULL, 6},
    {0, NULL, 0},

    {0, NULL, 0},
    {0, NULL, 0},

    {0, NULL, 7},
    {0, NULL, 3},
    {2, moveFile, 8},
    {2, copyFile, 8},
    {1, removeFile, 3},
    {1, getFileSize, 3},
    {1, getMD5OfFile, 3},
    {2, symbolicLink, 8},

    {0, NULL, 3},
    {1, createDirectory, 3},
    {1, removeFolder, 3},
    {2, moveDirectory, 8},
    {2, copyFolder, 8},
    {1, getContentDirectory, 3},
    {0, NULL, 0},

    {0, NULL, 0},
    {0, NULL, 0},
    {0, NULL, 0},
};

static_assert(sizeof(table_msgs) / sizeof(table_msg_t) == CLIENT_MSG_COUNT, "lookup table for messages isn't complete");

#endif /* SERVER_LOOKUP_H_ */
