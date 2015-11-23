/*******************************************************************************
*	File calrxsm.c: call modem receiver state machine.
*
*	Includes the subroutines:
*
*  1. CallRxSm.
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
#include "caltxsm.h"
#include "anstxsm.h"
#include "calrxsm.h"

long CallError=0;
#ifdef V32_PRINTING
extern long SamplesCount;
#endif
short ModemRxRate;

void CallRxSM(MODEM_V32 *receiver)
{
/*******************************************************************************
*	Description:
*  Function CallRxSm works on integer number of symbols.
*	Some parts of the function are designed to work on blocks of 8 symbols.
*	This function is responsible for:
*  1. Promote the states of the call receiver.
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
   	case CALL_RX_ANSTONE:

      	if(receiver->DetFlag)
      	{
      		receiver->rx_state = CALL_RX_AC1;
         	receiver->dtct.mode = DTCT_ENERGY;
            receiver->dtct.SpectrumFlag = DTCT_HALF_BAUD;
			receiver->dtct.filter.SignalDelayLineOffset=0;
#ifdef V32_PRINTING
			printf("CALL-Ans Tone Detected %ld\n",SamplesCount);
#endif
		}
      	break;


	   case CALL_RX_AC1:
			//to activate this state only after beginning of AA trx. !!!
      	if(receiver->DetFlag)
         {
      		receiver->rx_state = CALL_RX_CA;
            receiver->dtct.mode = DTCT_CORRELATION;
            //-----Prevent call modem to transmit CC too soon-----
         	receiver->DetFlag = FLAG_OFF;
			}
      break;
	   case CALL_RX_CA:
      	if(receiver->DetFlag)
			{
      		receiver->rx_state = CALL_RX_AC2;
#ifdef V32_PRINTING			
			printf("CALL-CA Detected %ld\n",SamplesCount);
#endif
         }
      break;


   	case CALL_RX_AC2:

      	if(receiver->DetFlag)
         {
#ifdef V32_PRINTING
			printf("CALL-AC2 Detected %ld\n",SamplesCount);
#endif
      		receiver->rx_state = CALL_RX_SILENCE;
            receiver->dtct.mode = DTCT_SILENCE;
         }
      	break;


   	case CALL_RX_SILENCE:

      	if(receiver->DetFlag)
      	{
      		receiver->rx_state = CALL_RX_S1;
            receiver->dtct.mode = DTCT_ENERGY;
      		receiver->dtct.SpectrumFlag = DTCT_ALL_SPECTRUM;
            //-----Prevent call modem to transmit S too soon-----
         	receiver->DetFlag = FLAG_OFF;
      	}
      	break;


   	case CALL_RX_S1:

      	if(receiver->DetFlag)
      	{
#ifdef V32_PRINTING
			printf("CALL-S1 Detected %ld\n",SamplesCount);
#endif
			receiver->rx_state = CALL_RX_SBAR1;
            receiver->dtct.mode = DTCT_CORRELATION;
         	receiver->AgcFlag  = FLAG_ON;
            //-----Prevent call modem to transmit S too soon-----
         	receiver->DetFlag = FLAG_OFF;
				receiver->rx_counter = 0;
      	}
      	break;


   	case CALL_RX_SBAR1:

      	//-----Finish agc collection before start timing-----
    		if(++receiver->rx_counter == 2)
       		receiver->TimingFlag = FLAG_ON;
      	if(receiver->DetFlag)
      	{
#ifdef V32_PRINTING
			printf("CALL-Sbar1 Detected %ld\n",SamplesCount);
#endif
			receiver->rx_state = CALL_RX_TRN1;
            receiver->dtct.mode = DTCT_OFF;
            //-----Init Equalizer first time-----
            receiver->Equ.State = EQU_START;
            if (EqualizerInit( &(receiver->Equ), receiver->timing.flag, receiver->dtct.DetLocation))
               //-----If the SBAR signal is not completed yet, turn off the Equalizer-----
               receiver->Equ.State = EQU_OFF;
            receiver->rx_counter = 0;
         	//-----Signal slicer to work in the TRAIN mode-----
            receiver->RxN = TRAIN_START;
         	SetRxRate(receiver);
            //-----Prevent call modem to transmit S too soon-----
         	receiver->DetFlag = FLAG_OFF;
      	}
      	break;


   	case CALL_RX_TRN1:

      	if (receiver->Equ.State == EQU_OFF)
         {
         	//-----If the Equalizer is turned off, activate it-----
#ifdef V32_PRINTING
			printf("CALL-Equ Active %ld\n",SamplesCount);
#endif
			receiver->Equ.State = EQU_START;
            /*-----Update Equalizer Timing Flag
            		 (in case it was changed since EqualizerInit)-----*/
            receiver->Equ.TimingFlag = receiver->timing.flag;
         }
      	if (receiver->rx_counter++==MOVE_TO_DDE_SIZE)
      	{
         	//-----Move to Decision Directed Equalizer mode-----
         	receiver->RxN = QPSK_2BITS;
		   	SetRxRate(receiver);
      		receiver->rx_state = CALL_RX_R1;
         	receiver->RatesDet.state = RATE_DET_LOOK_FOR_R;
      	}
      	break;


   	case CALL_RX_R1:

      	if(receiver->DetFlag)
      	{
#ifdef V32_PRINTING
			printf("CALL-R1 Detected %ld\n",SamplesCount);
#endif
			receiver->rx_state = CALL_RX_WAIT;
         	receiver->Equ.State = EQU_OFF;
         	receiver->TimingFlag = FLAG_OFF;
         	receiver->RatesDet.state = RATE_DET_OFF;
		}
     	 break;


   	case CALL_RX_WAIT:

      	if(receiver->TxToRxFlag)
      	{
         	//-----Start looking for S at the beginning of R2 transmission.-----
      		receiver->rx_state = CALL_RX_S2;
      		receiver->dtct.mode = DTCT_ENERGY;
      	}
      	break;


   	case CALL_RX_S2:

      	if(receiver->DetFlag)
      	{
#ifdef V32_PRINTING
			printf("CALL-S2 Detected %ld\n",SamplesCount);
#endif
			receiver->rx_state = CALL_RX_SBAR2;
            receiver->dtct.mode = DTCT_CORRELATION;
            //-----Prevent call modem to transmit E too soon-----
         	receiver->DetFlag = FLAG_OFF;
         	receiver->TimingFlag = FLAG_ON;
  	         TimingInit(&(receiver->timing), FLAG_ON, CALLER);
      	}
      	break;


   	case CALL_RX_SBAR2:

      	if(receiver->DetFlag)
      	{
#ifdef V32_PRINTING
			printf("CALL-Sbar2 Detected %ld\n",SamplesCount);
#endif
			receiver->rx_state = CALL_RX_TRN2;
            receiver->dtct.mode = DTCT_OFF;
            //-----Init Equalizer for the resynchronization-----
            receiver->Equ.State = EQU_START_RESYNC;
				if (EqualizerInit( &(receiver->Equ), receiver->timing.flag, receiver->dtct.DetLocation))
               //-----If the SBAR signal is not completed yet, turn off the Equalizer-----
               receiver->Equ.State = EQU_OFF;
            receiver->rx_counter = 0;
            //-----Prevent call modem to transmit E too soon-----
         	receiver->DetFlag = FLAG_OFF;
      	}
      	break;


   	case CALL_RX_TRN2:

   		receiver->rx_counter++;

      	if (receiver->Equ.State == EQU_OFF)
         {
            //-----If the Equalizer is turned off, activate it-----
      		receiver->Equ.State = EQU_START_RESYNC;
            /*-----Update Equalizer Timing Flag
            		 (in case it was changed since EqualizerInit)-----*/
            receiver->Equ.TimingFlag = receiver->timing.flag;
         }

    		if (receiver->rx_counter == RESYNC_SIZE)
    		{
      		receiver->rx_state = CALL_RX_R3;
            receiver->RDetFlag = FLAG_OFF;
            //-----Return from the resynchronization mode to regular Equalizer mode-----
         	receiver->Equ.State = EQU_REGULAR;
         	EqualizerInit( &(receiver->Equ),0,0);
            //-----Init Rates Detector for looking for R3-----
      		RatesInit( &(receiver->RatesDet));
      		receiver->RatesDet.state = RATE_DET_LOOK_FOR_R;
      	}
      	break;


   	case CALL_RX_R3:

      	if(receiver->DetFlag)
      	{
#ifdef V32_PRINTING
			printf("CALL-R3 Detected %ld\n",SamplesCount);
#endif
			receiver->rx_state = CALL_RX_E;
         	receiver->RatesDet.state = RATE_DET_LOOK_FOR_E;
            receiver->Rate = receiver->RatesDet.rates;
            //This redundancy is for call initiated rerate:

            receiver->RDetFlag = FLAG_ON;
		}
      	break;


   	case CALL_RX_E:

   		if (receiver->DetFlag)
      	{
#ifdef V32_PRINTING
			printf("CALL-E Detected %ld\n",SamplesCount);
#endif
			receiver->rx_state = CALL_RX_B1;
         	receiver->RatesDet.state = RATE_DET_OFF;
				receiver->rx_state = CALL_RX_B1;
			receiver->rx_counter = 0;
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
#else		//voice
			receiver->RxN = QAM32_4BITS;
			receiver->RxRate = 9600;				
#endif					
#else		//fax
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


   	case CALL_RX_B1: 

		if (++receiver->rx_counter == B1_SIZE)
       	{
#ifdef V32_PRINTING
			printf("CALL-Receiving Data  %d\n",receiver->RxRate);
#endif
			receiver->DataOutFlag = V32_DATA_START;
			receiver->rx_state = CALL_RX_START_DATA;
		}	      	
		break;


	case CALL_RX_START_DATA:
		if (receiver->LookForIdleFlag)
		{
			//receiver->LookForIdleFlag==V32_RESYNC_ENABLED;
			receiver->timing.state = TIMING_REGULAR;
			receiver->rx_state = CALL_RX_DATA;
			receiver->dtct.mode = DTCT_ENERGY;
			receiver->dtct.SpectrumFlag = DTCT_HALF_BAUD;
			receiver->dtct.DetCounter = 0;
			memset(receiver->dtct.filter.SignalDelayLine, 0, DTCT_SIGNAL_LEN*sizeof(short));
			receiver->dtct.filter.SignalDelayLineOffset=0;			
			memset(receiver->dtct.DelayLine, 0, DTCT_LEN*sizeof(short));
			receiver->RetrainRequestCounter = 0;					
			receiver->RetrainFlag= MODEM_RETRAIN_OFF;
		}
		break;
		
	case CALL_RX_DATA:
		if (receiver->RetrainFlag)
		{
			
#ifdef V32_PRINTING
			printf("CALL-RX RETRAIN %ld\n",SamplesCount);
#endif
			ModemInit(receiver);
			receiver->rx_state = CALL_RX_AC1;
			receiver->dtct.mode = DTCT_ENERGY;
			receiver->dtct.SpectrumFlag = DTCT_HALF_BAUD;

			receiver->tx_state = CALL_TX_START_RETRAIN;
		}


		break;


   	default:

   		break;
	}

}

