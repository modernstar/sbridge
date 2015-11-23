
#include "viterbi.h"

/* This file holds the tables of the viterbi files */

/*Each point in the constellation is stored in one word:
  real in the high byte and image in the low byte*/
  
const unsigned short V32_Z_72[16]  = {
 	 	0x03FD ,0xFF01 ,
 	 	0xFD03 ,0x01FF ,
		0x0301 ,0xFFFD ,
 	 	0xFDFF ,0x0103 ,
 	 	0xFF03 ,0x03FF ,
 	 	0x01FD ,0xFD01 ,
 	 	0xFDFD ,0x0101 ,
 	 	0x0303 ,0xFFFF
};

const unsigned short V32_Z_96[32]  = {
 	 	0xFC01 ,0x00FD ,0x0001 ,0x0401 ,
 	 	0x04FF ,0x0003 ,0x00FF ,0xFCFF ,
 	 	0xFE03 ,0xFEFF ,0x0203 ,0x02FF ,
 	 	0x02FD ,0x0201 ,0xFEFD ,0xFE01 ,
 	 	0xFDFE ,0x01FE ,0xFD02 ,0x0102 ,
 	 	0x0302 ,0xFF02 ,0x03FE ,0xFFFE ,
 	 	0x0104 ,0xFD00 ,0x0100 ,0x01FC ,
 	 	0xFFFC ,0x0300 ,0xFF00 ,0xFF04 
};


const unsigned short V32_Z_120[64]  = {
 	 	0x0701 ,0x0305 ,0x07F9 ,0xFB05 ,0x03FD ,0xFF01 ,0xFFF9 ,0xFBFD ,
 	 	0xF9FF ,0xFDFB ,0xF907 ,0x05FB ,0xFD03 ,0x01FF ,0x0107 ,0x0503 ,
 	 	0xFF05 ,0xFB01 ,0x0705 ,0xFBF9 ,0x0301 ,0xFFFD ,0x07FD ,0x03F9 ,
 	 	0x01FB ,0x05FF ,0xF9FB ,0x0507 ,0xFDFF ,0x0103 ,0xF903 ,0xFD07 ,
 	 	0xFBFF ,0xFFFB ,0xFB07 ,0x07FB ,0xFF03 ,0x03FF ,0x0307 ,0x0703 ,
 	 	0x0501 ,0x0105 ,0x05F9 ,0xF905 ,0x01FD ,0xFD01 ,0xFDF9 ,0xF9FD ,
 	 	0x01F9 ,0x05FD ,0xF9F9 ,0x0505 ,0xFDFD ,0x0101 ,0xF901 ,0xFD05 ,
 	 	0xFF07 ,0xFB03 ,0x0707 ,0xFBFB ,0x0303 ,0xFFFF ,0x07FF ,0x03FB
};


const unsigned short V32_Z_144[128]  = { 
 	 	0xF8FD ,0x08FD ,0x04FD ,0x04F9 ,0xFCFD ,0xFCF9 ,0x00FD ,0x00F9 ,
 	 	0xF801 ,0x0801 ,0x0401 ,0x0405 ,0xFC01 ,0xFC05 ,0x0001 ,0x0005 ,
 	 	0x0803 ,0xF803 ,0xFC03 ,0xFC07 ,0x0403 ,0x0407 ,0x0003 ,0x0007 ,
 	 	0x08FF ,0xF8FF ,0xFCFF ,0xFCFB ,0x04FF ,0x04FB ,0x00FF ,0x00FB ,
 	 	0x02F7 ,0x0207 ,0x0203 ,0x0603 ,0x02FB ,0x06FB ,0x02FF ,0x06FF ,
 	 	0xFEF7 ,0xFE07 ,0xFE03 ,0xFA03 ,0xFEFB ,0xFAFB ,0xFEFF ,0xFAFF ,
 	 	0xFE09 ,0xFEF9 ,0xFEFD ,0xFAFD ,0xFE05 ,0xFA05 ,0xFE01 ,0xFA01 ,
 	 	0x0209 ,0x02F9 ,0x02FD ,0x06FD ,0x0205 ,0x0605 ,0x0201 ,0x0601 ,
 	 	0x0902 ,0xF902 ,0xFD02 ,0xFD06 ,0x0502 ,0x0506 ,0x0102 ,0x0106 ,
 	 	0x09FE ,0xF9FE ,0xFDFE ,0xFDFA ,0x05FE ,0x05FA ,0x01FE ,0x01FA ,
 	 	0xF7FE ,0x07FE ,0x03FE ,0x03FA ,0xFBFE ,0xFBFA ,0xFFFE ,0xFFFA ,
 	 	0xF702 ,0x0702 ,0x0302 ,0x0306 ,0xFB02 ,0xFB06 ,0xFF02 ,0xFF06 ,
 	 	0xFD08 ,0xFDF8 ,0xFDFC ,0xF9FC ,0xFD04 ,0xF904 ,0xFD00 ,0xF900 ,
 	 	0x0108 ,0x01F8 ,0x01FC ,0x05FC ,0x0104 ,0x0504 ,0x0100 ,0x0500 ,
 	 	0x03F8 ,0x0308 ,0x0304 ,0x0704 ,0x03FC ,0x07FC ,0x0300 ,0x0700 ,
 	 	0xFFF8 ,0xFF08 ,0xFF04 ,0xFB04 ,0xFFFC ,0xFBFC ,0xFF00 ,0xFB00
};



// ** Branch_Met tables **  -------------------------------------------
                                                      // group
   const short V32_Vtr_Shift_TBL96[40] = {-1, 0, 1, 2, -1, 3, 		//  0
   												3, -1, 2, 1, 0, -1,		//  1
                                       1, 0, 3, 2,					//  2
                                       2, 3, 0, 1,					//  3
                                       0, 1, 2, 3, 				//  4
                                       3, 2, 1, 0, 				//  5
                                      -1, 3, 1, 2, -1, 0,		//  6
                                       0, -1, 2, 1, 3, -1};		//  7


 /*                                               // group
   const short Switch_TBL96[16] = {1, 0, 1, 3,			//  0
         		                    3, 1, 0, 1,			//  1
               		              3, 1, 1, 0,        //  6
                     		        0, 1, 1, 3};       //  7
*/

	const short V32_Vtr_Shift_TBL120[96] = {             // Group
		-1,6,7,-1, 2,4,5,3, -1,0,1,-1,      //  0
      -1,1,0,-1, 3,5,4,2, -1,7,6,-1,      //  1
      -1,3,-1, 7,5,1, 6,4,0, -1,2,-1,     //  2
      -1,2,-1, 0,4,6, 1,5,7, -1,3,-1,     //  3
      -1,3,-1, 1,5,7, 0,4,6, -1,2,-1,     //  2
      -1,2,-1, 6,4,0, 7,5,1, -1,3,-1,     //  3
      -1,0,1,-1, 2,4,5,3, -1,6,7,-1,      //  6
      -1,7,6,-1, 3,5,4,2, -1,1,0,-1};     //  7




	const short V32_Vtr_Shift_TBL144[160] = {											  // Group
       -1,0,8,-1, 5,4,12,13, 7,6,14,15, 3,2,10,11, -1,1,9,-1,	//  0
       -1,9,1,-1, 11,10,2,3, 15,14,6,7, 13,12,4,5, -1,8,0,-1,  //  1

       -1,13,15,11,-1, 8,12,14,10,9, 0,4,6,2,1, -1,5,7,3,-1,   //  2
       -1,3,7,5,-1, 1,2,6,4,0, 9,10,14,12,8, -1,11,15,13,-1,   //  3

       -1,11,15,13,-1, 9,10,14,12,8, 1,2,6,4,0, -1,3,7,5,-1,   //  4
       -1,5,7,3,-1, 0,4,6,2,1, 8,12,14,10,9, -1,13,15,11,-1,   //  5

       -1,1,9,-1, 3,2,10,11, 7,6,14,15, 5,4,12,13, -1,0,8,-1,  //  6
       -1,8,0,-1, 13,12,4,5, 15,14,6,7, 11,10,2,3, -1,9,1,-1}; //  7



// ** Back_Track tables ** ---------------------
	/*
   	TBL_1[i]: A first occurance, from the left, of i=0 2 1 3 4 6 5 7,
      in {0 2 0 2 1 3 1 3 4 6 4 6 5 7 5 7}, which is the order of the
      even members (when the count starts from zero) of the Comp_Buffer,
      i.e. the results of the first comparison in the path metric function.
   */
	const short	V32_Vtr_TBL_1[8] = {0,1,4,5,8,9,12,13};
   /*
      TBL_2: Reflects the order of calling to butterfly function,
      from path metric function.
   */
   const short	V32_Vtr_TBL_2[16] = {8,8,10,10,9,9,11,11,10,10,8,8,11,11,9,9};
   /*
   	Transmition_TBL: Gives the transmition from the right butterfly vertex.
      The first 16 entries are the correspond to smaller left vertex,
      and the last 16 entries correspond to the larger left vertexes.
   */
   const short V32_Vtr_Transmission_TBL[32] =   { 0,3,2,1,4,6,5,7,0,3,2,1,6,4,7,5,
												  3,0,1,2,6,4,7,5,3,0,1,2,4,6,5,7};




