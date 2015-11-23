//-----Delay line lengths-----
#define    DTCT_SIGNAL_LEN  6 

typedef struct
{
	short SignalDelayLineOffset;
	short *SignalFilter;
	short *SignalDelayLine;	//the delay line for the filter.
} FILTER;

void Filter(FILTER *filter,short *In,short *Out,short InLen);

 


