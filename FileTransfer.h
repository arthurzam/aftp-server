#ifndef FILETRANSFER_H_
#define FILETRANSFER_H_

#include <cstdio>
#include "defenitions.h"

class User;

/*
 * controls the transfer of files in the socket
 */
class FileTransfer {
    private:
        typedef enum {
            STATE_ERROR = 0,
            STATE_UPLOAD,
            STATE_DOWNLOAD
        } STATE;

        STATE state = STATE::STATE_ERROR; // the current state of transfer
        uint32_t blocksCount = 0; // the amount of blocks in the transfered file
        uint32_t currentCursorBlock = 0; // the block where the FILE's cursor is located
        FILE* file = nullptr; // the file itself
        const User* user = nullptr; // just a pointer to the user controlling the current transfer
    public:
        /*
         * the size of a block of a file
         */
        static constexpr unsigned FILE_BLOCK_SIZE = 0x200; //=512

        FileTransfer() {};

        /*
         * set for Download situation.
         */
        bool reset(char* relativePath, const User* user);
        /*
         * set for Upload situation.
         */
        bool reset(char* relativePath, const User* user, uint32_t blocksCount);

        /*
         * returns true if the initialize succeeded, otherwise false.
         */
        inline bool isLoaded() const
        {
            return (this->state);
        }

        inline operator bool() const
        {
            return isLoaded();
        }

        /*
         * returns the count of blocks in the file
         */
        inline uint32_t getBlocksCount() const
        {
            return (this->blocksCount);
        }

        /*
         * a function that manages when the user asks for block number.
         * the function would send the block in the appropriate format.
         */
        void askForBlock(uint32_t blockNum);
        /*
         * a function that manages when the user asks for a range of blocks.
         * see function askForBlock(uint32_t)
         */
        inline void askForBlocksRange(uint32_t start, uint32_t end)
        {
            for(; start < end; ++start)
                askForBlock(start);
        }

        /*
         * a function that manages when the user sends a block.
         * the function would read the block using the appropriate format, and send OK/FAILURE.
         */
        void recieveBlock(const char* buffer);

        void close()
        {
            if(this->file)
                fclose(this->file);
            this->file = nullptr;
            this->state = STATE::STATE_ERROR;
        }

        ~FileTransfer()
        {
            close();
        }
};

#endif
