/*******************************************************************************
*	File dcmf.c: demodulation and match filter.
*
*	Includes the subroutines:
*
*  1. Dcmf.
*
*	Written by: Guy Ben-Yehuda.
*  Combined at: version 2.0 .
*
*******************************************************************************/
#include  "dcmf.h"

short Dcmf(DCMF *dcmf, short *In, short *Out, short InLen)
{
/*******************************************************************************
*	Description:
*  Function Dcmf converts a passband real signal into complex baseband signal,
*	sampled at half rate of the passband signal.
*	The parameters of the function correspond to modulation freq. 1800 Hz
*  and passband sampling rate 9600 samples/sec.
*	The sinus and cosinus are calculated using a table.
*
*	Arguments:
*  DCMF *dcmf - setting and history.
*  short *In  - Input real samples array.
*  short *Out - Output complex samples array.
*  short InLen - Number of input samples.
*
*	Returned value: Number of output samples (complex pair is counted as two).
*******************************************************************************/


	short CosPtr; // Local Pointer To The Cos Table
   short *DelayLinePtr1, *DelayLinePtr2, *LpfTablePtr;
	short Al1,i,j,n;
	long Acc0;

   n=0;
   for (i=0; i<InLen; i++)
   {
   // --------------------- Finding Real Part: ------------------------------
      CosPtr = (short) (((dcmf->SinPtr)+8 ) & 0x1f); // Cyclic Table - 32
 		Acc0 = 2* (long) In[i] * dcmf->SinTable[CosPtr];
      Al1 = (short) ( (Acc0+0x8000)>>16 );

   // ------- Update Real Delay Line: ---------
   	DelayLinePtr1 = dcmf->RDelayLine+DCMF_LEN-1;
      DelayLinePtr2 = DelayLinePtr1-1;
     	for (j=0; j<DCMF_LEN-1; j++)
			*DelayLinePtr1-- = *DelayLinePtr2--;
		*DelayLinePtr1 = Al1;

   // ------- Real Low Pass Filtering: --------
   	LpfTablePtr = dcmf->LpfTable;
      DelayLinePtr2 = dcmf->RDelayLine;
      if (dcmf->DecimationCounter)
      {
	      Acc0 = 0L;
     		for (j=0; j<DCMF_LEN; j++)
				Acc0 += 2* (long) *DelayLinePtr2++ * *LpfTablePtr++;
			Out[n++] = (short) ( (Acc0+0x8000)>>16 );
      }

   // --------------------- Finding Image Part: -----------------------------
 		Acc0 = 2* (long) In[i] * dcmf->SinTable[dcmf->SinPtr];
      Al1 = (short) ( (-Acc0+0x8000)>>16 );

   // ------- Update Image Delay Line: ---------
   	DelayLinePtr1=dcmf->IDelayLine+DCMF_LEN-1;
      DelayLinePtr2=DelayLinePtr1-1;
     	for (j=0; j<DCMF_LEN-1; j++)
			*DelayLinePtr1-- = *DelayLinePtr2--;
		*DelayLinePtr1 = Al1;

   // ------- Image Low Pass Filtering: --------
   	LpfTablePtr=dcmf->LpfTable;
      DelayLinePtr2=dcmf->IDelayLine;
      if (dcmf->DecimationCounter)
      {
	      Acc0 = 0L;
     		for (j=0; j<DCMF_LEN; j++)
				Acc0 += 2* (long) *DelayLinePtr2++ * *LpfTablePtr++;
			Out[n++] = (short) ( (Acc0+0x8000)>>16 );
      }

   // ------- Update SinPtr and DecimationCounter: ---------
		dcmf->SinPtr = (short)((dcmf->SinPtr+6) & 0x1f);
      dcmf->DecimationCounter ^= 0xffff; // The Same As CMPL In The DSP
    }
    return n; //return the number of output samples.
}


