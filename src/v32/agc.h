
#define DESIRED_ENERGY  0x6000000L//0x8000000L   //(=0.0625=1/16) = 1/2*A^2
#define D_ENERGY_HIGH	(DESIRED_ENERGY>>16)	//the high part of desired energy
#define D_ENERGY_LOW	(DESIRED_ENERGY&0xffff)	//the low part of desired energy
#define AGC_NUM_OF_SMP  64        /* number of samples needed for calculating estimated
                                         energy  */

/* definition of agc states */
#define AGC_INIT        0
#define AGC_COLLECT		1
#define AGC_GAIN		2


typedef struct {
	long EstimatedEnergy;	//the collected energy
	short exp;	//the exponent of the gain
	short mantisa;	//the mantisa of the gain
	short state;
	short counter;
} AGC;



void Agc(AGC *agc,short *In,short *Out,short InLen);

