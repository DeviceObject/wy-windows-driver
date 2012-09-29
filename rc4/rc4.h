/**
 说明：这是一个RC4的实现，用于流加密系统
 作者：王煜
 时间：2012-09-26
 **/

#ifndef _RC4_H_
#define _RC4_H_

#ifdef __cplusplus
extern "C"
{
#endif

// RC算法
struct rc4_state
{
	int x, y, m[256];
};

#define BUFFER_FIX_SIZE 512

// RC4算法
void rc4_setup(struct rc4_state *s, unsigned char *key, int length);

// RC4算法
void rc4_crypt(struct rc4_state *s, unsigned char *data, int length);

// RC4算法
void rc4_crypt512(unsigned char *key, int keyLen, unsigned char *message, int messageLen);
 
// RC4算法
void rc4_cryptUnkownLen(unsigned char *key, int keyLen, unsigned char *message, int messageLen);

#ifdef __cplusplus
}
#endif

#endif