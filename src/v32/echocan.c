/*******************************************************************************
 *	File echocan.c: echo canceller.
 *
 *	Includes the subroutine:
 *
 *  1. EchoCanceller
 *  2. EchoOffset
 *
 *	Written by: Irit Hardy.
 *  Combined at: version 2.0 .
 *
 *******************************************************************************/
#include <math.h>
#include <memory.h>
#include <stdio.h>
#define SYM_PER_BLOCK 8
#include "echocan.h"
#ifdef V32_TESTING
#include "testing.h"
extern short identity;
#endif

void EchoCanceller(ECHO_CNL *EcCnl,short *In,short *Out, short InLen,short *InSymNear, short *InSymFar)
{
	/*******************************************************************************
	 *	Description:
	 *	Near and Far Echo cancellation.
	 *  The function works on integer number of symbols.
	 *  The function implements the LMS algorithm.
	 *  LMS coefficients are 32 bits long.
	 *  Converge Is Achieve Using Convergence Tables Using Increasing Lengths.
	 *  There are two samples per symbol, therefore two LMS filters are calculated.
	 *
	 *	Arguments:
	 *  ECHO_CNL EcCnl - Echo Canceller setting and history.
	 *  short *In  - Input complex samples array.
	 *  short *Out - Output complex samples array.
	 *  short InLen - Number of Samples*2 ( For Complex Values ).
	 *  short *InSym - Input complex symbols array.
	 *******************************************************************************/

	// Global Use Registers
	register short *Ptr1, *Ptr2;
	register long *LPtr;
	long Acc0, Acc1;

	short Error1R, Error1I, Error2R, Error2I; // Real & Complex LMS Errors
	short ConvIndexNear,ConvIndexFar;  // Indexes To The Converge Table
	short StepNear,StepFar;            // Near, Far Steps Temporary Registers
	short DelayNearLen_2, DelayFarLen_2;// Near, Far Current Delay Line Length/2
	short *DelayNearPtr, *DelayFarPtr; // Ptrs To The Start Of The Current Delay Line

	//-----Pointers to LMS Coefficients-----
	long *hn1Ptr, *hn2Ptr, *hf1Ptr, *hf2Ptr; // Ptrs To The Current LMS Filter Start

	short *DelayPtr;                            //]
	long *hPtr;                                 //] Loop Temporery Registers
	short DelayLen_2;       // Delay Length/2   //]
   short ErrorReal, ErrorImag, step; 			  //]
	short Shift,ShiftNear,ShiftFar;  // double talk step.
	unsigned short RoundDbl, RoundNear, RoundFar;  // double talk step.
	short i,j,k,m;
	short DelayNorm, StepNorm;
	m=0;

	//----- Copy Converge Parameters from the structure to local variables ----
	DelayNorm=EcCnl->DelayNorm;
	StepNorm =EcCnl->StepNorm;
	ShiftNear = (short)(ECH_SHIFT_NEAR-EcCnl->ShiftNorm);
	ShiftFar = (short)(ECH_SHIFT_FAR-EcCnl->ShiftNorm);
	RoundNear = RoundFar = 1;
   RoundNear <<= ShiftNear-1;
	RoundFar <<= ShiftFar-1;

	// Near Echo Converge Parameters...
   ConvIndexNear = EcCnl->ConvTblIndexNear;

	if ( EcCnl->SymCounter >= EcCnl->ConvTblNear[EcCnl->ConvTblIndexNear].Counter )
	{
		ConvIndexNear++;
		EcCnl->ConvTblIndexNear=ConvIndexNear;
		EcCnl->StepNear = (short)(EcCnl->ConvTblNear[ConvIndexNear].StepSize * StepNorm);
		EcCnl->DelayNearPtr = EcCnl->DelayLineNear + EcCnl->NearOff;
		EcCnl->DelayNearPtr +=  (EcCnl->ConvTblNear[ConvIndexNear].CoefStart);
		EcCnl->DelayNearLen = EcCnl->ConvTblNear[ConvIndexNear].CoefLen;
		EcCnl->hn1Ptr = EcCnl->hn1 + (EcCnl->ConvTblNear[ConvIndexNear].CoefStart);
		EcCnl->hn2Ptr = EcCnl->hn2 + (EcCnl->ConvTblNear[ConvIndexNear].CoefStart);
	}
	StepNear=EcCnl->StepNear;
	DelayNearPtr=EcCnl->DelayNearPtr;
	DelayNearLen_2=(short)( (EcCnl->DelayNearLen/2)-1 );
	hn1Ptr=EcCnl->hn1Ptr;
	hn2Ptr=EcCnl->hn2Ptr;

	// Far Echo Converge Parameters...
   ConvIndexFar = EcCnl->ConvTblIndexFar;
	if ( EcCnl->SymCounter >= EcCnl->ConvTblFar[EcCnl->ConvTblIndexFar].Counter )
	{
		ConvIndexFar++;
		EcCnl->ConvTblIndexFar=ConvIndexFar;
		EcCnl->StepFar = (short)(EcCnl->ConvTblFar[ConvIndexFar].StepSize * StepNorm);
		EcCnl->DelayFarPtr = EcCnl->DelayLineFar + EcCnl->FarOff;
		EcCnl->DelayFarPtr += (EcCnl->ConvTblFar[ConvIndexFar].CoefStart);
		EcCnl->DelayFarLen = EcCnl->ConvTblFar[ConvIndexFar].CoefLen;
		EcCnl->hf1Ptr = EcCnl->hf1 + (EcCnl->ConvTblFar[ConvIndexFar].CoefStart);
		EcCnl->hf2Ptr = EcCnl->hf2 + (EcCnl->ConvTblFar[ConvIndexFar].CoefStart);
	}
	StepFar=EcCnl->StepFar;
	DelayFarPtr=EcCnl->DelayFarPtr;
	DelayFarLen_2=(short) ( (EcCnl->DelayFarLen/2)-1 );
	hf1Ptr=EcCnl->hf1Ptr;
	hf2Ptr=EcCnl->hf2Ptr;


	for (k=0;k<(InLen>>1);k+=(short)2)
	{
		//----- Push input symbol into near delay line ----
		Ptr1 = EcCnl->DelayLineNear+ECH_SYM_DELAY_LEN-1;
		Ptr2 = EcCnl->DelayLineNear+ECH_SYM_DELAY_LEN-3;

		for (i=0; i<ECH_SYM_DELAY_LEN-2; i++)
			*Ptr1-- = *Ptr2--;
		*Ptr1-- = (short)(InSymNear[k+1] >> DelayNorm); // imaganary
		*Ptr1 = (short)(InSymNear[k] >> DelayNorm); 	 // real

		//----- Push input symbol into far delay line ----
		Ptr1 = EcCnl->DelayLineFar+ECH_SYM_DELAY_LEN-1;
		Ptr2 = EcCnl->DelayLineFar+ECH_SYM_DELAY_LEN-3;

		for (i=0; i<ECH_SYM_DELAY_LEN-2; i++)
			*Ptr1-- = *Ptr2--;
         
		*Ptr1-- = (short)(InSymFar[k+1] >> DelayNorm); // imaganary
		*Ptr1 = (short)(InSymFar[k] >> DelayNorm); 	 // real

		//------- Estimation of Phase1 --------
		Acc0=Acc1=0L;

		// Near echo
		Ptr1 = DelayNearPtr;
		LPtr = hn1Ptr;
		for (i=0;i<=DelayNearLen_2;i+=(short)1)
		{
			// Convoluting input symbols with LMS filter
			Acc0+=2*(long)( *Ptr1 * (short)(*LPtr>>16) )-
				   2*(long)( *(Ptr1+1) * (short)(*(LPtr+1)>>16) ); //Real
			Acc1+=2*(long)( *(Ptr1+1) * (short)(*LPtr>>16) )+
				   2*(long)( *Ptr1 * (short)(*(LPtr+1)>>16) );     //Image
			Ptr1+=2; LPtr+=2;
		}

		// Far echo
		Ptr1 = DelayFarPtr;
		LPtr = hf1Ptr;
		for (i=0;i<=DelayFarLen_2;i+=(short)1)
		{
			// Convoluting input symbols with LMS filter
			Acc0+=2* (long)( (*Ptr1 * (short)(*LPtr>>16)) )-
					2* (long)( (*(Ptr1+1) * (short)(*(LPtr+1)>>16)) );
			Acc1+=2* (long)( (*(Ptr1+1) * (short)(*LPtr>>16)) )+
					2* (long)( (*Ptr1 * (short)(*(LPtr+1)>>16)) );
			Ptr1+=2; LPtr+=2;
		}

		//-------- Calculation of error and output samples --------
		Out[m]=Error1R=(short)(In[m] - (short)((Acc0 + 0x8000L)>>16));
		Out[m+1]=Error1I=(short)(In[m+1] - (short)((Acc1 + 0x8000L)>>16));
		m+=(short)2;

		//-------- Estimation of Phase2 --------
		Acc0=Acc1=0L;

		// Near Echo
		Ptr1 = DelayNearPtr;
		LPtr = hn2Ptr;
		for (i=0;i<=DelayNearLen_2;i+=(short)1)
		{
			// Convoluting input symbols with LMS filter
			Acc0+=2*(long) ( *Ptr1 * (short)(*LPtr>>16) )-
					2*(long) ( *(Ptr1+1) * (short)(*(LPtr+1)>>16) );
			Acc1+=2*(long) ( *(Ptr1+1) * (short)(*LPtr>>16) )+
					2*(long) ( *Ptr1 * (short)(*(LPtr+1)>>16) );
			Ptr1+=2; LPtr+=2;
		}

		// Far Echo
		Ptr1 = DelayFarPtr;
		LPtr = hf2Ptr;
		for (i=0;i<=DelayFarLen_2;i+=(short)1)
		{
			// Convoluting input symbols with LMS filter
			Acc0+=2* (long)( *Ptr1 * (short)(*LPtr>>16) )-
					2* (long)( *(Ptr1+1) * (short)(*(LPtr+1)>>16) );
			Acc1+=2* (long)( *(Ptr1+1) * (short)(*LPtr>>16) )+
				  	2* (long)( *Ptr1 * (short)(*(LPtr+1)>>16) );
			Ptr1+=2; LPtr+=2;
		}

		//-------- Calculation of error and output samples --------
		Out[m]=Error2R=(short)(In[m] - (short)((Acc0 + 0x8000L)>>16));
		Out[m+1]=Error2I=(short)(In[m+1] - (short)((Acc1 + 0x8000L)>>16));
		m+=(short)2;

		//-------- Updating LMS Coefficients --------
		for (j=0; j<4; j++)
			// Two phases for near and far LMS filters
		{
			switch (j)
			{
				case 0:	// Near phase 1
					DelayPtr = DelayNearPtr;
					DelayLen_2 = DelayNearLen_2;
					ErrorReal = Error1R;
					ErrorImag = Error1I;
					hPtr = hn1Ptr;
					step = StepNear;
					Shift = ShiftNear;
					RoundDbl = RoundNear;
					break;
				case 1:	// Near phase 2
					DelayPtr = DelayNearPtr;
					ErrorReal = Error2R;
					ErrorImag = Error2I;
					hPtr = hn2Ptr;
					break;
				case 2:	// Far phase 2
					DelayPtr = DelayFarPtr;
					DelayLen_2 = DelayFarLen_2;
					step = StepFar;
					hPtr = hf2Ptr;
					Shift = ShiftFar;
					RoundDbl = RoundFar;
					break;
				case 3:	// Far phase 1
					DelayPtr = DelayFarPtr;
					ErrorReal = Error1R;
					ErrorImag = Error1I;
					hPtr = hf1Ptr;
					break;
			}


			if(EcCnl->mode==ECHO_CANCELLER_SINGLE)
			{
				for (i=0;i<=DelayLen_2;i+=(short)1)
				{
					// Real coefficient update
					Acc0  = 2* (long) (*DelayPtr * ErrorReal);
					Acc0 += 2* (long) (*(DelayPtr+1) * ErrorImag);
					*hPtr++ += 2*(long)( (short)((Acc0+0x8000L)>>16) * step );

					// Image coefficient update
					Acc1  = 2* (long) (*DelayPtr * ErrorImag);
					Acc1 -= 2* (long) (*(DelayPtr+1) * ErrorReal);
					*hPtr++ += 2*(long)( (short)((Acc1+0x8000L)>>16) * step );
					DelayPtr+=2;
				}
			}
			else
			{  // Double Talk
            hPtr += (short)(k&2);
            DelayPtr += (short)(k&2);
				for (i=0;i<=DelayLen_2;i+=(short)2)
				{
					// Real coefficient update
					Acc0  = 2* ((long) (*DelayPtr * ErrorReal));
					Acc0 += 2* ((long) (*(DelayPtr+1) * ErrorImag));
               Acc0 += (unsigned long) RoundDbl;
					*hPtr++ += (long)(Acc0 >> Shift);

					// Image coefficient update
					Acc1  = 2*((long) (*DelayPtr * ErrorImag));
					Acc1 -= 2*((long) (*(DelayPtr+1) * ErrorReal));
               Acc1 += (unsigned long) RoundDbl;
					*hPtr++ += (long)(Acc1 >> Shift);
					DelayPtr+=4; hPtr+=2;
				}
			}
		} // end of LMS coefficient update loop

	} // end of loop over input symbols

	// Increase Symbol Counter
	if ( (ConvIndexNear < ECHO_NEAR_CONVERGE_TBL_LEN-1) ||
		 (ConvIndexFar < ECHO_FAR_CONVERGE_TBL_LEN-1) )
		EcCnl->SymCounter+=(short)(InLen>>2);
} // end function

// *****************************************************************



void EchoOffset(ECHO_CNL *EcCnl, ECHO_BIT_BUF *EchoBitBuf)
/*******************************************************************************
 *	Description:
 *	Calculates Near And Far Initial Offset Values.
 *
 *	Inputs:
 *  ECHO_CNL EcCnl - Echo Canceller setting and history.
 *  extern short identity - "0" for call, "1" for ans
 *
 *  Outputs:
 *  EcCnl->NearOff
 *  EcCnl->FarOff
 *******************************************************************************/
{
	short ConvIndexNear, ConvIndexFar;

#ifndef V32_TESTING
      // Values For Running Without Testing
   	//EcCnl->NearOff=30;
      EchoBitBuf->NearOffBlock = 1;
      EcCnl->NearOff=6;
		//EcCnl->FarOff=300;
      EchoBitBuf->FarOffBlock = 12;
      EcCnl->FarOff=EchoBitBuf->FarOffSym=12;
#else
 	// Values For Running With Testing
	// Updating EcCnl->NearOff, EcCnl->FarOff  and   EchoBitBuf
	if ( !identity )
	{ // Call

		//EcCnl->NearOff = 10+7+9-18+1; // = (shape)+(2*rate)+(dcmf)-(echodelayline) [2400 bps]
		//EcCnl->NearOff += (short) ( 2*SIM_BLOCK_INITIAL_LEN*1.2/4 );
		//EcCnl->NearOff += (short) ( SIM_CODEC_LPF_LEN*1.2/2/4);
		//EcCnl->NearOff += (short) ( SIM_ECHO_DELAY_LEN*1.2/2/4);
		//EcCnl->NearOff += (short) ( 2*SIM_TIMING_LEN*1.2/2/4);
		//EcCnl->NearOff *= (short) 2; // [real and image]
      EchoBitBuf->NearOffBlock = 7;
      EcCnl->NearOff=10;
	}  //122
	else
	{ // Answer

		//EcCnl->NearOff = 10+7+9-18; // = (shape)+(2*rate)+(dcmf)-(echodelayline) [2400 bps]
		//EcCnl->NearOff += (short) ( SIM_CODEC_LPF_LEN*1.2/2/4);
		//EcCnl->NearOff += (short) ( SIM_ECHO_DELAY_LEN*1.2/2/4);
		//EcCnl->NearOff *= (short) 2; // [real and image]

      EchoBitBuf->NearOffBlock = 1;
      EcCnl->NearOff=2;
	}    //18
	if ( EcCnl->NearOff<0 )  EcCnl->NearOff = 0;

   //writen for ECHO_FAR_LEN =72 !!!

   if (EchoBitBuf->FarOffSym < 0)
   {
   	EchoBitBuf->FarOffSym += (short) 2*SYM_PER_BLOCK;
      EchoBitBuf->FarOffBlock--;
   }
   else if (EchoBitBuf->FarOffSym >= 2*SYM_PER_BLOCK)
   {
   	EchoBitBuf->FarOffSym -= (short) 2*SYM_PER_BLOCK;
      EchoBitBuf->FarOffBlock++;
   }
	if ( !identity ) // Call
   {
		//EcCnl->FarOff = (short)( ( EcCnl->rtd-(ECHO_FAR_LEN>>1)+2) & 0xfffe );
      EchoBitBuf->FarOffBlock-=0;
      EchoBitBuf->FarOffSym += (short)6;
	}
	else // Answer
   {
		//EcCnl->FarOff = (short)( ( EcCnl->rtd-(ECHO_FAR_LEN>>1)+2 -102)& 0xfffe ) ;// = 102=2*Buffer Delay+2*Timing Delay=2*30+2*21
      EchoBitBuf->FarOffBlock-=(short)6;
      EchoBitBuf->FarOffSym -= (short)0; 
	}

   if (EchoBitBuf->FarOffSym < 0)
  	{
  		EchoBitBuf->FarOffSym += (short) 2*SYM_PER_BLOCK;
     	EchoBitBuf->FarOffBlock--;
  	}

   EchoBitBuf->FarOffSym &= 0xfffe;
   EcCnl->FarOff = EchoBitBuf->FarOffSym;

  if ( EchoBitBuf->FarOffBlock >= ECH_BITS_BLOCK_NUM )
	  EchoBitBuf->FarOffBlock = ECH_BITS_BLOCK_NUM-1;
  else if ( EchoBitBuf->FarOffBlock < 0)
	  EchoBitBuf->FarOffBlock = 0;

#endif

	// ------ Initialize Near And Far Parameters Values... -------
	// Near Echo Converge Parameters...
	ConvIndexNear = EcCnl->ConvTblIndexNear;
	EcCnl->StepNear = (short)(EcCnl->ConvTblNear[ConvIndexNear].StepSize * EcCnl->StepNorm);
	EcCnl->DelayNearPtr = EcCnl->DelayLineNear + EcCnl->NearOff;
	EcCnl->DelayNearPtr +=  (EcCnl->ConvTblNear[ConvIndexNear].CoefStart);
	EcCnl->DelayNearLen = EcCnl->ConvTblNear[ConvIndexNear].CoefLen;
	EcCnl->hn1Ptr = EcCnl->hn1 + (EcCnl->ConvTblNear[ConvIndexNear].CoefStart);
	EcCnl->hn2Ptr = EcCnl->hn2 + (EcCnl->ConvTblNear[ConvIndexNear].CoefStart);

	// Far Echo Converge Parameters...
	ConvIndexFar = EcCnl->ConvTblIndexFar;
	EcCnl->StepFar = (short)(EcCnl->ConvTblFar[ConvIndexFar].StepSize * EcCnl->StepNorm);
	EcCnl->DelayFarPtr = EcCnl->DelayLineFar + EcCnl->FarOff;
	EcCnl->DelayFarPtr += (EcCnl->ConvTblFar[ConvIndexFar].CoefStart);
	EcCnl->DelayFarLen = EcCnl->ConvTblFar[ConvIndexFar].CoefLen;
	EcCnl->hf1Ptr = EcCnl->hf1 + (EcCnl->ConvTblFar[ConvIndexFar].CoefStart);
	EcCnl->hf2Ptr = EcCnl->hf2 + (EcCnl->ConvTblFar[ConvIndexFar].CoefStart);

}

void EchoNorm(ECHO_CNL *EcCnl)
/*******************************************************************************
 *	Description:
 *	Calculates The Normalization Factor For The LMS Coefficients.
 *
 *
 *  Outputs:
 *  ========
 *  EcCnl->DelayNorm=Delay Normalization Shift(To the right).
 *  EcCnl->StepNorm=Step Normalization Factor Multiplayer
 *  EcCnl->ShiftNorm=Shift(Double Talk) Normalization Factor(To The Left)
 *  EcCnl->DelayLine - Shifted Right By: EcCnl->DelayNorm
 *  EcCnl->hn1, EcCnl->hn2, EcCnl->hf1, EcCnl->hf2 -
 *										Shifted Left By: EcCnl->DelayNorm
 *
 *
 *
 *
 *******************************************************************************/
{
	long Acc0, Acc1;
	long *h1, *h2;
	short *DelayLine;
	short i, DelayNorm;

	// -------------- Find Biggest Coefficent... -------------
	Acc0=8000L;

	h1=EcCnl->hn1; h2=EcCnl->hn2;
	for (i=0;i<ECHO_NEAR_LEN;i++)
	{
		if (Acc0 < *h1)
			Acc0=*h1;
		if (Acc0 < *h2)
			Acc0=*h2;

		h1++; h2++;
	}

	h1=EcCnl->hf1; h2=EcCnl->hf2;
	for (i=0;i<ECHO_FAR_LEN;i++)
	{
		if (Acc0 < *h1)
			Acc0=*h1;
		if (Acc0 < *h2)
			Acc0=*h2;
		h1++; h2++;
	}


	// --------------- Find Normalization Factor... ---------------
	Acc1=1L; Acc1<<=30;
	i=0;
	while (Acc1 > Acc0)
	{
		Acc1 >>= 1;
		i++;
	}

	DelayNorm = 0;
	if (i-ECH_NO_OVERFLOW_BITS > 0)
	{
		if (i-ECH_NO_OVERFLOW_BITS > ECH_MAX_NORM_BITS)
			DelayNorm = ECH_MAX_NORM_BITS;
		else
			DelayNorm = (short) (i-ECH_NO_OVERFLOW_BITS);
	}

	// --------------- Normalizing All Coefficients... ---------------
	h1=EcCnl->hn1; h2=EcCnl->hn2;
	for (i=0;i<ECHO_NEAR_LEN;i++)
	{
		*h1++ <<= DelayNorm;
		*h2++ <<= DelayNorm;
	}
	h1=EcCnl->hf1; h2=EcCnl->hf2;
	for (i=0;i<ECHO_FAR_LEN;i++)
	{
		*h1++ <<= DelayNorm;
		*h2++ <<= DelayNorm;
	}

	// --------------- Normalizing Delay Line... ---------------
	DelayLine=EcCnl->DelayLineNear;
	for (i=0;i<ECH_SYM_DELAY_LEN;i++)
		*DelayLine++ >>= DelayNorm;

	DelayLine=EcCnl->DelayLineFar;
	for (i=0;i<ECH_SYM_DELAY_LEN;i++)
		*DelayLine++ >>= DelayNorm;

	EcCnl->DelayNorm=DelayNorm;
	EcCnl->StepNorm=EcCnl->NormTbl[DelayNorm].StepNorm;
	EcCnl->ShiftNorm=EcCnl->NormTbl[DelayNorm].ShiftNorm;
	EcCnl->StepNear = (short)(EcCnl->ConvTblNear[EcCnl->ConvTblIndexNear].StepSize * EcCnl->StepNorm);
	EcCnl->StepFar = (short)(EcCnl->ConvTblFar[EcCnl->ConvTblIndexFar].StepSize * EcCnl->StepNorm);
}




