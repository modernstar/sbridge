/*******************************************************************************
* File rates_det.c: detection of rates signals (R1,R2,R3,E).
*
* Includes the subroutines:
*
* 1. RatesDetInit.
* 2. RatesDetect.
*
* Writen by: Victor Elkonin.
* Combined at: version 2.0.
*******************************************************************************
*	Changes:
*	version 2.1: 	Changes in subroutine RatesDetect.
*******************************************************************************/
#include <stdio.h>
#include "viterbi.h"
#include "ratedet.h"
#include "scram.h"
#include "slicer.h"

void RatesInit(RATES_DET *R)
{
   R->examined = 0xffff; //to avoid false detection at the beginning.
   R->Word = 0;
   R->skip = 0;
   R->state = RATE_DET_OFF;
}

short RatesDetect(RATES_DET *R, unsigned short *In, short BitNum)
{
/*******************************************************************************
* Description:
* Function RatesDetect works for word, i.e. 8 symbols (dibits).
*
* Arguments:
* RATES_DET *R - setting and history.
* unsigned short InputWord - InputWord.
*
* Returned value: detection flag.
********************************************************************************
*	Changes:
*	1. version 2.1: R->rates has now the actual rate of the modem.
*******************************************************************************/

   short BitsInWord;
   unsigned short A;
   unsigned short InputWord;
   short i;

   BitsInWord=i=0;

   //----- When looking for R or E advance each step one dibit (i.e. one symbol)-----

   while(BitNum)
   {
   	if (BitsInWord==0)
      {
      	 BitsInWord = 16;
         InputWord = In[i++];
      }

   	  BitNum     -= (short)2;
      BitsInWord -= (short)2;

   	//--Add the first dibit of the input word at the end of the examined word.
      A = (unsigned short) (R->examined<<2);
      R->examined =(unsigned short)( A + (InputWord>>14) );

      //-----Promote the input word-----
      InputWord<<=2;

      switch (R->state)
      {

         case RATE_DET_LOOK_FOR_R :
         	if (R->skip)
            	{
               	R->skip--;
                	break;
               }
      		if ((R->examined & SYNC_BITS) == CORRECT_SYNC)
      		{
					//-----Check if the current word equals to the previous one-----
      			if (R->examined==R->Word)
         		{
         			//-----R word is found : Extract the rate-----
						//-----The rates for Standard modem-----
      				if (R->examined & 0x0008) 
						R->rates=QAM128_6BITS;
     				else if (R->examined & 0x0020) 
						R->rates=QAM64_5BITS;
      				else if (R->examined & 0x0200) 
						R->rates=QAM32_4BITS;
      				else if (R->examined & 0x0040) 
						R->rates=QAM16_3BITS;
            		else R->rates = QPSK_2BITS;
             		//------determinate the E word-----
            		//R->E = (unsigned short)(R->Word | SYNC_BITS);
					R->examined = 0;
    	     		return RATE_SUCCESS;
         		}
         		else
         		//-----R word is not found, look again for the second occurrence-----
         		{
     					R->Word = R->examined;
                  //-----Do not check till you get the entire next word-----
                  R->skip = 7;
         		} //end of else
      		} //end of if
      		//-----Start the search fot the first occurance-----
      		else R->Word=0;
            break;

         case RATE_DET_LOOK_FOR_E :
				if ((R->examined & SYNC_BITS) == SYNC_BITS )
   			{
         		return RATE_SUCCESS;
         	}
            break;

         default :
         	break;

      }	//end of switch.
   }	//end of while

   return RATE_FAILURE;
}





