/*******************************************************************************
*	File diff.c: packing single symbols into continuous bits stream.
*
*	Includes the subroutine:
*
*  1. packing.
*
*	Written by: Mor Reich.
*  Combined at: version 2.0 .
*
*******************************************************************************/

void Packing(unsigned short *In, unsigned short *Out, short SymNum, short N);
void Packing(unsigned short *In, unsigned short *Out, short SymNum, short N)
{
/*******************************************************************************
*	Description:
*  Function packing works on integer number of symbols.
*	The current packed bits are kept in Acc0.
*
*	Arguments:
*  unsigned short *In  - Input bits array, each symbol is in a separate word.
*  unsigned short *Out - Output bits array, continuous bits stream .
*  short SymNum - Number of input symbols.
*	short N - number of bits per symbol.
*******************************************************************************/
	unsigned long Acc0;	//current packed bits.
   unsigned short symbol;	//The current symbol to be packed. 
   unsigned short FixSymBits;	//N ones aligned to the right.
   short LenAcc0;	////Number of bits in Acc0.
   short i,m;


   Acc0 = 0xffff;
   FixSymBits = (unsigned short) (Acc0>>(16-N));

   Acc0 = 0L;
   LenAcc0 = 0;
   i = m = 0;

	while (SymNum--)
   {
   	//-----Throw away the extra bit of the convolution encoding-----
		symbol = (unsigned short)(In[i++] & FixSymBits);
      Acc0 = (Acc0<<N) + symbol;
      LenAcc0 += N;
      if (LenAcc0 >= 16)
      {
      	//-----Write new output word-----
         Out[m++] = (unsigned short) (Acc0>>(LenAcc0-16));
         LenAcc0 -= (short) 16;
      }
   }
   if (LenAcc0) Out[m] = (unsigned short) (Acc0<<(16-LenAcc0));
}





