#include <memory.h>
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
#include "ratedet.h"
#include "equ.h"
#include "timing.h"
#include "conv.h"
#include "viterbi.h"
#include "transmt.h"
#include "modem.h"
#include "anstxsm.h"
#include "caltxsm.h"
#include "ansrxsm.h"
#include "calrxsm.h"

extern const unsigned short V32_DiffEncTable_48[];
extern const unsigned short V32_DiffDecodTable_48[];
extern const unsigned short V32_DiffDecodTable_D[];
extern const unsigned short V32_DiffEncTable_D[];
extern const short V32_Z_72[];
extern const short V32_Z_96[];
extern const short V32_Z_120[];
extern const short V32_Z_144[];
extern const short V32_Vtr_Shift_TBL96[];
extern const short V32_Vtr_Shift_TBL120[];
extern const short V32_Vtr_Shift_TBL144[];
extern const short V32_Vtr_TBL_1[];
extern const short V32_Vtr_TBL_2[];
extern const short V32_Vtr_Transmission_TBL[];
extern const short V32_MAP_48[];
extern const unsigned short V32_ConvEnc_TBL[];
extern const short V32_ShapeFilter_21_25[];
extern const short V32_ShapeFilter_61_25[];
extern const short V32_SIN[];
extern const short V32_DownTable[];
extern const short V32_UpTable[];
extern const short V32_LpfTable[];
extern const short V32_EquSinTbl[];
extern const short V32_TimingSincTable128[];
extern const short V32_TimingSincTable64[];
extern const short V32_TimingSincTable32[];
extern const short V32_TimingSincTable16[];
extern const V32_EQU_CONVERGE_TBL V32_EquConvergeTbl[];
extern const V32_EQU_CONVERGE_TBL V32_EquConvergeResyncTbl[];
extern const V32_ECH_CONVERGE_TBL V32_EchConvergeTblNear[];
extern const V32_ECH_CONVERGE_TBL V32_EchConvergeTblFar[];
extern const V32_ECH_NORM_TBL V32_EchNormTbl[];
extern const V32_TIMING_CONVERGE_TBL V32_TimingConvergeCallTbl[];
extern const V32_TIMING_CONVERGE_TBL V32_TimingConvergeAnsTbl[];
extern const V32_TIMING_CONVERGE_TBL V32_TimingConvergeResyncTbl[];
extern const short V32_PositiveHalfBaudFilter[];
extern const short V32_NegativeHalfBaudFilter[];
extern const short V32_DCFilter[];
extern const short V32_DCEchoCancellFilter[];
extern const short V32_HalfBaudEchoCancellFilter[];
extern const V32_RENEGOTIATE_NOISE_TBL V32_RenegotiateNoiseTbl[];
extern const unsigned short V32_R_WordTbl[];
extern const short V32_MaxErrorTbl[];
extern const short V32_ResyncTbl[];
extern const short V32_MAP_96[];
extern const short DeMap96_TBL[];

void ModemInit(MODEM_V32 *modem)
{
   modem->GlobalFlag=modem->AnsToneCounter=0;
   modem->TxSymNum = SYM_PER_BLOCK;
   modem->BitGen.shift = 0;
	if (modem->Identity == CALLER)
	{
		modem->tx_state = CALL_TX_INIT;
		modem->rx_state = CALL_RX_ANSTONE;
	}
	else
	{
		modem->tx_state = ANS_TX_INIT;
		modem->rx_state = ANS_RX_START;
	}
	modem->DetFlag = FLAG_OFF;
   modem->DataOutFlag = FLAG_OFF;
   modem->DataInFlag = FLAG_OFF;
   modem->ConvEncodeFlag = FLAG_OFF;
	modem->LookForIdleFlag = FLAG_OFF;
	modem->InBit = modem->InputBitsBuffer;
	modem->OutBit = modem->OutputBitsBuffer;
	ScramInit( &(modem->scrambler), modem->Identity);
	ScramInit( &(modem->descrambler), (short)(!modem->Identity));
   modem->TxN = modem->RxN = QPSK_2BITS;
	SetTxRate(modem);
	MapperInit( &(modem->mapper), QPSK_2BITS);
	SetRxRate(modem);
	RatesInit( &(modem->RatesDet));
   modem->RWordTbl = (unsigned short *)V32_R_WordTbl;
   modem->MaxErrorTbl = (short *)V32_MaxErrorTbl;
   modem->ResyncTbl = (short *)V32_ResyncTbl;
   modem->RDetFlag = FLAG_OFF;
   modem->EchoBufDataFlag=ECHO_PUT_NOT_DATA;
   modem->TrnBeginFlag=0;


	//init Shape
	modem->shape.DelayLine = modem->ShapeDelayLine;
	ShapeInit( &(modem->shape), V32_ShapeFilter_21_25, V32_SIN);

	//init Timing
	(modem->timing).DelayLine = modem->TimingDelayLine;  
	TimingInit(&(modem->timing), FLAG_OFF, modem->Identity);
	modem->TimingFlag = FLAG_OFF;

	//init Equalizer
	modem->Equ.State = EQU_OFF;
	memset(modem->EquPrevCoef,0,2*EQU_LEN*sizeof(short));
	(modem->Equ).CoefPtr = modem->EquCoef;
	(modem->Equ).DelayLine = (modem->Equ).DelayLinePtr = modem->EquDelayLine; 


	//init DownRate
	modem->DownRate.PhaseMem = 0;
	modem->DownRate.DelayLine = modem->DownDelayLine;
	modem->DownRate.Table = (short *)V32_DownTable;
	memset(modem->DownRate.DelayLine,0,sizeof(short)*RATE_LEN);

	//init AnswerTone
	modem->AnsTone.counter = ANST_PHASE_INVERSION;
	modem->AnsTone.SinIndex = 0;
	modem->AnsTone.SinTable = (short *)V32_SIN;

	//init UpRate
	modem->UpRate.PhaseMem = 0;
	modem->UpRate.DelayLine = modem->UpDelayLine;
	modem->UpRate.Table = (short *)V32_UpTable;
	memset(modem->UpRate.DelayLine,0,sizeof(short)*RATE_LEN);

	//init Dcmf
	modem->dcmf.LpfTable = (short *)V32_LpfTable;
	modem->dcmf.SinTable = (short *)V32_SIN;
	modem->dcmf.RDelayLine = modem->RDelayLine;
	modem->dcmf.IDelayLine = modem->IDelayLine;
	modem->dcmf.SinPtr = 0;
	modem->dcmf.DecimationCounter = 0;
  	memset(modem->dcmf.RDelayLine,0,sizeof(short)*DCMF_LEN);
	memset(modem->dcmf.IDelayLine,0,sizeof(short)*DCMF_LEN);


	//init Detectors
	modem->dtct.DFTFilter1 = (short *)V32_PositiveHalfBaudFilter;
	modem->dtct.DFTFilter2 = (short *)V32_NegativeHalfBaudFilter;
	modem->dtct.DCFilter = (short *)V32_DCFilter;
   if (modem->Identity==CALL)
	   modem->dtct.filter.SignalFilter = (short *)(V32_DCEchoCancellFilter);
   else
   	modem->dtct.filter.SignalFilter = (short *)(V32_HalfBaudEchoCancellFilter);
   modem->dtct.filter.SignalDelayLine = modem->FilterSignalDelayLine;
   memset(modem->dtct.filter.SignalDelayLine, 0, DTCT_SIGNAL_LEN*sizeof(short));
   modem->dtct.DelayLine = modem->DtctDelayLine;
   memset(modem->dtct.DelayLine, 0, DTCT_LEN*sizeof(short));
   modem->dtct.PrevDelayLine = modem->DtctPrevDelayLine;
   modem->dtct.IdleFlag = FLAG_OFF;
   modem->dtct.DetCounter = 0;
   modem->dtct.mode = DTCT_OFF;


	//init EchoCanceller
   modem->EchoCnl.NormTbl=(V32_ECH_NORM_TBL*)V32_EchNormTbl;
   modem->EchoCnl.DelayNorm=0;
   modem->EchoCnl.StepNorm=V32_EchNormTbl[modem->EchoCnl.DelayNorm].StepNorm;
   modem->EchoCnl.ShiftNorm=V32_EchNormTbl[modem->EchoCnl.DelayNorm].ShiftNorm;
	modem->EchoCnl.mode = ECHO_CANCELLER_OFF;
   modem->EchoBitBuf.DelayLine = modem->EchoCanBitsDelayLine;
	modem->EchoCnl.DelayLineNear = modem->EchoCanDelayLineNear;
	modem->EchoCnl.DelayLineFar  = modem->EchoCanDelayLineFar;
	modem->EchoCnl.hn1 = modem->EchoCanCoefNear1;
	modem->EchoCnl.hn2 = modem->EchoCanCoefNear2;
	modem->EchoCnl.hf1 = modem->EchoCanCoefFar1;
	modem->EchoCnl.hf2 = modem->EchoCanCoefFar2;
	modem->EchoCnl.rtd = modem->EchoCnl.NearOff = modem->EchoCnl.FarOff = 0;
   memset(modem->EchoBitBuf.DelayLine,0,ECH_BITS_DELAY_LEN*sizeof(short));
	memset(modem->EchoCnl.DelayLineNear,0,ECH_SYM_DELAY_LEN*sizeof(short));
	memset(modem->EchoCnl.DelayLineFar,0,ECH_SYM_DELAY_LEN*sizeof(short));
	memset(modem->EchoCnl.hn1,0,ECHO_NEAR_LEN*sizeof(long));
	memset(modem->EchoCnl.hn2,0,ECHO_NEAR_LEN*sizeof(long));
	memset(modem->EchoCnl.hf1,0,ECHO_FAR_LEN*sizeof(long));
	memset(modem->EchoCnl.hf2,0,ECHO_FAR_LEN*sizeof(long));
   modem->EchoBitBuf.InIndex = 0;
	modem->EchoCnl.SymCounter = 1;
	modem->EchoCnl.ConvTblIndexNear = modem->EchoCnl.ConvTblIndexFar = 0;
	modem->EchoCnl.ConvTblNear = (V32_ECH_CONVERGE_TBL*)V32_EchConvergeTblNear;
	modem->EchoCnl.ConvTblFar = (V32_ECH_CONVERGE_TBL*)V32_EchConvergeTblFar;
	MapperInit( &(modem->EchoNearMapper), QPSK_2BITS);
	MapperInit( &(modem->EchoFarMapper), QPSK_2BITS);


	//init agc
	modem->AgcFlag  = FLAG_OFF;
	modem->agc.state = AGC_INIT;

	//init Goertzl
	modem->goertzl.FinalTimeEnergy=modem->goertzl.HighTimeEnergy=0L;
	modem->goertzl.counter=0;
	modem->goertzl.Vn1=0L;
	modem->goertzl.Vn2=0L;
	modem->goertzl.SmpBlock = 32;
	modem->goertzl.cos = 0x18f9; //cos(2*pi*2100/9600)
	modem->goertzl.sin = 0x7d89; //sin(2*pi*2100/9600)
	modem->goertzl.threshold = 56;    //the freq. energy is 70% of the time energy. 56=70/10*8
   modem->filter.SignalDelayLine = modem->FilterSignalDelayLine;
   memset(modem->filter.SignalDelayLine, 0, DTCT_SIGNAL_LEN*sizeof(short));
   modem->filter.SignalFilter = (short *)(V32_DCEchoCancellFilter);

	//init Renegotiation

}


void SetTxRate(MODEM_V32 *modem)
{
	short N;
   N = modem->TxN;
	if (N==QPSK_2BITS) //signaling: 4800 bps.
		DiffInit( &(modem->DiffEnc), V32_DiffEncTable_48, N);
	//mapper must be initiated separatly because of the first 256 signals.
	else //data: 7200, 9600, 12000, 14400 bps.
	{
#ifndef V32_NO_VITERBI
		DiffInit(  &(modem->DiffEnc), V32_DiffEncTable_D, N);
		ConvInit(  &(modem->conv), V32_ConvEnc_TBL, N);
#else
		DiffInit( &(modem->DiffEnc), V32_DiffEncTable_48, N);
#endif
		MapperInit( &(modem->mapper), N);
	}
}


void SetRxRate(MODEM_V32 *modem)
{
	short N;
   N = modem->RxN;
	SlicerInit(&(modem->Equ.Slcr), N, modem->Identity);
	if (N==TRAIN_START)
		return;
	else if (N==QPSK_2BITS) //signaling: 4800 bps.
	{
		DiffInit( &(modem->DiffDec), V32_DiffDecodTable_48, N);
	}
	else //data: 7200, 9600, 12000, 14400 bps.
	{
#ifndef V32_NO_VITERBI
		DiffInit( &(modem->DiffDec), V32_DiffDecodTable_D, N);
		(modem->vtrb).P_Met = modem->VtrPathMet;
		(modem->vtrb).Comp_Buffer = modem->VtrCompBuf;
		(modem->vtrb).Gmin = modem->VtrGmin;

		init_viterbi( &(modem->vtrb), N);
#else
		DiffInit( &(modem->DiffDec), V32_DiffDecodTable_48, N);
		DemapperInit(&modem->demapper);
#endif

	}

}

void MapperInit(MAPPER *mapper, short N)
{
	mapper->N = N;
	switch (N) {
		case TRAIN_START:
			mapper->map = (short *)(V32_MAP_48+4);
			mapper->N = QPSK_2BITS;
			mapper->kC=CC;
			break;

		case QPSK_2BITS:
			mapper->map = (short *)V32_MAP_48;
			mapper->kC=CC;
			break;

		case QAM16_3BITS:
			mapper->map = (short *)V32_Z_72;
			mapper->kC=CC;
			break;

		case QAM32_4BITS:
#ifndef V32_NO_VITERBI
			mapper->map = (short *)V32_Z_96;
#else
			mapper->map = (short *)V32_MAP_96;
#endif
			mapper->kC=CC;
			break;

		case QAM64_5BITS:
			mapper->map = (short *)V32_Z_120;
			mapper->kC=LC;
			break;

		case QAM128_6BITS:
			mapper->map = (short *)V32_Z_144;
			mapper->kC=LC;
			break;

		default:
			break;

	}
}

#ifndef V32_NO_VITERBI
void init_viterbi(VITERBI *viterbi, short N)
{

	viterbi->TBL_1 = (short *)V32_Vtr_TBL_1;
	viterbi->TBL_2 = (short *)V32_Vtr_TBL_2;
	viterbi->Transmission_TBL = (short *) V32_Vtr_Transmission_TBL;

	memset(viterbi->P_Met,0,8*sizeof(short));
	memset(viterbi->Comp_Buffer,0,2*VTR_DELAYLENGTH*sizeof(short));
	memset(viterbi->Gmin,0,8*VTR_DELAYLENGTH*sizeof(short));
	viterbi->p_Gmin = 0;
	viterbi->p_CompBuf = 0;

	viterbi->N = N;
	switch (N) {
		case QAM16_3BITS:
			viterbi->map = (short *)V32_Z_72;
			break;

		case QAM32_4BITS:
			viterbi->map = (short *)V32_Z_96;
			viterbi->Shift_TBL = (short *)V32_Vtr_Shift_TBL96;
			break;

		case QAM64_5BITS:
			viterbi->map = (short *)V32_Z_120;
			viterbi->Shift_TBL = (short *)V32_Vtr_Shift_TBL120;
			break;

		case QAM128_6BITS:
			viterbi->map = (short *)V32_Z_144;
			viterbi->Shift_TBL = (short *)V32_Vtr_Shift_TBL144;
			break;

		default:
			break;
	}

}
#endif

void TimingInit(TIMING *timing, short RestartFlag, short identity)
{
	timing->bm = timing->loc = 0L;
	timing->c1n = timing->c1p = timing->c2n = timing->c2p = 0;
	timing->flag = 0;
	timing->push_flag = 1;
	timing->SymCounter = timing->ConvTblIndex = 0;
	timing->IdleCounter = 0;
	timing->state = TIMING_START;

	if (RestartFlag == FLAG_OFF)
	{
#if TIMING_SINC_FILTER_NUM==128
		timing->SincTable = (short *)V32_TimingSincTable128;
#elif TIMING_SINC_FILTER_NUM==64
        timing->SincTable = (short *)V32_TimingSincTable64;
#elif TIMING_SINC_FILTER_NUM==32
		timing->SincTable = (short *)V32_TimingSincTable32;
#elif TIMING_SINC_FILTER_NUM==16
		timing->SincTable = (short *)V32_TimingSincTable16;
#endif
		timing->rate = 0L;
		if (identity==CALLER)
			timing->ConvTbl = (V32_TIMING_CONVERGE_TBL *)V32_TimingConvergeCallTbl;
		else
			timing->ConvTbl = (V32_TIMING_CONVERGE_TBL *)V32_TimingConvergeAnsTbl;
	}
	else
		timing->ConvTbl = (V32_TIMING_CONVERGE_TBL *)V32_TimingConvergeResyncTbl;

	memset(timing->DelayLine, 0, TIMING_DELAY_LEN*sizeof(short));
}

short EqualizerInit(EQU *Equ, short TimingFlag, short DetLocation)
{
	short offset;
	switch (Equ->State)
	{
		case EQU_START: //before the first call to equalizer.
			Equ->SinTbl = (short *)V32_EquSinTbl;
			Equ->ConvTbl = (V32_EQU_CONVERGE_TBL *)V32_EquConvergeTbl;
			// Clear Equalizer Fir Filter Start Coefficients
			memset(Equ->CoefPtr,0,2*EQU_LEN*sizeof(short));
			Equ->PllFreqEst32 = 0L;
			Equ->MaxIndex = EQU_CONVERGE_TBL_LEN - 1;
			// Clear Equalizer Delay Line
			memset(Equ->DelayLine,0,(4+2*EQU_LEN)*sizeof(short));
			// First Tree Of Convergence Table
			Equ->ConvTblIndex = 0;
			Equ->SymCounter = 1;
			// Reset Registers
			Equ->PllPhaseEst32 = 0L;
			Equ->PllPhaseEst = 0;
         Equ->EquError= 0L;
			break;

		case EQU_START_RESYNC: //before the beginning of resync in call modem.
			Equ->ConvTbl = (V32_EQU_CONVERGE_TBL *)V32_EquConvergeResyncTbl;
			Equ->MaxIndex = EQU_CONVERGE_RESYNC_TBL_LEN - 1;
			// Clear Equalizer Delay Line
			memset(Equ->DelayLine,0,(2*EQU_LEN+4)*sizeof(short));
			// First Tree Of Convergence Table
			Equ->ConvTblIndex = 0;
			Equ->SymCounter = 1;
			// Reset Registers
			Equ->PllPhaseEst32 = 0L;
			Equ->PllPhaseEst = 0;
         Equ->EquError= 0L;
			break;

		case EQU_REGULAR: //at the end of the resync before LMS restart.
			Equ->ConvTbl = (V32_EQU_CONVERGE_TBL *)V32_EquConvergeTbl;
			Equ->MaxIndex = EQU_CONVERGE_TBL_LEN - 1;
			Equ->ConvTblIndex = 3;
			Equ->SymCounter = 1200;
			return FLAG_OFF;//dummy return

		default:
			break;
	}

	Equ->TimingFlag = TimingFlag;

	Equ->MarkCnt =   (short)(MARKER_DELAY);// - (SMP_BLOCK_SIZE>>1));
	offset =   (short)(DetLocation - SBAR_DELAY);
	Equ->MarkCnt += offset;  

	if (offset<0)
	{
   	Equ->MarkCnt +=  (short)(2*SYM_PER_BLOCK);
		return FLAG_OFF;
	}
	else
		return FLAG_ON;
}


#ifdef V32_NO_VITERBI
void DemapperInit(DEMAPPER *demapper)
{
//	demapper->N = N;

	demapper->Table = (short *)DeMap96_TBL;
	demapper->Corner = -2*CC;
	demapper->Column = 4;
	demapper->StepSize = 2*CC;
}
#endif