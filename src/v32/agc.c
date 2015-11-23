/*******************************************************************************
 * File agc.c: Gain control.
 *
 * Includes the subroutine:
 *
 * 1. Agc.
 *
 * Writen by: Irit Hardy.
 * Combined at: version 2.0.
 *
 *******************************************************************************/
#include "agc.h"

void Agc(AGC *agc,short *In,short *Out,short InLen)
/*******************************************************************************
 * Description:
 * Function Agc: Find the gain that the received samples should be multiplied by
 * so their energy will be equal to the energy that was used
 * to design the constellations.
 * The calculation of the input energy is performed at the S segment,
 * then the gain is found by dividing the desired energy by the estimated energy
* and taking the squared root of the division.
 *
 * Arguments:
 * AGS *agc - Agc setting and history.
 * short *In - input complex samples array.
 * short InLen - length of the input array.
 *
 * Returned value: detection flag.
 *******************************************************************************/
{
	long Acc0, Acc1;
	short i,k;
	short exp, mantisa;	//for gain estimation.

	if (agc->state == AGC_GAIN )
	{
		for (i=0;i<InLen;i++)
		{
			Acc0=2* (long)In[i]*agc->mantisa;

			Acc1=0x8000;
			Acc1>>=agc->exp;
			exp=(short)Acc1;
			Acc1=Acc0>>16;
			if (Acc1<0) Acc1=-Acc1;

			Acc0<<=agc->exp;
			Out[i]=(short)((Acc0+0x8000)>>16);
		}
	}
	else
	{
		i=0;

		while (i < InLen)
		{
			switch (agc->state)
			{
				case AGC_INIT	:

					agc->EstimatedEnergy=0L;
					agc->exp=0;
					agc->mantisa=0x7fff;
					agc->counter=1;
					agc->state=AGC_COLLECT;

				case AGC_COLLECT	:
					agc->EstimatedEnergy+=((long)In[i]*In[i])<<1;
					agc->counter++;
					Out[i]=In[i];
					if (agc->counter<AGC_NUM_OF_SMP)
						break;
					agc->state=AGC_GAIN;

					//-----Gain Estimation-----

					//-----Divide 64 * DESIRED_ENERGY by the collected estimated energy
	            //-----Find the exponent of the squared gain-----
					exp=6;
					while (DESIRED_ENERGY > (agc->EstimatedEnergy<<(exp-6))) exp++;

	            //-----Find the mantisa of the squared gain-----
					Acc0 = agc->EstimatedEnergy<<(exp-6);
					Acc1 = 0L;
					mantisa = 0;
					for (k=1; k<16; k++)
					{
						if (Acc1 + (Acc0>>k) < DESIRED_ENERGY)
						{
							mantisa+=(short)(0x7fff>>k);
							Acc1 += Acc0>>k;
						}
					}

	   			//-----Taking the root of the squared gain-----
               i=InLen;
					agc->exp = (short)(exp>>1);
					if (exp&1)
					{
						//-----exp is odd-----
	      			agc->mantisa = (short)(((mantisa+1)>>1)+0x2000);
						agc->exp++;
					}
					else
						//-----exp is even-----
						agc->mantisa = (short)(((mantisa+1)>>1) + 0x4000);
					break;


				default            :
					break;
			}//end switch
			i++;
		}//end while
   }//end if
} //end agc()


