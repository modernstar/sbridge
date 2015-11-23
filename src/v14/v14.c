#include "v14.h"
#define DATA_BITS_NUM 8

//Adding start and stop bits. 
//The function builds a frame with a given length.
//in : points to the raw data buffer
//out : points to data with start/stop bits buffer
//inLen : number of raw data bits.
//outLen : number of bits in the output frame.
//pState: pointer to the position of the current bit in the character. Init to zero.
void v14_encode(unsigned char *in, unsigned char *out, int inLen, int outLen, int *pState)
{
	int state = *pState;
	unsigned char bit = 1;
	int num=0,i;
	*out=0;

	for (i=0; i<inLen; )
	{
		num++;
		(*out)<<=1;
		if (state<0)
			(*out)++;
		if (state>0)
		{
			i++;
			if (*in&bit)
				(*out)++;
			bit<<=1;
			if (!bit)
			{
				bit = 1;
				in++;
			}
		}		
		if ((num&7)==0)
			out++;
		if (state++==DATA_BITS_NUM)
			state=-1;
	}
	
	while (num++<outLen)
	{
		*out<<=1;
		(*out)++;
		if ((num&7)==0)
			out++;
	}

	num--;
	if (num&7)
		*out<<=(8-(num&7));

	*pState = state;
}	


//Removing start and stop bits. 
//in : points to data with start/stop bits buffer
//out : points to the raw data buffer
//inLen : number of bits including start and stop bits.
//pState: pointer to the position of the current bit in the character. Init to zero.
//return value : number of raw data bits.
int v14_decode(unsigned char *in, unsigned char *out, int inLen, int *pState)
{
	int state = *pState;
	unsigned char bit = 0x80, tbit, obit=1;
	int num=0,i;
	*out=0;
	
	for (i=0; i<inLen; i++)
	{
		tbit = *in&bit;
		if (state<0)
		{
			if (!tbit)
				state=1;
		}	
		else
		{
			if (tbit)
				(*out)|=obit;

			obit<<=1;
			num++;
			if (!obit)
			{
				obit=1;
				out++;
				*out=0; //Warning: zeroing next word, be sure that it does not overwrites forbidden memory.
			}
		
			if (state++==DATA_BITS_NUM)
				state=-1;
		}
		bit>>=1;
		if (!bit)
		{
			bit = 0x80;
			in++;
		}
	}
	
	*pState = state;
	return num;
}	

	
