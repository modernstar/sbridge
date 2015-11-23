/*******************************************************************************
*	File diff.c: rate convertions.
*
*	Includes the subroutines:
*
*  1. rat8t9_6.
*  2. rat9_6t8.
*
*	Written by: Guy Ben-Yehuda.
*  Combined at: version 2.0 .
*
*******************************************************************************/
#include  "rate.h"

/************************************************************
 *     Subroutines:
 *     ------------
 *     - rat8t9_6(RATE *Rate, short *In, short *Out, short InLen, short *OutLen)
 *
 *        Convert Input Vector Sampeld At 8000Hz Length InLen To Output Vector
 *      								  Sampeld At 9600Hz.
 *
 *        Inputs:
 *        =======
 *				RATE *Rate
 *				short *In     - Pointer To Input Vector(8000Hz)
 *				short InLen   - Length Of Input Vector
 *
 *        Outputs:
 *        ========
 *				RATE *Rate
 *				short *Out    - Pointer To The Output Vector(9600Hz)
 *				short *OutLen - Pointer To The OutPut Vector Length
 *
 *************************************************************/

short rat8t9_6(RATE *Rate, short *In, short *Out, short InLen)
{
/*******************************************************************************
*	Description:
*  Function rat8t9_6 converts a signal sampled at 8000 samples/second
*	into a signal sampled at 9600 samples/second.
*	The convertion is performed using sinc interpolation.
*
*	Arguments:
*  RATE *Rate - Rate convertor setting and history.
*  short *In  - Input real samples array.
*  short *Out - Output real samples array.
*  short InLen - Number of input samples.

*	Returned value: Number of output samples.
*******************************************************************************/
	register short Push; // Offset Of The Input Samples(0..InLen)
	register short Pop;  // Offset Of The Output Samples(0..OutLen)
	register short *DelayLinePtr1, *DelayLinePtr2, *SincTblPtr;

	short i, n;
	long Acc0;

	Push = Pop =0;
	while (Push<InLen)
	{
		if (Rate->PhaseMem != 0)    //addition of a sample when phase reaches its max.
		{
			// ------- Push To Delay Line --------
			DelayLinePtr1 = Rate->DelayLine+RATE_LEN-1;
			DelayLinePtr2 = DelayLinePtr1-1;
			for (i=0; i<RATE_LEN-1; i++)
				*DelayLinePtr1-- = *DelayLinePtr2--; // Delay Cmd In The DSP
			*DelayLinePtr1 = In[Push++];
		}

		// ------- Calculate Output Sample --------
		n = (short)((++Rate->PhaseMem) * RATE_LEN - 1); // End Of Current Sinc Table

		DelayLinePtr1 = Rate->DelayLine;
		SincTblPtr = Rate->Table+n;
		Acc0 = 0;
		for (i=0; i<RATE_LEN; i++)
			Acc0 += 2* (long) *DelayLinePtr1++ * *SincTblPtr--;
		Out[Pop++] = (short) ( (Acc0+0x8000)>>16 );

		if (Rate->PhaseMem == 6)
			Rate->PhaseMem = 0;
	}

	return Pop;
}

/*************************************************************
 *
 *     Subroutine:
 *     ------------
 *     - OutLen=rat9_6t8(RATE *Rate, short *In, short *Out, short InLen)
 *
 *        Convert Input Vector Sampeld At 9600Hz Length InLen To Output Vector
 *      								  Sampeld At 8000Hz.
 *
 *        Inputs:
 *        =======
 *				RATE *Rate
 *				short *In     - Pointer To Input Vector(9600Hz).
 *				short InLen   - Length Of Input Vector.
 *
 *        Outputs:
 *        ========
 *				RATE *Rate
 *				short *Out    - Pointer To The Modified Output Vector(8000Hz).
 *				short OutLen  - OutPut Vector Length.
 *
 *************************************************************/
short rat9_6t8(RATE *Rate, short *In, short *Out, short InLen)
{
/*******************************************************************************
*	Description:
*  Function rat9_6t8 converts a signal sampled at 9600 samples/second
*	into a signal sampled at 8000 samples/second.
*	The convertion is performed using sinc interpolation.
*
*	Arguments:
*  RATE *Rate - Rate convertor setting and history.
*  short *In  - Input real samples array.
*  short *Out - Output real samples array.
*  short InLen - Number of input samples.

*	Returned value: Number of output samples.
*******************************************************************************/
	register short Push; // Offset Of The Input Samples(0..InLen)
	register short Pop;  // Offset Of The Output Samples(0..OutLen)
	register short *DelayLinePtr1, *DelayLinePtr2, *SincTblPtr;
	short i,n;
	register long Acc0;

	Push = Pop =0;
	while (Push<InLen)
	{
		// ------- Push To Delay Line --------
		DelayLinePtr1 = Rate->DelayLine+RATE_LEN-1;
		DelayLinePtr2 = DelayLinePtr1-1;
		for (i=0; i<RATE_LEN-1; i++)
			*DelayLinePtr1-- = *DelayLinePtr2--; // Delay Cmd In The DSP
		*DelayLinePtr1 = In[Push++];

		if (Rate->PhaseMem > 0)
		{
			// ------- Calculate Output Sample --------
			n = (short)((Rate->PhaseMem) * RATE_LEN - 1);

			DelayLinePtr1 = Rate->DelayLine;
			SincTblPtr = Rate->Table+n;
			Acc0 = 0;
			for (i=0; i<RATE_LEN; i++)
				Acc0 += 2* (long) *DelayLinePtr1++ * *SincTblPtr--;
			Out[Pop++] = (short) ( (Acc0+0x8000)>>16 );
		}

		if (Rate->PhaseMem == 5)
			Rate->PhaseMem = -1; // No Calculate Next Loop
		Rate->PhaseMem++;
	}
	return Pop;
}

