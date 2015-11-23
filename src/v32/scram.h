typedef struct
{
	unsigned long	Register; // The previous 23 bits.
	short mode;				  // mode =0 -> CALL ; mode = 1 -> ANSWER
	
} SCRAMBLER;


void ScramInit(SCRAMBLER *scrm, short identity);
void Scrambler(SCRAMBLER	*scrm, unsigned short *In, unsigned short *Out, short BitNum);
void Descrambler(SCRAMBLER	*scrm, unsigned short *In, unsigned short *Out, short BitNum);

