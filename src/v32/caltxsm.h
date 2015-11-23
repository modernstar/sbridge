//caller transmitter states:
#define CALL_TX_INIT	  0
#define CALL_TX_SILENCE1  1
#define CALL_TX_AA	      2
#define CALL_TX_CC		  3
#define CALL_TX_SILENCE2  4
#define CALL_TX_S	      5
#define CALL_TX_SBAR	  6
#define CALL_TX_TRN	 	  7
#define CALL_TX_R2        8
#define CALL_TX_E         9
#define CALL_TX_B1       10
#define CALL_TX_DATA     11
#define CALL_TX_START_RETRAIN 12
#define CALL_TX_RERATE	 13


//----- The lengths of the segments: -----
#define WAIT_TO_SYM_DET_AFTER_TX_SIZE 	  13 //number of blocks before activating the detectors after the trx.
#define CALL_TX_SILENCE1_AFTER_DET_SIZE   300//1 sec
#define S_SIZE                			  32 //256 symbols
#define SBAR_SIZE             			   2 //16 symbols 
#define TRN_BEGIN_SIZE        			  32 //256 symbols
#define TRN_MIN_SIZE              	  	  160 //1280 symbols
#define NORM_ECHO_SIZE                   (TRN_MIN_SIZE - 20)
#define CALL_TX_B1_AFTER_DET_SIZE 	      16 //128 symbols

//-----The sizes for RTD calculations: -----
#define CALL_TX_AA_AFTER_DET_SIZE 8   //Overall Symbols Delay = 8 blocks = 64 symbols.
/*The internal delay of the modem is 28 symbols,
  in order to obtain the desired delay of 8 blocks
  we need to add delay of 36 symbols (for 8 symbols blocks) or
  68 symbols (for 12 symbols blocks)*/
#define CALL_TX_AA_AFTER_DET_SIZE_BLOCK 0 // Block Symbols Delay =8*4=32
#define CALL_TX_AA_AFTER_DET_SIZE_SMP 0 // The remaining delay in 4800 samples =8 (= 4 symbols).












