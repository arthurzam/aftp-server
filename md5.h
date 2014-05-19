#ifndef md5_INCLUDED
#define md5_INCLUDED
#define MD5_RESULT_LENGTH 16

#include "defenitions.h"

typedef unsigned int md5_word_t; /* 32-bit word */

/* Define the state of the MD5 Algorithm. */
typedef struct md5_state_s {
    md5_word_t count[2];	/* message length in bits, lsw first */
    md5_word_t abcd[4];		/* digest buffer */
    uint8_t buf[64];		/* accumulate block */
} md5_context;

#ifdef __cplusplus
extern "C"
{
#endif

/* Initialize the algorithm. */
void md5_init(md5_context *ctx);

/* Append a string to the message. */
void md5_append(md5_context *ctx, const uint8_t *data, int nbytes);

/* Finish the message and return the digest. */
void md5_finish(md5_context *ctx, uint8_t digest[MD5_RESULT_LENGTH]);

/* Calculate he whole md5 of the given data into digest */
void md5(const void* data, int nbytes, uint8_t digest[MD5_RESULT_LENGTH]);

#ifdef __cplusplus
}  /* end extern "C" */
#endif

#endif /* md5_INCLUDED */
