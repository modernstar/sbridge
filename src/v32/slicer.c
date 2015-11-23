/*******************************************************************************
*	File slicer.c: Slicer.
*
*	Includes the subroutine:
*  1. InitSlicer
*  2. Slicer
*
*	Written by: Irit Hardy.
*  Combined at: version 2.0 .
********************************************************************************
*	Changes:
*	version 2.4: 	Changes in function SlicerInit.
*******************************************************************************/
//#include <stdlib.h>
//#include <math.h>
//#include <stdio.h>
#include "scram.h"
#include "slicer.h"
#include "mapper.h"
//#include "viterbi.h"

void SlicerInit(SLCR *Slcr, short N, short identity)
{
/*******************************************************************************
*	Description:
*	This Function initiate the slicer setting and should be called before every
*	change of mode (i.e. constelation change or moving into/out from train mode.)
*
*	Arguments:
*  V32SLCR *v32slcr - Slicer setting and history.
*	short N - the slicer mode.
*  Modes (constelations):
*      TRAIN		 On Training The Symbol Is Chosen By Passing Ones To
*                  the Scrambler.
*      QPSK        4800bps
*      QAM16  	    7200bps
*      QAM32       9600bps
*      QAM64       12000bps
*      QAM128      14400bps
*	short identity - The mode off the modem ANSWER/CALL. The mode of the
*						  slicer's sclambler should be the opposite.
********************************************************************************
*	Changes:
*	2. version 2.4: Make the output points to be multiply of CC or LC.
*******************************************************************************/
#ifdef V32_NO_VITERBI
	if (N>3) N--;
#endif
   Slcr->constellation = N;
	switch (N)
   {
   	case TRAIN_START	:
   		Slcr->counter = 0;
   		ScramInit( &(Slcr->scrm), (short)!(identity));
   		break;

		case QPSK_2BITS  :
   	 		Slcr->RotateReal = 3663;
    		Slcr->RotateImag = 1832;
	    	Slcr->DeRotateReal = 29312;  // need to mul by 8 at Slcr->out
	    	Slcr->DeRotateImag = -29312;  // need to mul by 4 at Slcr->out
			Slcr->Edge = 0x0400;
			Slcr->RealShift = 3;
			Slcr->ImagShift = 2;
	    	break;

		case QAM16_3BITS :
			Slcr->RotateReal=9158;
			Slcr->RotateImag=0x0;
			Slcr->DeRotateReal=29312;  // need to mul by 4 at Slcr->out
			Slcr->DeRotateImag=0x0;
         Slcr->Edge = 0x0C00;
         Slcr->RealShift = 2;
         Slcr->ImagShift = 0;
	    	break;

		case QAM32_4BITS :
			Slcr->RotateReal=9158;
			Slcr->RotateImag=9158;
			Slcr->DeRotateReal=29312;  // need to mul by 2 at Slcr->out
			Slcr->DeRotateImag=-29312;  // need to mul by 2 at Slcr->out
         Slcr->Edge = 0x1400;
         Slcr->RealShift = 1;
         Slcr->ImagShift = 1;
			break;

		case QAM64_5BITS :
			Slcr->RotateReal=18808;
			Slcr->RotateImag=0x0;
			Slcr->DeRotateReal=28544;  // need to mul by 2 at Slcr->out
			Slcr->DeRotateImag=0x0;
         Slcr->Edge = 0x1C00;
         Slcr->RealShift = 1;
         Slcr->ImagShift = 0;
			break;

		case QAM128_6BITS:
			Slcr->RotateReal=18808;
			Slcr->RotateImag=18808;
			Slcr->DeRotateReal=28544;
			Slcr->DeRotateImag=-28544;
         Slcr->Edge = 0x2C00;
         Slcr->RealShift = 0;
         Slcr->ImagShift = 0;
			break;

	default:
			break;
   }
}

// *****************************************************************

void Slicer(SLCR *Slcr)
{
/*******************************************************************************
*	Description:
*	This Function Gets A Single Point On The Constelattion And Output The
*  Nearest Symbol.
*  This Is Achived By This Steps:
*  - Rotating The Constelattion If Needed (With Normalization Factor).
*  - Checking For External Points To Avoid Overflow.
*  - Flooring The Normalized And Rotated Point.
*  - Derotating To Get The Nearest Symbol.
*
*	Arguments:
*  V32SLCR *v32slcr - Slicer setting and history.
*******************************************************************************/
	long acc0, acc1;
	short res_real, res_imag;               // Temporary Results Register
	unsigned short in, out;                 // Input/Output For The Scrambler
	short BitNum;

	if (Slcr->constellation == TRAIN_START)
   {
			in=0xC000;
			BitNum=2;
			Scrambler(&(Slcr->scrm),&in, &out, BitNum);
			if (Slcr->counter<256)
			{
				if ((out==0) | (out==0x4000))
				{
					Slcr->out_real=-3*CC;
					Slcr->out_imag=-1*CC;
				}
				else if ((out==0xC000) | (out==0x8000))
				{
					Slcr->out_real=3*CC;
					Slcr->out_imag=1*CC;
				}
			}
			else
         {
				if (out==0x0)
				{
					Slcr->out_real=-3*CC;
					Slcr->out_imag=-1*CC;
				}
				else if (out==0x4000)
				{
					Slcr->out_real=CC;
					Slcr->out_imag=-3*CC;
				}
				else if (out==0x8000)
				{
					Slcr->out_real=-1*CC;
					Slcr->out_imag=3*CC;
				}
				else if (out==0xC000)
				{
					Slcr->out_real=3*CC;
					Slcr->out_imag=1*CC;
				}
			}
			Slcr->counter++;
			return;
   }

  	// Rotating Real Part...
  	acc0  = ( (long)Slcr->RotateReal*Slcr->in_real ) << 1;
  	acc0 -= ( (long)Slcr->RotateImag*Slcr->in_imag ) << 1;
	res_real = (short)(acc0>>16);
  	// Rotating Imagenary Part...
	acc0  = ( (long)Slcr->RotateImag*Slcr->in_real ) << 1;
	acc0 += ( (long)Slcr->RotateReal*Slcr->in_imag ) << 1;
	res_imag = (short)(acc0>>16);

  	// Checking Extrnal Points...
   if (res_real>Slcr->Edge)   res_real = Slcr->Edge;
	else if (res_real<-Slcr->Edge)   res_real = (short) -Slcr->Edge;
	if (res_imag>Slcr->Edge)   res_imag = Slcr->Edge;
	else if (res_imag<-Slcr->Edge)   res_imag = (short) -Slcr->Edge;

  	// Flooring...
	res_real = (short)( (res_real & 0xf800)+ONE_ );
	res_imag = (short)( (res_imag & 0xf800)+ONE_ );

  	// DeRotating...
	acc0  = ( (long)2*Slcr->DeRotateReal*res_real ) << Slcr->RealShift;
   acc1  = ( (long)2*Slcr->DeRotateReal*res_imag ) << Slcr->RealShift;
   acc0 -= ( (long)2*Slcr->DeRotateImag*res_imag ) << Slcr->ImagShift;
	acc1 += ( (long)2*Slcr->DeRotateImag*res_real ) << Slcr->ImagShift;
	Slcr->out_real = (short)(acc0>>16);
	Slcr->out_imag = (short)(acc1>>16);
}
