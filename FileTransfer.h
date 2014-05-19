#ifndef FILETRANSFER_H_
#define FILETRANSFER_H_

#include <cstdio>
#include <cstdlib>
#include "md5.h"
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
    unsigned int blocksCount;
    unsigned int currentCursorBlock; // the block where the file's cursor is located
    FILE* file;
    User* user; // just a pointer
public:
    FileTransfer(char* relativePath, User* user); // Download
    FileTransfer(char* relativePath, User* user, unsigned int blocksCount); // Upload

    inline bool isLoaded() const
    {
        return (this->state);
    }
    inline unsigned int getBlocksCount() const
    {
        return (this->blocksCount);
    }

    void askForBlock(unsigned int blockNum);
    void askForBlocksRange(unsigned int start, unsigned int end);

    void recieveBlock(char* buffer, int dataLen);

    ~FileTransfer();
};

#endif
