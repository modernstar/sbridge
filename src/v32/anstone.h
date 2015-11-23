//Number of samples at 9600 smp/sec rate before the phase inversion:
#define ANST_PHASE_INVERSION 4320
//The definition of the answer tone frequency: 2100 Hz sampled at 9600 Hz.
// 2100/9600 = 7/32.
#define ANST_SIN_PERIOD			 31	//the length of the sinus table(starting from 0).
#define ANST_FREQ 				  7   //the jump of the pointer to the table.
#define ANST_SIN_HALF_PERIOD	 16   //the addition to invert the sinus phase.


typedef struct {
	short counter;	//counter to phase inversion.
	short SinIndex;	
	short *SinTable;
} ANS_TONE;

void AnswerTone(ANS_TONE *anst,short *Out,short OutLen);

