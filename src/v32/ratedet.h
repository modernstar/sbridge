
#define	RATE_FAILURE			0
#define	RATE_SUCCESS		    1

#define	RATE_DET_OFF			0
#define RATE_DET_LOOK_FOR_R		1
#define RATE_DET_LOOK_FOR_E		2

#ifdef V32_NO_VITERBI
#define   SYNC_BITS       0xf111
#define   CORRECT_SYNC    0x0111
#else
#define   SYNC_BITS       0xf191
#define   CORRECT_SYNC    0x0191
#endif

typedef struct{
	unsigned short examined; //The word that is examined each time.
	unsigned short Word; /*The first occurance of the R word
	Word=0 means that zero legal R words were found.*/
	unsigned short rates;//The rate information that is extracted after R word is found.
	short state;
	short skip;//The counter of the skips after R word detection.
	unsigned short E;//The E word that is deduced after R word was found.
} RATES_DET;

void RatesInit(RATES_DET *R);
short RatesDetect(RATES_DET *R, unsigned short *In, short BitNum);
