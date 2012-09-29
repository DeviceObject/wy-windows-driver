
#include "rc4.h"

// RC4算法
void rc4_setup(struct rc4_state *s, unsigned char *key, int length)
{
	int i, j, k, *m, a;

	s->x = 0;
	s->y = 0;
	m = s->m;

	for(i = 0; i < 256; i++)
	{
		m[i] = i;
	}

	j = k = 0;

	for(i = 0; i < 256; i++)
	{
		a = m[i];
		j =(unsigned char)(j + a + key[k]);
		m[i] = m[j]; m[j] = a;
		if(++k >= length) k = 0;
	}
}

// RC4算法
void rc4_crypt(struct rc4_state *s, unsigned char *data, int length)
{ 
	int i, x, y, *m, a, b;

	x = s->x;
	y = s->y;
	m = s->m;

	for(i = 0; i < length; i++)
	{
		x =(unsigned char)(x + 1); a = m[x];
		y =(unsigned char)(y + a);
		m[x] = b = m[y];
		m[y] = a;
		data[i] ^= m[(unsigned char)(a + b)];
	}

	s->x = x;
	s->y = y;
}

// RC4算法
void rc4_crypt512(unsigned char *key, int keyLen, unsigned char *message, int messageLen)
{
	struct rc4_state rc4_test;
	rc4_setup(&rc4_test, key, keyLen);
	rc4_crypt(&rc4_test, message, messageLen);
}

//char* t_key = "123456789123456789"; 
// RC4算法
void rc4_cryptUnkownLen(unsigned char *key, int keyLen, unsigned char *message, int messageLen)
{
	int i =0;
	int total = messageLen/BUFFER_FIX_SIZE;
	int rem = messageLen %BUFFER_FIX_SIZE;
	//key = t_key;
	if(0 == total)
		return;

	if(total>0)
	{
		if(0 == rem)
		{
			do
			{
				rc4_crypt512(key, keyLen, message+(i*(BUFFER_FIX_SIZE)), BUFFER_FIX_SIZE);
				i++;
				// total--;
			}while(total>i);

		}
		else
		{
			do
			{
				rc4_crypt512(key, keyLen, message+(i*(BUFFER_FIX_SIZE)), BUFFER_FIX_SIZE);
				i++;
				// total--;
			}while(total>i);
			rc4_crypt512(key, keyLen, message+(i*(BUFFER_FIX_SIZE)), rem);

		}
	}
	else
	{
		rc4_crypt512(key, keyLen, message+(i*(BUFFER_FIX_SIZE)), rem);

	}
}