#include "v32_int.h"
#include <string.h>



int v32int_modem(MODEM_V32 *V32Modem, unsigned short *tx_data, unsigned short *rx_data, unsigned short *InSmp, unsigned short *OutSmp);
int v32int_get_bits(V32_INTER *v32int, unsigned short *V32OutBits, short bitSum);
void v32int_put_bits(unsigned short *in, unsigned short *out, short words, short rate);
void v14_encode(unsigned char *in, unsigned char *out, int inLen, int outLen, int *pState);
int v14_decode(unsigned char *in, unsigned char *out, int inLen, int *pState);


void v32int_start(V32_INTER *v32int)
{
	MODEM_V32 *V32Modem = &(v32int->V32Modem);

	v32int->txFlag =
	v32int->rxFlag = 
	v32int->V32Modem.LookForIdleFlag = 
	v32int->V32Modem.RetrainFlag	 =	0;
	v32int->startFlag = 1;
	v32int->retrainStarted=0;
	memset(v32int->outBits,0,18);
	v32int->outBitsNum=0;

	// Set Rates...
	V32Modem->MaxRate = QAM32_4BITS;
	V32Modem->MinRate = QAM32_4BITS;

	// Modem Initialization...
	V32Modem->Identity=ANSWER;
	ModemInit(V32Modem);
}




int v32_int(V32_INTER *v32int, unsigned char *TxBits, unsigned char *RxBits, unsigned short *InSmp, unsigned short *OutSmp, int V32DataBytes)
{
	MODEM_V32 *V32Modem = &(v32int->V32Modem);
	unsigned short *tx_data = v32int->tx_data;
	unsigned short *rx_data = v32int->rx_data;
	int outBytesNum;
	
	int bitSum;
						  
	if (V32Modem->RetrainFlag==0)
		v32int->retrainStarted=0;

	if ((V32Modem->RetrainFlag) && (v32int->retrainStarted==0))		
	{
		v32int->retrainStarted=1;
		v32int_start(v32int);			
	}
	
	// Bits Transmitter...
	if (V32Modem->DataInFlag == V32_DATA_START)
	{
		V32Modem->DataInFlag = V32_DATA_ON;
		v32int->txFlag = 1;
	}


	if (v32int->txFlag)
	{
		v32int_put_bits((unsigned short *)TxBits,tx_data,V32DataBytes,V32Modem->TxN);
	}


	bitSum=v32int_modem(V32Modem, tx_data, rx_data, InSmp, OutSmp);
	
	// Bits Receiver...
	if (V32Modem->DataOutFlag == V32_DATA_START)
	{
		V32Modem->DataOutFlag = V32_DATA_ON;
		v32int->rxFlag = 1;
		v32int->rxstate=-1;
	}

	if (v32int->rxFlag)
	{
		if ( (outBytesNum=v32int_get_bits(v32int,(unsigned short *)RxBits,bitSum)) )
		{
			outBytesNum+=v32int_get_bits(v32int,(unsigned short *)(&RxBits[9]),0);
		}
	}

	return outBytesNum;
	
}


int BuildFrame(unsigned short *block, unsigned short *frame, short num, short sum);
int BuildFrame(unsigned short *block, unsigned short *frame, short num, short sum)
//Build frame of received bits (before v14).
{
	int i;
	int k=sum>>4;
	int sr=sum&0xf;
	int sl=16-sr;
	int b=num;
	sum+=num;

	for (i=0;i<(num+15)>>4;i++)
	{
		frame[k++]+=block[i]>>sr;
		b-=sl;
		if (b>0)
			frame[k]=block[i]<<sl;
		b-=sr;
	}
	return sum;
}

int BuildFrame1(unsigned short *block, unsigned short *frame, short num, short sum);
int BuildFrame1(unsigned short *block, unsigned short *frame, short num, short sum)
//Build frame of received data (after v14).
{
	int i;
	int k=sum>>4;
	int sl=sum&0xf;
	int sr=16-sl;
	int b=num;
	sum+=num;

	if (sr==0)
	{ sl=0; sr=16;}
	for (i=0;i<(num+15)>>4;i++)
	{
		frame[k++]+=block[i]<<sl;
		b-=sr;
		if (b>0)
			frame[k]=block[i]>>sr;
		b-=sl;
	}
	return sum;
}


void v32int_put_bits(unsigned short *in, unsigned short *out, short words, short rate)
{
	int txstate=0;

	v14_encode((unsigned char *)in,(unsigned char *)out,16*words,24*rate,&txstate);

}
	
int v32int_modem(MODEM_V32 *V32Modem, unsigned short *tx_data, unsigned short *rx_data, unsigned short *InSmp, unsigned short *OutSmp)
{
	short smpNum, bitNum, bitSum;
	V32Modem->OutSmp = (short *)OutSmp;
	V32Modem->InSmp = (short *)InSmp;
	V32Modem->InSmpNum = 26;
	memset(rx_data,0,10);
	bitNum=ModemReceiver(V32Modem);
	if (V32Modem->DataOutFlag!=V32_DATA_OFF)
		bitSum=BuildFrame(V32Modem->OutBit,rx_data,bitNum,0);
	V32Modem->InBit = tx_data;
	smpNum = ModemTransmitter(V32Modem);


	V32Modem->OutSmp += smpNum;
	V32Modem->InSmp += 26;
	V32Modem->InSmpNum = 27;
	bitNum=ModemReceiver(V32Modem);
	if (V32Modem->DataOutFlag!=V32_DATA_OFF)
		bitSum=BuildFrame(V32Modem->OutBit,rx_data,bitNum,bitSum);
	V32Modem->InBit = tx_data + ((V32Modem->TxN+1)>>1);
	smpNum = ModemTransmitter(V32Modem);


	V32Modem->OutSmp += smpNum;
	V32Modem->InSmp += 27;
	V32Modem->InSmpNum = 27;
	bitNum=ModemReceiver(V32Modem);
	if (V32Modem->DataOutFlag!=V32_DATA_OFF)
		bitSum=BuildFrame(V32Modem->OutBit,rx_data,bitNum,bitSum);

	V32Modem->InBit = tx_data + (((V32Modem->TxN+1)>>1)<<1);
	smpNum = ModemTransmitter(V32Modem);

	return bitSum;
}

int v32int_get_bits(V32_INTER *v32int, unsigned short *outBits, short bitSum)
{
	int rxnum;
	short frames=0;
	unsigned short tempBits[9];
	short i;
	unsigned short *rx_data = v32int->rx_data;
	memset(outBits,0,9/*DATA_WORD_LEN*/);
	memset(tempBits,0,9);


	rxnum=v14_decode((unsigned char *)rx_data,(unsigned char *)tempBits,bitSum,&(v32int->rxstate));

	if (rxnum>0)
		v32int->outBitsNum=BuildFrame1(tempBits,v32int->outBits,rxnum,v32int->outBitsNum);
	
	//find the frame
	
	i=0;
	while (v32int->outBitsNum > 16)
	{
		v32int->outBitsNum-=16;
		outBits[i]=v32int->outBits[i];
		i++;
	}
	v32int->outBits[0]=v32int->outBits[i];
	memset(v32int->outBits+1,0,17);
	frames=i;



	return frames;	

}
	
		






