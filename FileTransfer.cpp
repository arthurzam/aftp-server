#include <cstring>
#include <openssl/md5.h>

#include "FileTransfer.h"

FileTransfer::FileTransfer(char* relativePath, const User* user) // Download
{
    char fPath[FILENAME_MAX];

    if(!user->getRealFile(relativePath, fPath) || !(this->file = fopen(fPath, "rb")))
    {
        this->state = STATE_ERROR;
        this->file = NULL;
        return;
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

    this->blocks = (uint8_t*)malloc(this->blocksCount);
    memset(this->blocks, 0, this->blocksCount);
}

FileTransfer::FileTransfer(char* relativePath, const User* user, unsigned int blocksCount) // Upload
{
    char fPath[FILENAME_MAX];

    if(!user->getRealFile(relativePath, fPath) || !(this->file = fopen(fPath, "w+b")))
    {
        this->state = STATE_ERROR;
        this->file = NULL;
        return;
    }

    this->state = STATE_UPLOAD;
    this->user = user;
    this->blocksCount = blocksCount;
    this->currentCursorBlock = 0;
    this->blocks = (uint8_t*)malloc(blocksCount);
    memset(this->blocks, 0, blocksCount);
}

FileTransfer::~FileTransfer()
{
    if (this->blocks)
        free(this->blocks);
    this->blocks = NULL;
    if(this->file)
        fclose(this->file);
    this->file = NULL;
}

void FileTransfer::recieveBlock(const char* buffer)
{
    struct dat_t {
        uint32_t blockNum;
        uint16_t size;
        uint8_t md5Res[16];
        uint8_t dataFile[FILE_BLOCK_SIZE];
    };
    struct dat_t* data = (struct dat_t*)buffer;
    uint8_t md5R[16];
    if(this->state != STATE_DOWNLOAD)
    {
        this->user->sendData(300);
        return;
    }
    MD5(data->dataFile, data->size, md5R);
    if(memcmp(md5R, data->md5Res, 16)) // not equal
    {
        this->user->sendData(310);
        return;
    }
    if(this->currentCursorBlock != data->blockNum)
        fseek(this->file, (data->blockNum - this->currentCursorBlock) * FILE_BLOCK_SIZE, SEEK_CUR);
    fwrite(data->dataFile, 1, data->size, this->file);
    this->currentCursorBlock = data->blockNum + 1;
    this->blocks[data->blockNum] = 1;
    this->user->sendData(200, &data->blockNum, sizeof(data->blockNum));
}

void FileTransfer::askForBlocksRange(uint32_t start, uint32_t end)
{
    for(; start < end; ++start)
        askForBlock(start);
}

void FileTransfer::askForBlock(uint32_t blockNum)
{
    if(blockNum > this->blocksCount)
    {
        this->user->sendData(312, (char*)&(blockNum), sizeof(blockNum));
        return;
    }
    struct {
        uint32_t blockNum;
        uint16_t size;
        uint8_t md5[16];
        uint8_t data[FILE_BLOCK_SIZE];
    } buffer;
    if(this->state != STATE_UPLOAD)
    {
        this->user->sendData(300);
        return;
    }
    if(this->currentCursorBlock != blockNum)
        fseek(this->file, (blockNum - this->currentCursorBlock) * FILE_BLOCK_SIZE, SEEK_CUR);
    buffer.size = fread(buffer.data, 1, FILE_BLOCK_SIZE, this->file);
    this->currentCursorBlock = blockNum + 1;
    MD5(buffer.data, buffer.size, buffer.md5);
    buffer.blockNum = blockNum;
    this->user->sendData(210, &buffer, 22 + buffer.size);
    this->blocks[blockNum] = 1;
}
