/*
There are 5 modes, which are in the struct MODE:
Bit Mode : TRN R1 R2 R3 E B1
Sym Mode: AC CA AA CC S SBAR
Data Mode: Data B1 (which is in Bit Mode, too)
Answer Tone Mode.
Silence Mode.
*/
//#define V32_C_SOURCE_INCLUDE	1

#define V32_NO_PRINT			0
#define V32_IMPORTANT_PRINT     1
#define V32_RESYNC_PRINT        2
#define V32_PRINT_ALL			3
#define V32_SYS_PRINT			V32_NO_PRINT

#define TX_MODE_SILENCE 		1
#define TX_MODE_ANSWER_TONE 	2
#define TX_MODE_SYM 			3
#define TX_MODE_BIT 			4
#define TX_MODE_DATA 			5

#define TRN_WORD	    0xffff
#define B1_WORD			0xffff

#define AA_WORD 	      0
#define CC_WORD 		 0x0f
#define AC_WORD 	      3
#define CA_WORD 		 0x0c
#define S_WORD  		 0x1
#define SBAR_WORD   	 0x0e

#define TX_SEG_TRN			  1
#define TX_SEG_R              2
#define TX_SEG_B1	            4
#define TX_SEG_B2	            5

#define TX_SEG_AC             6
#define TX_SEG_CA             7
#define TX_SEG_AA             8
#define TX_SEG_CC             9
#define TX_SEG_S              10
#define TX_SEG_SBAR           11


#define MODEM_ERROR		0x7777
typedef struct
{
	short segment;
	short shift;
	unsigned short word;
} BIT_GEN;

void SymGenerator(short mode, unsigned short *Out, short SymNum, short SymPairs);

void BitGenerator(BIT_GEN *BitGen, unsigned short *Out, short OutNum);
