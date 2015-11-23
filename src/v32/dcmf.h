#define DCMF_LEN 73 //  FIR Order = 73

typedef struct
{
	short *SinTable;   //- *Table Of Sin(32)
	short DecimationCounter; //0 or 0xffff: Take out only even samples (0xffff).
	short *RDelayLine; //- *FIR Filter Real Delay Line Buffer(DCMF_LEN)
	short *IDelayLine; //- *FIR Filter Image Delay Line Buffer(DCMF_LEN)
	short *LpfTable;   //- *Table Of Low Pass Filter(DCMF_LEN)
	short SinPtr;      //- Not Used
} DCMF;

short Dcmf(DCMF *dcmf, short *In, short *Out, short InLen);



