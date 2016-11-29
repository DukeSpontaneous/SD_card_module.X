#include <xc.h>

#include "RS-485_protocol.h"

void T4initial(void)
{
	T4CON = 0b0010000000001000;
	PR4 = 0xC200; // 5 сек
	PR5 = 0x0BEB;
	IFS1bits.T5IF = 0;
	IPC7bits.T5IP = 1;
	IEC1bits.T5IE = 1;
}
//------------------------------------------------------------------------------

void T2initial(void)// RS-485
{
	PR2 = timerx;
	//Load the timer period value. PR3 or PR5 con-tains the most significant word of the value,
	//while PR2 or PR4 contains the least significant
	//word.
	T2CONbits.TSIDL = 0;
	//TSIDL:Stop in Idle Mode bit
	//1= Discontinue timer operation when device enters Idle mode
	//0= Continue timer operation in Idle mode
	T2CONbits.TGATE = 0;
	//TGATE:Timerx Gated Time Accumulation Enable bit
	//When TCS = 1:
	//This bit is ignored.
	//When TCS = 0:
	//1= Gated time accumulation enabled
	//0= Gated time accumulation disabled
	T2CONbits.TCKPS = 0b01;
	//TCKPS<1:0>:Timerx Input Clock Prescale Select bits
	//11= 1:256 prescale value
	//10= 1:64 prescale value
	//01= 1:8 prescale value
	//00= 1:1 prescale value
	T2CONbits.T32 = 0;
	//T32:32-bit Timerx Mode Select bit
	//1= TMRx and TMRy form a 32-bit timer
	//0= TMRx and TMRy form separate 16-bit timer
	T2CONbits.TCS = 0;
	//TCS:Timerx Clock Source Select bit
	//1= External clock from TxCK pin
	//0= Internal clock (FOSC/2)
	IPC1bits.T2IP = 6;
	//TxIP<2:0>:Timerx Interrupt Priority bits
	//111= Interrupt is priority 7 (highest priority interrupt)
	//001= Interrupt is priority 1
	//000= Interrupt source is disabled
	IFS0bits.T2IF = 0;
	//T2IF:Timer2 Interrupt Flag Status bit
	//1= Interrupt request has occurred
	//0= Interrupt request has not occurred
	IEC0bits.T2IE = 1;
	//T2IE:Timer2 Interrupt Enable bit
	//1= Interrupt request enabled
	//0= Interrupt request not enabled
	T2CONbits.TON = 1;
	//TON:Timerx On bit
	//When T32 = 1(in 32-bit Timer mode):
	//1= Starts 32-bit TMRx:TMRy timer pair
	//0= Stops 32-bit TMRx:TMRy timer pair
	//When T32 = 0(in 16-bit Timer mode):
	//1= Starts 16-bit timer
	//0= Stops 16-bit timer
}

//------------------------------------------------------------------------------

void T1initial(void)// счетчик реального времени
{
	T1CON = 0b1000000000110000;
	PR1 = 39065;
	IEC0bits.T1IE = 1;
	IFS0bits.T1IF = 0;
	IPC0bits.T1IP = 5; // приоритет
}
