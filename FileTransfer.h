#ifndef FILETRANSFER_H_
#define FILETRANSFER_H_

#include <cstdio>
#include <cstdlib>
#include <openssl/md5.h>
#include "defenitions.h"
#include "User.h"

#define FILE_BLOCK_SIZE 0x200 //=512

class User;

class FileTransfer {
private:
    typedef enum {
        STATE_ERROR = 0,
        STATE_UPLOAD,
        STATE_DOWNLOAD
    } STATE;

    STATE state;
    uint8_t* blocks; // pointer to HEAP located array
    uint32_t blocksCount;
    uint32_t currentCursorBlock; // the block where the file's cursor is located
    FILE* file;
    const User* user; // just a pointer
public:
    FileTransfer(char* relativePath, const User* user); // Download
    FileTransfer(char* relativePath, const User* user, unsigned int blocksCount); // Upload

    inline bool isLoaded() const
    {
        return (this->state);
    }
    inline uint32_t getBlocksCount() const
    {
        return (this->blocksCount);
    }

    void askForBlock(uint32_t blockNum);
    void askForBlocksRange(uint32_t start, uint32_t end);

    void recieveBlock(const char* buffer);

    ~FileTransfer();
};

#endif
