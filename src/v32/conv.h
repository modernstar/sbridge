typedef struct
{
	short state;              // the register state 0..7: s1 s2 s3.
	short N;                  // N = number of bits per symbol
	unsigned short *Table;	  /* convolution encoding table:
	32 entries indexed by Q1 Q2 (input dibit) s1 s2 s3 (old state)
	values in the range 0..63: Y0 Y1 Y2 (encoded tribit) s1 s2 s3 (new state).*/ 
} CONV;



void ConvEncode(CONV *conv, unsigned short *In, unsigned short *Out, short SymNum);
void ConvInit(CONV *conv, const unsigned short *Table, short N);

