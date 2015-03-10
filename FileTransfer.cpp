#include <cstring>
#include <cstdlib>
#include <openssl/md5.h>

#include "FileTransfer.h"
#include "User.h"
#include "messages.h"

bool FileTransfer::reset(char* relativePath, const User* user) // Download
{
    char fPath[FILENAME_MAX];

    close();

    if(!(user->getRealFile(relativePath, fPath) && (this->file = fopen(fPath, "rb"))))
    {
        this->state = STATE_ERROR;
        return false;
    }

    this->state = STATE_DOWNLOAD;
    this->user = user;
    this->currentCursorBlock = 0;

    fseek(this->file, 0, SEEK_END);
    uint64_t temp = ftell(this->file);
    this->blocksCount = temp / FILE_BLOCK_SIZE;

    static_assert(((FILE_BLOCK_SIZE - 1) & FILE_BLOCK_SIZE) == 0, "FILE_BLOCK_SIZE is not 2^x");
    if((temp & (FILE_BLOCK_SIZE - 1)) != 0) // check that there is a partial block
        this->blocksCount++;
    fseek(this->file, 0, SEEK_SET);
    return true;
}

bool FileTransfer::reset(char* relativePath, const User* user, uint32_t blocksCount) // Upload
{
    char fPath[FILENAME_MAX];

    close();

    if(!(user->getRealFile(relativePath, fPath) && (this->file = fopen(fPath, "w+b"))))
    {
        this->state = STATE_ERROR;
        return false;
    }

    this->state = STATE_UPLOAD;
    this->user = user;
    this->blocksCount = blocksCount;
    this->currentCursorBlock = 0;
    return true;
}

struct __attribute__((packed)) buffer_t {
    uint32_t blockNum;
    uint16_t size;
    uint8_t md5Res[MD5_DIGEST_LENGTH];
    uint8_t dataFile[FileTransfer::FILE_BLOCK_SIZE];
};

constexpr unsigned FILE_BLOCK_HEADER_SIZE = sizeof(buffer_t) - FileTransfer::FILE_BLOCK_SIZE;

static_assert(sizeof(buffer_t) + sizeof(msgCode_t) < SERVER_BUFFER_SIZE, "FILE_BLOCK_SIZE is too big");

void FileTransfer::recieveBlock(const char* buffer, size_t len)
{
    struct buffer_t* data = (struct buffer_t*)buffer;
    uint8_t md5R[MD5_DIGEST_LENGTH];
    data->size = ntohs(data->size);
    uint32_t blockNum = htonl(data->blockNum);
    msgCode_t msgCodeRes = SERVER_MSG::ACTION_COMPLETED;
    if((this->state != STATE_UPLOAD) | (len - FILE_BLOCK_HEADER_SIZE != data->size) | (blockNum >= this->blocksCount))
    {
        msgCodeRes = SERVER_MSG::ERROR_OCCURED;
    }
    else
    {
        MD5(data->dataFile, data->size, md5R);
        if(memcmp(md5R, data->md5Res, MD5_DIGEST_LENGTH)) // not equal
        {
            msgCodeRes = SERVER_MSG::FILE_BLOCK_MD5_MISMATCH;
        }
        else
        {
            if(this->currentCursorBlock != blockNum)
                fseek(this->file, (blockNum - this->currentCursorBlock) * FILE_BLOCK_SIZE, SEEK_CUR);
            fwrite(data->dataFile, 1, data->size, this->file);
            this->currentCursorBlock = blockNum + 1;
        }
    }
    this->user->sendData(msgCodeRes, &data->blockNum, sizeof(data->blockNum));
}

void FileTransfer::askForBlock(uint32_t blockNum)
{
    struct buffer_t buffer;
    buffer.blockNum = htonl(blockNum);
    if(blockNum > this->blocksCount)
    {
        this->user->sendData(SERVER_MSG::BLOCK_NUM_OUTRANGE, &buffer.blockNum, sizeof(buffer.blockNum));
        return;
    }
    if(this->state != STATE_DOWNLOAD)
    {
        this->user->sendData(SERVER_MSG::ERROR_OCCURED);
        return;
    }
    if(this->currentCursorBlock != blockNum)
        fseek(this->file, (blockNum - this->currentCursorBlock) * FILE_BLOCK_SIZE, SEEK_CUR);
    size_t readBytes = fread(buffer.dataFile, 1, FILE_BLOCK_SIZE, this->file);
    this->currentCursorBlock = blockNum + 1;
    MD5(buffer.dataFile, readBytes, buffer.md5Res);
    buffer.size = htons((uint16_t)readBytes);
    this->user->sendData(SERVER_MSG::DOWNLOAD_FILE_BLOCK, &buffer, readBytes + FILE_BLOCK_HEADER_SIZE);
}
