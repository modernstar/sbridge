/*******************************************************************************
*	File detectors.c: all the symbol detection in the start up procedure.
*
*	Includes the subroutine:
*
*  1. Detectors.
*
*	Written by: Irit Hardy.
*  Combined at: version 2.0 .
********************************************************************************
*	Changes:
*	version 2.1: 	Changes in function Detectors.
*	version 2.10: 	Changes in function Detectors.
*	version 2.12: 	Changes in function Detectors.
*	version 2.13: 	Changes in function Detectors.
*	version 2.14: 	Changes in function Detectors.
*	version 2.15: 	Addition of  function DetectorsFilter.
*******************************************************************************/


#include <memory.h>
#include <stdio.h>
#include "filter.h"
#include "detect.h"

extern FILE *general;

short Detectors(DTCT *dtct,short *In,short InLen)
{
/*******************************************************************************
*	Description:
*  Function Detectors works on arbitrary number of samples.
*	The function detects three types off signals:
*	- The signals for calculating Round Trip Delay for call modem.
*  - The signals for calculating Round Trip Delay for answer modem.
*	- S/Sbar (same for answer and call modems).
*	The function works in four stages:
*	1. Echo cancellation.
*	2. Detector's delay line promotion.
*	3. Energies or correlation calculation.
*	4. Detection counting and state promotion.
*
*	Arguments:
*  DIFF *dtct - Detectors setting and history.
*  short *In  - Input complex samples array.
*  short InLen - Twice the number of the input samples.
*
*	Returned value: Detection flag.
********************************************************************************
*	Changes:
*	1. version 2.1: Addition of IdleFlag that prevents execution of switch 3.
*	2. version 2.1: LowEnergyThreshold is relative to energy and part of the
*                  structure rather than constant.
*	3. version 2.10: S detections performs ones per block.
*  4. version 2.10: Reduce the delay line length to 8 complex samples.
*  5. version 2.12: Detect the renegotiation signals.
*  6. version 2.13: Perform the energy calculations once per block.
*  7. version 2.14: Erase the detector states and state machines move the control
*						  of the detector to the callrxsm and ansrxsm.
*						  Remove the renegotiation parts.
*	8. version 2.15: The filtering are performed by a function DetectorsFilter.
*******************************************************************************/

	// Global Use Registers
	register short *Ptr1;
	long Acc0, Acc1;

   long TimeEnergy;
	long FreqEnergy=0L;  //The energy calculated as a sum of energies of the relavant frequencies.

	short flag; //returned value
	short i, k; //indexes

	flag = DTCT_FAILURE;

   memcpy(dtct->PrevDelayLine, dtct->DelayLine, sizeof(short)*DTCT_LEN);

   if ((dtct->mode == DTCT_RERATE) || (dtct->SpectrumFlag == DTCT_ALL_SPECTRUM))
		memcpy(dtct->DelayLine, In, sizeof(short)*DTCT_LEN);
	else
   	//-----Echo cancellation-----
   	Filter(&(dtct->filter), In, dtct->DelayLine, InLen);

	if ((dtct->mode == DTCT_ENERGY) || (dtct->mode == DTCT_SILENCE) || (dtct->mode == DTCT_RERATE))
   //----- Energies calculations -----
   /*----- The calculation is performed once per block on the last
           DCTC_LEN - DCTC_START_SMP samples.                -----*/
   {

   	//----- Energy in the time representation-----

      Acc0 = 0L;
      Ptr1 = dtct->DelayLine;
      for (i=0;i<DTCT_SMP_NUM;i++)
      {
      	Acc0 += (long)*Ptr1 * *Ptr1 << 1;
         Ptr1++;
      }

      TimeEnergy = (Acc0+DTCT_ROUND)>>DTCT_SHIFT;

      //-----Energy in the frequency representation-----

      if ((dtct->SpectrumFlag == DTCT_ALL_SPECTRUM) || (dtct->SpectrumFlag == DTCT_HALF_BAUD))
      //-----Energy of the +/- half baud components-----
      {
      	//-----plus half baud-----

         FreqEnergy = DetectorsFilter(dtct->DelayLine,dtct->DFTFilter1, DTCT_SMP_NUM);

         //-----minus half baud-----

         FreqEnergy += DetectorsFilter(dtct->DelayLine,dtct->DFTFilter2, DTCT_SMP_NUM);

     }


      if ((dtct->SpectrumFlag == DTCT_ALL_SPECTRUM) || (dtct->SpectrumFlag == DTCT_DC))
      //-----Energy of the DC component -----
      	FreqEnergy += DetectorsFilter(dtct->DelayLine,dtct->DCFilter, DTCT_SMP_NUM);

      if (dtct->mode != DTCT_SILENCE)
		{
      	//-----The absolute difference between time energy and frequency energy
      	if (FreqEnergy>=TimeEnergy)
				Acc0=FreqEnergy-TimeEnergy;
      	else
      		Acc0=TimeEnergy-FreqEnergy;

	      //-----Energy threshold-----
   	   Acc1=(TimeEnergy)>>3;
      }
      else
      //---- Detect Silence -----
      {
      	 Acc1 = dtct->LowEnergyThreshold;
         Acc0 = TimeEnergy;
		 dtct->DetCounter = DTCT_ENERGY_NUM - 1;
      }

		if (Acc0<Acc1)
		{
			dtct->DetCounter++;
			if (dtct->DetCounter>=DTCT_ENERGY_NUM)
			{
            dtct->LowEnergyThreshold = (TimeEnergy+8)>>4;
				dtct->DetCounter=0;
            dtct->PrevCorr = 0L;
            flag = DTCT_SUCCESS;
			}
		}
 		else
 			dtct->DetCounter=0;

   }
   else //if ((dtct->mode == DTCT_CORRELATION) || (dtct->mode == DTCT_RELAXTION))
   //----- Calculate signal auto correlation-----
   {
      k=0;
      while (k < InLen)
      {
	  		//-----The correlation between the new and old sample-----
         Acc0=((long)dtct->DelayLine[k]*dtct->PrevDelayLine[k]+dtct->DelayLine[k+1]*dtct->PrevDelayLine[k+1])<<1;
         //-----Correlator exponential averaging-----
         dtct->PrevCorr+=(Acc0-dtct->PrevCorr)>>DTCT_CORR_AVERAGE_WEIGHT;


	      if (dtct->IdleFlag == FLAG_ON)
   	   //-----If the idle flag is on do not detect-----
      	{
         	k+=(short)2;
            continue;
      	}

         if (dtct->mode == DTCT_RELAXATION)
         	Acc0 = - dtct->PrevCorr;
         else
            Acc0 = dtct->PrevCorr;

			if (Acc0<0)
			{
				dtct->DetCounter++;
            if (dtct->DetCounter>=DTCT_CORR_NUM)
            {
               dtct->DetCounter=0;
               if (dtct->mode != DTCT_RELAXATION)
               {
               	dtct->DetLocation = (short)(k/2);
                  dtct->mode = DTCT_RELAXATION;
               	flag = DTCT_SUCCESS;
               }
               else
               	dtct->mode = DTCT_CORRELATION;

            }
         }
         else
         	dtct->DetCounter=0;

         k+=(short)2;

      } //end of while

   } // end of else

   return flag;

} // end of function


long DetectorsFilter(short *DelayLine, short *Filter, short InLen)
{
	register short *Ptr1, *Ptr2;
	long Acc0, Acc1;

	long FreqEnergy;  //The energy calculated as a sum of energies of the relavant frequencies.
   short PartFreq;	//The Fourier transform of the signal and the relavant frequency.
   short i;

   Acc0 = Acc1 = DTCT_ROUND;
   Ptr1 = DelayLine;
   Ptr2 = Filter;

   for (i=0;i<InLen-1;i+=(short)2)
   {
   	Acc0 += (long)*Ptr1 * *Ptr2 - (long)*(Ptr1+1) * *(Ptr2+1);
      Acc1 += (long)*Ptr1 * *(Ptr2+1) + (long)*(Ptr1+1) * *Ptr2;
      Ptr1+=2; Ptr2+=2;
   }

   PartFreq = (short)(Acc0>>DTCT_SHIFT);
   FreqEnergy = (long)PartFreq * PartFreq <<1;

   PartFreq = (short)(Acc1>>DTCT_SHIFT);
   FreqEnergy += (long)PartFreq * PartFreq <<1;

   return FreqEnergy;
}


