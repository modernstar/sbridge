#include "viterbi.h"
#include "memory.h"

/*
	void viterbi(VITERBI *v, short *in,unsigned short *out, short SymNum)
	By: Mor Reich
   Date:	7.9.98

   &v - stracture of viterbi.
   *in - Buffer of input points on the constellation.
   *out - output symbols.
   SymNum - number of symbol to be processed.
*/

#ifndef V32_NO_VITERBI
void viterbi(VITERBI *v, short *in, unsigned short *out, short SymNum)
{
	short k;
	long B_Met[4]; // short->long for alignment of 2
   short x,y, MinStat;
	long Free16Words[8], Free8Words[4]; // Local Variables For PathMet)aligned 2)

#ifndef V32_MODEM_FOR_SNAPFONE	
#ifndef V32_MODEM_FOR_VOICE					
   N = v->N;
#endif
#endif
	for (k=0; k<SymNum; k++)
	{
		x = in[2*k];
		y = in[2*k+1];
#ifndef V32_MODEM_FOR_SNAPFONE
#ifndef V32_MODEM_FOR_FAX		
#ifndef V32_MODEM_FOR_VOICE					
		switch (N) {
			case 3:
				BranchMet_72(v, x, y, (short *)B_Met);
				break;
			case 5:
				BranchMet_120(v, x, y, (short *)B_Met);
				break;
			case 6:
				BranchMet_144(v, x, y, (short *)B_Met);
				break;
			case 4:
				BranchMet_96(v, x, y, (short *)B_Met);
				break;
		} // end of switch.
#else	//voice
		BranchMet_96(v, x, y, (short *)B_Met);
#endif
#else	//fax
		if (N==6)
			BranchMet_144(v, x, y, (short *)B_Met);
		else
			BranchMet_96(v, x, y, (short *)B_Met);
#endif

#else	//snapfone
		BranchMet_120(v, x, y, (short *)B_Met);
#endif		
		MinStat = PathMet(v, (short *)B_Met, (short *)Free16Words, (short *)Free8Words);

		out[k] = BackTrak(v, MinStat);
	}

}
#endif

