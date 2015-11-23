#define ECHO_FAR_LEN		96 	// Far Echo LMS Filter Length
#define ECHO_NEAR_LEN		96 	// Near Echo LMS Filter Length
#define ECH_BITS_BLOCK_NUM 350
#define ECH_BITS_DELAY_LEN		(ECH_BITS_BLOCK_NUM*4)   	// Echo Canceller Bits Delay Line Length
#define ECH_SYM_DELAY_LEN		(ECHO_NEAR_LEN+14)   	// Echo Canceller Symbols Delay Line Length
#define	ECH_MAX_NORM_BITS     2 	// Max Bits For Echo Normalization
#define	ECH_NO_OVERFLOW_BITS  2 	// Number Of Bits Reserved For
                                  	// Echo Coeff Overflow
#define ECH_SHIFT_NEAR		  15  	// Shift And Rounding-Near Echo, Max=15
#define ECH_SHIFT_FAR		  15  	// Shift And Rounding-Far Echo, Max=15

#define ECHO_CANCELLER_OFF	   0
#define ECHO_CANCELLER_SINGLE  1
#define ECHO_CANCELLER_DOUBLE  2

#define ECHO_NEAR_CONVERGE_TBL_LEN 	4 //] Coverge Tables Length=
#define ECHO_FAR_CONVERGE_TBL_LEN 	3 //] Number Of Step,Len Variations


#define ECHO_PUT_NOT_DATA 	0
#define ECHO_NEAR_NOT_DATA  1
#define ECHO_NEAR_DATA 		2
#define ECHO_FAR_DATA 		3

/* structures */
typedef struct
{
	short Counter;      // Symbols Counter
	short CoefStart;    // Coef Start Index
	short CoefLen;      // Coefficients Length
	short StepSize;     // LMS Step Size
} V32_ECH_CONVERGE_TBL;

typedef struct
{
	short StepNorm;     // The Value For Multiplaying The Step
	short ShiftNorm;    // The Shift Norm For Double Talk
} V32_ECH_NORM_TBL;

typedef struct
{
	long *hn1;         	// Echo LMS Filter Phase 1 Length ECHO_NEAR_LEN
	long *hn2;         	// Echo LMS Filter Phase 2 Length ECHO_NEAR_LEN
	long *hf1;         	// Echo LMS Filter Phase 1 Length ECHO_FAR_LEN
	long *hf2;         	// Echo LMS Filter Phase 2 Length ECHO_FAR_LEN
	short DelayNorm;	// Norm Factor Of The Echo Canceller DelayLine
	short StepNorm; 	// Norm Factor Of The Echo Canceller Step
	short ShiftNorm;    // Norm Factor Of The Echo Canceller Coefficients
	short StepNear;     //]
	short *DelayNearPtr;//] Near Echo Parameters
	short DelayNearLen;	//]
	long *hn1Ptr;       //]
	long *hn2Ptr;       //]
	short StepFar;      // ]
	short *DelayFarPtr; // ] Far Echo Parameters
	short DelayFarLen;  // ]
	long *hf1Ptr;       // ]
	long *hf2Ptr;    	// ]
	short *DelayLineNearPtr;// LMS(Cyclic) Delay Line Ptr
	short rtd;         	// Round Trip Delay.
	short NearOff;	    // Offset To Near Echo Delay Line.
	short FarOff;   	// Offset To Far Echo Delay Line.
	short mode;        	// Echo Canceller Mode
	short ConvTblIndexNear; //] Indexes To The Current Converge
	short ConvTblIndexFar;  //] Table Parameters.
	short SymCounter;  	// Symbols Trained By The Echo Canceller Number
	V32_ECH_CONVERGE_TBL *ConvTblNear;
	V32_ECH_CONVERGE_TBL *ConvTblFar;
	V32_ECH_NORM_TBL *NormTbl; // Echo Normalizing Table
	short *DelayLineNear;	// LMS Delay Line Length ECH_DELAY_LEN
	short *DelayLineFar;	// LMS Delay Line Length ECH_DELAY_LEN
	short *DelayLineFarPtr;// LMS(Cyclic) Delay Line Ptr
} ECHO_CNL;

typedef struct
{
	unsigned short *DelayLine;
   short 			NearOffBlock;
   short 			FarOffBlock;
   short 			FarOffSym;
   short 			InIndex;
   short 			OutIndexNear;
   short 			OutIndexFar;
} ECHO_BIT_BUF;

void EchoCanceller(ECHO_CNL *EcCnl,short *In,short *Out,
				   short InLen,short *InSymNear,short *InSymFar);
void EchoOffset(ECHO_CNL *EcCnl, ECHO_BIT_BUF *EchoBitBuf);
void EchoNorm(ECHO_CNL *EcCnl);


