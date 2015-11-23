#define FLAG_ON  1
#define FLAG_OFF 0


//-----detection status-----
#define   DTCT_SUCCESS		1
#define   DTCT_FAILURE		0


//-----Delay line lengths-----
#define    DTCT_LEN        32
#define    DTCT_SMP_NUM	   32
#define    DTCT_SIGNAL_LEN  6 
#define	   DTCT_SIGNAL_HALF_LEN 3	
#define    DTCT_SHIFT       4
#define    DTCT_ROUND       8



//-----averaging factors-----
#define   DTCT_CORR_AVERAGE_WEIGHT			  3

//-----required detection numbers-----
#define  DTCT_ENERGY_NUM 		 3 
#define  DTCT_CORR_NUM    		 4

#define SBAR_DELAY 			9   /* Delay (in samples) between SBar beginning and its detection.
The computed value 9. */

//----- detector modes -----                                  
#define DTCT_OFF				0
#define DTCT_ENERGY 			1
#define DTCT_SILENCE			2
#define DTCT_CORRELATION		3
#define DTCT_RELAXATION			4
#define DTCT_RERATE			    -3

//----- detected spectrum -----
#define DTCT_ALL_SPECTRUM   -5	/*DC and Half Baud*/
#define DTCT_DC		        0
#define DTCT_HALF_BAUD      1




typedef struct{
	long  PrevCorr;//the average correlation between the new sample and sample 4 period before.
	long  LowEnergyThreshold;
	short *DFTFilter1;	//Plus half baud filter.
	short *DFTFilter2;   //Minus half baud filter.
	short *DCFilter;     //DC Filter: for ASM functions!!
	short DetLocation;   //Number of the sample inside the input block where the detection was occured.
	short mode; 
	short DetCounter;	//number of detections.
	short IdleFlag;	//If this flag is on do not make desicions to prevent false alarms.
	short SpectrumFlag;	//Flag that indicates what is the spectrum of the signal to be detected.
	short *PrevDelayLine;	//the previous detector delay line.
	short *DelayLine;	//the detector delay line of samples after echo cancellation.
	FILTER filter;
} DTCT;

short Detectors(DTCT *dtct,short *In,short InLen);
long DetectorsFilter(short *DelayLine, short *Filter, short InLen);
long DetectorsFilterASM(short *DelayLine, short *Filter, short InLen);




