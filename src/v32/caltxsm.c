/*******************************************************************************
*	File calltxsm.c: call modem transmitter state machine.
*
*	Includes the subroutines:
*
*  1. CallTxSm.
*
*	Written by: Victor Elkonin.
*  Combined at: version 2.0 .
*
*******************************************************************************/
#include "stdio.h"
#include "v32rec.h"
#include "transmt.h"
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
#include "ratedet.h"
#include "equ.h"
#include "conv.h"
#include "viterbi.h"
#include "modem.h"
#include "caltxsm.h"
#include "anstxsm.h"




#ifdef V32_PRINTING
extern long SamplesCount;
#endif
short CallModemMode,ModemTxRate;

void CallTxSM(MODEM_V32 *transmitter)
{
/*******************************************************************************
*	Description:
*  Function CAllTxSm works on integer number of symbols.
*	Some parts of the function are designed to work on blocks of 8 symbols.
*	This function is responsible for:
*  1. Promote the states of the call transmiiter.
*  	The promotion is caused by a detection flag that is obtained from the
*     receiver or by counter that reaches certain value, some counters start
*     working after obtaining a detection flag.
*     The states corresponding one-to-one to figure 3 in V32 bis standard.
*  2. To set the general nature of the symbol to be transmitted using
*		the tx_mode variable, i.e. whether it is silence, symbols,
*		bits or data transmission.
*		The precise definition of the transmitted symbols is set using
*		variable tx_segment.
*	3. To calculate the Round Trip Delay for the Far Echo.
*	4. Signaling the call receiver using the TxToRxFlag variable.
*
*	Arguments:
*	MODEM_V32 *transmitter - modem setting and history.
*******************************************************************************/
	short Temp;

	switch(transmitter->tx_state)
	{
		case CALL_TX_INIT:
      //------State machine initialization-----

      	transmitter->tx_state = CALL_TX_SILENCE1;
      	transmitter->tx_mode = TX_MODE_SILENCE;
      	transmitter->tx_counter = -1;
      	break;


		case CALL_TX_SILENCE1:

      	if(transmitter->DetFlag)
         //--Start measure the silence time  only after the answer tone detection--
         	transmitter->tx_counter = CALL_TX_SILENCE1_AFTER_DET_SIZE;
      	if(transmitter->tx_counter-- == 0)
      	{
#ifdef V32_PRINTING
				printf("CALL-Transmitting AA %ld\n",SamplesCount);
#endif
				transmitter->tx_state = CALL_TX_AA;
				transmitter->tx_mode = TX_MODE_SYM;
				transmitter->tx_segment = TX_SEG_AA;
				//-----Start checking AA counting only after AC detection-----
				transmitter->tx_counter = -1;
				//-----Prevent SymGenerator to move to CC trx. too soon-----
				transmitter->SymPairs = -1;
			}
			break;

   	case CALL_TX_AA:

      	if(transmitter->DetFlag)
      	{
         	//-----Start count the AA pairs only after AC detection-----
         	transmitter->tx_counter = CALL_TX_AA_AFTER_DET_SIZE_BLOCK;
            //-----Determinate the number of the complete blocks-----
         	if ( transmitter->dtct.DetLocation > 2 * SYM_PER_BLOCK - 1 - CALL_TX_AA_AFTER_DET_SIZE_SMP)
         		transmitter->tx_counter++;
      	}
      	if(transmitter->tx_counter == 0)
      	{
#ifdef V32_PRINTING
			printf("CALL-Transmitting CC %ld\n",SamplesCount);
#endif
			transmitter->tx_state = CALL_TX_CC;
            /*-----Determinate the number of the sample in which the
                   transmitter starts to transmit AC-----*/
				transmitter->dtct.DetLocation += (short) (CALL_TX_AA_AFTER_DET_SIZE_SMP);
            //-----Insure that DetLocation is inside the block (0..2*SYM_PER_BLOCK-1)-----
            if(transmitter->dtct.DetLocation >= 2*SYM_PER_BLOCK)
					transmitter->dtct.DetLocation -= (short)(2*SYM_PER_BLOCK);
            /*-----Determinate number of AA pairs to transmit in the block
              		 the rest will be CC pairs-----*/
				transmitter->SymPairs = (short)(transmitter->dtct.DetLocation >> 2);
            //-----Round SymPairs to the nearest number-----
				if ( (transmitter->dtct.DetLocation & 0x2) > 0)
            	transmitter->SymPairs++;
            //-----Start Round Trip Delay calculation-----
				transmitter->EchoBitBuf.FarOffBlock = 0;
				transmitter->EchoBitBuf.FarOffSym =
				transmitter->EchoCnl.rtd = (short)(-transmitter->SymPairs << 2);
				
      	}
      	break;


   	case CALL_TX_CC:
   		if (++transmitter->tx_counter == WAIT_TO_SYM_DET_AFTER_TX_SIZE)
      		//-----Remove the detector from the idle position-----
         	transmitter->dtct.IdleFlag = FLAG_OFF;
      	transmitter->tx_segment = TX_SEG_CC;
      	if(transmitter->DetFlag)
      	{
#ifdef V32_PRINTING
			printf("CALL-Transmitting Silence2 %ld\n",SamplesCount);
#endif
			transmitter->tx_state = CALL_TX_SILENCE2;
         	transmitter->tx_mode = TX_MODE_SILENCE;
			//-----Add to RTD number of samples in the last (incomplete) block-----
         	transmitter->EchoCnl.rtd += (short)(transmitter->dtct.DetLocation+1);
            transmitter->EchoBitBuf.FarOffSym += (short)(transmitter->dtct.DetLocation+1);
            //------Substract from RTD the delay of answer modem response-----
         	transmitter->EchoCnl.rtd -= (short)(2 * SYM_PER_BLOCK * ANS_TX_CA_AFTER_DET_SIZE);
            transmitter->EchoBitBuf.FarOffBlock -= (short)ANS_TX_CA_AFTER_DET_SIZE;
            //-----Deduce echo far offset from the RTD-----
         	EchoOffset(&(transmitter->EchoCnl),&(transmitter->EchoBitBuf));
            printf("CALL RTD=%d\n",transmitter->EchoCnl.rtd);
      	}
      	else
         {
         	//-----Increase RTD. It is measured at 4800 smp/sec----
      		transmitter->EchoCnl.rtd += (short)(2 * SYM_PER_BLOCK);
            transmitter->EchoBitBuf.FarOffBlock++;
         }
      	break;


		case CALL_TX_SILENCE2:

   		if(transmitter->DetFlag)
      	{
#ifdef V32_PRINTING
			printf("CALL-Transmitting S %ld\n",SamplesCount);
#endif
			transmitter->tx_state = CALL_TX_S;
         	transmitter->tx_mode = TX_MODE_SYM;
         	transmitter->tx_segment = TX_SEG_S;
         	transmitter->tx_counter = 0;
      	}
      	break;


   	case CALL_TX_S:

      	//-----Add to S trx. time RTD-----
//			if(++transmitter->tx_counter == S_SIZE + (short)(1+1 + (transmitter->EchoCnl.rtd>>(1+SYM_PER_BLOCK_LOG2))))
//			if( ++transmitter->tx_counter == S_SIZE )
			if(transmitter->tx_counter >= S_SIZE*(SYM_PER_BLOCK<<1) + (short)(transmitter->EchoCnl.rtd))
      	{
#ifdef V32_PRINTING
			printf("CALL-Transmitting Sbar %ld\n",SamplesCount);
#endif
			transmitter->tx_state = CALL_TX_SBAR;
         	transmitter->tx_mode = TX_MODE_SYM;
         	transmitter->tx_segment = TX_SEG_SBAR;
         	transmitter->tx_counter = 0;
      	}
			else
				transmitter->tx_counter+=(short)(SYM_PER_BLOCK<<1);
      	break;


   	case CALL_TX_SBAR:

   		if(++transmitter->tx_counter == SBAR_SIZE)
      	{
#ifdef V32_PRINTING
			printf("CALL-Transmitting Trn %ld\n",SamplesCount);
#endif
			transmitter->tx_state = CALL_TX_TRN;
         	transmitter->tx_mode = TX_MODE_BIT;
				transmitter->BitGen.segment = TX_SEG_TRN;
         	transmitter->tx_counter = 0;
         	transmitter->EchoCnl.mode = ECHO_CANCELLER_SINGLE;
            transmitter->EchoBitBuf.OutIndexNear = (short)(transmitter->EchoBitBuf.InIndex);
            transmitter->EchoBitBuf.OutIndexNear -= (short)(transmitter->EchoBitBuf.NearOffBlock);
            if (transmitter->EchoBitBuf.OutIndexNear < 0)
               transmitter->EchoBitBuf.OutIndexNear += (short)ECH_BITS_BLOCK_NUM;
            transmitter->EchoBitBuf.OutIndexFar = (short)(transmitter->EchoBitBuf.InIndex);
            transmitter->EchoBitBuf.OutIndexFar -= (short)(transmitter->EchoBitBuf.FarOffBlock);
            if (transmitter->EchoBitBuf.OutIndexFar < 0)
               transmitter->EchoBitBuf.OutIndexFar += (short)ECH_BITS_BLOCK_NUM;
         	ScramInit( &(transmitter->scrambler), transmitter->Identity);
            //-----Set mapper to transmit only A and C points-----
				MapperInit(&(transmitter->mapper), TRAIN_START);
            transmitter->TrnBeginFlag = 1;
			}
      	break;


   	case CALL_TX_TRN:

      	if(++transmitter->tx_counter == TRN_BEGIN_SIZE)
         {
         	//-----Set mapper to transmit all the 4 points-----
         	MapperInit(&(transmitter->mapper), QPSK_2BITS);
            transmitter->TrnBeginFlag = 0;
         }
			if(transmitter->tx_counter == NORM_ECHO_SIZE + (transmitter->EchoCnl.rtd >>5))
         	EchoNorm(&(transmitter->EchoCnl));
			if(transmitter->tx_counter == TRN_MIN_SIZE + (transmitter->EchoCnl.rtd >>5) )
      	{
#ifdef V32_PRINTING
			printf("CALL-Transmitting R2 %ld\n",SamplesCount);
#endif
			transmitter->tx_state = CALL_TX_R2;
            transmitter->tx_counter = 0; //the counter is for rerate
            transmitter->BitGen.segment = TX_SEG_R;

            //Determinate the maximum rate that the constellation can hold
            Temp=(short)(transmitter->Equ.EquError>>15);
#ifdef V32_PRINTING
			printf("CALL-EquError %d\n",Temp);
#endif
#ifndef V32_NO_VITERBI
            transmitter->Rate =(short)( transmitter->MaxRate-2);
            while (--transmitter->Rate)
            {
            	if (Temp < transmitter->MaxErrorTbl[transmitter->Rate])
                   break;
            }

            if (transmitter->Rate < transmitter->MinRate-3)
               transmitter->Rate =(short)(transmitter->MinRate-3);

            transmitter->BitGen.word = transmitter->RWordTbl[transmitter->Rate];
            transmitter->Rate+=(short)3;
#else
            transmitter->BitGen.word = 0x0311;
            transmitter->Rate=4;
#endif
            //-----Signal the receiver to start looking for S-----
         	transmitter->TxToRxFlag = FLAG_ON;
            //-----Activate echo canceller in the double talk mode-----
         	transmitter->EchoCnl.mode = ECHO_CANCELLER_DOUBLE;
      	}
      	break;


   	case CALL_TX_R2:

      	transmitter->tx_counter--;
      	if(transmitter->RDetFlag && (transmitter->tx_counter<0))
      	{
#ifdef V32_PRINTING
			printf("CALL-Transmitting E %ld\n",SamplesCount);
#endif
			transmitter->tx_state = CALL_TX_E;
            transmitter->BitGen.word |= SYNC_BITS;
      	}
      	break;


   	case CALL_TX_E:

#ifdef V32_PRINTING
		printf("CALL-Transmitting B1 %ld\n",SamplesCount);
#endif
		transmitter->tx_state = CALL_TX_B1;
        transmitter->BitGen.segment = TX_SEG_B1;
        transmitter->ConvEncodeFlag = FLAG_ON;
       	transmitter->tx_counter = -1;
#ifndef V32_MODEM_FOR_SNAPFONE			
#ifndef V32_MODEM_FOR_FAX			
#ifndef V32_MODEM_FOR_VOICE					
        transmitter->TxN = transmitter->Rate;
		switch(transmitter->TxN)
		{
			case QAM16_3BITS:
				transmitter->TxRate = 7200;
				break;
			case QAM32_4BITS:
				transmitter->TxRate = 9600;
				break;
			case QAM64_5BITS:
				transmitter->TxRate = 12000;
				break;
			case QAM128_6BITS:
				transmitter->TxRate = 14400;
				break;
			default:
				transmitter->TxRate = MODEM_ERROR;
				break;
		}
#else			//voice
		transmitter->TxN = QAM32_4BITS;
		transmitter->TxRate = 9600;
#endif						
#else			//fax
		if((transmitter->Rate)==QAM128_6BITS)
		{
			transmitter->TxN = QAM128_6BITS;
			transmitter->TxRate = 14400;
		}
		else
		{
			transmitter->TxN = QAM32_4BITS;
			transmitter->TxRate = 9600;
		}
#endif
#else			//snapfone			
			transmitter->TxN = QAM64_5BITS;
			transmitter->TxRate = 12000;				
#endif
		SetTxRate( transmitter);
				
      	break;


   	case CALL_TX_B1:

      	if(transmitter->DetFlag)
         	//-----Start count the B1 symbols only after E detection-----
         	transmitter->tx_counter = CALL_TX_B1_AFTER_DET_SIZE;

   		if(transmitter->tx_counter-- == 0)
      	{

#ifdef V32_PRINTING
			printf("CALL-Transmitting DATA %ld\n",SamplesCount);
#endif
			transmitter->tx_state = CALL_TX_DATA;
    	    transmitter->tx_mode = TX_MODE_DATA;
				transmitter->DataInFlag = V32_DATA_START;
		}
		break;

   	case CALL_TX_DATA:
			break;


		case CALL_TX_START_RETRAIN:
			

#ifdef V32_PRINTING
				printf("CALL-TRX RETRAIN  %ld",*(((int *)&SamplesCount)+1 ),SamplesCount);
#endif
			transmitter->tx_state = CALL_TX_AA;
			transmitter->tx_mode = TX_MODE_SYM;
			transmitter->tx_segment = TX_SEG_AA;
			//-----Start checking AA counting only after AC detection-----
			transmitter->tx_counter = -1;
			//-----Prevent SymGenerator to move to CC trx. too soon-----
			transmitter->SymPairs = -1; 

  			break;

   	default:

   		break;
   }

}
