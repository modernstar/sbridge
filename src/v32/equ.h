/************************************************************
 *
 * 			  -------   equ.h   -------
 *	Written For equ.c File - Equalizer(),EquInit() Subroutines 
 *
 *     Written By: Guy Ben-Yehuda
 *
 *     Date: 26.10.98
 *
 *************************************************************/

// ************ Definitions **************
#define EQU_LEN 60        // EQU_L = FIR Order = 60*2+4(Complex)
#define CONV_NUM 6        // the width of the convergence table
#define MARKER_DELAY 39   // Delay From Marker Sign including timing delay.
#define ENABLED 1         //
#define DISABLED 0        //
#define SIN_TBL_LEN 128   // 128 = Half Cycle
#define STEP (32768/SIN_TBL_LEN) // Step = 256
#define SIN_TBL_BITS 8    // = log2(SIN_TBL_LEN)+1 = 8
#define EQU_ERROR_PARAM 6
#define EQU_OFF						0	//
#define EQU_START					1   //
#define EQU_REGULAR					2	//
#define EQU_START_RESYNC			3	//
#define EQU_RESYNC					4	//
#define EQU_PUSH					5	//
#define EQU_CONSTELLATION_CHANGE	6	//
#define EQU_CONVERGE_TBL_LEN		8
#define EQU_CONVERGE_RESYNC_TBL_LEN 4
typedef struct
{
	short Retrain;
	short Rerate;
} V32_RENEGOTIATE_NOISE_TBL;

typedef struct
{
	short Counter;      // Symbols Counter
	short CoefStart;    // Coef Start Index
	short CoefLen;      // Coefficients Length
	short StepSize;     // LMS Step Size
	short PllKf;        // Pll Kf(Frequency)
	short PllKp;        // Pll Kp(Phase)
}
V32_EQU_CONVERGE_TBL;

typedef struct
{
	long PllFreqEst32;     // Pll Frequency Estimate
	long PllPhaseEst32;    // Pll Phase Estimate
	long SymCounter;       // Symbols Counter
	short State;           // Equalizer Mode
	short ConvTblIndex;    //
	short MaxIndex;        // The maximium ConvTblIndex
	short *CoefPtr;        // Length EQU_L*2 (Complex - Real, Image, ReaL,...)
	short *DelayLinePtr;   // The Equalizer Cyclic Delay Line - Length EQU_L*2
	V32_EQU_CONVERGE_TBL *ConvTbl;
	short MarkCnt;         // Count Number Of Symbols After Marker
	short PllPhaseEst;     // Pll Phase Convergence Constant
	short *SinTbl;         //
	short TimingFlag;      // Indicates whether the sample is on symbol 
	short *EquTable;       // Equalzer Convergence Table
	short RealError;       // Real part of the error of the received symbol.
	short ImagError;       // Image part of the error of the received symbol.
	short *DelayLine;      // The NOn Cyclic Delay Line Start Address
	long  EquError;		   // The Average Error Of The Equalizer
	SLCR Slcr;		   	   // Slicer
}
EQU;

short EqualizerInit(EQU *Equ, short TimingFlag, short DetLocation);
short Equalizer(EQU *Equ, short InLen, short *In, short *Out);
short EqualizerFilter(EQU *Equ, short InLen, short *In, short *Out,
							 short EqualizerSwitch, short LmsSwitch, short PllSwitch,
                      short PllErrorEstSwitch, short FilterLen, short PllKf,
                      short PllKp, short *CoefStartPtr, short DelayStartOff,
                      short Step);

