#define SHAPE_LEN 21	//Number of  symbols used in calculation of each sample.

typedef struct{
	short *DelayLine;		//complex symbol buffer.
	short *SinPtr;			//]
	short *CosPtr;			//] indexes to sinus table.
	short *PolyPhaseFilter;	//rise cosine filter.
	short *sin;				//sinus table.
	short DelayLineIndex;	//index to delay line.
} SHAPE;

void Shape(SHAPE *shape, short *In, short *Out, short InLen);
void ShapeInit(SHAPE *shape, const short *ShapeFilter, const short *SinTable);
