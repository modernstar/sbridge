/*******************************************************************************
*	File filter.c: filter the input signal .
*
*	Includes the subroutine:
*
*  1. Detectors.
*
*	Written by: Irit Hardy.
*  Combined at: version 2.0 .
*******************************************************************************/

#include <memory.h>
#include "filter.h"



void Filter(FILTER *filter,short *In,short *Out,short InLen)
//-----Echo cancellation-----
{
	short i,k;
   register short *Ptr1, *Ptr2;
   long Acc0;

   k=0;
   while (k < InLen)
   {
  		//----- Push input sample into echo cancellation delay line ----
     	Ptr1 = filter->SignalDelayLine+DTCT_SIGNAL_LEN-1;
     	Ptr2 = filter->SignalDelayLine+DTCT_SIGNAL_LEN-2;

     	for (i=0; i<DTCT_SIGNAL_LEN-1; i++)
     		*Ptr1-- = *Ptr2--;
     	*Ptr1 = In[k];

		Ptr2 = filter->SignalFilter;
      Acc0=0L;

      for (i=0; i<DTCT_SIGNAL_LEN; i+=(short)2)
      {
         Acc0 += 2*(long)((*Ptr1) * (*Ptr2++));
         Ptr1+=(short)2;
      }

      Out[k++] = (short)((Acc0+0x8000)>>16);

   }
}

