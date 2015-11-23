
#define CC  3664    //the small constellation unit length = 1/sqrt(8*10)
#define LC 	1784	// Large Constallations Coeff (1200 & 14400).


typedef struct
{
      short N;
	  	short kC;
      short *map;
} MAPPER ;

typedef struct {
      short *Table;
      short Corner;	//The coordinate of the leftmost lower point.
      short StepSize;//The distance between two neighbour points in one row/column;
      short Column;  //number of points in each column;
	  short N; 
} DEMAPPER;


void MapperInit(MAPPER *mapper, short N);
void Mapper(MAPPER *mapper, unsigned short *In, short *Out, short SymNum);
void DemapperInit(DEMAPPER *demapper);
void Demapper(short *In, unsigned short *Out, short SymNum);
void DemapperN(DEMAPPER *demapper, short *in, unsigned short *out, short SymNum);

