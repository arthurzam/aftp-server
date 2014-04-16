#include <cstring>
#include "FileTransfer.h"

FileTransfer::FileTransfer(char* relativePath, User* user) // Download
{
    char fPath[FILENAME_MAX];

    user->getRealFile(relativePath, fPath);
    this->file = fopen(fPath, "rb");
    if(!this->file)
    {
        this->state = FILE_TRANSFER_STATE_ERROR;
        return;
    }

    this->state = FILE_TRANSFER_STATE_DOWNLOAD;
    md5_init(&this->allFile);
    this->user = user;

    fseek(this->file, 0, SEEK_END);
    long temp = ftell(this->file);
    this->blocksCount = temp / FILE_BLOCK_SIZE;
    if((temp & (FILE_BLOCK_SIZE - 1)) != 0) // works only if FILE_BLOCK_SIZE is 2^x; check that there is a partial block
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
    md5(data->dataFile, data->size, md5R);
    if(memcmp(md5R, data->md5Res, 16)) // not equal
    {
        this->user->sendData(310);
        return;
    }
    fseek(this->file, data->blockNum * FILE_BLOCK_SIZE, SEEK_SET);
    fwrite(data->dataFile, 1, data->size, this->file);
    if(!this->blocks[data->blockNum])
        md5_append(&this->allFile, data->dataFile, data->size);
    this->blocks[data->blockNum] = 1;
    this->user->sendData(200, &data->blockNum, sizeof(data->blockNum));
}

bool_t FileTransfer::finishUpload(char* Buffer)
{
    byte_t md5file[16];
    md5_finish(&this->allFile, md5file);
    if(memcmp(md5file, Buffer, 16)) // not equal
    {
        this->user->sendData(311);
        return (FALSE);
    }
    else
    {
        this->user->sendData(200);
        return (TRUE);
    }
}

void FileTransfer::askForBlocksRange(unsigned int start, unsigned int end)
{
    for(; start < end; start++)
        askForBlock(start);
}

void FileTransfer::askForBlock(unsigned int blockNum)
{
    if(blockNum > this->blocksCount)
    {
        this->user->sendData(312, (char*)&(blockNum), sizeof(blockNum));
        return;
    }
    struct {
        int blockNum;
        unsigned short size;
        byte_t md5[16];
        byte_t data[FILE_BLOCK_SIZE];
    } buffer;
    fseek(this->file, blockNum * FILE_BLOCK_SIZE, SEEK_SET);
    buffer.size = fread(buffer.data, 1, FILE_BLOCK_SIZE, this->file);
    md5(buffer.data, buffer.size, buffer.md5);
    buffer.blockNum = blockNum;
    this->user->sendData(210, &buffer, 22 + buffer.size);
    if(!this->blocks[blockNum])
        md5_append(&this->allFile, buffer.data, buffer.size);
    this->blocks[blockNum] = 1;
}

void FileTransfer::finishDownload()
{
    byte_t md5file[16];
    md5_finish(&this->allFile, md5file);
    this->user->sendData(212, md5file, 16);
}
