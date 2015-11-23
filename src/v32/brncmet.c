#include "viterbi.h"

/*----------------------------------------------------------------------
-	void BranchMet_XXX(struct VITERBI &v, short x, short y, short *BMet);
-   By: Mor Reich.
-   Date: 7.9.98
-
-   XX - can be: 72 for 7200 bits/sec rate,
-                96 for 9600 bits/sec,
-   				 120  for 12000 bits/sec
-                144 for 14400 bits/sec.
-   &v - viterbi structure.
-   x - x cordinet of the input symbol.
-   y - y cordinet of the input symbol.
-   BMet - Buffer that will be filed with 8 Branch Metric.
-
-   The constellation is devided to 8 groups,
-   according to three first bits of the points in the constellation.
-   For every group, each point has a region that is related to it
-   (every place in this area is closer to the point than to another
-   point that belongs to the same group).
-   The boundaries of the regions are straight lines,
-   with slope of 0,45 or 90 degrees.
-   Computation stages:
-   1. For each group find the closest point to the received point(x,y);
-      by finding in which region of the recived point is.
-   2. Find the distance between the received point and these 8 points.
-
-   Note: This function gives a constant VTR_MAXDISXX Distance for received
-         points that are far from all the points of the group.
-
----------------------------------------------------------------------*/

//----------------------------------------------------------------------
#ifndef V32_NO_VITERBI

void BranchMet_72(VITERBI *v, short x, short y, short *BMet)
{
	long A, B;
   short k;
   short index;
   short AR0, G2;
   short *Gmin;
   short r1,r2;

   Gmin=v->Gmin+v->p_Gmin;

	//	put min group index in circular buffer.
   v->p_Gmin += (short) 8;
   if (v->p_Gmin == 8*VTR_DELAYLENGTH)		v->p_Gmin = 0;

	G2 = 2*CC;

//------ Find Min Index ----
//	Rotate the constelation, such  that the boundaries between
//	the regions are parallel to the axis:

   A = x+y;

   // ----- GROUP 6. -----
	B = A+G2;
	AR0 = 0;
   if (B>=0)		AR0++;
   Gmin[6] = AR0;

// ----- GROUP 2&3 -----
	B = 0;
   if (A>=0)
   {
   	Gmin[2] = (short) B;
   	B++;
		Gmin[3] = (short) B;
   }
   else
   {
      Gmin[3] = (short) B;
      B++;
      Gmin[2] = (short) B;
   }

// ---- GROUP 7 ----
	B = A-G2;
   AR0 = 0;
   if (B<0)		AR0++;
   Gmin[7] = AR0;

//rotation:
	A = y-x;

// ---- GROUP 0 ----
	B = A+G2;
   AR0 = 0;
   if (B>=0)		AR0++;
   Gmin[0] = AR0;

// ---- GROUP 4&5 ----
	B = 1;
   Gmin[4] = 0;
   Gmin[5] = 0;
   AR0 = 2*VTR_GP72;    // # of members in each group (Re&Im).
   if (A>=0)		AR0++;
   Gmin[AR0] = (short) B;

// ----GROUP 1 ----
   B = A-G2;
   AR0 = 0;
   if (B<0)		AR0++;
   Gmin[1] = AR0;
// -------- End of finding Min Index ----------

   // find Branch Met.
   AR0 = VTR_GP72;    // # of members in each group.
	index = 0;
   for (k=0; k<8; k++)
   {
		r1 = v->map[index+(*Gmin++)];
		r2 = (short)(r1<<8);
      r2 >>= 8;  // Image Minimum Point
      r2 *= (short)CC;
      r1 >>= 8;  // Real Minimum Point
      r1 *= (short)CC;
      index += AR0;

      A = (x-r1)*(x-r1);  // Real Error
      A += (y-r2)*(y-r2); // Image Error
		A = -A;
      A += VTR_ROUNDNUM72;
		BMet[k] = (short) (A>>VTR_MSHIFT72);
   }


}
//----------------------------------------------------------------------


void BranchMet_96(VITERBI *v, short x, short y, short *BMet)
{
	long A, B;
   short k;
   short index, index_2;
   short AR0;
   short G1, G2, G4;
   short r1,r2;
   short *Gmin ,*GminSave;

   Gmin=GminSave=v->Gmin+v->p_Gmin;

	//	put min group index in circular buffer.
   v->p_Gmin += (short) 8;
   if (v->p_Gmin == 8*VTR_DELAYLENGTH)		v->p_Gmin = 0;

	G1 = 1*CC; G2 = 2*CC; G4 = 4*CC;

   //Stage 1
/*First divide the plane into rectangles with edges parallel to the axes.
For groups 0,1,6,7 there are 6 rectangles,
i.e two rectangles are without points
and for groups 2,3,4,5 there are 4 rectangles.
MinIndex = -1 means that there is no group point in this rectangle.*/

// ----- GROUP 0&1. -----
   AR0 = 2;
   index = 0;
	// find x shift.
	A = x;
   A += G2;
   B = A - G4;
   if (A>=0)		index += AR0;
   if (B>=0)		index += AR0;
   // x shift is in index

   // find y shift of group 0.
   index_2 = 0;
   A = y;
   B = A + G1;
   if (B>=0)		index_2++;
   B = v->Shift_TBL[index+index_2];
   *Gmin++ = (short) B;

   // find y shift for group 1.
	index_2 = 0;
   B = A - G1;
   if (B>=0)		index_2++;
   B = v->Shift_TBL[6+index+index_2];
   *Gmin++ = (short) B;
// ----- End of Group 0&1 ------

// ---- GROUP 2&3. ----
	AR0 = 2;
   index = 0;
	// find x shift.
   A = x;
   			// A = A + 0;
   if (A>=0)		index += AR0;
   // x shift is in index.

   //find y shift of group 2.
   index_2 = 0;
   A = y;
   B = A - G1;
   if (B>=0)		index_2++;
   B = v->Shift_TBL[12+index+index_2];
   *Gmin++ = (short) B;

   // find y shift of group 3
   index_2 = 0;
   B = A + G1;
   if (B>=0)		index_2++;
   B = v->Shift_TBL[16+index+index_2];
   *Gmin++ = (short) B;
// --- End of Group 2&3 ----

// ------ GROUP 4&5 ------
	// AR0 is 2.
	index = 0;
   // find y shift
   A = y;    // A = y-0.
   if (A>=0)		index += AR0;
   // y shift is in index

   // find x shift of group 4
   index_2 = 0;
   A = x;
   B = A + G1;
   if (B>=0)		index_2++;
   B = v->Shift_TBL[20+index+index_2];
   *Gmin++ = (short) B;

   // find x shift of group 5.
   index_2 = 0;
   B = A - G1;
   if (B>=0)		index_2++;
   B = v->Shift_TBL[24+index+index_2];
   *Gmin++ = (short) B;
// ---- End of Group 4&5 -----

// ---- GROUP 6&7 ----
	AR0 = 2;
   index = 0;
   // find y shift.
   A = y;
   A += G2;
   B = A - G4;
   if (A>=0)		index += AR0;
   if (B>=0)		index += AR0;
   // y shift is in index

   // find x shift of group 6
   index_2 = 0;
   A = x;
   B = A + G1;
   if (B>=0)		index_2++;
   B = v->Shift_TBL[28+index+index_2];
   *Gmin++ = (short) B;

   // find x shift of group 7
   index_2 = 0;
   B = A - G1;
   if (B>=0)		index_2++;
   B = v->Shift_TBL[34+index+index_2];
   *Gmin = (short) B;
// ---- End of Group 6&7 ----

//Stage 2

   // find Branch Met.
   AR0 = VTR_GP96;    // # of members in each group.
   Gmin=GminSave;
	index = 0;
   for (k=0; k<8; k++)
   {
		r1 = v->map[index+(*Gmin)];
		r2 = (short)(r1<<8);
      r2 >>= 8;  // Image Minimum Point
      r2 *= (short)CC;
      r1 >>= 8;  // Real Minimum Point
      r1 *= (short)CC;
      index += AR0;

      A = (x-r1)*(x-r1);  // Real Error
      A += (y-r2)*(y-r2); // Image Error
		A = -A;
      A += VTR_ROUNDNUM96;
		BMet[k] = (short) (A>>VTR_MSHIFT96);
		/* If *Gmin = -1 this means that the received point belongs to a
		rectangle with no group point, i.e. it is far from all points of the group
		We give to this group a constant large distance.*/
		if (*Gmin++ < 0) BMet[k] = -VTR_MAXDIS96;// put MAX_DIS for -1 shift index
   }

}

//----------------------------------------------------------------------
void BranchMet_120(VITERBI *v, short x, short y, short *BMet)
{
	long A,B, r1, r2;
   short k;
   short index;
   short AR0;
   short G2,G4,G6,G8,G10;
   short *Gmin ,*GminSave;

   Gmin=GminSave=v->Gmin+v->p_Gmin;

	//	put min group index in circular buffer.
   v->p_Gmin += (short) 8;
   if (v->p_Gmin == 8*VTR_DELAYLENGTH)		v->p_Gmin = 0;

	G2=2*LC; G4=4*LC; G6=6*LC;
   G8=8*LC; G10=10*LC;

   r1=y+x;	r2=y-x;	// rotate input.

// --- Find group min Index ---

// ----- GROUP 0&1. -----
   index=0;
   A = r1;
   A += G4;
   AR0 = 4;
   B = A-G8;
   if (A>=0) 	index +=AR0;
   if (B>=0)	index += AR0;
   // y+x shift is in index.

   // find y-x shift of group 0.
   A = r2;
   A += G10;
   AR0 = 0;
   B = A-G8;
   if (A>=0)	AR0++;
   A = A - (G8<<1);
   if (B>=0)	AR0++;
   if (A>0)		AR0++;
   index += AR0;
   A = v->Shift_TBL[index];
   index -= AR0;
   *Gmin++ = (short) A; // v->Gmin[v->p_Gmin+0]=Al
   // end of group 0.

   //	find y-x shift of group 1.
   A = r2;
   A += G6;
   B = A-G8;
   AR0 = 0;
   if (A>=0)	AR0++;
   A = A - (G8<<1);
   if (B>=0)	AR0++;
   if (A>0)		AR0++;
   index += AR0;
   A = v->Shift_TBL[VTR_SHIFT_NUM12+index];
   *Gmin++ = (short) A; // v->Gmin[v->p_Gmin+1]=Al
   // end of group 1.

// --- GRUOP 2&3 ---
	index=0;
   A = r1;
   A += G8;
   AR0 = 3;
   B = A-G8;
   if (A>=0)	index += AR0;
   A = A - (G8<<1);
   if (B>=0)	index += AR0;
   if (A>0)		index += AR0;
   // y+x shift is in index.

   // find y-x shift for group 2.
   A = r2;
   A += G6;
   AR0 = 0;
   B = A-G8;
   if (A>=0) 	AR0++;
   if (B>0)		AR0++;
   index += AR0;
   A = v->Shift_TBL[2*VTR_SHIFT_NUM12+index];
	index -= AR0;
   *Gmin++ = (short) A; // v->Gmin[v->p_Gmin+2]=Al
   // end of group 2

   // find y-x shift of group 3.
   A = r2;
   A += G2;
   B = A - G8;
   AR0 = 0;
   if (A>=0)	AR0++;
   if (B>0)		AR0++;
   index += AR0;
   A = v->Shift_TBL[3*VTR_SHIFT_NUM12+index];
   *Gmin++ = (short) A; // v->Gmin[v->p_Gmin+3]=Al
   // end of group 3.

// --- GRUOP 4&5 ---
   // find y-x shift.
   A = r2;
   A += G8;
	index = 0;
   AR0 = 3;
   B = A - G8;
   if (A>=0)	index += AR0;
   A = A - (G8<<1);
   if (B>=0)	index += AR0;
   if (A>0)		index += AR0;
   // y+x shift is in index.

   // find y+x shift for group 4.
   A = r1;
   A += G2;
   AR0 = 0;
   B = A - G8;
   if (A>=0)	AR0++;
   if (B>0)		AR0++;
   index += AR0;
   A = v->Shift_TBL[4*VTR_SHIFT_NUM12+index];
   index -= AR0;
   *Gmin++ = (short) A; // v->Gmin[v->p_Gmin+4]=Al
   // end of group 4.

   // y+x shift for group 5.
   A = r1;
   A += G6;
   B = A - G8;
   AR0 = 0;
   if (A>=0)	AR0++;
   if (B>0)		AR0++;
   index += AR0;
   A = v->Shift_TBL[5*VTR_SHIFT_NUM12+index];
   *Gmin++ = (short) A; // v->Gmin[v->p_Gmin+5]=Al
   // end of group 5.

// --- GROUP 6&7 ---
	// find y-x shift.
   A = r2;
   A += G4;
   index = 0;
   AR0 = 4;
   B = A - G8;
   if (A>=0)	index += AR0;
   if (B>0)		index += AR0;

   // find y+x shift for group 6.
   A = r1;
   A += G10;
   AR0 = 0;
   B = A - G8;
   if (A>=0)	AR0++;
   A = A - (G8<<1);
   if (B>=0)	AR0++;
   if (A>0)		AR0++;
   index += AR0;
   A = v->Shift_TBL[6*VTR_SHIFT_NUM12+index];
   index -= AR0;
   *Gmin++ = (short) A; // v->Gmin[v->p_Gmin+6]=Al
   // end of group 6.

   // find y+x shift for group 7.
   A = r1;
   A += G6;
   B = A - G8;
   AR0 = 0;
   if (A>=0)	AR0++;
   A = A - (G8<<1);
   if (B>=0)	AR0++;
   if (A>0)		AR0++;
   index += AR0;
   A = v->Shift_TBL[7*VTR_SHIFT_NUM12+index];
   *Gmin = (short) A; // v->Gmin[v->p_Gmin+7]=Al
   // end of group 7.
// -- End of: Find group min Index ---

   // find Branch Met.
   AR0 = VTR_GP12;    // # of members in each group.
   Gmin=GminSave;
	index = 0;
   for (k=0; k<8; k++)
   {
		r1 = v->map[index+(*Gmin)];
		r2 = (short)(r1<<8);
      r2 >>= 8;  // Image Minimum Point
      r2 *= (short)LC;
      r1 >>= 8;  // Real Minimum Point
      r1 *= (short)LC;
      index += AR0;

      A = (x-r1)*(x-r1);  // Real Error
      A += (y-r2)*(y-r2); // Image Error
		A = -A;
      A += VTR_ROUNDNUM12;
		BMet[k] = (short) (A>>VTR_MSHIFT12);
		/* If *Gmin = -1 this means that the received point belongs to a
		rectangle with no group point, i.e. it is far from all points of the group
		We give to this group a constant large distance.*/
		if (*Gmin++ < 0)		BMet[k] = -VTR_MAXDIS12;  // put MAX_DIS for -1 shift index.
   }

}

//----------------------------------------------------------------------
void BranchMet_144(VITERBI *v, short x, short y, short *BMet)
{
	long A,B;
   short k;
   short index;
   short AR0;
   short G3,G4,G5,G6,G7,G8;
   short *Gmin ,*GminSave;
   short r1,r2;

   Gmin=GminSave=v->Gmin+v->p_Gmin;

	//	put min group index in circular buffer.
   v->p_Gmin += (short) 8;
   if (v->p_Gmin == 8*VTR_DELAYLENGTH)		v->p_Gmin = 0;

	G3=3*LC; G4=4*LC; G5=5*LC;
   G6=6*LC; G7=7*LC; G8=8*LC;

// --- Find group min Index ---

// ----- GROUP 0&1. -----
   A = x;
   A += G6;
   index = 0;
   AR0 = 4;
   B = A - G4;
   if (A>=0)	index += AR0;
   A -= G8;
   if (B>=0)	index += AR0;
   B -= G8;
   if (A>=0)	index += AR0;
   if (B>0)    index += AR0;   // x shift is in index

   // find y shift of group 0
   A = y;
   A += G5;
   AR0 = 0;
   B = A - G4;
   if (A>=0)	AR0++;
   A -= G8;
   if (B>=0)	AR0++;
   if (A>0)		AR0++;
   index += AR0;
   A = v->Shift_TBL[index];
   index -= AR0;
   *Gmin++ = (short) A; // v->Gmin[v->p_Gmin+0]=Al

   // find y shift of group 1.
   A = y;
   A += G3;
   B = A - G4;
   AR0 = 0;
   if (A>=0)	AR0++;
   A -= G8;
   if (B>=0)	AR0++;
   if (A>0)		AR0++;
   index += AR0;
   A = v->Shift_TBL[VTR_SHIFT_NUM14+index];
   *Gmin++ = (short) A; // v->Gmin[v->p_Gmin+1]=Al

// --- GROUP 2&3 ---
	A = x;
   A += G4;
   index = 2*VTR_SHIFT_NUM14;
   AR0 = 5;
   B = A - G4;
   if (A>=0)	index += AR0;
   A -= G8;
   if (B>=0)	index += AR0;
   if (A>0)		index += AR0;

   // find y shift for group 2.
   A = y;
   A += G7;
   AR0 = 0;
   B = A - G4;
   if (A>=0)	AR0++;
   A -= G8;
   if (B>=0)	AR0++;
   B -= G8;
   if (A>=0)	AR0++;
   if (B>0)		AR0++;
   index += AR0;
   A = v->Shift_TBL[index];
   index -= AR0;
   *Gmin++ = (short) A; // v->Gmin[v->p_Gmin+2]=Al

   // find y shift for group 3.
   A = y;
   A += G5;
   B = A - G4;
   AR0 = 0;
   if (A>=0)	AR0++;
   A -= G8;
   if (B>=0)	AR0++;
   B -= G8;
   if (A>=0)	AR0++;
   if (B>0)		AR0++;
   index += AR0;
   A = v->Shift_TBL[VTR_SHIFT_NUM14+index];
   *Gmin++ = (short) A; // v->Gmin[v->p_Gmin+3]=Al

// --- GROUP 4&5 ---
	A = y;
   A += G4;
   index = 4*VTR_SHIFT_NUM14;
   AR0 = 5;
   B = A - G4;
   if (A>=0)	index += AR0;
   A -= G8;
   if (B>=0)	index += AR0;
   if (A>0)		index += AR0;

   // find x shift for group 4.
   A = x;
   A += G5;
   AR0 = 0;
   B = A - G4;
   if (A>=0)	AR0++;
   A -= G8;
   if (B>=0)	AR0++;
   B -= G8;
   if (A>=0)	AR0++;
   if (B>0)		AR0++;
   index += AR0;
   A = v->Shift_TBL[index];
   index -= AR0;
   *Gmin++ = (short) A; // v->Gmin[v->p_Gmin+4]=Al

   // find x shift for group 5.
   A = x;
   A += G7;
   B = A - G4;
   AR0 = 0;
   if (A>=0)	AR0++;
   A -= G8;
   if (B>=0)	AR0++;
   B -= G8;
   if (A>=0)	AR0++;
   if (B>0)		AR0++;
   index += AR0;
   A = v->Shift_TBL[VTR_SHIFT_NUM14+index];
   *Gmin++ = (short) A; // v->Gmin[v->p_Gmin+5]=Al

// ---- GROUP 6&7 ----
	A = y;
   A += G6;
   index = 6*VTR_SHIFT_NUM14;
   AR0 = 4;
   B = A - G4;
   if (A>=0)	index += AR0;
   A -= G8;
   if (B>=0)	index += AR0;
   B -= G8;
   if (A>=0)	index += AR0;
   if (B>0)		index += AR0;

   // find x shift for group 6.
   A = x;
   A += G5;
   AR0 = 0;
   B = A - G4;
   if (A>=0)	AR0++;
   A -= G8;
   if (B>=0)	AR0++;
   if (A>0)		AR0++;
   index += AR0;
   A = v->Shift_TBL[index];
   index -= AR0;
   *Gmin++ = (short) A; // v->Gmin[v->p_Gmin+6]=Al

   // find x shift of group 7.
   A = x;
   A += G3;
   B = A - G4;
   AR0 = 0;
   if (A>=0)	AR0++;
   A -= G8;
   if (B>=0)	AR0++;
   if (A>0)		AR0++;
   index += AR0;
   A = v->Shift_TBL[VTR_SHIFT_NUM14+index];
   *Gmin = (short)A; // v->Gmin[v->p_Gmin+7]=Al
// -- End of: Find group min Index ---

   // find Branch Met.
   AR0 = VTR_GP14;    // # of members in each group.
   Gmin=GminSave;
	index = 0;
   for (k=0; k<8; k++)
   {
		r1 = v->map[index+(*Gmin)];
		r2 = (short)(r1<<8);
      r2 >>= 8;  // Image Minimum Point
      r2 *= (short)LC;
      r1 >>= 8;  // Real Minimum Point
      r1 *= (short)LC;
      index += AR0;

      A = (x-r1)*(x-r1);  // Real Error
      A += (y-r2)*(y-r2); // Image Error
		A = -A;
      A += VTR_ROUNDNUM14;
		BMet[k] = (short) (A>>VTR_MSHIFT14);
		/* If *Gmin = -1 this means that the received point belongs to a
		rectangle with no group point, i.e. it is far from all points of the group
		We give to this group a constant large distance.*/
		if (*Gmin++ < 0) BMet[k] = -VTR_MAXDIS14;// put MAX_DIS for -1 shift index
   }

}

#endif