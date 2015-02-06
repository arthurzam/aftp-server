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
    if((temp & (FILE_BLOCK_SIZE - 1)) != 0) // works only if FILE_BLOCK_SIZE is 2^x; check that there is a partial block
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

void FileTransfer::recieveBlock(const char* buffer)
{
    struct __attribute__((packed)) dat_t {
        uint32_t blockNum;
        uint16_t size;
        uint8_t md5Res[16];
        uint8_t dataFile[FILE_BLOCK_SIZE];
    };
    struct dat_t* data = (struct dat_t*)buffer;
    uint8_t md5R[16];
    data->size = ntohs(data->size);
    data->blockNum = htonl(data->blockNum);
    if(this->state != STATE_DOWNLOAD)
    {
        this->user->sendData(SERVER_MSG::SOURCE_BAD);
        return;
    }
    MD5(data->dataFile, data->size, md5R);
    if(memcmp(md5R, data->md5Res, 16)) // not equal
    {
        this->user->sendData(SERVER_MSG::FILE_BLOCK_MD5_MISMATCH);
        return;
    }
    if(this->currentCursorBlock != data->blockNum)
        fseek(this->file, (data->blockNum - this->currentCursorBlock) * FILE_BLOCK_SIZE, SEEK_CUR);
    fwrite(data->dataFile, 1, data->size, this->file);
    this->currentCursorBlock = data->blockNum + 1;
    this->user->sendData(SERVER_MSG::ACTION_COMPLETED, &data->blockNum, sizeof(data->blockNum));
}

void FileTransfer::askForBlock(uint32_t blockNum)
{
    if(blockNum > this->blocksCount)
    {
        this->user->sendData(SERVER_MSG::BLOCK_NUM_OUTRANGE, &blockNum, sizeof(blockNum));
        return;
    }
    struct __attribute__((packed)){
        uint32_t blockNum;
        uint16_t size;
        uint8_t md5[16];
        uint8_t data[FILE_BLOCK_SIZE];
    } buffer;
    if(this->state != STATE_UPLOAD)
    {
        this->user->sendData(SERVER_MSG::SOURCE_BAD);
        return;
    }
    if(this->currentCursorBlock != blockNum)
        fseek(this->file, (blockNum - this->currentCursorBlock) * FILE_BLOCK_SIZE, SEEK_CUR);
    buffer.size = fread(buffer.data, 1, FILE_BLOCK_SIZE, this->file);
    this->currentCursorBlock = blockNum + 1;
    MD5(buffer.data, buffer.size, buffer.md5);
    buffer.blockNum = htonl(blockNum);
    buffer.size = htons(buffer.size);
    this->user->sendData(SERVER_MSG::DOWNLOAD_FILE_BLOCK, &buffer, 22 + buffer.size);
}
