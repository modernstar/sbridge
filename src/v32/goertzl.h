#ifndef _GOERTZL
#define _GOERTZL

#define GRTZL_FLAG_OFF			 0
#define GRTZL_FLAG_ON			 1
#define GRTZL_FLAG_NOT_VALID	-1 

typedef struct{
	long HighTimeEnergy;	//The time energy of the samples
	long FinalTimeEnergy;	//The time energy of the samples
	long FinalFreqEnergy;	//The frequency energy of the samples
	long Vn1;
	long Vn2;
	short SmpBlock;	//The length of the Goertzel block.
	short threshold;	//The threshold to check frequency energy relatively to time energy.
	short counter;	//Goertzel block counter
	short cos;	//The sinus that corresponds to the frequency.
	short sin;  //The cosinus that corresponds to the frequency.

} GRTZL;


short Goertzel(GRTZL *goertzel, short *In, short InLen);

#endif //_GOERTZL
