/*******************************************************************************
*	File ansrxsm.c: answer modem receiver state machine.
*
*	Includes the subroutines:
*
*  1. AnsRxSm.
*
*	Written by: Victor Elkonin.
*  Combined at: version 2.0 .
*
*******************************************************************************/
#include <memory.h>
#include <stdio.h>
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
#include "ansrxsm.h"

long AnsError=0;
#ifdef V32_PRINTING
extern long SamplesCount;
#endif
extern short ModemRxRate;

void AnsRxSM(MODEM_V32 *receiver)
{
/*******************************************************************************
*	Description:
*  Function AnsRxSm works on integer number of symbols.
*	Some parts of the function are designed to work on blocks of 8 symbols.
*	This function is responsible for:
*  1. Promote the states of the answer receiver.
*  	The promotion is caused by a detection flag that is obtained from the
*     receiver's function, by counter that reaches certain value,
*     or by TxToRxFlag that is received from the transmitter.
*     The states corresponding one-to-one to figure 3 in V32 bis standard.
*	2. Set the correct init detector state.
*	3. Promote the rates detectors states.
*
*	Arguments:
*	MODEM_V32 *receiver - modem setting and history.
*******************************************************************************/

	switch(receiver->rx_state)
	{

   	case ANS_RX_START:

      	if(receiver->TxToRxFlag)
      	{
         	//-----Start looking for AA only after finishing answer tone trx.-----
      		receiver->rx_state = ANS_RX_AA;
         	receiver->dtct.mode = DTCT_ENERGY;
            receiver->dtct.SpectrumFlag = DTCT_DC;
			receiver->dtct.filter.SignalDelayLineOffset=0;
      	}
      	break;


   	case ANS_RX_AA:

      	if(receiver->DetFlag)
         {
      		receiver->rx_state = ANS_RX_CC;
            receiver->dtct.mode = DTCT_CORRELATION;
#ifdef V32_PRINTING
         	printf("ANS-AA Received At: %ld\n",SamplesCount);
#endif
         }
      	break;


   	case ANS_RX_CC:

      	if(receiver->DetFlag)
         {
      		receiver->rx_state = ANS_RX_SILENCE;
            receiver->dtct.mode = DTCT_SILENCE;
#ifdef V32_PRINTING			
			printf("ANS-CC Received %ld\n",SamplesCount);
#endif
         }
      	break;


   	case ANS_RX_SILENCE:

      	if(receiver->DetFlag)
			{
      		receiver->rx_state = ANS_RX_WAIT;
            receiver->dtct.mode = DTCT_OFF;
			}
      	break;


   	case ANS_RX_WAIT:

      	if(receiver->TxToRxFlag)
      	{
         	//-----Start looking for S only after beginng of the R1 trx.------
	     		receiver->rx_state = ANS_RX_S;
            receiver->dtct.mode = DTCT_ENERGY;
            receiver->dtct.SpectrumFlag = DTCT_ALL_SPECTRUM;
      	}
      	break;


   	case ANS_RX_S:

       	if(receiver->DetFlag)
      	{
#ifdef V32_PRINTING
			printf("ANS-S Received %ld\n",SamplesCount);
#endif
			receiver->rx_state = ANS_RX_SBAR;
            receiver->dtct.mode = DTCT_CORRELATION;
         	receiver->AgcFlag  = FLAG_ON;
         	//receiver->dtct.state = DTCT_SBAR;
         	receiver->rx_counter = 0;
		}
      	break;


	   case ANS_RX_SBAR:

//		if( ++receiver->rx_counter == 4 )
			if ((receiver->rx_counter >= receiver->EchoCnl.rtd) && !receiver->GlobalFlag)
         {
         	//-----Start timing without presence off the far echo-----
				receiver->GlobalFlag=1;
      		receiver->TimingFlag = FLAG_ON;
         }
      	else if(receiver->DetFlag)
      	{
#ifdef V32_PRINTING
			printf("ANS-Sbar Received %ld\n",SamplesCount);
#endif
			receiver->dtct.mode =  DTCT_OFF;
      		receiver->rx_state = ANS_RX_TRN;
            //-----Init Equalizer first time-----
#ifdef V32_PRINTING
			printf("ANS-Equ Active %ld\n",SamplesCount);
#endif
			receiver->Equ.State = EQU_START;
            if (EqualizerInit( &(receiver->Equ), receiver->timing.flag, receiver->dtct.DetLocation))
					//-----If the SBAR signal is not completed yet, turn off the Equalizer-----
               receiver->Equ.State = EQU_OFF;
            receiver->rx_counter = 0;
  				//-----Signal slicer to work in the TRAIN mode-----
            receiver->RxN = TRAIN_START;
         	SetRxRate(receiver);
            //------Prevent answer modem to transmit S too soon-----
         	receiver->DetFlag = FLAG_OFF;
      	}
		else
			receiver->rx_counter+=(short)(2*SYM_PER_BLOCK);
      	break;


   	case ANS_RX_TRN:

      	if (receiver->Equ.State == EQU_OFF)
         {
         	//-----If the Equalizer is turned off, activate it-----
      		receiver->Equ.State = EQU_START;
            /*-----Update Equalizer Timing Flag
            		 (in case it was changed since EqualizerInit)-----*/
            receiver->Equ.TimingFlag = receiver->timing.flag;
         }

     		if (receiver->rx_counter++==MOVE_TO_DDE_SIZE)
      	{
        		//-----Move to Decision Directed Equalizer mode-----
#ifdef V32_PRINTING
			printf("ANS-D.D Active %ld\n",SamplesCount);
#endif
			receiver->rx_state = ANS_RX_R2;
            receiver->RDetFlag = FLAG_OFF;
            receiver->RxN = QPSK_2BITS;
            SetRxRate(receiver);
         	receiver->RatesDet.state = RATE_DET_LOOK_FOR_R;
      	}
      	break;


   	case ANS_RX_R2:

      	if(receiver->DetFlag)
      {
#ifdef V32_PRINTING
			printf("ANS-R2 Received %ld\n",SamplesCount);
#endif
      		receiver->rx_state = ANS_RX_E;

            receiver->RDetFlag = FLAG_ON;
      		receiver->RatesDet.state = RATE_DET_LOOK_FOR_E;
      	}
      	break;




   	case ANS_RX_E:

   		if (receiver->DetFlag)
      	{
			receiver->rx_state = ANS_RX_B1;
#ifdef V32_PRINTING
			printf("ANS-E Detected %ld\n",SamplesCount);
#endif
            receiver->RxN = receiver->Rate;
         	receiver->RatesDet.state = RATE_DET_OFF;
			receiver->RetrainFlag= MODEM_RETRAIN_OFF;
         	receiver->rx_counter=0;
#ifndef V32_MODEM_FOR_SNAPFONE
#ifndef V32_MODEM_FOR_FAX			
#ifndef V32_MODEM_FOR_VOICE									
			receiver->RxN = receiver->Rate;
			switch(receiver->RxN)
			{
				case QAM16_3BITS:
					receiver->RxRate = 7200;
					break;
				case QAM32_4BITS:
					receiver->RxRate = 9600;
					break;
				case QAM64_5BITS:
					receiver->RxRate = 12000;
					break;
				case QAM128_6BITS:
					receiver->RxRate = 14400;
					break;
				default:
					receiver->RxRate = MODEM_ERROR;
					break;
			}
#else	//voice
			receiver->RxN = QAM32_4BITS;
			receiver->RxRate = 9600;
#endif
#else	//fax		
			
			if((receiver->Rate)==QAM128_6BITS)
			{
				receiver->RxN = QAM128_6BITS;
				receiver->RxRate = 14400;
			}
			else
			{
				receiver->RxN = QAM32_4BITS;
				receiver->RxRate = 9600;
			}
#endif				
#else			//snapfone			
			receiver->RxN = QAM64_5BITS;
			receiver->RxRate = 12000;				
#endif
			SetRxRate(receiver);
		}
      	break;


   	case ANS_RX_B1:
		if (++receiver->rx_counter == B1_SIZE)
		{
#ifdef V32_PRINTING
			printf("ANS-Receiving Data  %d\n",receiver->RxRate);
#endif
     	    receiver->DataOutFlag = V32_DATA_START;
            receiver->rx_state = ANS_RX_START_DATA;
		}
      	break;

	  case ANS_RX_START_DATA:

		if (receiver->LookForIdleFlag)	
		{
			//receiver->LookForIdleFlag = V32_RESYNC_ENABLED;
			receiver->timing.state = TIMING_REGULAR;
			receiver->rx_state = ANS_RX_DATA;

			receiver->dtct.mode = DTCT_ENERGY;
			receiver->dtct.SpectrumFlag = DTCT_DC;
			receiver->dtct.DetCounter = 0;
			memset(receiver->dtct.filter.SignalDelayLine, 0, sizeof(short)*DTCT_SIGNAL_LEN);
			receiver->dtct.filter.SignalDelayLineOffset=0;			
            memset(receiver->dtct.DelayLine, 0, DTCT_LEN*sizeof(short));
			

			receiver->RetrainRequestCounter = 0;
			receiver->RetrainFlag= MODEM_RETRAIN_OFF;

		}
		break;
				
	  case ANS_RX_DATA:
		if (receiver->RetrainFlag)
		{

#ifdef V32_PRINTING
			printf("ANS-RX RETRAIN %ld\n",SamplesCount);
#endif
			ModemInit(receiver);
			receiver->rx_state = ANS_RX_AA;
			receiver->dtct.mode = DTCT_ENERGY;
			receiver->dtct.SpectrumFlag = DTCT_DC;

			receiver->tx_state = ANS_TX_START_RETRAIN;
		}
 		break;


   	default:

   		break;
		}

}









