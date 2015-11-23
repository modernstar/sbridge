#define CALLER 0
#define CALL 0
#define ANSWER 1

#define MODEM_RETRAIN_NUM 3
#define MODEM_RERATE_NUM 3

#define MODEM_RETRAIN_OFF 0
#define MODEM_ASK_FOR_RETRAIN 1
#define MODEM_REACT_FOR_RETRAIN	2

typedef struct
{
	unsigned short *OutBit;
	short *IntMem;
	short *Ext1Mem;
	short Identity;			// calling or answering
	short tx_state;
	short rx_state;
	short tx_mode;
	short tx_segment;
	short tx_counter;
	short rx_counter;
	short ConvEncodeFlag;//To activate the convolution encoding by the transmitter.
	short DetFlag;      //Receiver detected the signal he was looking for.
	short AgcFlag;      //Activate AGC.
	short TimingFlag;   //Activate timing.
	short TxToRxFlag;   //
	short DataOutFlag;
	short DataInFlag;
	short RDetFlag;
	//short WaitDetFlag;//Do not detect when this flag is on.
	short TxN;          //The Rx and Tx might be different at
	short RxN;          //the end of the start up procedure.
	short TxRate;		  //The Transmitting Rate
	short RxRate;		  //The Receiving Rate
	short TxSymNum;	  //Transmitted Symbols Number
	short RxSymNum;     //for testing!!!
	short SymPairs;	  //For transition to CC and AC trx.
	short InBitNum;
	short InSmpNum;
	short MaxRate;
	short MinRate;
	short Rate;
	long Error; 		  //for testing!!!
	short BlockNum;	  //First or Second Half Frame(0/1)
	short GlobalFlag; // Used As AnsRxSm() Flag

	//Non constant buffers: 
	short DownDelayLine[RATE_LEN];
	short UpDelayLine[RATE_LEN];
	short RDelayLine[DCMF_LEN];
	short IDelayLine[DCMF_LEN];
	short FilterSignalDelayLine[DTCT_SIGNAL_LEN];
	short DtctDelayLine[DTCT_LEN];
	short DtctPrevDelayLine[DTCT_LEN];
	short ShapeDelayLine[2*SHAPE_LEN];
	unsigned short EchoCanBitsDelayLine[ECH_BITS_DELAY_LEN];
	short EchoCanDelayLineNear[ECH_SYM_DELAY_LEN];
	short EchoCanDelayLineFar[ECH_SYM_DELAY_LEN];
	long EchoCanCoefNear1[ECHO_NEAR_LEN];
	long EchoCanCoefNear2[ECHO_NEAR_LEN];
	long EchoCanCoefFar1[ECHO_FAR_LEN];
	long EchoCanCoefFar2[ECHO_FAR_LEN];
	short EquCoef[2*EQU_LEN];
	short EquPrevCoef[2*EQU_LEN];
	short EquDelayLine[2*EQU_LEN+4];
	short TimingDelayLine[TIMING_DELAY_LEN];
	short VtrPathMet[8];
	unsigned short VtrCompBuf[2*VTR_DELAYLENGTH];
	short VtrGmin[8*VTR_DELAYLENGTH];

	unsigned short OutputBitsBuffer[5];
	unsigned short InputBitsBuffer[5];



	//transmitter:
	ANS_TONE AnsTone;
	SCRAMBLER scrambler;
	DIFF DiffEnc;
	CONV conv;
	BIT_GEN BitGen;
	MAPPER mapper;
	SHAPE shape;
	RATE DownRate;
	//receiver:
	RATE UpRate;
	FILTER filter;
	GRTZL goertzl;
	DCMF dcmf;
	DTCT dtct;  
	ECHO_CNL EchoCnl;
   MAPPER EchoNearMapper;
   MAPPER EchoFarMapper;
   ECHO_BIT_BUF EchoBitBuf;
	AGC agc;
	TIMING timing;
	EQU Equ;
	DIFF DiffDec;
	SCRAMBLER descrambler;
#ifdef V32_NO_VITERBI
	DEMAPPER demapper;
#else
	VITERBI vtrb;
#endif
	RATES_DET RatesDet;
	V32_RECORDING V32Record;
	unsigned short *RWordTbl;
	short *MaxErrorTbl;
	short *ResyncTbl;
	unsigned short *InBit;
	short *InSmp;
	short *OutSmp;
	short AnsToneCounter;
	short StopResyncCounter;
	short StopNoiseCounter;
	short RetrainRequestCounter;
	long EquPrevFreq;
	long EquPrevPhase;
	long EquPrevError;
//	short *EquPrevCoef;        // Length EQU_L*2 (Complex - Real, Image, ReaL,...)
    short EchoBufDataFlag;
    short TrnBeginFlag;
    short LookForIdleFlag;
	short RetrainFlag;
} MODEM_V32;

short ModemTransmitter(MODEM_V32 *modem);
short ModemReceiver(MODEM_V32 *modem);
void AnsTxSM(MODEM_V32 *transmitter);
void CallTxSM(MODEM_V32 *transmitter);
void AnsRxSM(MODEM_V32 *receiver);
void CallRxSM(MODEM_V32 *receiver);
void SetTxRate(MODEM_V32 *modem);
void SetRxRate(MODEM_V32 *modem);
void ModemInit(MODEM_V32 *modem);


