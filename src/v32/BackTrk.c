#include "viterbi.h"

/*
	short BackTrak(struct VITERBI *v, short FinalStat)
	By: Mor Riech

	This function gets the final state and finds the transmitted symbol.
   In every step, till the last one, it finds from the state at time T
   the state at time T-1.
   In the last step it finds the transmitted group and then,
   using the Gmin array, the full transmitted symbol.
   input:	v.Comp_Buffer - Buffer of TRN's.
            v.Gmin - index of min indexes.
            FinalStat - 15-final state:
            				The states are organized: [7 5 6 4 3 1 2 0].
   returned value: the transmitted symbol.
*/

#ifndef V32_NO_VITERBI
unsigned short BackTrak(VITERBI *v, short FinalStat)
{
	long B,A;
   short k;  //time index.
   short i1; //represents the state.
   short i2; //represent the right vertex of the butterfly.
   short	AR3, T;
   short AR2;//The index of Comp_Buffer.
   short TC; //the bit from Comp_Buffer.
   unsigned short bit;  //Calculated for TC.

   i1 = (short) (15 - FinalStat);
	AR2 = v->p_CompBuf;
   AR2--;
   if (AR2 == -1)		AR2 = 2*VTR_DELAYLENGTH-1;


   // Perform back track, not including the last step before the end:
   // find the previous state.
   for (k=0; k<VTR_DELAYLENGTH-1; k++) {
      //first stage: determinate the butterfly, and its right vertex.
		T = i1;
      i2 = v->TBL_1[i1-8];
   	bit = (unsigned short) ( (v->Comp_Buffer[AR2])>>(15-T) );
		AR2--;
      TC = (short) (bit & 1);
      if (TC == 1)	i2 += (short) 2;
      //second stage: determinate the left vertex of the chosen butterfly.
      T = i2;
   	bit = (unsigned short) ( (v->Comp_Buffer[AR2])>>(15-T) );
		AR2--;
      if (AR2 == -1)		AR2 = 2*VTR_DELAYLENGTH-1;
		TC = (short) (bit & 1);
      i1 = v->TBL_2[i2];
      if (TC == 1)		i1 += (short) 4;
      }	// End of for.

   // Perform the last step of the back track:
   // find the transmitted group.
   T = i1;
	i2 = v->TBL_1[i1-8];
  	bit = (unsigned short) ( (v->Comp_Buffer[AR2])>>(15-T) );
	AR2--;
   if (AR2 == -1)		AR2 = 2*VTR_DELAYLENGTH-1;
   TC = (short) (bit & 1);
   if (TC == 1)	i2 += (short) 2;
	T = i2;
  	bit = (unsigned short) ( (v->Comp_Buffer[AR2])>>(15-T) );
	TC = (short) (bit & 1);
   if (TC == 1)		i2 += (short)16;
   A = (long) (v->Transmission_TBL[i2]);  //The transmitted group.
   // The transmited group is in A.

   // find symbol.
   B = (short) (v->N - 2);
   T = (short) B;
   B = A+v->p_Gmin;
   AR3 = (short) B;
   A= (short) (A<<T);  //The 3 significant (i.e. left) bits.
   A = A+v->Gmin[AR3];  //add the N-2 less significant (i.e. right) bits.
   if (A<0)		A = 0;

   return((short) A);
}
#endif


