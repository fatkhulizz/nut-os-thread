#define F_CPU 14745600UL											//pertama mendefiisikan clock
#include <compiler.h>												//kemudian mendefinisikan library
#include <string.h>
#include <stdio.h>
#include <io.h>
#include <dev/board.h>
#include <sys/timer.h>

uint32_t      baud = 9600;											//Inisialisasi modul uart untuk 9600 baud rate;
FILE        *uart;
static char inbuf[50];												//untuk menginput karakter dan menyimpan input
static char inbuf_motor[50];
static char inbuf_led[50];


//===================================DEFINE VARIABLE				//untuk sensor
#define    DATA_DDR_1     DDRE |= 4
#define    DATA_DDR_0     DDRE &= ~4
#define    DATA_IN        (PINE & 4)
#define    SHT_SCK_1      PORTE |= 8
#define    SHT_SCK_0      PORTE &= ~8
//=================================================
#define noACK 0
#define ACK   1
//adr  command  r/w
#define STATUS_REG_W 0x06   //000   0011    0
#define STATUS_REG_R 0x07   //000   0011    1
#define MEASURE_TEMP 0x03   //000   0001    1
#define MEASURE_HUMI 0x05   //000   0010    1
#define RESET        0x1e   //000   1111    0
//=================================================
const float C1=-4.0;												//untuk sensor
const float C2=+0.0405;
const float C3=-0.0000028;
const float T1=+0.01;
const float T2=+0.00008;

unsigned char     TEMP_cksum, HUMI_cksum;
unsigned int     TEMP_val, HUMI_val;

//=======================================SHT RELATED
unsigned char SHT11_ByteWR(unsigned char value){
	unsigned char i, error=0;
	for(i=0x80; i>0; i>>=1){
		if(i&value)DATA_DDR_0;
		else       DATA_DDR_1;
		NutMicroDelay(2); SHT_SCK_1; NutMicroDelay(6); SHT_SCK_0; NutMicroDelay(3);
	}
	DATA_DDR_0; SHT_SCK_1; NutMicroDelay(3); error=DATA_IN; NutMicroDelay(2); SHT_SCK_0;
	return error;
}
unsigned char SHT11_ByteRD(unsigned char ack){
	unsigned char i, val=0;
	DATA_DDR_0;
	for(i=0x80; i>0; i>>=1){ SHT_SCK_1; NutMicroDelay(3); if(DATA_IN)val|=i; SHT_SCK_0; NutMicroDelay(3); }
	if(ack)DATA_DDR_1;
	else   DATA_DDR_0;
	SHT_SCK_1; NutMicroDelay(6); SHT_SCK_0; NutMicroDelay(3); DATA_DDR_0;
	return val;
}
void SHT11_Start(void){
	DATA_DDR_0;
	SHT_SCK_0;  NutMicroDelay(3); SHT_SCK_1; NutMicroDelay(3);
	DATA_DDR_1; NutMicroDelay(3);
	SHT_SCK_0;  NutMicroDelay(6); SHT_SCK_1; NutMicroDelay(3);
	DATA_DDR_0; NutMicroDelay(3);
	SHT_SCK_0;
}
void SHT11_Reset(void){
	unsigned char i;
	DATA_DDR_0; SHT_SCK_0;  NutMicroDelay(3);
	for(i=0;i<9;i++){ SHT_SCK_1;  NutMicroDelay(3); SHT_SCK_0;  NutMicroDelay(3); }
	SHT11_Start();
}
unsigned char SHT11_HUMI(void){
	unsigned char error=0;
	long i;
	error+=SHT11_ByteWR(MEASURE_HUMI);
	for(i=0; i<400000; i++){ NutMicroDelay(5); if(!DATA_IN)break; } // 2?
	if(DATA_IN)error++;
	HUMI_val=SHT11_ByteRD(ACK);
	HUMI_val<<=8;
	HUMI_val+=SHT11_ByteRD(ACK);
	HUMI_cksum=SHT11_ByteRD(noACK);
	return error;
}
unsigned char SHT11_TEMP(void){
	unsigned char error=0;
	long i;
	SHT11_Start();
	error+=SHT11_ByteWR(MEASURE_TEMP);
	for(i=0; i<400000; i++){ NutMicroDelay(5); if(!DATA_IN)break; } // 2?
	if(DATA_IN)error++;
	TEMP_val=SHT11_ByteRD(ACK);
	TEMP_val<<=8;
	TEMP_val+=SHT11_ByteRD(ACK);
	TEMP_cksum=SHT11_ByteRD(noACK);
	return error;
}
void calc_sth11(void){
	double rh_lin, rh_true, t_C, TEMP_f, HUMI_f;

	TEMP_f=(float)TEMP_val;
	HUMI_f=(float)HUMI_val;

	t_C=TEMP_f*0.01-40;
	rh_lin=C3*HUMI_f*HUMI_f + C2*HUMI_f + C1;
	rh_true=(t_C-25)*(T1+T2*HUMI_f)+rh_lin;
	if(rh_true>100)rh_true=100;
	if(rh_true<0.1)rh_true=0.1;

	TEMP_val=(unsigned int)t_C;
	HUMI_val=(unsigned int)rh_true;
}
//=======================================SHT RELATED

void InitUART0(void)														//untuk inisialisasi UART0
{
	NutRegisterDevice(&DEV_UART, 0, 0);
	uart = fopen(DEV_UART_NAME, "r+");
	_ioctl(_fileno(uart), UART_SETSPEED, &baud);
}

																			//untuk kontrol led turn off led
void led_on(int ledPin, int duration){
  PORTB |= 0b1000 << ledPin; 												//hidupkan led sesuai nilai ledPin					diubah
  NutSleep(duration);
  PORTB &= ~(0b1000 << ledPin); 											//matikan led sesuai nilai ledPin
  NutSleep(duration);
}

																			//rotate 4 step
void rotate_Right(int speed){
  int j=0;
  for(; j<4; j++){  														//rotasi 4 step
    PORTD &= 0b11110000;
    PORTD |= 0b1 << j; 														//rotasi 1 step
    NutSleep(speed);
  }
}

void rotate_Left(int speed){
  int j=0;
  for(; j<4; j++){  														//rotasi 4 step
    PORTD &= 0b11110000;
    PORTD |= 0b1000 >> j; 													//rotasi 1 step
    NutSleep(speed);
  }
}

THREAD(Thread1, arg) 														//led 1 nyala mati 0.5s
{
  while(1){
  PORTB=PORTB | 0b00010000;
  NutSleep(500);
  PORTB=PORTB & 0b11101111;
  NutSleep(500);
  }
}
THREAD(Thread2, arg) 														//led 2 nyala mati 1s
{
  while(1){
  PORTB=PORTB | 0b00100000;
  NutSleep(1000);
  PORTB=PORTB & 0b11011111;
  NutSleep(1000);
  }
}
THREAD(Thread3, arg) 														//led 3 4 nyala mati bergantian delay 2s
{
  while(1){
  PORTB=PORTB | 0b01000000;
  NutSleep(2000);
  PORTB=PORTB & 0b10111111;
  PORTB=PORTB | 0b10000000;
  NutSleep(2000);
  PORTB=PORTB & 0b01111111;
    }
}
THREAD(Thread4, arg) 														//kontrol led dengan uart
{
	unsigned char error;													//untuk sensor sht
  InitUART0();
  while(1){
    char *argv[16] = {NULL};
    int argc = 0;
    
    fprintf(uart,"\r\n\r\n= Welcome to RemLab K2 =\r\n");
    fprintf(uart,"1.kontrol motor stepper\r\n2.tampilkan pengukuran suhu\r\n3.kontrol led\r\n");
    
    fputs("masukkan pilihan:", uart);										//input 1, 2, 3 blablabla
    fflush(uart);
    fgets(inbuf, sizeof(inbuf), uart);

    
    char* cp = strchr(inbuf, '\n');   //get first occurrence/position of the character '\n'
    if (cp) *cp = 0;            //if found then remove	

    char pil;
    sscanf(inbuf, "%c", &pil);

    switch (pil) {
    	case '1':																// jika 1: motor stepper
    		fprintf(uart,"\r\n\r\nkontrol motor stepper\r\n");
    		fputs("perintah [<step><space><1=kanan><space><speed>]:", uart);	//3 angka(step,arah putaran, kecepatan putaran) misal 25; 1kanan 0kiri; 100 semakin dikit semakin cepat
    		fflush(uart);
    		fgets(inbuf_motor, sizeof(inbuf), uart);
    		
    		char* cp1 = strchr(inbuf_motor, '\n');   //get first occurrence/position of the character '\n'
    		if (cp1) *cp1 = 0;            //if found then remove	
				
    		//parsing
    		for (; argc < 16; argc++) {
      		argv[argc] = strtok(argc == 0 ? inbuf_motor : 0, " ");
      		if (argv[argc] == NULL) break;
      	}
      		
    		//assign inbuf into their variable 
    		int step,direction=1, speed=100; //default value, if input less than 2 parameter
    		sscanf(argv[0], "%d", &step);
    		if (argc > 0) sscanf(argv[1], "%d", &direction);
    		if (argc>1) sscanf(argv[2], "%d", &speed);
    		
    		if (direction){
    			fprintf(uart,"\r\nmemutar motor %d/200 step dengan kecepatan %d kekanan\r\n",step*4, speed);
  				int i=0;
  				for(; i<step; i++) rotate_Right(speed);
  			}
  			else {
    			fprintf(uart,"\r\nmemutar motor %d/200 step dengan kecepatan %d kekiri\r\n",step*4, speed);
  				int i=0;
  				for(; i<step; i++) rotate_Left(speed);
  			}
    		break;
    	case '2':																// suhu
				SHT11_Reset();
				error=0;
				error+=SHT11_HUMI();
				error+=SHT11_TEMP();
				if(error!=0)SHT11_Reset();
				else{ calc_sth11(); }
    		fprintf(uart, "\r\n\r\n=======||K2||=======\r\nSuhu : %2d oC\r\nKelembapan : %2d %%\r\n", TEMP_val, HUMI_val);
    		break;
    	case '3':																//led
    		fprintf(uart,"\r\n\r\nkontrol led\r\n");
    		fputs("perintah [<urutan led><space><durasi><space><jumlah blink]:", uart); //3 angka(urutan led, durasi, jumlah blink) 1 50 2
    		fflush(uart);
    		fgets(inbuf_led, sizeof(inbuf), uart);

    		char* cp2 = strchr(inbuf_led, '\n');   									//misal input ada line feed,\n line feed dakan dihapus
    		if (cp2) *cp2 = 0;            											//if found then remove

    		//parsing
    		for (; argc < 16; argc++) {												//mengurai input, didefinisikan sebagai masing masing variable
      		argv[argc] = strtok(argc == 0 ? inbuf_led : 0, " ");
      		if (argv[argc] == NULL) break;
      	}
      
    		//assign inbuf into their variable
    		int pin, dur=1000, rec=1; 												//default value, if input less than 3 parameter
    		sscanf(argv[0], "%d", &pin);
    		if (argc > 0) sscanf(argv[1], "%d", &dur);
    		if (argc>1) sscanf(argv[2], "%d", &rec);
    		
    		fprintf(uart,"\r\nkontrol led %d blink sebanyak %d dengan kecepatan %d\r\n", pin, rec, dur/1000);							//diubah
    		int i=0;
    		for (; i < rec; i++) led_on(pin, dur); 									//calling led function recursively for blinking
    		break;
    }
  }
}

int main(void){												
	DDRE |= 0b1000; 																//pin E3 sebagai input sht11
  DDRB |= 0b11110000;																// untuk input led, pake pin B4-B7
  DDRD |= 0b00001111;																//untuk motor stepper, pake pin D0-D3
  PORTB=0b00000000;
  NutThreadCreate("t1", Thread1, 0, 512);
  NutThreadCreate("t2", Thread2, 0, 512);
  NutThreadCreate("t3", Thread3, 0, 512);
  NutThreadCreate("t4", Thread4, 0, 512);
  while(1){
  PORTB=PORTB | 0b00000001;
  NutSleep(500);
  PORTB=PORTB & 0b11111110;
  NutSleep(500);
  }
  return 0;
}
