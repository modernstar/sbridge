/*******************************************************************************
 *
 *	File timing.c: V32bis Symbol Timing.
 *
 *	Includes the subroutine:
 *  1. Timing
 *
 *	Written by: Irit Hardy.
 *
 *******************************************************************************/
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include "timing.h"

#ifdef V32_PRINTING
extern long SamplesCount;
#endif

short Timing(TIMING *timing,short *In, short *Out, short InLen)
{
	/*******************************************************************************
	 *	Description:
	 *	Timing Adjustment By Tone Estimation.
	 *  The Timing Estimation Is Done By Passing The Input Signal Through Two
	 *  BandPass(+-1200Hz) Filters.
	 *  The Timing Is Correct When The Real Result Of Multiplying This Two
	 *  Outputs Is Zero.
	 *  The Timing Adjustment Is Achived By Using 128 Sinc Tables.
	 *
	 *	Arguments:
	 *  Timing *Timing - Timing setting and history.
	 *  short *In      - Input Complex Samples Array.
	 *  short *Out     - Output complex samples array.
	 *  short InLen    - Number of Input  Samples *2 ( For Complex Values ).
	 *  Returns        - Number Of Output Samples *2.
	 *******************************************************************************/

	long acc0, acc1;
	long RateRound, LocRound;  //] Current Step Values.
	register short *DelayLinePtr1, *DelayLinePtr2, *SincTablePtr;
	short RateShift, LocShift; //]
	register short *OutPtr;
	short o1n, o1p, o2n, o2p;  // Estimation Filters Memory.
	short OutReal, OutImage;   // Temp Output Values.
	short OutLen=0;            // Out Length (Return Value).
	short index, HalfFilterLen;// Offset To The Start Of The Current Sinc
	short error;
	short i, k;

	// Selecting Converge Parameters From Table...
	if ( timing->SymCounter >= timing->ConvTbl[timing->ConvTblIndex].Counter )
	{
		timing->ConvTblIndex++;
	}
	LocShift  = timing->ConvTbl[timing->ConvTblIndex].LocShift;
	RateShift = timing->ConvTbl[timing->ConvTblIndex].RateShift;
	LocRound  = timing->ConvTbl[timing->ConvTblIndex].LocRound;
	RateRound = timing->ConvTbl[timing->ConvTblIndex].RateRound;
	// Increase Symbol Counter For Next Run...
	if ( timing->ConvTblIndex < TIMING_CONVERGE_TBL_LEN-1 )
	{
		timing->SymCounter += (short) (InLen>>2);
	}

	OutPtr = Out;
	k = 0;
	while (k<InLen)
	{
		while (timing->push_flag)
		{
			// If it is a double push, check if there are remaining input samples.
			if (k==InLen)
			{
				return OutLen;
			}

			//----- Push input symbol into delay line ----
			DelayLinePtr1 = timing->DelayLine+TIMING_DELAY_LEN-1;
			DelayLinePtr2 = timing->DelayLine+TIMING_DELAY_LEN-3;
			for (i=0; i<TIMING_DELAY_LEN-2; i++)
				*DelayLinePtr1-- = *DelayLinePtr2--;
			*DelayLinePtr1-- = In[k+1]; // Imaginary
			*DelayLinePtr1 = In[k]; 	// Real

			timing->push_flag--;
			k += (short)2;
		}
		timing->push_flag++;

		// ****************** Select Filter *******************
		// Select Filter...
		//take n upper bits of value (and sign).
		index=(short)((timing->loc+(TIMING_SELECT_FILTER_ROUND<<16))>>TIMING_SELECT_FILTER_SHIFT);

		//-------Passing Throw Sinc Filter-------

		//---Choose the correct sinc filter---
		index *= (short) TIMING_SINC_FILTER_LEN;

		DelayLinePtr1 = timing->DelayLine;

		//---Number of samples that are filtered during the sinc filter is ascending---
		HalfFilterLen = TIMING_HALF_DELAY_LEN;

		//---Place the sinc table pointer in the beginning of the sinc filter---
		SincTablePtr = timing->SincTable + index;
		if (index<=0)
		{
			HalfFilterLen+=(short)2;
			//---Move to the reversed sinc filter---
			SincTablePtr += TIMING_SINC_FILTER_NUM * TIMING_SINC_FILTER_LEN;
		}
		else
			//---Start from the second entry of the sinc filter---
			SincTablePtr++;

		acc0 = acc1 = 0L;
		for (i=0;i<(HalfFilterLen/2);i++)
		{
			acc0 += (long) (*DelayLinePtr1++ * *SincTablePtr);
			acc1 += (long) (*DelayLinePtr1++ * *SincTablePtr++);
		}
		
		//---Move to the reversed sinc filter---
		SincTablePtr = timing->SincTable-index + TIMING_SINC_FILTER_LEN - 1;
		if (index>0)
			//---Move to the reversed sinc filter---
			SincTablePtr += TIMING_SINC_FILTER_NUM * TIMING_SINC_FILTER_LEN;

		for (;i<(TIMING_DELAY_LEN/2);i++)
		{
			acc0 += (long) (*DelayLinePtr1++ * *SincTablePtr);
			acc1 += (long) (*DelayLinePtr1++ * *SincTablePtr--);
		}
		acc0<<=1;
		acc1<<=1;
		// Save Sample Outputs...

		acc0=(acc0+0x8000L)>>16;
		acc1=(acc1+0x8000L)>>16;
		*OutPtr++ = (short)(acc0);
		*OutPtr++ = (short)(acc1);
		OutLen += (short)2;

		// ****************** Error Estimation *******************
		// Normelizing Outputs To Avoid Overflow...
		OutReal  = (short) ((acc0 + ( 1<<(TIM_POWER-1) )) >> TIM_POWER);
		OutImage = (short) ((acc1+ ( 1<<(TIM_POWER-1) )) >> TIM_POWER);

		// Real 1200Hz
		acc0=2* (long)(timing->c1n*TIMING_CONST_A);
		o1p = (short)(OutReal - (short)((acc0+0x8000L)>>16));
		// Image 1200Hz
		acc1=2* (long)(timing->c1p*TIMING_CONST_A);
		o1n = (short)(OutImage + (short)((acc1+0x8000L)>>16));
		timing->c1p = o1p;
		timing->c1n = o1n;

		// Real -1200Hz
		acc0=2* (long)(timing->c2n*TIMING_CONST_A);
		o2p= (short)(OutReal + (short)((acc0+0x8000L)>>16));
		// Image -1200Hz
		acc1=2* (long)(timing->c2p*TIMING_CONST_A);
		o2n= (short)(OutImage - (short)((acc1+0x8000L)>>16));
		timing->c2p = o2p;
		timing->c2n = o2n;

			// Finding Real Of The Multiplication...
		acc0=2* (long)(o1p*o2p)+ 2* (long)(o1n*o2n);
		error=(short)((acc0+0x8000)>>16);

		// Finding Phase...
		acc0=2* (long)(TIMING_CONST_B*(short)((timing->bm+0x8000L)>>16));
		acc1=2* (long)(TIMING_CONST_B_1*error);
		timing->bm=acc1-acc0;

		//----------------- Checking The Timing State --------------
		
			//memcpy(LocalRecBufError++,&timing->bm,sizeof(long));

		error = (short)abs(error);
		error -= (short)TIMING_IDLE_THRESHOLD;

		if (timing->state == TIMING_IDLE)
		{
			if (error < 0)
			{
				timing->IdleCounter++;
			}
			else
				timing->IdleCounter=0;
			if (timing->IdleCounter >= TIMING_IDLE_NUM)
			{
#ifdef V32_PRINTING
				printf("TIMING_REGULAR %ld\n",SamplesCount);
#endif
				timing->state = TIMING_REGULAR;
				timing->IdleCounter=0;
			}
		}


		else if (timing->state == TIMING_REGULAR)
		{
			error -= (short)TIMING_REGULAR_THRESHOLD;

			if (error > 0)
			{
				timing->IdleCounter += (short)(error>>(-TIMING_IDLE_SHIFT));
				/*
				timing->IdleCounter++;
				if (error > 256)
				{
					timing->IdleCounter+=2;
					if (error > 1024) timing->IdleCounter+=14;
				}
				*/
			}
				else
					timing->IdleCounter=0;
			if (timing->IdleCounter >= TIMING_REGULAR_NUM)
			{
#ifdef V32_PRINTING
				printf("TIMING_IDLE %ld\n",SamplesCount);
#endif
				timing->state = TIMING_IDLE;
				timing->IdleCounter=0;
			}
		}



		// *********************** End Of Error Estimation ***********************

		timing->flag ^= 0xffff;
		// ************* Second Order PLL... ************

		if( timing->flag  )
		{
			//-----Idle or Regular State detection-----


		 //-----Rate and Phase correction-----
			if ((timing->state == TIMING_IDLE) || (timing->state == TIMING_SILENCE))
				timing->loc += timing->rate;
			else  //Regular and Start states
			{
				if ( RateShift != 32 ) // In C: Number=Number>>32
				{
					timing->rate += (((timing->bm)+RateRound)>>RateShift);
				}
				timing->loc += (((timing->bm)+LocRound)>>LocShift) + timing->rate;
			}

			// Checking If Double Push Is Needed...
			if ( (short) (timing->loc>>16) > TIME_THRESH )
			{
				timing->loc -= ONE31;
				timing->push_flag = 2;
			}

			// Checking If No Push Is Needed...
			if( (short) (timing->loc>>16) < -TIME_THRESH )
			{
				timing->loc += ONE31;
				timing->push_flag = 0;
			}

		}
		///// end PLL /////
	}

	return OutLen;

}// end timing function
