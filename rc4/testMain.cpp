

#include "rc4.h"
#include <iostream>
#include <WINDOWS.H>
using namespace std;

#define DATALEN	511
void main()
{
	//unsigned char *data = (unsigned char *)"1234567890qwertyuiop[]';lkjhgfdsazxcvbnm,./";
	int dataLength = DATALEN;

	unsigned char o[1024] = {0};

	unsigned char *key = (unsigned char *)"wangy";
	int keylen = strlen((char*)key);
	memset(o, 33, DATALEN);

	rc4_cryptUnkownLen(key, keylen, o, dataLength);

	rc4_cryptUnkownLen(key, keylen, o, dataLength);

	getchar();
}