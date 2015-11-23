/*******************************************************************************
 *	File mapping.c: mapping and demapping.
 *
 *	Includes the subroutines:
 *
 *  1. Mapper.
*  2. Demapper: for modem with viterbi.
*  3. Demapper: for modem with slicer.
 *
 *	Written by: Mor Reich.
 *  Combined at: version 2.0 .
 *
********************************************************************************
*	Changes:
*	version 2.4: 	Changes in function Mapper.
*						Addition of function Demapper for modem with slicer.
*  version 3.0:   Change in Function Mapper.
********************************************************************************/

#include "scram.h"
#include "slicer.h"
#include "mapper.h"
//#include "viterbi.h"


void Mapper(MAPPER *mapper, unsigned short *In, short *Out, short SymNum)
{
	/*******************************************************************************
	 *	Description:
	 *  Function mapper works on integer number of symbols.
	 *	The mapping is performed using a table.
	 *	The current bits to be mapped are kept in Acc0.
	 *
	 *	Arguments:
	 *  mapper *mapper - Mapper setting.
	 *  unsigned short *In  - Input bits array.
	 *  short *Out - Output samples complex array.
	 *  short SymNum - Number of input symbols.
********************************************************************************
*	Changes:
*	1. version 2.4: Modem with slicer : do not change N.
*  2. verdion 3.0: Changes for the implementation.
*******************************************************************************/
	unsigned long Acc0;	//the bits to be mapped.
	unsigned short symbol;	//the current symbol to be mapped.
	short i, m;
	short ExtraBitNum,SymComponent;	//N minus number of bits in Acc0.
	short *map;	//the constellation.
	short N;

	map = mapper->map;
	N = (short) (mapper->N);
#ifndef V32_NO_VITERBI
	if (N!=2) N++;
#endif
	i = m = 0;
   ExtraBitNum = N;
	Acc0 = 0L;

	//----Loop over symbols-----
	while (SymNum--)
	{
      //-----Promote Acc0 to the next symbol-----
      Acc0 <<= N;

   	if (ExtraBitNum>0)
		{
			//-----Load the new word into Acc0-----
         Acc0 += (unsigned long) (In[i++]<<(ExtraBitNum));
         ExtraBitNum -= (short) 16;
		}
      //-----map the current symbol-----
		symbol = (unsigned short) (Acc0 >> 16);
		Acc0 &= (unsigned long)(0xffff); //Clear the undeeded bits
  		//-----Update the new ExtraBitNum-----
		ExtraBitNum += (short)N;

		SymComponent=map[symbol];
		SymComponent>>=8;
		Out[m++] =(short)(SymComponent*mapper->kC);
		SymComponent=map[symbol];
		SymComponent<<=8;
		SymComponent>>=8;
		Out[m++] =(short)(SymComponent*mapper->kC);
   }
}


void Demapper(short *In, unsigned short *Out, short SymNum)
{
	/*******************************************************************************
	 *	Description:
	 *  Function demapper works only for the constellation of 4800 bps.
*  It works on integer number of symbols.
	 *
	 *	Arguments:
	 *  short *In  - Input samples complex array.
	 *  unsigned short *Out - Output bits array.
	 *  short SymNum - Number of input symbols.
	 *******************************************************************************/
	short i,k;
	short x,y;
	unsigned short dibit;

	i = k = 0;

	//----Loop over symbols-----
	while (SymNum--)
	{
		x = In[i++];
		y = In[i++];

		dibit = 0;
		if (y>0) dibit+=(unsigned short)2;
		if (x>0) dibit+=(unsigned short)1;
		Out[k++] = dibit;

	}
}


void DemapperN(DEMAPPER *demapper, short *In, unsigned short *Out, short SymNum)
{
	/*******************************************************************************
	 *	Description:
	 *  Function demapper works on integer number of symbols.
	 *
	 *	Arguments:
	 *  DEMAPPER *demapper - Demapper setting.
	 *  short *In  - Input samples complex array.
	 *  unsigned short *Out - Output bits array.
	 *  short SymNum - Number of input symbols.
	 *******************************************************************************/
	long Point;
	short i,k;
	short *tbl;

	i = k = 0;

	//----Loop over symbols-----
	while (SymNum--)
	{
		tbl=demapper->Table;
		
		Point = (long)In[i++] - (long)demapper->Corner;
		
		if(Point>0)
		{
			tbl += demapper->Column;
			Point -= demapper->StepSize;
		}

		if(Point>0)
		{
			tbl += demapper->Column;
			Point -= demapper->StepSize;
		}

		if(Point>0)
		{
			tbl += demapper->Column;
		}

		Point = (long)In[i++] - (long)demapper->Corner;
		if(Point>0)
		{
			tbl++;
			Point -= demapper->StepSize;
		}

		if(Point>0)
		{
			tbl++;
			Point -= demapper->StepSize;
		}

		if(Point>0)
		{
			tbl++;
		}

		Out[k++] = *tbl;
	}
}
