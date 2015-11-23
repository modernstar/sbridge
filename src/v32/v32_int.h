#include "mdm_v32.h"


typedef struct
{
	unsigned	StartUp			:1; // Bit 0,Valid If Rec or Trnsmt Data Didn't Start
	unsigned	TrnsmtData		:1; // Bit 1,Valid If Data Is Needed From Next Symbol
	unsigned	ReceiveData		:1; // Bit 2,Valid If Data Is Being Receive By Modem
	unsigned	NoFrame			:1; // Bit 4,Valid If No Frame Is Waiting
	unsigned	TwoFrames		:1; // Bit 5,Valid If Two Frames Are Waiting
	unsigned	HeaderError		:1; // Bit 6,Valid If Error In Header Was Found
	unsigned	OneFrame		:1; // Bit 7,Valid If Two\One Frames Are Waiting
	unsigned	Idle			:1; // Bit 8,Valid If The Modem Is in Idle State (e.g. Waiting Call).
	unsigned	Retrain			:1; // Bit 9,Valid If The Modem Is Retraining.
} STATUS;

typedef struct
{
	unsigned	Connect			:1; // Bit 0
	unsigned	LookForIdle		:1; // Bit 1
	unsigned	EnableRetrain	:1; // Bit 2 if switched on enables resync too.
} COMMAND;

typedef struct
{
	STATUS Status;
	COMMAND Command;
	MODEM_V32 V32Modem;
	int startFlag;
	int txFlag;
	int rxFlag;
	int idleFlag;
	int retrainStarted;
	int rxstate;
	unsigned short tx_data[10];
	unsigned short rx_data[10];
	unsigned short outBits[18];
	int outBitsNum;
} V32_INTER;

void v32int_start(V32_INTER *v32int);
/*
Called once per session.
*/

int v32_int(V32_INTER *v32int, unsigned char *TxBits, unsigned char *RxBits, unsigned short *InSmp, unsigned short *OutSmp, int V32DataBytes);
/*
Called once per 10 msec.
v32int : pointer to structure of the modem with the interface.
TxBits : data to be sent, max 9 bytes.
RxBits : received bits, 
V32DataBytes : Number of byts to be sent.
InSmp : Received 16-bit linear Samples.
OutSmp : Transmitted 16-bit linear Samples.
Return value : number of received bytes.
*/



