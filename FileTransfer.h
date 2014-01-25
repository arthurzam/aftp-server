#ifndef FILETRANSFER_H_
#define FILETRANSFER_H_

#include <stdio.h>
#include <stdlib.h>
#include "md5.h"
#include "defenitions.h"
#include "User.h"
#include "server.h"

typedef enum{
	FILE_TRANSFER_STATE_ERROR = 0,
	FILE_TRANSFER_STATE_UPLOAD,
	FILE_TRANSFER_STATE_DOWNLOAD
} FILE_TRANSFER_STATE;

#define FILE_BLOCK_SIZE 0x200 //=500

class FileTransfer {
private:
	FILE_TRANSFER_STATE state;
	md5_context allFile;
	byte_t* blocks; // pointer to HEAP located array
	unsigned int blocksCount;
	FILE* file;
	User* user; // just a pointer
public:
	FileTransfer(char* relativePath, User* user); // Download
	FileTransfer(char* relativePath, User* user, unsigned int blocksCount); // Upload

	bool_t isLoaded() const;
	void askForBlock(unsigned int blockNum);
	void askForBlocksRange(unsigned int start, unsigned int end);
	void recieveBlock(char* buffer, int dataLen);

	~FileTransfer();
};

#endif
