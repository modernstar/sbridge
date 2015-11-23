MODEM_V32 v32struct;
short TxSmpNum;
short RxSmpNum;
short RxBitNum;

//The samples buffers are buffers of 8KHz 16-bit linear samples.
v32struct.InSmp points to 30 received samples buffer.
v32struct.OutSmp points to 30 samples to be transmitted buffer.

//Set whether the moddem is caller or answerer:
v32 struct.Identity = CALL or ANSWER

//Set the desired rates limits using the following defines:
//   QAM16_3BITS	-> 7200 bps
//   QAM32_4BITS	-> 9600 bps
//   QAM64_5BITS	-> 12000 bps
//   QAM128_6BITS	-> 14400 bps

v32 struct.MaxRate = ...
v32 struct.MinRate = ...

//Disable the recording:
v32 struct.V32Record.Command.All=0; 

//Call modem initialization function:
ModemInit(&v32 struct);

//Loop that repeats each 10 milliseconds.

//The number of transmitted bits is fixed and depends on the modem rate 
//It is 72,96,120 or 144 bits.
//The number of the received bits may vary due to clock differences between the two sides. 

//Copy the bits to be transmitted (including v14 start and stop bits) into v32struct.InputBitsBuffer.
//Call the transmitter function 
//The returned value is the number of samples to transmit:
TxSmpNum=ModemTransmitter(&v32 struct);

//Transmit the generated samples to the other side.

//Receive samples from the other side. 

//Set the number of the received samples:
v32 struct.InSmpNum = RxSmpNum;
//Call the receiver function. 
//The returned value is the number of the received bits (including start and stop bits):
RxBitNum=ModemReceiver(&v32 struct);

//Copy the received bits from v32struct.OutputBitsBuffer.
//Call v14 decode function to remove start and stop bits.
