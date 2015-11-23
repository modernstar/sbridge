/*******************************************************************************
 *	File ans_tone.c: Answer tone generation.
 *
 *	Includes the subroutine:
 *
 *  1. AnswerTone.
 *
 *	Written by: Irit Hardy.
 *  Combined at: version 2.0 .
 *
 *******************************************************************************/
#include "anstone.h"

void AnswerTone(ANS_TONE *anst,short *Out,short OutLen)
/*******************************************************************************
 *	Description:
 *  Function AnswerTone generates a constant frequency tone,
 *  using a sinus table. Each time the index of the sinus table jumps
 *	according to sinus frequency.
 *	The table's length and the jump's magnitude correspond to tone
 *	frequency 2100 Hz, sampled at 9600 Hz.
 *	Every 450 msec the tone phase is inverted. 
 *
 *	Arguments:
 *  ANS_TONE *anst - Answer tone setting and history.
 *  short *Out - Output real samples.
 *  short OutLen - Number of samples to be generated.
 *******************************************************************************/
{
	short 	SinIndexVar;

	//-----Loop over input samples-----
	while(OutLen)
	{
  	   SinIndexVar=anst->SinIndex;
		OutLen--;
		//-----Promote the sinus table index according to the tone frequency-----
		anst->SinIndex = (short)((anst->SinIndex + ANST_FREQ) & ANST_SIN_PERIOD);
//		anst->counter--;
		if(anst->counter--==0)
		{
			//-----Invert the phase of the sinus-----
			anst->SinIndex = (short)((anst->SinIndex + ANST_SIN_HALF_PERIOD) & ANST_SIN_PERIOD);
			anst->counter=ANST_PHASE_INVERSION;
		}
      //-----Get an enetry from the sinus table-----
		*Out++ = (short)(anst->SinTable[SinIndexVar]>>2);
	}
}
