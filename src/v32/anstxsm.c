/*******************************************************************************
*	File anstxsm.c: answer modem transmitter state machine.
*
*	Includes the subroutines:
*
*  1. AnsTxSm.
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
#include "anstxsm.h"
#include "caltxsm.h"


extern long SamplesCount;
extern short ModemTxRate;
short AnsModemMode;
void AnsTxSM(MODEM_V32 *transmitter)
{
/*******************************************************************************
*	Description:
*  Function AnsTxSm works on integer number of symbols.
*	Some parts of the function are designed to work on blocks of 8 symbols.
*	This function is responsible for:
*  1. Promote the states of the answer transmiiter.
*  	The promotion is caused by a detection flag that is obtained from the
*     receiver or by counter that reaches certain value, some counters start
*     working after obtaining a detection flag.
*     The states corresponding one-to-one to figure 3 in V32 bis standard.
*  2. To set the general nature of the symbol to be transmitted using
*		the tx_mode variable, i.e. whether it is silence, answer-tone, symbols,
*		bits or data transmission.
*		The precise definition of the transmitted symbols is set using
*		variable tx_segment.
*	3. To calculate the Round Trip Delay for the Far Echo.
*	4. Signaling the answer receiver using the TxToRxFlag variable.
*
*	Arguments:
*	MODEM_V32 *transmitter - modem setting and history.
*******************************************************************************/
	short Temp;

	switch(transmitter->tx_state)
	{
		case ANS_TX_INIT:
      //-----State machine initialization-----

      	transmitter->tx_state = ANS_TX_SILENCE1;
      	transmitter->tx_mode = TX_MODE_SILENCE;
      	transmitter->tx_counter = 0;
      	break;


		case ANS_TX_SILENCE1:

   		if(++transmitter->tx_counter == ANS_TX_SILENCE1_SIZE)
      	{
         	transmitter->tx_state = ANS_TX_ANSTONE;
         	transmitter->tx_mode = TX_MODE_ANSWER_TONE;
         	transmitter->tx_counter = 0;
      	}
         break;


   	case ANS_TX_ANSTONE:

      	if(++transmitter->tx_counter == ANSWER_TONE_SIZE)
      	{
      		transmitter->tx_state = ANS_TX_SILENCE2;
	      	transmitter->tx_mode = TX_MODE_SILENCE;
            //-----Signal the receiver to start looking for AA-----
         	transmitter->TxToRxFlag = FLAG_ON;
      	}
      	break;


		case ANS_TX_SILENCE2:

      	if(transmitter->DetFlag)
      	{
#ifdef V32_PRINTING
			printf("ANS-Transmitting AC1 %ld\n",SamplesCount);
#endif
      		transmitter->tx_state = ANS_TX_AC1;
	      	transmitter->tx_mode = TX_MODE_SYM;
	      	transmitter->tx_segment = TX_SEG_AC;
         	transmitter->tx_counter = 0;
      	}
      	break;


   	case ANS_TX_AC1:

      	if(++transmitter->tx_counter == ANS_TX_AC1_SIZE)
      	{
#ifdef V32_PRINTING
			printf("ANS-Transmitting CA %ld\n",SamplesCount);
#endif
      		transmitter->tx_state = ANS_TX_CA;
	      	transmitter->tx_segment = TX_SEG_CA;
            //-----Start checking the CA counting only after CC detection-----
         	transmitter->tx_counter = -1;
            //-----Start Round Trip Delay calculation------
				transmitter->EchoCnl.rtd = 0;
            transmitter->EchoBitBuf.FarOffBlock = 0;
				transmitter->EchoBitBuf.FarOffSym = 0;
            //-----Prevent SymGenerator to move to AC2 trx. too soon-----
         	transmitter->SymPairs = -1; 
      	}
      	break;


   	case ANS_TX_CA:

   		if(transmitter->tx_counter ==  - WAIT_TO_SYM_DET_AFTER_TX_SIZE)
         	//-----Remove the detector from the idle position-----
         	transmitter->dtct.IdleFlag = FLAG_OFF;
      	if(transmitter->DetFlag)
      	{
         	//-----Start count CA pairs after CC detection-----
         	transmitter->tx_counter = ANS_TX_CA_AFTER_DET_SIZE_BLOCK;
            //-----Determinate number of the complete blocks-----
         	if (transmitter->dtct.DetLocation > 2 * SYM_PER_BLOCK - 1 -ANS_TX_CA_AFTER_DET_SIZE_SMP)
         		transmitter->tx_counter++;
            //----Add to RTD number of samples in the last (incomplete) block----
         	transmitter->EchoCnl.rtd += (short)(transmitter->dtct.DetLocation+1);
            transmitter->EchoBitBuf.FarOffSym += (short)(transmitter->dtct.DetLocation+1);
            //-----Substract from RTD the delay of call modem response-----
         	transmitter->EchoCnl.rtd -= (short)(2 * SYM_PER_BLOCK * CALL_TX_AA_AFTER_DET_SIZE);
            transmitter->EchoBitBuf.FarOffBlock -= (short)CALL_TX_AA_AFTER_DET_SIZE;
      	}
      	else if(transmitter->tx_counter<0)
         {
         	//-----Increase RTD. It is measured at 4800 smp/sec-----
      		transmitter->EchoCnl.rtd += (short)(2 * SYM_PER_BLOCK);
            transmitter->EchoBitBuf.FarOffBlock++;
         }
			if(transmitter->tx_counter-- == 0)
      	{
#ifdef V32_PRINTING			
			printf("ANS-Transmitting AC2 %ld\n",SamplesCount);
#endif
      		transmitter->tx_state = ANS_TX_AC2;
         	transmitter->tx_counter = 0;
            /*-----Determinate the number of the sample in which the
                   transmitter starts to transmit AC-----*/
				transmitter->dtct.DetLocation += (short)ANS_TX_CA_AFTER_DET_SIZE_SMP;
            //-----Insure that DetLocation is inside the block (0..2*SYM_PER_BLOCK-1)-----
            if(transmitter->dtct.DetLocation >= 2*SYM_PER_BLOCK)
					transmitter->dtct.DetLocation -= (short)(2*SYM_PER_BLOCK);
            /*-----Determinate number of CA pairs to transmit in the block,
                   the rest will be AC pairs-----*/
				transmitter->SymPairs = (short)((transmitter->dtct.DetLocation>>2));
            //-----Round SymPairs to nearest number-----
				if ( (transmitter->dtct.DetLocation & 0x2) > 0 )
					transmitter->SymPairs++;
            //-----Deduce echo far offset from the RTD-----
         	EchoOffset(&(transmitter->EchoCnl),&(transmitter->EchoBitBuf));
            printf("ANS RTD=%d\n",transmitter->EchoCnl.rtd);
      	}
      	break;


   	case ANS_TX_AC2:

      	//-----To move this line into SymGenerator????
   		transmitter->tx_segment = TX_SEG_AC;
      	if(transmitter->DetFlag)
      	{
#ifdef V32_PRINTING
			printf("ANS-Transmitting Silence %ld\n",SamplesCount);
#endif
      		transmitter->tx_state = ANS_TX_SILENCE3;
	      	transmitter->tx_mode = TX_MODE_SILENCE;
         	transmitter->tx_counter = 0;
      	}
      	break;


   	case ANS_TX_SILENCE3:

   		if(++transmitter->tx_counter == ANS_TX_SILENCE3_SIZE)
      	{
#ifdef V32_PRINTING
			printf("ANS-Transmitting S1 %ld\n",SamplesCount);
#endif
         	transmitter->tx_state = ANS_TX_S1;
  	      	transmitter->tx_mode = TX_MODE_SYM;
         	transmitter->tx_segment = TX_SEG_S;
         	transmitter->tx_counter = 0;
      	}
      	break;


   	case ANS_TX_S1:

   		if(++transmitter->tx_counter == S_SIZE)
      	{
#ifdef V32_PRINTING
			printf("ANS-Transmitting Sbar1 %ld\n",SamplesCount);
#endif
         	transmitter->tx_state = ANS_TX_SBAR1;
         	transmitter->tx_segment = TX_SEG_SBAR;
         	transmitter->tx_counter = 0;
      	}
      	break;


   	case ANS_TX_SBAR1:

   		if(++transmitter->tx_counter == SBAR_SIZE)
      	{
#ifdef V32_PRINTING
			printf("ANS-Transmitting Trn1 %ld\n",SamplesCount);
#endif
         	transmitter->tx_state = ANS_TX_TRN1;
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


   	case ANS_TX_TRN1:

      	if(++transmitter->tx_counter == TRN_BEGIN_SIZE)
         {
         	//-----Set mapper to transmit all the 4 points-----
         	MapperInit(&(transmitter->mapper), QPSK_2BITS);
				transmitter->TrnBeginFlag = 0;
         }
			if(transmitter->tx_counter == NORM_ECHO_SIZE + (transmitter->EchoCnl.rtd >>5))
         	EchoNorm(&(transmitter->EchoCnl));
			if(transmitter->tx_counter == TRN_MIN_SIZE + (transmitter->EchoCnl.rtd >> 5))
      	{
#ifdef V32_PRINTING
			printf("ANS-Transmitting R1 %ld\n",SamplesCount);
#endif
         	transmitter->tx_state = ANS_TX_R1;
            transmitter->BitGen.segment = TX_SEG_R;
#ifndef V32_NO_VITERBI
            transmitter->BitGen.word = transmitter->RWordTbl[transmitter->MaxRate - 3];
#else
            transmitter->BitGen.word = 0x0311;
#endif
            //-----Signal the receiver to start looking for S-----
         	transmitter->TxToRxFlag = FLAG_ON;
            //-----Activate echo canceller in the double talk mode-----
         	transmitter->EchoCnl.mode = ECHO_CANCELLER_DOUBLE;
      	}
      	break;


   	case ANS_TX_R1:

      	if(transmitter->DetFlag)
      	{
#ifdef V32_PRINTING
			printf("ANS-Transmitting Silence %ld\n",SamplesCount);
#endif
      		transmitter->tx_state = ANS_TX_SILENCE4;
         	transmitter->tx_mode = TX_MODE_SILENCE;
      	}
      	break;


   	case ANS_TX_SILENCE4:

      	if(transmitter->DetFlag)
      	{
#ifdef V32_PRINTING
			printf("ANS-Transmitting S2 %ld\n",SamplesCount);
#endif
      		transmitter->tx_state = ANS_TX_S2;
			transmitter->tx_mode = TX_MODE_SYM;
  			transmitter->tx_segment = TX_SEG_S;
         	transmitter->tx_counter = 0;
      	}
      	break;


   	case ANS_TX_S2:

   		if(++transmitter->tx_counter == S_SIZE)
      	{
#ifdef V32_PRINTING
			printf("ANS-Transmitting Sbar2 %ld\n",SamplesCount);
#endif
         	transmitter->tx_state = ANS_TX_SBAR2;
  			transmitter->tx_segment = TX_SEG_SBAR;
         	transmitter->tx_counter = 0;
      	}
      	break;


   	case ANS_TX_SBAR2:

   		if(++transmitter->tx_counter == SBAR_SIZE)
      	{
#ifdef V32_PRINTING			
			printf("ANS-Transmitting Trn2 %ld\n",SamplesCount);
#endif
         	transmitter->tx_state = ANS_TX_TRN2;
         	transmitter->tx_mode = TX_MODE_BIT;
            transmitter->BitGen.segment = TX_SEG_TRN;
         	transmitter->tx_counter = 0;
         	ScramInit( &(transmitter->scrambler), transmitter->Identity);
				MapperInit(&(transmitter->mapper), TRAIN_START);
            transmitter->TrnBeginFlag = 1;
            //-----Set mapper to transmit only A and C points-----
			}
      	break;


   	case ANS_TX_TRN2:

      	if(++transmitter->tx_counter == TRN_BEGIN_SIZE)
         {
         	//-----Set mapper to transmit all the 4 points-----
         	MapperInit(&(transmitter->mapper), QPSK_2BITS);
            transmitter->TrnBeginFlag = 0;
         }
   		if(transmitter->tx_counter == TRN2_SIZE)
      	{
#ifdef V32_PRINTING
			printf("ANS-Transmitting R3 %ld\n",SamplesCount);
#endif
         	transmitter->tx_state = ANS_TX_R3;
            transmitter->tx_counter = 0; //because of the rerate
            transmitter->BitGen.segment = TX_SEG_R;

				if ( transmitter->MaxRate < transmitter->RatesDet.rates)
	            transmitter->Rate=transmitter->MaxRate;
            else
					transmitter->Rate=transmitter->RatesDet.rates;

            //Determinate the maximum rate that the constellation can hold
            Temp=(short)(transmitter->Equ.EquError>>15);
#ifdef V32_PRINTING
            printf("ANS-EquError %d\n",Temp);
#endif
			transmitter->Rate-=(short)2;
            while (--transmitter->Rate)
            {
            	if (Temp < transmitter->MaxErrorTbl[transmitter->Rate])
                   break;
            }

            if (transmitter->Rate < transmitter->MinRate-3)
               transmitter->Rate = (short)(transmitter->MinRate-3);
#ifndef V32_NO_VITERBI
            transmitter->BitGen.word = transmitter->RWordTbl[transmitter->Rate];
#else
            transmitter->BitGen.word = 0x0311;
#endif
				transmitter->Rate+=(short)3;
            //transmitter->RatesDet.rates = transmitter->Rate; //for what???
      	}
      	break;



   	case ANS_TX_R3:

         /*-----During the start-up procedure begin E trx. after getting
                E from the CALL -----*/
        	transmitter->RDetFlag = transmitter->DetFlag;	//=E detection.
			transmitter->tx_counter--;
			if(transmitter->RDetFlag && (transmitter->tx_counter<0))
         /*-----During rerate procedure start E trx after getting R2 from
                the CALL, in condition that 5 blocks of R3 were trx -----*/
        	{
      		transmitter->tx_state = ANS_TX_E;
            transmitter->BitGen.word |= SYNC_BITS;
      		}
      	break;


   	case ANS_TX_E:

#ifdef V32_PRINTING
		printf("ANS-Transmitting B1 %ld\n",SamplesCount);
#endif
		transmitter->tx_state = ANS_TX_B1;
        transmitter->BitGen.segment = TX_SEG_B1;
        transmitter->ConvEncodeFlag = FLAG_ON;
        transmitter->tx_counter = 0;
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
#else	//voice
		transmitter->TxN = QAM32_4BITS;
		transmitter->TxRate = 9600;
#endif
#else	//fax				
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


   	case ANS_TX_B1:

   		if(++transmitter->tx_counter == B1_SIZE)
      	{
#ifdef V32_PRINTING
			printf("ANS-Transmitting DATA %ld\n",SamplesCount);
#endif
			transmitter->tx_state = ANS_TX_DATA;
			transmitter->tx_mode = TX_MODE_DATA;
			transmitter->DataInFlag = V32_DATA_START;
      	}
      	break;


   	case ANS_TX_DATA:
			break;

		case ANS_TX_START_RETRAIN:	

#ifdef V32_PRINTING
			printf("ANS-TRX RETRAIN  %ld\n",SamplesCount);
#endif
			transmitter->tx_state = ANS_TX_SILENCE2; //move to CA trx only after detecting AA
			transmitter->tx_mode = TX_MODE_SYM;
			transmitter->tx_segment = TX_SEG_AC;

  			break;

   	default:

   		break;
   }
}









