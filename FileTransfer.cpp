/*
 * FileTransfer.cpp
 *
 *  Created on: 5 ???? 2013
 *      Author: Arthur Zamarin
 */

#include <cstring>
#include "FileTransfer.h"

FileTransfer::FileTransfer(char* relativePath, User* user) // Download
{
    char fPath[FILENAME_MAX];
    char buffer[FILE_BLOCK_SIZE];

    user->getRealFile(relativePath, fPath);
    this->file = fopen(fPath, "r+b");
    if(!this->file)
    {
        this->state = FILE_TRANSFER_STATE_ERROR;
        return;
    }

    this->state = FILE_TRANSFER_STATE_DOWNLOAD;
    md5_init(&this->allFile);
    this->user = user;

    this->blocksCount = 0;
    while(fread(buffer, FILE_BLOCK_SIZE, 1, this->file))
        this->blocksCount++;
    fseek(this->file, 0, SEEK_SET);

    this->blocks = (byte_t*)malloc(this->blocksCount);
    memset(this->blocks, 0, this->blocksCount);
}

FileTransfer::FileTransfer(char* relativePath, User* user, unsigned int blocksCount) // Upload
{
    char fPath[FILENAME_MAX];
    user->getRealFile(relativePath, fPath);
    this->file = fopen(fPath, "w+b");
    if(!this->file)
    {
        this->state = FILE_TRANSFER_STATE_ERROR;
        return;
    }

    this->state = FILE_TRANSFER_STATE_UPLOAD;
    md5_init(&this->allFile);
    this->user = user;
    this->blocksCount = blocksCount;
    this->blocks = (byte_t*)malloc(blocksCount);
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

bool_t FileTransfer::isLoaded() const
{
    return (this->state);
}

void FileTransfer::askForBlocksRange(unsigned int start, unsigned int end)
{
    for(; start <= end; start++)
        askForBlock(start);
}

void FileTransfer::recieveBlock(char* buffer, int dataLen)
{
    struct dat_t {
        unsigned int blockNum;
        unsigned short size;
        byte_t md5Res[16];
        byte_t dataFile[FILE_BLOCK_SIZE];
    };
    struct dat_t* data = (struct dat_t*)buffer;
    byte_t md5R[16];
    md5_context ctx;
    md5_init(&ctx);
    md5_append(&ctx, data->dataFile, data->size);
    md5_finish(&ctx, md5R);
    if(memcpy(md5R, data->md5Res, 16)) // not equal
    {
        this->user->sendData(310);
        return;
    }
    fseek(this->file, data->blockNum * FILE_BLOCK_SIZE, SEEK_SET);
    fwrite(data->dataFile, data->size, 1, this->file);
    if(!this->blocks[data->blockNum])
        md5_append(&this->allFile, data->dataFile, data->size);
    this->blocks[data->blockNum] = 1;
    this->user->sendData(200);
}

void FileTransfer::askForBlock(unsigned int blockNum)
{
    if(blockNum > this->blocksCount)
    {
        this->user->sendData(312);
        return;
    }

    md5_context ctx;
    char buffer[FILE_BLOCK_SIZE + 23];
    unsigned short size;

    md5_init(&ctx);
    fseek(this->file, blockNum * FILE_BLOCK_SIZE, SEEK_SET);
    size = fread(buffer + 22, FILE_BLOCK_SIZE, 1, this->file);
    md5_append(&ctx, (byte_t*)buffer + 22, size);
    md5_finish(&ctx, (byte_t*)buffer + 6);

    memcpy(buffer, &blockNum, 4);
    memcpy(buffer + 4, &size, 2);
    this->user->sendData(210, buffer, 23 + size);
    if(!this->blocks[blockNum])
        md5_append(&this->allFile, (byte_t*)buffer + 22, size);
    this->blocks[blockNum] = 1;
}
