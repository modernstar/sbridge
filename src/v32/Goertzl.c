/*******************************************************************************
 * File goertzl.c: Goertzl Algorithm.
 *
 * Includes the subroutine:
 *
 * 1. Goertzel.
 *
 * Writen by: Danny Von Der Walde.
 * Combined at: version 2.0.
 *
 *******************************************************************************/
#include "goertzl.h"
//#ifdef V32_C_SOURCE_INCLUDE

short Goertzel(GRTZL *goertzel,short *In ,short InLen)
/*******************************************************************************
 * Description:
 * Function Goertzel: DFT calculation for one frequency with help
 * of the Goertzl algorithm.
 * The program follows the algorithm DSP, Proakis & Manolakis, p.480.
 *
 * Arguments:
 * GOERZEL *goertzel - Goertzel setting and history.
 * short *In - input samples array.
 * short InLen - length of the input array.
 *
 * Returned value: detection flag.
 *******************************************************************************/
{

   long Acc1, NormTimeEnergy,NormFreqEnergy;
   unsigned long LowTimeEnergy=0L;
   long Vn0;
   short Flag;
   long NormSmp; //Normalized sample.
   short *InPtr;
   short exp_time,exp_freq,NormShift;
   short InShift;
	unsigned long Engy1;
   long Engy2,Engy3,Acc0h,Acc0l;
   unsigned short W0,W1;
   long HighYnReal;
   long HighYnImage;
   long HighFreqEnergy;

   InPtr = In;
   Flag=GRTZL_FLAG_NOT_VALID;
   InShift=7;
   if ((goertzel->SmpBlock>=64)&&(goertzel->SmpBlock<192))
   {
      InShift=6;
   }
   if (goertzel->SmpBlock>=160)
	{
      InShift=5;
   }

	while (InLen > 0)
	{
		//-----Perform the algorithm over the Goertzel block-----
		while ( goertzel->counter < goertzel->SmpBlock )
		{
			if (InLen--)
			{

            Acc0l=((long)((short)goertzel->cos*(unsigned short)(goertzel->Vn1)));
            Acc0h=((long)((short)goertzel->cos * (short)(goertzel->Vn1>>16)));
            Vn0=((long)(Acc0h+(short)(Acc0l>>16)))<<2;

 				NormSmp=(long)((*InPtr)<<InShift);
				InPtr++;
				Vn0+= (long)(NormSmp - goertzel->Vn2);

            Engy1=((unsigned long)((unsigned short)(NormSmp) * (unsigned short)(NormSmp)));
            W0=(short)(Engy1);
            Engy2=((long)((unsigned short)NormSmp*(short)(NormSmp>>16)))<<1;
            Engy2+=Engy1>>16;
            W1=(short)(Engy2);
            Engy3=((long)((short)(NormSmp>>16)*(short)(NormSmp>>16)));
            goertzel->HighTimeEnergy+=(long)((Engy2>>16)+Engy3);
            LowTimeEnergy +=(unsigned long)((long)(W1<<16)+W0);
            if (LowTimeEnergy < (unsigned long)((long)(W1<<16)+W0))
                goertzel->HighTimeEnergy++;
				goertzel->Vn2 = goertzel->Vn1;
				goertzel->Vn1 = Vn0;
     //       Vn[goertzel->counter]=goertzel->Vn2;
				goertzel->counter++;
			} //end if
			else
				break;
		}

		//-----At the end of the Goertzel block calculate the energies-----
		if ( goertzel->counter == goertzel->SmpBlock )
		{

 
            Acc0l=((long)((short)goertzel->cos*(unsigned short)(goertzel->Vn2)));
            Acc0h=((long)((short)goertzel->cos * (short)(goertzel->Vn2>>16)));
            HighYnReal=(long)((Acc0h+(short)(Acc0l>>16))<<1)-Vn0;


            Acc0l=((long)((short)goertzel->sin*(unsigned short)(goertzel->Vn2)));
            Acc0h=((long)((short)goertzel->sin * (short)(goertzel->Vn2>>16)));
            HighYnImage=(long)(Acc0h+(short)(Acc0l>>16))<<1;


			//-----The energy of the seeked frequency-----

            Engy1=((unsigned long)((unsigned short)(HighYnReal) * (unsigned short)(HighYnReal)));
            Engy2=((long)((unsigned short)HighYnReal*(short)(HighYnReal>>16)))<<1;
            Engy2+=Engy1>>16;
            Engy3=((long)((short)(HighYnReal>>16)*(short)(HighYnReal>>16)));
            HighFreqEnergy =(long)((Engy2>>16)+Engy3);


            Engy1=((unsigned long)((unsigned short)(HighYnImage) * (unsigned short)(HighYnImage)));
            Engy2=((long)((unsigned short)HighYnImage*(short)(HighYnImage>>16)))<<1;
            Engy2+=Engy1>>16;
            Engy3=((long)((short)(HighYnImage>>16)*(short)(HighYnImage>>16)));
            HighFreqEnergy +=(long)((Engy2>>16)+Engy3);
			   goertzel->FinalFreqEnergy=HighFreqEnergy;
			//----------Find the norm factor of NormFreqEnergy--------------
         Acc1=1L;	Acc1<<=30;
         exp_freq=0;
         while (Acc1>HighFreqEnergy)
         {
         	Acc1>>=1;
            exp_freq++;
         }
  		   if (exp_freq==31)
         	exp_freq=0;
         NormFreqEnergy=(long)(HighFreqEnergy<<exp_freq);
         NormFreqEnergy>>=16;
         //----------Find the norm factor of goertzel->TimeEnergy--------
         Acc1=1L;	Acc1<<=30;
         exp_time=0;
         while (Acc1>goertzel->HighTimeEnergy)
         {
         	Acc1>>=1;
            exp_time++;
         }
         if (exp_time==31)
         	exp_time=0;
         NormTimeEnergy=(long)(goertzel->HighTimeEnergy << exp_time);
         NormTimeEnergy>>=16;

        	//-----------------Energies normalization-----------------------
         NormShift=(short)(exp_time-exp_freq);
			NormFreqEnergy *= 5L;
			NormTimeEnergy = (long)( (goertzel->threshold)*(short)NormTimeEnergy );

			//-----Check whether frequency energies exceeds the threshold-----
         if (NormShift<0)
				NormFreqEnergy>>=-NormShift;
         else
            NormFreqEnergy<<=NormShift;

			if (NormTimeEnergy < NormFreqEnergy)
				//------The tone was detected-----
				Flag = GRTZL_FLAG_ON;
			else
				Flag = GRTZL_FLAG_OFF;
			//-----Start a new Goertzel block-----
			goertzel->FinalTimeEnergy=goertzel->HighTimeEnergy;
			goertzel->HighTimeEnergy=0L;
			goertzel->Vn1 = goertzel->Vn2 = goertzel->counter = 0;
		}
	}
	return Flag;

}
//#endif
