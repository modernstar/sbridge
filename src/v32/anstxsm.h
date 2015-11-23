
//answer transmitter states:
#define ANS_TX_INIT     0
#define ANS_TX_SILENCE1 1
#define ANS_TX_ANSTONE  2
#define ANS_TX_SILENCE2 3
#define ANS_TX_AC1      4
#define ANS_TX_CA       5
#define ANS_TX_AC2      6
#define ANS_TX_SILENCE3 7
#define ANS_TX_S1       8
#define ANS_TX_SBAR1    9
#define ANS_TX_TRN1     10
#define ANS_TX_R1       11
#define ANS_TX_SILENCE4 12
#define ANS_TX_S2       13
#define ANS_TX_SBAR2    14
#define ANS_TX_TRN2     15
#define ANS_TX_R3       16
#define ANS_TX_E        17
#define ANS_TX_B1       18
#define ANS_TX_DATA     19
#define ANS_TX_START_RETRAIN 20


//----- The lengths of the segments: -----
#if (CODEC_INCLUDE == 1)
#define ANS_TX_SILENCE1_SIZE  	540//(54/4)//=540 //1.8 sec
#define ANSWER_TONE_SIZE      	780//(78/4)//=780 //2.6 sec
#else
#define ANS_TX_SILENCE1_SIZE  	(54/4)
#define ANSWER_TONE_SIZE      	(78/4)
#endif
#define ANS_TX_AC1_SIZE       	16    //128 symbols
#define ANS_TX_SILENCE3_SIZE  	2     //16 symbols
#define S_SIZE                	32    //256 symbols
#define SBAR_SIZE             	2     //16 symbols
#define TRN_BEGIN_SIZE        	32    //256 symbols
#define TRN_MIN_SIZE            160   //1280 symbols
#define TRN2_SIZE               160   //1280 symbols
#define NORM_ECHO_SIZE           (TRN_MIN_SIZE - 20)
#define B1_SIZE		 	         16    //128 symbols


#define ANS_TX_CA_AFTER_DET_SIZE 8    //Overall Symbols Delay = 8 blocks.
/*The internal delay of the modem is 28 symbols,
  in order to obtain the desired delay of 8 blocks
  we need to add delay of 36 symbols (for 8 symbols blocks) or
  68 symbols (for 12 symbols blocks)*/
#define ANS_TX_CA_AFTER_DET_SIZE_BLOCK 0 // Block Symbols Delay =8*4=32
#define ANS_TX_CA_AFTER_DET_SIZE_SMP 0   // The remaining delay in 4800 samples =8 (= 4 symbols).

