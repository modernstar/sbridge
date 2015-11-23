#define   TIMING_START 		0
#define   TIMING_REGULAR  	1
#define   TIMING_IDLE 		2
#define   TIMING_SILENCE 	3
#define   TIMING_NOISE		(TIMING_SILENCE)

#define   TIMING_CONST_A		 31129       // q15(0.95)
#define   TIMING_CONST_B		 31129       // q15(0.95)
#define   TIMING_CONST_B_1		 1638	     // q15(0.05)
#define   TIMING_SINC_FILTER_NUM 32    // Number of sinc filters
#define   TIMING_SELECT_FILTER_SHIFT 26  //=index/2^26---> take 5 upper bits of value (and sign).
#define   TIMING_SELECT_FILTER_ROUND 0x200L
#define   TIMING_SINC_FILTER_LEN	 10  			// The length of each sinc filter
#define   TIMING_DELAY_LEN		 38          // = 19*2 (For Complex Values)
#define   TIMING_HALF_DELAY_LEN 18           // = floor(19/2) * 2
#define	  ONE31					 0x7fffffff  // q31(1)
#define	  TIME_THRESH			 0x6666		 // q15(0.8)
#define   TIMING_CONVERGE_TBL_LEN 7          // Length Of Converge Table
#define   TIM_POWER 3                        // Timing Filters Normalization Value

#define   TIMING_IDLE_THRESHOLD 	  256
#define   TIMING_IDLE_NUM		  	  60
#define   TIMING_REGULAR_THRESHOLD	  640
#define   TIMING_REGULAR_NUM	 	  21
#define   TIMING_IDLE_SHIFT           -7


typedef struct
{
	short Counter;      // Symbols Counter
	short RateShift;    // Pll Rate Shift Value
	long RateRound;     // Pll Rate Rounding(Before Shift)
	long LocRound;      // Pll Phase Rounding(Before Shift)
	short LocShift;     // Pll Phase Shift Value
} V32_TIMING_CONVERGE_TBL;

typedef struct{
	long bm;            // The Phase Value After Timing Estimation
	long loc;           // The Phase Value After Pll
	long rate;          // The Rate Value Of The Pll
	short flag;         // Flag For Skiping Odd Samples
	short push_flag;    // Indicate If Push To Delay Line Is Necessary
	short SymCounter;   // Symbols Counter For Changing Step Values
	short c1n;          // +1200Hz Image Memory
	short c1p;          // +1200Hz Real Memory
	short c2n;          // -1200Hz Image Memory
	short c2p;          // -1200Hz Real Memory
	short *DelayLine;   // Length TIMING_DELAY_LEN(Multiply By 2 For Complex Values)
	short *SincTable;   // Length 2561
	V32_TIMING_CONVERGE_TBL *ConvTbl;
	short ConvTblIndex; // Index To The Active Converge Table
	//short SelFilShift;	// Select Filter Shift(depend on TIMING_SINC_INDEX_INCREMENT)
	short *DelayLinePtr;// Delay Line pointer
	short state;        // Idle, Regular or Start
	short IdleCounter;  // Samples counter to set Idle or Regular states
} TIMING;

void TimingInit(TIMING *timing, short RestartFlag, short identity); 
short Timing(TIMING *timing,short *In, short *Out, short InLen);

