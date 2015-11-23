#define RATE_LEN 27  // RATE_LEN = FIR Order = 2*P+1

typedef struct
{
	short *TablePtr;  // Memory Of The Current Sinc Table Pointer
	short PhaseMem;   // Memory Of The Last Phase
	short *DelayLine; // Delay Line Buffer - Length RATE_LEN
	short *Table;     // *Table Of 6 Sincs - Length 6*RATE_LEN
} RATE;

short rat8t9_6(RATE *Rate, short *In, short *Out, short InLen);
short rat9_6t8(RATE *Rate, short *In, short *Out, short InLen);

