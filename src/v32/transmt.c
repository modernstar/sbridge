/*******************************************************************************
 * File transmit.c: the main modem transmitter functions.
 *
 * Includes the subroutines:
 *
 * 1. ModemTransmitter.
 *
 * Writen by: Victor Elkonin.
 * Combined at: version 2.0.
 *
********************************************************************************
*	Changes:
*	version 2.4: 	Changes in function ModemTransmitter.
 *******************************************************************************/
#include "memory.h"
#include <stdio.h>
#include "v32rec.h"
#include "scram.h"
#include "diff.h"
#include "shape.h"
#include "anstone.h"
#include "goertzl.h"
#include "rate.h"
#include "dcmf.h"
#include "filter.h"
#include "detect.h"
#include "agc.h"
#include "echocan.h"
#include "slicer.h"
#include "mapper.h"
#include "timing.h"
#include "equ.h"
#include "conv.h"
#include "viterbi.h"
#include "ratedet.h"
#include "transmt.h"
#include "modem.h"

extern FILE *ans_rx, *call_rx;

short ModemTransmitter(MODEM_V32 *modem)
{
	/*******************************************************************************
	 * Description:
	 * Function ModemTransmitter is the main function of the modem transmitter,
	 * that is called by V14. It calls to all the transmitter components.
	 * This function works on integer number of symbols.
	 * The signaling stage is restricted to 8 symbols per block.
	 *
	 * Arguments:
	 * MODEM_V32 *modem - setting and history of the modem.
	 *
	 * Returned value: number of the output samples at 8000 smp/sec rate.
	 ********************************************************************************
	 *	Changes:
	 *	1. version 2.4: Modem with slicer : do not call convolution encoding.
	 *******************************************************************************/

//	short TmpSmpBuf[TMP_SMP_BUF_SIZE];
	short TmpSmpBuf[48];
	short TmpSmpBuf1[SYM_PER_BLOCK*2];
	unsigned short TmpBitBuf[TMP_BIT_BUF_SIZE];
	unsigned short TmpBitBuf1[TMP_BIT_BUF_SIZE];
	short Smp9600Num;
	short OutSmpNum;

	Smp9600Num = (short) (modem->TxSymNum * SMP_PER_SYM);
	modem->TxToRxFlag = FLAG_OFF;
   memset(TmpBitBuf,0,sizeof(TmpBitBuf1));

	if (modem->Identity==ANSWER) AnsTxSM(modem);
	else CallTxSM(modem);

	modem->InBitNum = (short) (modem->TxSymNum * modem->TxN);

	switch(modem->tx_mode)
	{
		case TX_MODE_ANSWER_TONE:

			AnswerTone( &(modem->AnsTone), TmpSmpBuf, Smp9600Num);
			break;

		case TX_MODE_BIT:
			// Handle Trn,R,E,B1
         BitGenerator(&(modem->BitGen), modem->InBit, modem->InBitNum);

		case TX_MODE_DATA:

			Scrambler( &(modem->scrambler), modem->InBit, TmpBitBuf, modem->InBitNum);

			if (modem->BitGen.segment != TX_SEG_TRN)

			DiffEncode( &(modem->DiffEnc), TmpBitBuf, TmpBitBuf, modem->TxSymNum);
#ifndef V32_NO_VITERBI
         if (modem->ConvEncodeFlag == FLAG_ON)
			{
				memcpy(TmpBitBuf1, TmpBitBuf, sizeof(short)*TMP_BIT_BUF_SIZE);
				ConvEncode( &(modem->conv), TmpBitBuf1, TmpBitBuf, modem->TxSymNum);
  			}
#endif
		case TX_MODE_SYM:

			if (modem->tx_mode == TX_MODE_SYM)
				// Handle AA,CC,AC,CA,S,SBAR
         {
             SymGenerator(modem->tx_segment, TmpBitBuf, SYM_PER_BLOCK, modem->SymPairs);
				 //if (modem->V32Record.Command.Single.Global)
             	//memcpy(modem->V32Record.GlobalPtr,TmpBitBuf,2*sizeof(short));
         }


			Mapper( &(modem->mapper), TmpBitBuf, TmpSmpBuf1, modem->TxSymNum);

		case TX_MODE_SILENCE:

			if (modem->tx_mode == TX_MODE_SILENCE)
				memset(TmpSmpBuf1, 0, sizeof(short)*2*modem->TxSymNum);
         else if (modem->TxN == QPSK_2BITS)
         {
         	memcpy(TmpBitBuf1+1, TmpBitBuf, sizeof(short)*(BIT_BUF_BLOCK_SIZE-1));
            memcpy(TmpBitBuf, TmpBitBuf1, sizeof(short)*BIT_BUF_BLOCK_SIZE);
            TmpBitBuf[0]= (unsigned short)(modem->TxN - modem->TrnBeginFlag);
         }
         else if (modem->EchoBufDataFlag == ECHO_PUT_NOT_DATA)  //check if is it the first data rate block
         {
            	modem->EchoBufDataFlag =ECHO_NEAR_NOT_DATA;  //switch on the data rate flag
               //mark in the last entry of the  EchoBitBuf.DelayLine the move to data rate
               //otherwise it is zero.
               if (modem->EchoBitBuf.InIndex)
                  modem->EchoBitBuf.DelayLine[BIT_BUF_BLOCK_SIZE*modem->EchoBitBuf.InIndex-1] = modem->TxN;
               else
                  modem->EchoBitBuf.DelayLine[ECH_BITS_DELAY_LEN-1] = modem->TxN;
         }

     	 	//insert BIT_BUF_BLOCK_SIZE words from TmpBitBuf into the delay buffer:
     		if (modem->EchoCnl.mode != ECHO_CANCELLER_OFF)
	      {
				memcpy(modem->EchoBitBuf.DelayLine+BIT_BUF_BLOCK_SIZE*modem->EchoBitBuf.InIndex,TmpBitBuf,BIT_BUF_BLOCK_SIZE*sizeof(short));
         	if (++(modem->EchoBitBuf.InIndex) == ECH_BITS_BLOCK_NUM)
           		modem->EchoBitBuf.InIndex = 0;
         }

			Shape( &(modem->shape), TmpSmpBuf1, TmpSmpBuf, modem->TxSymNum);

			break;

		default:

			break;
	}

	// Recording...
	if (modem->V32Record.Command.Single.Tx)
		memcpy(modem->V32Record.TxPtr,TmpSmpBuf,Smp9600Num*sizeof(short));

	OutSmpNum = rat9_6t8( &(modem->DownRate), TmpSmpBuf, modem->OutSmp, Smp9600Num);

	return OutSmpNum;
}

