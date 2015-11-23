/*******************************************************************************
 *	File shape.c: transmission shaping and modulation.
 *
 *	Includes the subroutines:
 *
 *  1. ShapeInit.
 *  2. Shape.
 *
 *	Written by: Mor Reich.
 *  Combined at: version 2.0 .
 *
 *******************************************************************************/
#include <memory.h>
#include "shape.h"

void ShapeInit(SHAPE *shape, const short *ShapeFilter, const short *SinTable)
{
	shape->PolyPhaseFilter = (short *)ShapeFilter;
	shape->sin = (short *)SinTable;
	shape->SinPtr = (short *) (shape->sin + 8);
	shape->CosPtr = (short *) (shape->sin + 0);

	shape->DelayLineIndex = 0;
	memset(shape->DelayLine, 0, 2*SHAPE_LEN*sizeof(short));
}


void Shape(SHAPE *shape, short *In, short *Out, short InLen)
{
	/*******************************************************************************
	 *	Description:
	 *  Function Shape produces four samples for each input complex symbol.
	 *	Then it modulates the signal and gets out a real signal.
	 *  The modulation is performed using a sinus table.
	 *	The function uses cyclic buffers.
	 *
	 *	Arguments:
	 *  SHAPE *shape - Shape setting and history.
	 *  short *In  - Input symbols complex array.
	 *  short *Out - Output samples real array.
	 *  short InLen - Number of input symbols.
	 *******************************************************************************/

	// Global Use Registers
	register short *Ptr1, *Ptr2;

	register long Acc0, Acc1;
	register short Al0, Al1;

	short i,k,m,n;

	i = m = 0;

	//-----Loop over input symbols-----
	while (i<InLen)
	{
		//Read the real and image symbol's coordinates into the delay line-----
		shape->DelayLine[shape->DelayLineIndex++] = *In++;
		shape->DelayLine[shape->DelayLineIndex++] = *In++;
		//-----Take care for the cyclicity of the delay line-----
		if (shape->DelayLineIndex == 2*SHAPE_LEN)
			shape->DelayLineIndex = 0;

		Ptr1 = shape->DelayLine + shape->DelayLineIndex;
		Ptr2 = shape->PolyPhaseFilter ;

		//-----Loop over phases-----
		for (k=0; k<4; k++)
		{
			Acc0 = Acc1 = 0L;

			for (n=0; n<SHAPE_LEN; n++)
			//-----Convoluting input symbols with rise cosine filter-----
			{
				//-----Real-----
				Acc0 += (long)*Ptr1++ * *Ptr2 << 1;
				//-----Image-----
				Acc1 += (long)*Ptr1++ * *Ptr2++ << 1;
				if (Ptr1 ==  shape->DelayLine + 2*SHAPE_LEN)
					Ptr1 = shape->DelayLine;
			}

			Al0 = (short)((Acc0+0x8000L)>>16);
			//-----modulation: x*cos-----
			Acc0 = (long) ( Al0 * *(shape->CosPtr) ) << 1;
			shape->CosPtr += 6;
			if ( shape->CosPtr > shape->sin+31)
				shape->CosPtr -= 32;

			Al1 = (short)((Acc1+0x8000L)>>16);
			//-----modulation:  add y*sin  -----
			Acc0 += (long) ( Al1 * *(shape->SinPtr) ) << 1;
			shape->SinPtr += 6;
			if ( shape->SinPtr > shape->sin+31)
				shape->SinPtr -= 32;

			Out[m++] = (short)((Acc0+0x8000L)>>16);
		}	//end of loop over phases
		i+=(short)1;
	}	//end of loop over symbols
}
