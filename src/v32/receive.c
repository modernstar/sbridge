/*******************************************************************************
* File receive.c: the main modem transmitter functions.
*
* Includes the subroutines:
*
* 1. ModemReceiver.
*
* Writen by: Victor Elkonin.
* Combined at: version 2.0.
*
********************************************************************************
*	Changes:
*	version 2.3: 	Changes in function ModemReceiver.
*	version 2.4: 	Changes in function ModemReceiver.
*******************************************************************************/
#include <memory.h>
#include <stdio.h>
#include "v32rec.h"
#include "receive.h"
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
#include "calrxsm.h"
#include "transmt.h"
#include "modem.h"
#include "packing.h"

#ifdef V32_PRINTING
extern long SamplesCount;
#endif



short ModemReceiver(MODEM_V32 *modem)
{
/*******************************************************************************
* Description:
* Function ModemReceiver is the main function of the modem receiver,
* that is called by V14. It calls to all the receier components.
* This function works on integer number of symbols.
* The signaling stage is restricted to 8 symbols per block.
*
* Arguments:
* MODEM_V32 *modem - setting and history of the modem.
*
* Returned value: number of the output bits.
********************************************************************************
*	Changes:
*	1. version 2.3: Correction in copying to TmpSmpBuf1 before calling timing.
*	2. version 2.4: Modem with slicer : call demapper instead of viterbi.
*******************************************************************************/

	short k;
	short Flag;
	short BitNum;
	short SymNum;
	short Smp4800Num;
	short Smp9600Num;
	short TmpSmpBuf[TMP_SMP_BUF_SIZE];
	short TmpSmpBuf1[TMP_SMP_BUF_SIZE];
   short TxSymNear[2*SYM_PER_BLOCK], TxSymFar[2*SYM_PER_BLOCK];
	unsigned short TmpBitBuf[TMP_BIT_BUF_SIZE];
  	unsigned short *DelayLineStart;


	modem->DetFlag = FLAG_OFF;

	Smp9600Num = rat8t9_6( &(modem->UpRate), modem->InSmp, TmpSmpBuf, modem->InSmpNum);
	// Recording...
	if (modem->V32Record.Command.Single.Rx)
		memcpy(modem->V32Record.RxPtr,TmpSmpBuf,Smp9600Num*sizeof(short));
              
	do
	{
		if (modem->Identity==CALL)
      {
			if(modem->rx_state == CALL_RX_ANSTONE)
			{
         		Filter( &(modem->filter),TmpSmpBuf,TmpSmpBuf, Smp9600Num);
				Flag = Goertzel( &(modem->goertzl), TmpSmpBuf, Smp9600Num);
  				if ( (Flag==GRTZL_FLAG_ON) && (modem->goertzl.FinalTimeEnergy>V32_ANS_TONE_ENERGY_THR))
				{
					modem->AnsToneCounter++;
					printf("time energy %ld\n",modem->goertzl.FinalTimeEnergy);
					printf("freq energy %ld\n",modem->goertzl.FinalFreqEnergy);
				}
				else
					modem->AnsToneCounter = 0;
				if ( modem->AnsToneCounter == V32_ANS_TONE_NUM_TO_DETECT )
					 modem->DetFlag = Flag;
				else
					modem->DetFlag = 0;

				BitNum = 0;
				break;
			}
      }

		memcpy(TmpSmpBuf1, TmpSmpBuf, sizeof(short)*Smp9600Num);
		Smp4800Num = Dcmf( &(modem->dcmf), TmpSmpBuf1, TmpSmpBuf, Smp9600Num);

		if (modem->EchoCnl.mode != ECHO_CANCELLER_OFF)
      {
         //----Translate the bits into symbols for the near echo delay line ----
         DelayLineStart = modem->EchoBitBuf.DelayLine + BIT_BUF_BLOCK_SIZE*modem->EchoBitBuf.OutIndexNear;

         if(modem->EchoBufDataFlag <= ECHO_NEAR_NOT_DATA) //the bits of the near echo are not in data rate
         {
         	if(DelayLineStart[0]==0) //silence
         	 	memset(TxSymNear, 0, sizeof(TxSymNear));
            else
            {
            	//choose regular 2bits constellation or TRN_BEGIN one
            	modem->EchoNearMapper.map += 4*(2-DelayLineStart[0]);
           		Mapper( &(modem->EchoNearMapper), DelayLineStart+1, TxSymNear, modem->TxSymNum);
               //restore the map pointer to regular 2bits constellation
            	modem->EchoNearMapper.map -= 4*(2-DelayLineStart[0]);
            }

            //beginning of the data rate near echo:
            if (DelayLineStart[BIT_BUF_BLOCK_SIZE-1]!=0)
            {
            	MapperInit(&(modem->EchoNearMapper),modem->TxN);
               modem->EchoBufDataFlag = ECHO_NEAR_DATA;
            }
         }
         else
				Mapper( &(modem->EchoNearMapper), DelayLineStart, TxSymNear, modem->TxSymNum);

         if (++(modem->EchoBitBuf.OutIndexNear) == ECH_BITS_BLOCK_NUM)
           	modem->EchoBitBuf.OutIndexNear = 0;

         //----Translate the bits into symbols for the far echo delay line ----
         DelayLineStart = modem->EchoBitBuf.DelayLine + BIT_BUF_BLOCK_SIZE*modem->EchoBitBuf.OutIndexFar;

         if(modem->EchoBufDataFlag <= ECHO_NEAR_DATA)
         {
         	if(DelayLineStart[0]==0)
         	 	memset(TxSymFar, 0, sizeof(TxSymFar));
            else
            {
            	modem->EchoFarMapper.map += 4*(2-DelayLineStart[0]);
           		Mapper( &(modem->EchoFarMapper), DelayLineStart+1, TxSymFar, modem->TxSymNum);
            	modem->EchoFarMapper.map -= 4*(2-DelayLineStart[0]);
            }

            if (DelayLineStart[BIT_BUF_BLOCK_SIZE-1]!=0)
            {
            	MapperInit(&(modem->EchoFarMapper),modem->TxN);
               modem->EchoBufDataFlag = ECHO_FAR_DATA;
            }
         }
         else
				Mapper( &(modem->EchoFarMapper), DelayLineStart, TxSymFar, modem->TxSymNum);

         if (++(modem->EchoBitBuf.OutIndexFar) == ECH_BITS_BLOCK_NUM)
           	modem->EchoBitBuf.OutIndexFar = 0;

			//if (modem->V32Record.Command.Single.Global)
			  //	memcpy(modem->V32Record.GlobalPtr,TxSymNear,24*sizeof(short));

		  	EchoCanceller( &(modem->EchoCnl), TmpSmpBuf, TmpSmpBuf, Smp4800Num, TxSymNear, TxSymFar);
      }
		// Recording...
		if (modem->V32Record.Command.Single.Echo)
      {
			//memcpy(modem->V32Record.EchPtr,TmpSmpBuf,Smp4800Num*sizeof(short));
			if (modem->V32Record.Command.Single.EchoCoefN)
	      {
				memcpy(modem->V32Record.EchCoefNPtr,modem->EchoCnl.hn1,ECHO_NEAR_LEN*sizeof(long));
				memcpy((modem->V32Record.EchCoefNPtr+2*ECHO_NEAR_LEN),modem->EchoCnl.hn2,ECHO_NEAR_LEN*sizeof(long));
	      }
			if (modem->V32Record.Command.Single.EchoCoefF)
	      {
				memcpy(modem->V32Record.EchCoefFPtr,modem->EchoCnl.hf1,ECHO_FAR_LEN*sizeof(long));
				memcpy((modem->V32Record.EchCoefFPtr+2*ECHO_FAR_LEN),modem->EchoCnl.hf2,ECHO_FAR_LEN*sizeof(long));
	      }
      }

		if (modem->dtct.mode != DTCT_OFF)
      {
			modem->DetFlag = Detectors( &(modem->dtct), TmpSmpBuf, Smp4800Num);
      }

		if (modem->AgcFlag == FLAG_ON)
			Agc( &(modem->agc), TmpSmpBuf,  TmpSmpBuf, Smp4800Num);

		if (modem->TimingFlag == FLAG_ON)
		{
			memcpy(TmpSmpBuf1, TmpSmpBuf, sizeof(short)*Smp4800Num);
			Smp4800Num = Timing( &(modem->timing),  TmpSmpBuf1, TmpSmpBuf, Smp4800Num);
			// Recording...
			if (modem->V32Record.Command.Single.TimingInfo)
         {
				memcpy(modem->V32Record.TimingInfoPtr,TmpSmpBuf,Smp4800Num*sizeof(short));
				modem->V32Record.TimingInfoPtr[48]=(short)(modem->timing.loc);
				modem->V32Record.TimingInfoPtr[49]=(short)(modem->timing.loc>>16);
				modem->V32Record.TimingInfoPtr[50]=(short)(modem->timing.rate);
				modem->V32Record.TimingInfoPtr[51]=(short)(modem->timing.rate>>16);
				modem->V32Record.TimingInfoPtr[52]=(short)(modem->timing.bm);
				modem->V32Record.TimingInfoPtr[53]=(short)(modem->timing.bm>>16);
         }
		}

  		if (modem->timing.state == TIMING_IDLE)
		{
			if (modem->StopNoiseCounter++ >= V32_RESTART_NOISE_NUM)
			{
				modem->RetrainFlag = MODEM_ASK_FOR_RETRAIN;
			}
			modem->DataOutFlag = V32_DATA_IDLE;
			modem->StopResyncCounter = 0;
			modem->Equ.State = EQU_CONSTELLATION_CHANGE;
			modem->EquPrevPhase = modem->Equ.PllPhaseEst32; //promote EquPrevPhase
		}

		if ((modem->Equ.State != EQU_REGULAR) && (modem->timing.state == TIMING_REGULAR))
		{

			if (modem->StopResyncCounter == V32_STOP_RESYNC_NUM)
			{

				modem->Equ.State = EQU_REGULAR;
				modem->DataOutFlag = V32_DATA_ON;
				modem->StopNoiseCounter = 0;
			}
			modem->StopResyncCounter++;
		}
		if (modem->Equ.State != EQU_OFF)
      {
			short temp;

			if (modem->DetFlag)
			{
				modem->RetrainFlag = MODEM_REACT_FOR_RETRAIN;
				modem->DataOutFlag = V32_DATA_IDLE;
			}
			
			SymNum = Equalizer( &(modem->Equ), Smp4800Num, TmpSmpBuf, TmpSmpBuf);

			temp = (short)(modem->Equ.EquError>>15);
			if ((modem->timing.state == TIMING_REGULAR) || (modem->timing.state == TIMING_NOISE))
			{
				if ((temp > modem->ResyncTbl[modem->RxN-3]) && (modem->StopNoiseCounter < V32_STOP_NOISE_NUM))
				{
					modem->StopNoiseCounter++;
#ifdef V32_PRINTING				
					if (modem->Equ.ConvTblIndex < EQU_CONVERGE_TBL_LEN)
						printf("EQU NOISE %ld\n",SamplesCount);
#endif
					modem->timing.state = TIMING_NOISE;
					modem->Equ.ConvTblIndex = EQU_CONVERGE_TBL_LEN;
					modem->Equ.PllFreqEst32 = modem->EquPrevFreq;
					memcpy(modem->Equ.CoefPtr,modem->EquPrevCoef,2*EQU_LEN*sizeof(short));
					modem->Equ.PllPhaseEst32 =
					modem->EquPrevPhase =
					modem->EquPrevPhase + SYM_PER_BLOCK*modem->EquPrevFreq;
				}
				else
				{


					modem->timing.state = TIMING_REGULAR;
					modem->Equ.ConvTblIndex = EQU_CONVERGE_TBL_LEN-1;
					modem->EquPrevFreq  -= modem->EquPrevFreq/16;
					modem->EquPrevFreq  += modem->Equ.PllFreqEst32/16;
					modem->EquPrevPhase += SYM_PER_BLOCK*modem->EquPrevFreq;
					modem->EquPrevPhase -= modem->EquPrevPhase/16;
					modem->EquPrevPhase += modem->Equ.PllPhaseEst32/16;

					//memcpy(modem->EquPrevCoef,modem->Equ.CoefPtr,2*EQU_LEN*sizeof(short));
					for (k=0;k<2*EQU_LEN;k++)
					{
						modem->EquPrevCoef[k] -= (short)(modem->EquPrevCoef[k]/16);
						modem->EquPrevCoef[k] += (short)(modem->Equ.CoefPtr[k]/16);
					}

					if (modem->StopNoiseCounter >= V32_RESTART_NOISE_NUM)
					{
						if (temp > modem->ResyncTbl[modem->RxN-3]+2)
							modem->RetrainFlag = MODEM_ASK_FOR_RETRAIN;
						else modem->StopNoiseCounter = 0;
					}
					else if (modem->StopNoiseCounter >= V32_STOP_NOISE_NUM)
						modem->StopNoiseCounter++;
					if  (temp <= modem->ResyncTbl[modem->RxN-3])
						modem->StopNoiseCounter = 0;
				}
				//modem->EquPrevPhase = modem->Equ.PllPhaseEst32;
				modem->Equ.PllPhaseEst = (short) ( (modem->Equ.PllPhaseEst32+0x8000) >> 16 );
				
				if (temp > modem->ResyncTbl[modem->RxN-3]+2)
					modem->DataOutFlag = V32_DATA_IDLE;
				else
					modem->DataOutFlag = V32_DATA_ON;
			}
			
			if(modem->LookForIdleFlag != V32_ENABLE_RETRAIN)
				modem->RetrainFlag = MODEM_RETRAIN_OFF;


			// Recording...
			if (modem->V32Record.Command.Single.EquInfo)
	      {
				memcpy(modem->V32Record.EquInfoPtr,TmpSmpBuf,2*SymNum*sizeof(short));
				if (modem->V32Record.Command.Single.EquCoef)
				{
					memcpy(modem->V32Record.EquCoefPtr,modem->Equ.CoefPtr,2*EQU_LEN*sizeof(short));
					memcpy(modem->V32Record.EquCoefPtr+2*EQU_LEN,&(modem->Equ.PllFreqEst32),sizeof(long));
					memcpy(modem->V32Record.EquCoefPtr+2*EQU_LEN+2,&(modem->Equ.PllPhaseEst32),sizeof(long));
				}
				if (modem->V32Record.Command.Single.EquError)
             		memcpy(modem->V32Record.EquErrorPtr,&modem->Equ.EquError,sizeof(long));
	      }
      }
		else
		{
			//-----No bits processing is required at this stage-----
			BitNum = 0;
			break;
		}

		if (modem->RxN == TRAIN_START)
      	//------No bit processing is needed in training-----
			break;

		//-----Bit Processing-----
		BitNum = (short) (modem->RxN * SymNum);

		if (modem->RxN == QPSK_2BITS)
			//-----Signalling-----
			Demapper( TmpSmpBuf, TmpBitBuf, SymNum);
		else
			//-----Data and B1-----
			{
#ifndef V32_NO_VITERBI
				viterbi( &(modem->vtrb), TmpSmpBuf, TmpBitBuf, SymNum);
#else
				DemapperN( &(modem->demapper), TmpSmpBuf, TmpBitBuf, SymNum);
				if (modem->V32Record.Command.Single.Global)
					memcpy(modem->V32Record.GlobalPtr,TmpBitBuf,8*sizeof(short));

#endif
			}

		Packing ( TmpBitBuf, TmpBitBuf, SymNum, modem->RxN);

		DiffDecode( &(modem->DiffDec), TmpBitBuf, TmpBitBuf, SymNum);

		Descrambler( &(modem->descrambler), TmpBitBuf, modem->OutBit, BitNum);

		if (modem->RatesDet.state != RATE_DET_OFF)
      		modem->DetFlag = RatesDetect( &(modem->RatesDet), modem->OutBit, BitNum);
		
		if (modem->RatesDet.state == RATE_DET_LOOK_FOR_E)
		{
			if (!modem->DetFlag)
				modem->EquPrevError = modem->Equ.EquError;
			else
				modem->Equ.EquError = modem->EquPrevError ;
		}

	}
	while (0);	//-----Execute the loop only once(Out with Break...)-----

	modem->RxSymNum = SymNum;

	if (modem->Identity == ANSWER)
		AnsRxSM(modem);
	else
		CallRxSM(modem);

	return BitNum;
}

