/*******************************************************************************
 *	File diff.c: differntial encoding and decoding.
 *
 *	Includes the subroutines:
 *
 *  1. DiffInit.
 *  2. DiffEncode.
 *  3. DiffDecode.
 *
 *	Written by: Mor Reich.
 *  Combined at: version 2.0 .
 *
 *******************************************************************************/

#include "diff.h"

void DiffInit(DIFF *diff, const unsigned short *Table, short N)
{
	diff->state = 0;
	diff->N = N;
	diff->Table = (unsigned short *)Table;
}

void DiffEncode(DIFF *diff, unsigned short *In, unsigned short *Out, short SymNum)
{
/*******************************************************************************
*	Description:
*  Function DiffEncode works on integer number of symbols.
*	The current bits before encoding are kept in Acc0.
*	The current encoded bits are kept in Acc1.
*
*	Arguments:
*  DIFF *diff - Encoder setting and history.
*  unsigned short *In  - Input bits array.
*  unsigned short *Out - Output bits array.
*  short SymNum - Number of input symbols.
*******************************************************************************/

	unsigned long Acc0;	//Hold The Data In
	unsigned long Acc1;  //For Calculate
	unsigned long  Temp;
	short ip=0,op=0,Flag;
   signed short ShT=15;			//for shifting

   //----Loop over symbols-----
  	Acc0=(unsigned long)In[ip++]<<16;   //Read Input Data
   Acc0+=(unsigned long)In[ip++];
	while (SymNum--)     					//Loop For Symbols
	{
      Flag=0;        					//Initiate Flag
   	Temp=Acc1=(0x18000<<ShT);   //Build Number To Mask Bits
      Acc1&=Acc0;                //Find The 2 Bits That Need To Change
      Acc0&=(~Temp);  				//Zero The Two Bits  That Need To Be Change
      Acc1=(Acc1>>ShT)>>15;   	//Bring The Two Bits To Begining
      Acc1+= diff->state<<2;     //Calc Offset Table
      Acc1=diff->Table[Acc1];    //Take The 2 New Bits From Table
      diff->state=(unsigned short)Acc1; 	//Update Diff->state
      Acc0+=(Acc1<<15)<<ShT;		//Update The Two Bits In Acc0
      ShT-=diff->N;
      if (ShT<0)               //Check If Finished To Changed One Word
      {
        ShT+=(short)16;
        Out[op++]=(unsigned short)(Acc0>>16); //Write Word To OutPut
        Acc0<<=16;
        Acc0+=(unsigned long)In[ip++];   		//read New Word
        Flag=1;
      }
   }  // end of while
   if (Flag==0) Out[op]=(unsigned short)(Acc0>>16);  //If There Are Bits Left
}                                                    //Write Them To Out




void DiffDecode(DIFF *diff, unsigned short *In, unsigned short *Out, short SymNum)
{
/*******************************************************************************
*	Description:
*  Function DiffDecode works on integer number of symbols.
*	The current bits before decoding are kept in Acc0.
*	The current decoded bits are kept in Acc1.
*
*	Arguments:
*  DIFF *diff - Decoder setting and history.
*  unsigned short *In  - Input bits array.
*  unsigned short *Out - Output bits array.
*  short SymNum - Number of input symbols.
*******************************************************************************/
	unsigned long Acc0;	//Hold The Data In
	unsigned long Acc1;  //For Calculate
	unsigned long  Temp;
	short ip=0,op=0,Flag;
   signed short ShT=15;			//for shifting

   //----Loop over symbols-----
  	Acc0=(unsigned long)In[ip++]<<16;  //Read Input Data
   Acc0+=(unsigned long)In[ip++];
	while (SymNum--)          			 //Loop For Symbols
	{
      Flag=0;                       //Initiate Flag
   	Temp=Acc1=(0x18000<<ShT);    //Build Number To Mask Bits
      Acc1&=Acc0;                  //Find The 2 Bits That Need To Change
      Acc0&=(~Temp);  					//Zero The Two Bits
      Acc1=(Acc1>>ShT)>>15;   	//Bring The Two Bits To Begining
      Acc1+= diff->state<<2;      //Calc Offset Table
      diff->state=(unsigned short)(Acc1&3); //Calc New diff->state
      Acc1=diff->Table[Acc1];      //Take The 2 New Bits From Table
      Acc0+=(Acc1<<15)<<ShT;		//Update The Two Bits In Acc0
      ShT-=diff->N;
      if (ShT<0)                //Check If Finished To Changed One Word
      {
        ShT+=(short)16;
        Out[op++]=(unsigned short)(Acc0>>16);  //Write Word To OutPut
        Acc0<<=16;
        Acc0+=(unsigned long)In[ip++];     	//read New Word
        Flag=1;
      }
   }  // end of while
   if (Flag==0) Out[op]=(unsigned short)(Acc0>>16);  //If There Are Bits Left
}                                                   //Write Them To Out


