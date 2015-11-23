/*******************************************************************************
 *	File scramblr.c: scrambling and descrambling.
 *
 *	Includes the subroutines:
 *
 *  1. ScramInit.
 *  2. Scrambler.
 *  3. Descrambler.
 *
 *	Written by: Mor Reich.
 *  Combined at: version 2.0 .
 ********************************************************************************
 *	Changes:
 *	version 2.2: 	Changes in function Scrambler.
 *******************************************************************************/

#include "scram.h"

void	ScramInit(SCRAMBLER *scrm, short identity)
{
	scrm->mode = identity;
	scrm->Register = 0;
}

void	Scrambler(SCRAMBLER	*scrm, unsigned short *In, unsigned short *Out, short BitNum)
{
	/*******************************************************************************
	 *	Description:
	 *  Function Scrambler works on arbitrary number of bits.
	 *  It performs a loop over input bits, and for each bit it calculates
	 *  the corresponding scrambeled bit.
	 *
	 *	Arguments:
	 *  SCRAMBLER scrm - Scrambler setting and history.
	 *  unsigned short *In  - Input bits array.
	 *  unsigned short *Out - Output bits array.
	 *  short BitNum - Number of input bits.
	 ********************************************************************************
	 *	Changes:
	 *	1. version 2.2: Correction in loading input words.
	 *******************************************************************************/
	short	i;
	short BitsInWord;	//Number of bits left in the current input word.
	unsigned short Al0,Al1;	//The input and output words.
	unsigned short bit;	//bit:New Bit of the Output Word.

	i =  0;
	BitsInWord = 16;
	Al0 = In[0];
	Al1 = 0;

	//-----Loop over bits-----
	while(BitNum--)
	{
		if (BitsInWord == 0)
		{
			//-----Move the scrambeled word to the output-----
			Out[i++] = Al1;
			//-----Load the new word into Al0-----
			Al0 = In[i];
			Al1 = 0;
			BitsInWord = 16;
		}
		//-----Calculate the new output bit-----
		if (scrm->mode)
			bit = (unsigned short)((Al0 ^ (unsigned short)(scrm->Register>>7) ^ (unsigned short)(scrm->Register<<11))& 0x8000);
		else
			bit = (unsigned short)((Al0 ^ (unsigned short)(scrm->Register>>7) ^ (unsigned short)(scrm->Register>>2)) & 0x8000);
		//-----Align the bit to right (the bit can be 0 or 0x8000)-----
		if (bit) bit=1;
		//Update output word, the history register and the counters-----
		Al1 = (unsigned short)  ((Al1<<1) | bit);
		scrm->Register = (scrm->Register<<1) | bit;
		BitsInWord--;
		//-----advance to the next input bit-----
		Al0<<=1;
	} //end while
	//-----Align the last word to the left.
	if (BitsInWord)
		Al1<<=BitsInWord;
	Out[i]=Al1;
} //end Scrambler


void Descrambler(SCRAMBLER	*scrm, unsigned short *In, unsigned short *Out, short BitNum)
{
	/*******************************************************************************
	 *	Description:
	 *  Function Descrambler works on arbitrary number of bits.
	 *  It performs a loop over input bits, and for each bit it calculates
	 *  the corresponding descrambeled bit.
	 *
	 *	Arguments:
	 *  SCRAMBLER scrm - Descrambler setting and history.
	 *  unsigned short *In  - Input bits array.
	 *  unsigned short *Out - Output bits array.
	 *  short BitNum - Number of input bits.
	 *******************************************************************************/
	short	i;
	short BitsInWord;	//Number of bits left in the current input word.
	unsigned short Al0,Al1;	//The input and output words.
	unsigned short bit;	//New Bit of the Output Word.

	i =  0;
	BitsInWord = 16;
	Al0 = In[0];
	Al1 = 0;

	//-----Loop over bits-----
	while(BitNum--)
	{
		if (BitsInWord == 0)
		{
			//-----Move the descrambeled word to the output-----
			Out[i++] = Al1;
			//-----Load the new word into Al0-----
			Al0 = In[i];
			Al1 = 0;
			BitsInWord = 16;
		}
		//-----Calculate the new output bit-----
		if (scrm->mode)
			bit = (unsigned short)((Al0 ^ (unsigned short)(scrm->Register>>7) ^ (unsigned short)(scrm->Register<<11))& 0x8000);
		else
			bit = (unsigned short)((Al0 ^ (unsigned short)(scrm->Register>>7) ^ (unsigned short)(scrm->Register>>2)) & 0x8000);
		//-----Align the bit to right (the bit can be 0 or 0x8000)-----
		if (bit) bit=1;
		//Update output word, the history register and the counters-----
		Al1 = (unsigned short)  ((Al1<<1) | bit);
		scrm->Register <<= 1;
		if (Al0 & 0x8000) scrm->Register |= 1;
		BitsInWord--;
		//-----advance to the next input bit-----
		Al0<<=1;
	} //end while
	//-----Align the last word to the left.
	if (BitsInWord)
		Al1<<=BitsInWord;
	Out[i]=Al1;
} //end Descrambler



