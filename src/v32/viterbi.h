
//#define V32_MODEM_FOR_FAX
//9600 and 14400
#define V32_MODEM_FOR_VOICE
//only 9600
//#define V32_MODEM_FOR_SNAPFONE
//only 12000
//#define V32_NO_VITERBI

//#ifdef V32_NO_VITERBI
//#define V32_MODEM_FOR_VOICE
//#endif


#define CC 	3664   // Costalation Coeff  (7200 & 9600).
#define LC 	1784	// Large Constallations Coeff (1200 & 14400).


#define VTR_DELAYLENGTH 20

#define VTR_GP72	2	// # of members in each group 7200 bit/sec
#define VTR_GP96 	4   // 		--		"		--       9600
#define VTR_GP12	8	// 		--		"		--       1200
#define VTR_GP14	16	// 		--		"		--       1400



// --- #define for Branch_Met. ----
// 7200 bit/sec.
#define VTR_MSHIFT72		16	// Must Be Equal To 16(For Assembler)
#define VTR_ROUNDNUM72	0x8000  // if MSHIFT is changed it must be changed.

// 9600 bit/sec
#define VTR_MSHIFT96		14  // Must Be Smaller Then 16(For Assembler)
#define VTR_ROUNDNUM96	0x2000  // if MSHIFT is changed it must be changed.
#define VTR_MAXDIS96	 10000  // if MSHIFT is changed it must ne changed.

// 12000 bit/sec
#define VTR_MSHIFT12		13	// Must Be Smaller Then 16(For Assembler)
#define VTR_ROUNDNUM12  0x1000	// if MSHIFT is changed it must be changed.
#define VTR_MAXDIS12	 10000
#define VTR_SHIFT_NUM12		12	// Number of elements in shift Table for each group.

// 14400 bit/sec
#define VTR_MSHIFT14    12		// Must Be Smaller Then 16(For Assembler)
#define VTR_ROUNDNUM14  0x0800
#define VTR_MAXDIS14    10000
#define VTR_SHIFT_NUM14	20		// Number of elements in shift Table for each group.



typedef struct  {
	short N;
	short *P_Met; 
	unsigned short *Comp_Buffer;
	short *Gmin;
	short p_CompBuf, p_Gmin;
	short *map;
	short *Shift_TBL;
	short *TBL_1;
	short *TBL_2;
	short *Transmission_TBL;
} VITERBI;

void viterbi(VITERBI *v, short *in, unsigned short *out, short SymNum);
void init_viterbi(VITERBI *v, short N);

void BranchMet_72(VITERBI *v, short x, short y, short *BMet);
void BranchMet_96(VITERBI *v, short x, short y, short *BMet);
void BranchMet_120(VITERBI *v, short x, short y, short *BMet);
void BranchMet_144(VITERBI *v, short x, short y, short *BMet);

short PathMet(VITERBI *v, short *BMet, short *temp_Pmet, short *Final_Pmet);
void Butterfly(short M1, short M2, short D1, short D2, unsigned short *TRN, short *M);

unsigned short BackTrak(VITERBI *v, short FinalStat);


