#include	"viterbi.h"

/*---------------------------------------------------------------
  - functions:	1. "Butterfly"									-
  -				2. "PathMet"									-
  --------------------------------------------------------------*/

/*---------------------------------------------------------------
  -
void Butterfly(short M1, short M2, short D1, short D2, unsigned short *TRN, short *M)
  - 	By: Mor Riech.
  -		This program makes the viterbi butterfly.
  -		M1,M2: The path metrics.
  -		D1,D2: The branch metrics of the last transmission.
  -		M array contains the temporary path metric, which is used in the
  -		PathMet function, for the calculation of the path metric.
  -		M[0] = Max{M1+D1 , M2+D2} & fix TRN.
  -		M[1] = Max{M1+D2 , M2+D1} & fix TRN.
  -		TRN contains the results of the comparisons,
  -		i.e. which of the two sums is put in the array M.
  -		TRN will be used for back track.
  -
  ---------------------------------------------------------------*/
#ifndef V32_NO_VITERBI
void Butterfly(short M1, short M2, short D1, short D2, unsigned short *TRN, short *M)
{
	short sum1, sum2;

	sum1 = (short) (M1 + D1);
	sum2 = (short) (M2 + D2);
	if (sum1 > sum2)
	{
		*M = sum1;
		*TRN = (short) ( ((*TRN)<<1) + 0 );
	}
	else
	{
		*M = sum2;
		*TRN = (short) ( ((*TRN)<<1) + 1 );
	}

	M++;
	sum1 = (short) (M1 + D2);
	sum2 = (short) (M2 + D1);
	if (sum1 > sum2)
	{
		*M = sum1;
		*TRN = (short) ( ((*TRN)<<1) + 0 );
	}
	else
	{
		*M = sum2;
		*TRN = (short) ( ((*TRN)<<1) + 1 );
	}

}
// End of Butterfly.


/*---------------------------------------------------------------
  -
  -	 short PathMet(VITERBI *v, short *BMet, short *temp_Pmet, short *Final_Pmet);
  -  By: Mor Riech.
  -  This program gets Branch_Met and using the old Path Metrics gives
  -  back the new Path Metric.
  -  input:	v.P_Met - Old Path Metric.
  -  BMet - Branch Metric [0 2 3 1 7 5 4 6].
  -  output:	v.P_Met - New Path Metric.
  -  returned value: final state, in the following order: [0 2 1 3 4 6 5 7].
  -
  ---------------------------------------------------------------*/

short PathMet(VITERBI *v, short *BMet, short *temp_Pmet, short *Final_Pmet)
{
	unsigned short TRN;
	short k;
	short a, d, LeftMet, RightMet; //temporary variables.
	short norm; // a normalization constant.
	short MaxState;

	TRN = 0;
	//Path metric: [0 2 1 3 4 6 7 5]
	// Swap Branch Met to the order: [0 3 2 1 4 6 5 7]
	a = BMet[1];
	BMet[1] = BMet[3];
	BMet[3] = a; 
	a = BMet[5];
	BMet[5] = BMet[6];
	BMet[6] = a; 

	// final state 0&4.
	//           S0           S2       
	Butterfly(v->P_Met[0], v->P_Met[1], BMet[0],  BMet[1], &TRN, &temp_Pmet[0]);
	//           S1           S3       
	Butterfly(v->P_Met[2], v->P_Met[3], BMet[2],  BMet[3], &TRN, &temp_Pmet[2]);

	// final state 1&7.
	//           S4           S6       
	Butterfly(v->P_Met[4], v->P_Met[5], BMet[4],  BMet[5], &TRN, &temp_Pmet[4]);
	//           S7           S5       
	Butterfly(v->P_Met[6], v->P_Met[7], BMet[6],  BMet[7], &TRN, &temp_Pmet[6]);


	// Swap Branch Met to the order: [0 3 2 1 6 4 7 5]
	a = BMet[4];
	BMet[4] = BMet[5];
	BMet[5] = a; 
	a = BMet[6];
	BMet[6] = BMet[7];
	BMet[7] = a; 	
	
	// final state 2&6
	//           S1           S3       
	Butterfly(v->P_Met[2], v->P_Met[3], BMet[0],  BMet[1], &TRN, &temp_Pmet[8]);
	//           S0           S2	   
	Butterfly(v->P_Met[0], v->P_Met[1], BMet[2],  BMet[3], &TRN, &temp_Pmet[10]);

	// final state 3&5
	//           S7           S5       
	Butterfly(v->P_Met[6], v->P_Met[7], BMet[4],  BMet[5], &TRN, &temp_Pmet[12]);
	//           S6           S6       
	Butterfly(v->P_Met[4], v->P_Met[5], BMet[6],  BMet[7], &TRN, &temp_Pmet[14]);

	v->Comp_Buffer[v->p_CompBuf] = TRN;
	v->p_CompBuf++;
	//--- End of the first compare step.

	// Swap first stage results.
	//				             0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
	// Before Swap: temp_Pmet = [0, 4, 0, 4, 1, 7, 1, 7, 2, 6, 2, 6, 3, 5, 3, 5]
	// After Swap:  temp_Pmet = [0, 0, 4, 4, 1, 1, 7, 7, 2, 2, 6, 6, 3, 3, 5, 5]
	a = temp_Pmet[1];
	temp_Pmet[1] = temp_Pmet[2];
	temp_Pmet[2] = a;

	a = temp_Pmet[5];
	temp_Pmet[5] = temp_Pmet[6];
	temp_Pmet[6] = a;

	a = temp_Pmet[9];
	temp_Pmet[9] = temp_Pmet[10];
	temp_Pmet[10] = a;

	a = temp_Pmet[13];
	temp_Pmet[13] = temp_Pmet[14];
	temp_Pmet[14] = a;
	// End of Swap.

	/*
	At this stage, for each state there are two temp path metrics.
	Now we have to compare these two metrics and choose the bigger
	one us the final path metric of the state.
	The information from which temp metric came the final metric
	is again kept in TRN.
	*/

	// -- Second Compare step.
	TRN = 0;
	for (k=0; k<8; k++)
	{
		LeftMet = temp_Pmet[2*k];
		RightMet = temp_Pmet[2*k+1];
		if (LeftMet > RightMet)
		{
			Final_Pmet[k] = LeftMet;
			TRN = (short) ( (TRN<<1) + 0 );
		}
		else
		{
			Final_Pmet[k] = RightMet;
			TRN = (short) ( (TRN<<1) + 1 );
		}
	}	// End of for
	v->Comp_Buffer[v->p_CompBuf] = TRN;
	v->p_CompBuf++;
	if (v->p_CompBuf == 2*VTR_DELAYLENGTH)		v->p_CompBuf = 0;
	// End of second compare step.

	//	find the index of the Max Final Path Metric.
   d=Final_Pmet[0];
   MaxState=0;
	for (k=1; k<8; k++)
	{
		if (Final_Pmet[k] >= d)
		{
			MaxState = k;
			d = Final_Pmet[k];
		}
	} // end of for.
	MaxState = (short) (7-MaxState);

	//	Normarization, such that the max metric becomes: 0x7fff.
	/* adding a constant to the path metrics does not effect the calculations,
	and on the other hand by pushing the metrics to the left we avoid
	loosing bits */

	norm = (short) (0x7fff-d);   // to be changed
	for (k=0; k<8; k++)		Final_Pmet[k] += norm;
	// end of norm.

	//	Organize the new Path Metric in the following order:              
	//  Old: [0 4 1 7 2 6 3 5] -> New: [0 2 1 3 4 6 7 5]

	v->P_Met[0] = Final_Pmet[0];
	v->P_Met[1] = Final_Pmet[4];
	v->P_Met[2] = Final_Pmet[2];
	v->P_Met[3] = Final_Pmet[6];
	v->P_Met[4] = Final_Pmet[1];
	v->P_Met[5] = Final_Pmet[5];
	v->P_Met[6] = Final_Pmet[3];
	v->P_Met[7] = Final_Pmet[7];
		
	return MaxState;

}

#endif