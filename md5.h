#ifndef md5_INCLUDED
#define md5_INCLUDED
#define MD5_RESULT_LENGTH 16
typedef unsigned char md5_byte_t; /* 8-bit byte */
typedef unsigned int md5_word_t; /* 32-bit word */

/* Define the state of the MD5 Algorithm. */
typedef struct md5_state_s {
    md5_word_t count[2];	/* message length in bits, lsw first */
    md5_word_t abcd[4];		/* digest buffer */
    md5_byte_t buf[64];		/* accumulate block */
} md5_context;

#ifdef __cplusplus
extern "C" 
{
#endif

/* Initialize the algorithm. */
void md5_init(md5_context *ctx);

/* Append a string to the message. */
void md5_append(md5_context *ctx, const md5_byte_t *data, int nbytes);

/* Finish the message and return the digest. */
void md5_finish(md5_context *ctx, md5_byte_t digest[16]);

#ifdef __cplusplus
}  /* end extern "C" */
#endif

#endif /* md5_INCLUDED */
