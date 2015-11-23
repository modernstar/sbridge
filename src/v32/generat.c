/*******************************************************************************
 * File generat.c: the signaling generators for the transmitter.
 *
 * Includes the subroutines:
 *
 * 1. BitGenerator.
 * 2. SymGenerator.
 *
 * Writen by: Victor Elkonin.
 * Combined at: version 2.0.
 *
 *******************************************************************************/
#include "transmt.h"

void BitGenerator(BIT_GEN *BitGen, unsigned short *Out, short OutNum)
/*******************************************************************************
 *	Description:
 *	Function BitGenerator generates signaling, that are defined by
 *	their bits content, i.e. TRN, R1-R3, E, B1.
 *	This function works only for 8 symbols per block.
 *	Besides B1 it return a single word.
 *
 *	Arguments:
 *	mode - the type of the signaling.
 *	unsigned short *Out - output bits array.
 *	short OutNum - number of bits to be generated.
 ********************************************************************************
 *	Changes:
 *	1. version 2.7: Generalization for arbitrary number of bits in one block.
 *******************************************************************************/
{
	unsigned short Temp;
	
	while (OutNum>0)
	{
		switch(BitGen->segment)
		{
			case TX_SEG_TRN:
				*(Out++) = TRN_WORD;
				break;

			case TX_SEG_R:
				Temp = (unsigned short) (BitGen->word << BitGen->shift);
				Temp += (unsigned short) (BitGen->word >> (16 - BitGen->shift));
				*(Out++) = (unsigned short) Temp;
				break;

			case TX_SEG_B1:
				*(Out++) = B1_WORD;
				break;

			case TX_SEG_B2:
				if (OutNum>16)
					*(Out++) = B1_WORD;
				else
				{
					*(Out-1) <<= (16-OutNum);
					*Out = 0;
				}
				break;
			default:
				break;
		} //end switch
		OutNum -= (short)16;
	} //end while

	BitGen->shift += OutNum; //promote the shift.
	BitGen->shift &= 0xf;	//make shift between 0 and 15.
}

void SymGenerator(short mode, unsigned short *Out, short SymNum, short SymPairs)
/*******************************************************************************
 *	Description:
 *	Function SymGenerator generates signaling, that are defined by
 *	the point on the constellation, i.e. AA, CC, AC, CA, S and Sbar.
 *	This function works only for 8 symbols per block.
 *	The function moves by itself from AA to CC and from CA to AC transmissions,
 *	when SymPairs equals to zero.
 *	In other modes there is no need to promote the mode inside the function.
 *
 *	Arguments:
 *	mode - the type of the signaling.
 *	unsigned short *Out - output bits array.
 *	short SymPairs - counter to transmission promotion (only for AA and AC).
 ********************************************************************************
 *	Changes:
 *	1. version 2.7: Generalization for arbitrary number of symbol pairs in
 one block.
 *******************************************************************************/
{
	short k;
	*Out=0;
	for(k=0; k<(SymNum>>1); k++)
		//-----Loop over symbols pairs-----
	{
		*Out<<=4;
		switch(mode)
		{
			case TX_SEG_AC:
				*Out |= AC_WORD;
				break;

			case TX_SEG_CA:
				if (SymPairs--) *Out |= CA_WORD;
				else
				{
					mode=TX_SEG_AC;
					*Out |= AC_WORD;
				}
				break;

			case TX_SEG_AA:
				if (SymPairs--) *Out |= AA_WORD;
				else
				{
					mode=TX_SEG_CC;
					*Out |= CC_WORD;
				}
				break;

			case TX_SEG_CC:
				*Out |= CC_WORD;
				break;

			case TX_SEG_S:
				*Out |= S_WORD;
				break;

			case TX_SEG_SBAR:
				*Out |= SBAR_WORD;
				break;

			default:
				break;
		} //end switch


		if((k&3) == 3)
		{
			//-----Start a new output word-----
			Out++;
			*Out = 0;
		}

	}//end for

	*Out <<= 4*(4-(k&3)); //align the last word to the right.
}
