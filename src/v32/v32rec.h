typedef struct
{
	unsigned	Tx				:1; // Bit 0 Before Rate Convert[48]
	unsigned	Rx				:1; // Bit 1 After Rate Convert[48]
	unsigned	EchoCoefN	:1; // Bit 2
	unsigned	EchoCoefF	:1; // Bit 3
	unsigned	Echo			:1; // Bit 4 After Echo Canceller[24]
	unsigned	TimingInfo	:1; // Bit 5 After Timing
	unsigned	EquInfo		:1; // Bit 6 After Equalizer
	unsigned	EquCoef		:1; // Bit 7
	unsigned	EquError	:1; // Bit 8
	unsigned	EquPllFreq	:1; // Bit 9
	unsigned	EquPllPhase	:1; // Bit 10
	unsigned	Global		:1; // Bit 12
} V32_REC_SINGLE_COMMAND;

typedef union
{
	unsigned short All;
	V32_REC_SINGLE_COMMAND Single;
} V32_REC_COMMAND;

/* NOTE:
   For dsp_v32 project:
   *2 means that both 5msec frames are recorded,
   *1 means that only the last frame is recorded
 */

typedef struct
{
	V32_REC_COMMAND Command;
	short	*TxPtr; 			// 48 Samples
	short	*RxPtr; 			// 48 Samples
	short	*EchCoefNPtr; 	// 288 longs only when RecEchPtr
	short	*EchCoefFPtr; 	// 288 longs only when RecEchPtr
	short	*EchPtr; 		// 48 Samples
	short	*TimingInfoPtr;// 48+ Samples
	short	*EquInfoPtr;	// 24+ Samples
	short	*EquCoefPtr;	// 120+Samples
	short	*EquErrorPtr;	// 2
	short	*EquPllFreqPtr;	// 2 * 2
	short	*EquPllPhasePtr;// 2 * 2
	short	*GlobalPtr;		// less then 100 samples
} V32_RECORDING;

