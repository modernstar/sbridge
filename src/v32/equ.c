/************************************************************
 *
 * 	         		 -------   equ.c   -------
 *
 * short Equalizer(EQU *Equ, short InLen, short *In, short *Out)
 *
 *  		---------------------------------------------------------------
 *  		- V32 2/T FSE- Fractional Symbol Equalizer.                   -
 *        - Includes: #  EQU_L = 60 Taps Fir LMS Filter                 -
 *        -           #  Carrier Frequency,Phase Estimation Using PLL   -
 *			---------------------------------------------------------------
 *
 * Inputs:
 * -------
 *   -  InLen = Factor 2 From The Symbols Samples Number.
 *              Values: 2, 4, 6 ,...
 *   -  *In = Input Complex Vector Sampled In 4800Hz.
 *   -  *Out = Output Vector With The Estimated Symbols.
 *   -  Equ->Timing = "0x0000" Run On The First Symbol
 *                    "0xffff" Only Push The First Symbol
 *   -  Equ->State =
 *         EQU_START:
 *					   Inputs: Equ->Marker.
 * 					      Equ->Marker =
 *							When Calling In The First Time In EQU_START Mode, Marker
 *							Should Be Added In Order To Point On The Starting Symbol.
 *                    When Calling This Procedure In The Next Times Equ->Marker
 *                    Must Not Be Changed(Only In This Modes) !!!
 *                    Values: Sample 0..15
 *                 Algorithm: Counts (Equ->MarkCnt) MARKER_DELAY Symbols With
 *									 Only Delay Line Pushes And Switch In The Correct
 *									 Symbol To EQU_REGULAR State.
 *
 *         EQU_REGULAR:
 *						Run With LMS & Pll According To EquConversionTbl.
 *                 Output The Estimated Symbol To *Out.
 *
 * Outputs:
 * --------
 *     *Out = Complex Vector = Real,Image,Real,Image,...
 * 				 This Vector Is The Input Symbols After The Equalizer Fir Filter
 * 				 And The PLL.
 *
 * Constants:
 * ----------
 *     - After How Many Symbols Change To This Converge Table:
 *   			EquConvergeTbl[k].Counter = 0...2^31
 *		- Start Of The LMS Coefficents = 0 To EQU_LEN/2-1 = 0 To 29
 *   			EquConvergeTbl[k].CoefStart = 0...(EQU_L/2-1)*2
 *     - Length Of The LMS Coefficents(From CoefStart) = 0 To EQU_LEN-1
 *  			EquConvergeTbl[k].CoefLen = 0...59
 *     - Step Size Of The LMS: = 0...{ 2/(CoefLen*Sigma^2)} For Converge
 * 		      EquConvergeTbl[k].StepSize = 0...2^15
 *     - Step Size Of The PLL Frequency:
 * 				EquConvergeTbl[k].PllKf = 0...2^15
 *     - Step Size Of The PLL Phase:
 *			   EquConvergeTbl[k].PllKp = 0...2^15
 *
 *     where: k = 0...CONV_NUM-1 = Converge Table Index
 *     Note: Equ.ConvTbl = EquConvergeTbl;
 *
 *  Table:
 *  ------
 *     Equ.SinTbl = EquSinTbl;
 * 		Where EquSinTbl = q15(sin(pi* [0:1/SIN_TBL_LEN:1]));
 *
 *  --------------------------------------------------------
 *
 *     Written By: Guy Ben-Yehuda
 *
 *     Date: 25.10.98
 *
*************************************************************
*	Changes:
*	1. version 2.4: Modem with slicer : return the slicer result as an equalizer output.
*	2. version 2.5: Moving the LMS coefficient update into equalizer filtering loop.
						 Correction in finding Taylor variables.
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "scram.h"
#include "slicer.h"
#include "equ.h"

extern long BlockNum;
extern FILE *ans_tx, *call_tx, *ans_equ, *call_equ;

short Equalizer(EQU *Equ, short InLen, short *In, short *Out)
{
	// Global Use Registers
	short Al0;

	// Reserved Registers
	short i, OutLen, DelayStartOff;
	register short ConvIndex;
	register short *CoefStartPtr;
	short FilterLen,Step, PllKf, PllKp;
	short EqualizerSwitch, LmsSwitch, PllSwitch, PllErrorEstSwitch;
	short StartActivateFlag;

	Step=
	DelayStartOff=
	PllKp=
	PllKf=
	FilterLen=
	PllErrorEstSwitch=
	LmsSwitch=0;
	CoefStartPtr=NULL;







   
	Al0 = (short)(InLen >> 1);

	//-----Check if this is a start state or a working state----
	if ( Equ->MarkCnt <= Al0 )
	{
		//-----Working state-----

		//-----Check whether to update Conversion Table Index-----
		if ( Equ->SymCounter >= Equ->ConvTbl[Equ->ConvTblIndex].Counter )
			Equ->ConvTblIndex++;

		//-----Copy the Conversion Table entries into local variables-----
		ConvIndex = Equ->ConvTblIndex;
		FilterLen = Equ->ConvTbl[ConvIndex].CoefLen;
		Step = Equ->ConvTbl[ConvIndex].StepSize;
		PllKf = Equ->ConvTbl[ConvIndex].PllKf;
		PllKp = Equ->ConvTbl[ConvIndex].PllKp;

		// Set Current Coefficients Pointer
		CoefStartPtr = Equ->CoefPtr + Equ->ConvTbl[ConvIndex].CoefStart;

		// Set Current Delay Line Pointer
//		DelayStartPtr = Equ->DelayLinePtr + Equ->ConvTbl[ConvIndex].CoefStart;
      DelayStartOff = Equ->ConvTbl[ConvIndex].CoefStart;

		//-----To signal the state machine to move to the appropriate working state----
		StartActivateFlag = ENABLED;
	}
	else
	{
		//-----Start (only push) state-----
		Equ->MarkCnt -= Al0;
		StartActivateFlag = DISABLED;
	}

	// ----------------------- State Mechine: --------------------------
	switch ( Equ->State )
	{
		case EQU_START:

			EqualizerSwitch = DISABLED;
			//LmsSwitch = DISABLED; //no need: without equalizer there is no LMS.
			PllSwitch = DISABLED;
			//PllErrorEstSwitch = DISABLED;

			if (StartActivateFlag==ENABLED)
			{
				Equ->State = EQU_REGULAR;
				for (i=0; i<Al0-Equ->MarkCnt; i+=(short)2)
					Slicer(&(Equ->Slcr));
				Equ->MarkCnt = 0;
            Equ->RealError = Equ->ImagError = 0;
			}
			break;

		case EQU_REGULAR:
			/* Working only With Pll:
			LMS is off.
			DDE mode.
			*/

			EqualizerSwitch = ENABLED;
			LmsSwitch = ENABLED;
			PllSwitch = ENABLED;
			PllErrorEstSwitch = ENABLED;

			break;

		case EQU_CONSTELLATION_CHANGE:
			/*
			constellation is changed :
			turn off the LMS,
			continue to push
			and rotate, without calculating new phase error,
			but promoting the phase due to freq. correction.
			*/

			// Set Switches
			EqualizerSwitch = DISABLED;
			LmsSwitch = DISABLED; //no need: without equalizer there is no LMS.
         Equ->RealError = Equ->ImagError = 0; //for the next call
			PllSwitch = ENABLED;
			PllErrorEstSwitch = DISABLED;

			break;


		case EQU_START_RESYNC:

			EqualizerSwitch = DISABLED;
			//LmsSwitch = DISABLED; //no need: without equalizer there is no LMS.
			PllSwitch = DISABLED;
			//PllErrorEstSwitch = DISABLED;

			if (StartActivateFlag==ENABLED)
			{
				Equ->State = EQU_RESYNC;
            Equ->RealError = Equ->ImagError = 0;
				Equ->MarkCnt = 0;
			}
			break;

		case EQU_RESYNC:
			/* Working only With Pll:
			LMS is off.
			DDE mode.
			*/

			// Set Switches
			EqualizerSwitch = ENABLED;
			LmsSwitch = DISABLED;
         Equ->RealError = Equ->ImagError = 0;
			PllSwitch = ENABLED;
			PllErrorEstSwitch = ENABLED;

			break;

		default:
			break;
	}

	OutLen=EqualizerFilter(Equ, InLen, In, Out, EqualizerSwitch, LmsSwitch,
   							  PllSwitch, PllErrorEstSwitch, FilterLen, PllKf,
                      	  PllKp, CoefStartPtr, DelayStartOff, Step);

	return(OutLen);
}


short EqualizerFilter(EQU *Equ, short InLen, short *In, short *Out,
							 short EqualizerSwitch, short LmsSwitch, short PllSwitch,
                      short PllErrorEstSwitch, short FilterLen, short PllKf,
                      short PllKp, short *CoefStartPtr, short DelayStartOff,
                      short Step)
{
	// Global Use Registers
	register short *Ptr1, *Ptr2;
   short Al0;
	long Acc0, Acc1;

	// Reserved Registers
	short i, k, OutLen, *DelayStartPtr;
	short Sign, SinPtr, Delta, sin_x_high, cos_x_high, CosPtr; //for sinus calculations.
	short Real16, Imag16;
	short Real_Out, Imag_Out;
	short Real_Error, Imag_Error;
	short Real_Rotator, Imag_Rotator;
	Real_Out = Imag_Out = OutLen = 0;

  	DelayStartPtr = Equ->DelayLinePtr + DelayStartOff;
	Real_Error = Equ->RealError;
	Imag_Error = Equ->ImagError;

	//-----Loop over samples------
	for ( i = 0; i < InLen; i += (short)2)
	{
		// Push Sample To Delay Line
		for ( k = 2*EQU_LEN+3; k>=2; k-- )
			Equ->DelayLinePtr[k] = Equ->DelayLinePtr[k-2];

		Equ->DelayLinePtr[0] = In[i];   // Real Delay Line
		Equ->DelayLinePtr[1] = In[i+1]; // Image Delay Line

		//-----Perform the calculations only over the samples on the symbol-----
		if ( Equ->TimingFlag == 0 )
		{
			if ( EqualizerSwitch == ENABLED)
			{

				//----- Multiply the error by the step -----
				Acc1   = 2* (long) Real_Error * Step;
				Real_Error = (short) ( ( Acc1 + 0x8000 ) >> 16 );
				Acc1   = 2* (long) Imag_Error * Step;
				Imag_Error = (short) ( ( Acc1 + 0x8000 ) >> 16 );

            // -------- Equalizer Filtering --------
            // ----- The LMS coeifficent are updated during the filtering -----

            // ----- Image Part -----
            Ptr1 = DelayStartPtr;
            Ptr2 = CoefStartPtr;
            Acc0 = 0;

            for (k = 0; k <= 2*FilterLen; k += (short)2 )
            {

               //-----Update LMS coefficient-----
               Acc1 = 2*(long) ( Real_Error * *(Ptr1+4) ) +
               		 2*(long) ( Imag_Error * *(Ptr1+5) );
					Acc1 = ((Acc1 + 0x8000L)>>16)<<16; // For MACR
               Acc1 += (((long) *Ptr2)<<16);
					*Ptr2 = (short) ( Acc1 >> 16 );
               Acc0 += 2*(long) *(Ptr1+1) * *Ptr2; // Filtering

               Acc1 = 2*(long) ( Imag_Error * *(Ptr1+4) ) -
                      2*(long) ( Real_Error * *(Ptr1+5) );
					Acc1 = ((Acc1 + 0x8000L)>>16)<<16; // For MACR
					Acc1 += (long)(((long)*(Ptr2+1))<<16);
					*(Ptr2+1) = (short) ( Acc1 >> 16 );
					Acc0 += 2*(long) *Ptr1 * *(Ptr2+1); //Filtering

               Ptr2 += 2;
               Ptr1 += 2;
				}


            //-----The rotated output point : Image part-----
            Imag16 = (short) ( (Acc0 + 0x8000) >> 16 );

            // ----- Real Part-----
            Ptr1 = DelayStartPtr;
            Ptr2 = CoefStartPtr;
            Acc0 = 0;

            for (k = 0; k <= 2*FilterLen; k += (short)2 )
            {
            	Acc0 += 2*(long) *Ptr1 * *Ptr2 -
                       2*(long) *(Ptr1+1) * *(Ptr2+1);

               Ptr1 += 2; Ptr2 +=2;
				}

            //-----The rotated output point : Real part-----
            Real16 = (short) ( (Acc0 + 0x8000) >> 16 );

				// ----- Calculating Exp(i*Equ->PllPhase) -------
				if (Equ->PllPhaseEst == -32768)
				{
					//-----Check this case seperatly to avoid overflow-----
					Real_Rotator = -32768;
					Imag_Rotator = 0;
				}
				else
				{
					//----Handeling negative angles-----
					Sign = 1;
					if ( Equ->PllPhaseEst < 0 )
					{
						Equ->PllPhaseEst = (short)(-Equ->PllPhaseEst);
						Sign = -1;
					}

					//-----Finding Taylor Variables-----
					SinPtr = (short)(Equ->PllPhaseEst >> SIN_TBL_BITS);
					Delta = (short)(Equ->PllPhaseEst << (16-SIN_TBL_BITS));
					Delta >>= ( 16-SIN_TBL_BITS );
					if ( Delta < 0 )
						SinPtr++;

					//-----Calculating Sin(fi0), Cos(fi0)-----
					sin_x_high = Equ->SinTbl[SinPtr];
					CosPtr = (short)(SinPtr + (SIN_TBL_LEN >> 1));
					if ( CosPtr >= SIN_TBL_LEN )
						cos_x_high = (short)(-Equ->SinTbl[CosPtr - SIN_TBL_LEN]);
					else
						cos_x_high = Equ->SinTbl[CosPtr];

					//-----Finding Sin(fi), Cos(fi)-----
					Acc0 = (long) 2 * 25736 * Delta * 4; // pi_4 = Pi/4 = 25736
					Al0 = (short) ( (Acc0 + 0x8000) >> 16 );
					Acc1 = 2* (long) Al0 * sin_x_high;
					Real_Rotator = (short)(cos_x_high - (short) ( (Acc1 + 0x8000) >> 16 ));
					Acc0 = 2* (long) Al0 * cos_x_high;
					Imag_Rotator =(short)( sin_x_high + (short) ( (Acc0 + 0x8000) >> 16 ));

					if ( Sign < 0 )
						Imag_Rotator = (short)(-Imag_Rotator);
				}//exp calculation.

				// -------- Rotator --------
				Acc0 = 2* (long) Real16 * Real_Rotator -
					   2* (long) Imag16 * Imag_Rotator;
				Real_Out = (short) ( ( Acc0 + 0x8000 ) >> 16 );
				Acc0 = 2* (long) Real16 * Imag_Rotator +
					   2* (long) Imag16 * Real_Rotator;
				Imag_Out = (short) ( ( Acc0 + 0x8000 ) >> 16 );



				Equ->Slcr.in_real = Real_Out;
				Equ->Slcr.in_imag = Imag_Out;
				Slicer(&(Equ->Slcr));

				//-----Error Calculation: the not rotated errors-----
				Real16 = (short)(Equ->Slcr.out_real-Real_Out);
				Imag16 = (short)(Equ->Slcr.out_imag-Imag_Out);

				if (LmsSwitch == ENABLED)
				//----- Rotate the error -----
				{
					//-----DeRotating-----
					Acc0 = 2* (long) Real16 * Real_Rotator +
							 2* (long) Imag16 * Imag_Rotator;
					Acc1 = 2* (long) Imag16 * Real_Rotator -
							 2* (long) Real16 * Imag_Rotator;

					//-----The rotated error-----
					Real_Error = (short) (( Acc0 + 0x8000 )>>16);
					Imag_Error = (short) (( Acc1 + 0x8000 )>>16);
				}	//LMS Switch

				// Error Estimation...
				Real16=Real_Error;
				Acc0=2* (long) Real16*Real_Error;
				Imag16=Imag_Error;
				Acc0+=2* (long) Imag16*Imag_Error;
				Acc0>>=EQU_ERROR_PARAM;
				Acc1=Equ->EquError>>EQU_ERROR_PARAM;
				Equ->EquError+=Acc0-Acc1;

			}  //Equalizer Switch

			if ( PllSwitch == ENABLED )
			{
				//------Check whether to re-estimate the PLL error-----
				if (PllErrorEstSwitch == ENABLED)
				{
					//-----Calculating the difference between the angles------
					Acc0 = 2*(long) Equ->Slcr.out_real * Imag_Out -
						   2*(long) Equ->Slcr.out_imag * Real_Out;
					//-----The PLL error-----
					Al0 = (short) (( Acc0 + 0x8000 )>>16);
				}
				else
					//-----Use only the phase correction due to previous freq. correction-----
					Al0 = 0;
				Equ->PllFreqEst32 -= 2*(long) PllKf * Al0;
				Equ->PllPhaseEst32 += Equ->PllFreqEst32 - 2*(long) PllKp * Al0;
				Equ->PllPhaseEst = (short) ( (Equ->PllPhaseEst32+0x8000) >> 16 );

				//-----Promote the symbol counter-----
				if(Equ->ConvTblIndex < Equ->MaxIndex)
					Equ->SymCounter++; // Increase Symbol Counter

			}	//Pll Switch

			//----Prepare the output-----
			*Out++ = Real_Out;
			*Out++ = Imag_Out;
			OutLen++;
		}//if sample on symbol

		//-----Update the sample parity-----
		Equ->TimingFlag ^= 0xffff;

	}//loop over samples

	Equ->RealError = Real_Error;
	Equ->ImagError = Imag_Error;
	return(OutLen);
}

