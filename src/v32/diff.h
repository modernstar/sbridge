typedef struct
{
      unsigned short state;     // previous input/output dibit.
      signed short N;   		  // number of bits per symbol.
      unsigned short *Table;    // differential encoding/decoding table.
} DIFF;

void DiffEncode(DIFF *diff, unsigned short *In, unsigned short *Out, short BitNum);
void DiffDecode(DIFF *diff, unsigned short *In, unsigned short *Out, short BitNum);
void DiffInit(DIFF *diff, const unsigned short *Table, short N);

