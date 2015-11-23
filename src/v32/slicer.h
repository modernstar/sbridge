#define ONE_  0x0400	// =1/32=1024

// Definition Of Slicer States (Different Constalations)
#define   TRAIN_START	1  // Two Symbols Are Transmitted In Training
#define   QPSK_2BITS	2
#define   QAM16_3BITS	3
#define   QAM32_4BITS	4
#define   QAM64_5BITS	5
#define   QAM128_6BITS	6

typedef struct
{
	short in_real;          // Input - Real
	short in_imag;          // Input - Image
	short out_real;         // Output Symbol - Real
	short out_imag;         // Output Symbol - Image
	short constellation;    // Constellation Type
	short counter;          // Counter For Training Switch
	short RotateReal;       // Real Rotation Value
	short RotateImag;       // Image Rotation Value
	short DeRotateReal;     // Real DeRotation Value
	short DeRotateImag;     // Image DeRotation Value
	short Edge;             // Value For Extern Points
	short RealShift;        // Normalization Of Real Part In Constellation
	short ImagShift;        // Normalization Of Imag Part In Constellation
	SCRAMBLER scrm;
} SLCR;

void SlicerInit(SLCR *Slcr, short N, short identity);
void Slicer(SLCR *slcr);

