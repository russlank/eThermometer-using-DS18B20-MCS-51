#include <reg51.h>
//#include <intrins.h>

#define DISPCLR	0x01
#define FUNCSET	0x38
#define ENTRMOD	0x06
#define DISPON		0x0E

#define SEARCHROM	0xF0
#define READROM	0x33
#define MATCHROM	0x55
#define SKIPROM	0xCC
#define ALRMSRCH	0xEC

#define CONVTEMP	0x44
#define WRITEPAD	0x4E
#define READPAD	0xBE
#define COPYPAD	0x48
#define RECALLEE	0xB8
#define RDPWSUP	0xB4


#define RS			P3_0
#define EN			P3_1
#define RW			P3_2
#define DQ			P3_3

#define LCDData	P1
#define LCDCtrl	P3

unsigned char temp_intc;
unsigned int  temp_frac;


void write_inst(unsigned char value){
	unsigned int i;

	LCDCtrl &= 0xF8;
	LCDCtrl |= 0x04;
	LCDData = value;
	LCDCtrl &= 0xF8;
	for (i = 0; i < 300; i++);
}

void write_data(unsigned char value){
	unsigned char i;

	LCDCtrl |= 0x01;
	LCDCtrl |= 0x04;
	LCDData = value;
	LCDCtrl &= 0xF8;
	for (i = 0; i < 50; i++);
}

void out2lcd(unsigned char value){
	write_data(value);
}

void init_lcd(){
	write_inst(0x30);
	write_inst(0x30);
	write_inst(0x30);
	write_inst(0x04);
	write_inst(0x0C);
	write_inst(0x01);
}

// Temperature Sensor Functions
void delay(int useconds){
	int s;
	for (s = 0; s < useconds; s++);
}
unsigned char ow_reset(void) {
	unsigned char presence;
	DQ = 0;						//pull DQ line low
	delay(29);					// leave it low for 480us
	DQ = 1;						// allow line to return high
	delay(3);					// wait for presence
	presence = DQ;				// get presence signal
	delay(25);					// wait for end of timeslot
	return(presence);			// presence signal returned
}									// 0=presence, 1 = no part

void write_bit(char bitval){
	DQ = 0;					// pull DQ low to start timeslot
	if (bitval == 1) DQ = 1; // return DQ high if write 1
	delay(5);				// hold value for remainder of timeslot
	DQ = 1;
}								// Delay provides 16us per loop, plus 24us. Therefore delay(5) = 104us
unsigned char read_bit(void){
	unsigned char i;
	DQ = 0;					// pull DQ low to start timeslot
	DQ = 1;					// then return high
	for (i = 0; i < 3; i++); // delay 15us from start of timeslot
	return(DQ);				// return value of DQ line
}
void write_byte(char val){
	unsigned char i;
	unsigned char temp;
	for (i = 0; i < 8; i++)	// writes byte, one bit at a time
	{
		temp = val >> i;		// shifts val right 'i' spaces
		temp &= 0x01;		// copy that bit to temp
		write_bit(temp);	// write bit in temp into
	}
	delay(5);
}
unsigned char read_byte(void){
	unsigned char i;
	unsigned char value = 0;
	for (i = 0; i < 8; i++)
	{
		if (read_bit()) value |= 0x01 << i;	// reads byte in, one byte at a time and then
		// shifts it left
		delay(6);								// wait for rest of timeslot
	}
	return(value);
}

void Read_Temperature(void){
	unsigned char get[2];
	unsigned char temp_lsb, temp_msb;
	int k;
	//char tmp;

	ow_reset();
	write_byte(0xCC);					//Skip ROM
	write_byte(0x44);					// Start Conversion
	delay(1000);

	ow_reset();
	write_byte(0xCC);					// Skip ROM
	write_byte(0xBE);					// Read Scratch Pad
	for (k = 0; k < 2; k++){
		get[k] = read_byte();
	}

	temp_msb = get[1];				// Sign byte + lsbit
	temp_lsb = get[0];				// Temp data plus lsb

	temp_frac = ((temp_lsb & 0x0F) * 625) / 1000;
	//temp_frac *= 25;

	temp_intc = (temp_lsb & 0xF0) >> 4;
	temp_intc |= (temp_msb & 0x07) << 4;

	write_inst(0x80);

	out2lcd(0x30 + (temp_intc / 10));
	out2lcd(0x30 + (temp_intc % 10));

	out2lcd('.');

	out2lcd(0x30 + temp_frac);
	//out2lcd(0x30+(temp_frac%10));


	out2lcd(' ');
	out2lcd('C');

	/*
		if (temp_msb <= 0x80){
		temp_lsb = (temp_lsb/2);
		}										// shift to get whole degree

		temp_msb = temp_msb & 0x80;	// mask all but the sign bit

		if (temp_msb >= 0x80) {
		temp_lsb = (~temp_lsb)+1;
		}										// twos complement
		if (temp_msb >= 0x80) {
		temp_lsb = (temp_lsb/2);
		}										// shift to get whole degree
		if (temp_msb >= 0x80) {
		temp_lsb = ((-1)*temp_lsb);
		}										// add sign bit
		printf( "\nTempC= %d degrees C\n", (int)temp_lsb ); // print temp. C
		temp_c = temp_lsb; // ready for conversion to Fahrenheit
		*/
}

void main(void){

	register unsigned int  i;
	register unsigned char j;

	LCDCtrl = 0xf8;
	init_lcd();

	for (j = 0; j < 2; j++){
		for (i = 0; i < 50000; i++);
	}

	while (1){
		Read_Temperature();

		for (j = 0; j < 10; j++){
			delay(50000);
			//for (i=0; i<50000; i++);
		}
	}
}
