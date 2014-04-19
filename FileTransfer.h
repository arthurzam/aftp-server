#ifndef FILETRANSFER_H_
#define FILETRANSFER_H_

#include <stdio.h>
#include <stdlib.h>
#include "md5.h"
#include "defenitions.h"
#include "User.h"
#include "server.h"



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
    md5_context allFile;
    byte_t* blocks; // pointer to HEAP located array
    unsigned int blocksCount;
    FILE* file;
    User* user; // just a pointer
public:
    FileTransfer(char* relativePath, User* user); // Download
    FileTransfer(char* relativePath, User* user, unsigned int blocksCount); // Upload

    bool_t isLoaded() const;
    inline unsigned int getBlocksCount() const
    {
        return (this->blocksCount);
    }

    void askForBlock(unsigned int blockNum);
    void askForBlocksRange(unsigned int start, unsigned int end);
    void finishDownload();

    void recieveBlock(char* buffer, int dataLen);
    /*
     * finished the upload with last check. return true if success.
     * the function automatically sends the message code
     */
    bool_t finishUpload(char* Buffer);

    ~FileTransfer();
};

#endif
