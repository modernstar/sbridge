/*******************************************************************************
 *	File conv.c: convolution encoding.
 *
 *	Includes the subroutines:
 *
 *  1. ConvInit.
 *  2. ConvEncode.
 *
 *	Written by: Mor Reich.
 *  Combined at: version 2.0 .
 *
 *******************************************************************************/

#include	"conv.h"

void ConvInit(CONV *conv, const unsigned short *Table, short N)
{
	conv->state = 0;
	conv->N = N;
	conv->Table = (unsigned short *)Table;
}


void ConvEncode(CONV *conv, unsigned short *In, unsigned short *Out, short SymNum)
{
	/*******************************************************************************
	 *	Description:
	 *  Function ConvEncode works on integer number of symbols.
	 *	The encoding is performed using a table, whose index is combined from
	 *	the old state and the 2 input bits and value combined from the new state
	 *	and 3 output bits.
	 *	The current bits before encoding are kept in Acc0.
	 *	The current encoded bits are kept in Acc1.
	 *  The Conv Table Size Is 32 = [3 Memory Bits , 2 New Bits]
	 *  The Conv Table Outputs 6 Bits = [1 Redundent, 2 Outputs Bits, 3 Memory Bits]
	 *
	 *	Arguments:
	 *  CONV conv - Encoder setting and history.
	 *  unsigned short *In  - Input bits array.
	 *  unsigned short *Out - Output bits array.
	 *  short SymNum - Number of input symbols.
	 *
	 *******************************************************************************/

	register unsigned long Acc0;	//current bits before encoding.
	register unsigned long Acc1;  //current encoded bits.
	unsigned short FixLowBits;	//N-2 ones aligned to the right.
	unsigned short InSym, OutSym;
	unsigned short dibit;	//Most significant two bits of the input symbol.
	unsigned short TblValue;	//The values of the convolution table,0..63: Y0 Y1 Y2 s1 s2 s3.
	short N; // = 6 For 14400 bps
	short LenAcc0, LenAcc1;	//Number of bits in Acc0 and Acc1.
	register short i, m;

	N = conv->N;
	Acc0 = 0xf;
	FixLowBits =(unsigned short)(Acc0>>(6-N));
	LenAcc0 = LenAcc1 = 0;
	i = m = 0;
	Acc0 = Acc1 = 0L;

	//----Loop over symbols-----
	while (SymNum--)
	{
		if (LenAcc0<N)
		{
			//-----Load the new word into Acc0-----
			Acc0 += (unsigned long) (In[i++] << (N-LenAcc0));
			LenAcc0 += (short) 16;
		}
		//-----Encode the symbol-----
		InSym = (unsigned short)(Acc0 >> 16);
		dibit = (unsigned short)( (InSym >> (N-2)) & 3);
		TblValue = conv->Table[dibit+( (conv->state) << 2)];
		conv->state = (short) (TblValue & 7);
      OutSym = (unsigned short)(TblValue >> 3);
		OutSym = (unsigned short)((InSym & FixLowBits) + ( OutSym << (N-2) ));
		Acc1 <<= (short) (N+1);
		Acc1 += (unsigned short) OutSym;
		//-----Update counters-----
		LenAcc0 -= (short) N;
		LenAcc1 += (short) (N+1);
		//-----Promote Acc0 to the next symbol-----
		Acc0 <<= (unsigned short) N;
		if (LenAcc1>=16)
		{
			//-----Write new output word-----
			Out[m] = (unsigned short) (Acc1 >> (LenAcc1-16));
			LenAcc1 -= (short)16;
			m++;
		}
	}	// end of while

	//-----write the last,not completed word-----
	if (LenAcc1)
		Out[m] = (unsigned short) (Acc1 << (16-LenAcc1));
}

