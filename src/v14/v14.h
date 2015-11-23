#ifndef __V14_H__
#define __V14_H__
void v14_encode(unsigned char *in, unsigned char *out, int inLen, int outLen, int *pState);
int v14_decode(unsigned char *in, unsigned char *out, int inLen, int *pState);
#endif
