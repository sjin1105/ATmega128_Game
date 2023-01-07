#define F_CPU 16000000UL
#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#define EOS -1 // 음계 종료
#define LIGHT_VALUE 800

unsigned char FND_SEGNP[11] = { 0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x27,0x7F,0x6F,0x40 }; // 0 1 2 3 4 5 6 7 8 9 - 세그먼트 숫자표시  
unsigned char FND_SEGPOS[4] = { 0x01,0x02,0x04,0x08 }; // 7seg 위치
unsigned char LEDNP_YA[3] = { 0x03,0x01,0x00 }; // LED 1 2 3 4 -  주사위 게임 LED 위치 지정 
unsigned char LEDNP[5] = { 0x00,0x01,0x03,0x07,0x0F }; // LED 1 2 3 4 LED 위치  야구게임 LED 위치 지정 
unsigned char SEGPLAY[7] = { 0x40, 0x06, 0x5B, 0x73, 0x5E, 0x37, 0x77 }; // - 1 2 P d n A 
unsigned char LEDNN[4] = { 0, 0, 0, 0 }; //  주사위 게임 LED

int song[4] = { 0, 2, 5, EOS }; // 딩동댕 계명
int song_1[2] = { 8, EOS }; // 떙 계명
char f_table[8] = { 17, 43, 66, 77, 97, 114, 117, 137 };  // 음계

char nand[4] = { 8, 8, 8, 8 }; // 주사위 게임  
char p1[] = "Player Pa : aaaa\r\na Strike a Ball\r\n"; // 야구게임 
char win[] = "Winner a | a -- a\r\n";
char vic[] = "\b\r--Victory Player : 1--\r\n"; // 승리자 표시 
char p2[] = "----Game Start----\r\n"; // 게임 시작 
char clr[] = "\b\r                    \r";
char A[] = " USER1 | USER2 |                |\r\n";  // 문자 출력
char B[] = "--------------------------------|\r\n";
char C[] = "       |       |TOTAL SCORE     |\r\n";
char D[] = "       |       |Bonus           |\r\n";
char F[] = "       |       |DICE NUM        |\r\n";
char H[] = "       |       |Ones            |\r\n";
char I[] = "       |       |Twos            |\r\n";
char J[] = "       |       |Threes          |\r\n";
char K[] = "       |       |Fours           |\r\n";
char L[] = "       |       |Fives           |\r\n";
char M[] = "       |       |Sixs            |\r\n";
char N[] = "       |       |Three of a kind |\r\n";
char O[] = "       |       |Two and Two     |\r\n";
char P[] = "       |       |Choice          |\r\n";
char Q[] = "       |       |YACHAT          |\r\n";

char game[] = "SW 1 = HIT & BLOW GAME, SW 2 = YACHAT GAME\r\n\n"; // SW1 누를시 야구게임 , 스위치 2번 누를시 주사위 게임 
char game1[] = "GAME RULE\r\nSW 1 = STATE, SW 2 = SWITCH POSITION, SENSOR = NAND\r\nPRESS SW 1\r\n\n"; // GAME RULE  
char game2[] = "GAME RULE\r\nSW 1 = NUM++, SW 2 = SWITCH POSITION, SENSOR = SCORE BOARD\r\nPRESS SW 1\r\n\n";
char TAP[] = "\n";
char user2[] = "       |";
char dice[] = "%d %d %d %d\b\b\b\b\b\b\b\n\n";
char clr_YA[] = "\b\r";
char write[] = " a \r";

int user1_score[12] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
int user2_score[12] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

volatile int pos_x, y, state_z, state_j, g, back, count, count_tap, i, j, o, user1_count, user2_count, all_count, yy;
volatile int num0, num1, sta, STA, STATE_GAME, random1;

volatile unsigned char score_user1, score_user2;
volatile unsigned int pos = 0, tone, state, k, x, num, cc, count_user1, count_user2, cl, state_rand;
volatile unsigned char a, b, c, d, a1, b1, c1, d1, a2, b2, c2, d2, pnum, hit, blow;

volatile unsigned char adc_low, adc_high;
volatile unsigned short value;

void TX0_ch(char data);
void TX0_STR(char* str);
void SEG_YA();
void write_back();
void LED();
void write_score();
void NAND_YA();
void NAND();
void SEG();
void SEG_YA();
void cell();
void hitblow();
void NOISE();
void SEGP();
void CLR_YA(int b);
void YACHAT();
void BASEBALL();
void SEG_ALL(unsigned char a, unsigned char b, unsigned char c, unsigned char d);
void SONG_A();

int main(void)
{
	DDRC = 0xFF;  // 7 SEG POWER
	DDRG |= 0x0F;

	EICRA = 0x00;
	EICRB = 0x0A;  // 4, 5 interrupt 하강엣지
	EIMSK = 0x30;  // 4, 5 interrupt 활성화

	DDRA = 0xFF;  // LED POWER

	TCNT0 = 0;  // 타이머
	TIMSK = 0x01;  // nomal mode timer

	TCCR0 = 0x03;  // 32분주

	DDRB = DDRB | 0x10;  // 부저 PORTB 4 활성화 PB4 ON

	DDRF &= 0xFE;  // ADC와 PORTF 가 연결
	ADMUX = 0x40;  // ADC 기본 설정
	ADCSR = 0xEF;  // 7bit ADC Enable, 6bit ADC start, 5bit ADC freerunning, 3bit ADC interrupt Enable, 4bit ADC interrupt flag

	UBRR0H = 0; // 하위 4비트 사용
	UBRR0L = 0x67;  // 정의에 의해 103 9600bps 사용
	UCSR0B = 0x08; //TXEN0 = 1 송신부 활성화

	cli();

	while (1) {
		if (((PINE & 0x10) == 0x00) || ((PINE & 0X20) == 0x00)) {
			_delay_ms(2000);
			break;
		}
	}
	TX0_STR(game);

	while (1) {
		if ((PINE & 0x20) == 0x00) {
			STATE_GAME = 1;
			break;
		}
		if ((PINE & 0x10) == 0x00) {
			STATE_GAME = 2;
			break;
		}
	}
	if (STATE_GAME == 1) {
		YACHAT();
	}

	if (STATE_GAME == 2) {
		BASEBALL();
	}
}

void write_score() {
	if (count_tap == 0) {
		i = (nand[0] == 1) + (nand[1] == 1) + (nand[2] == 1) + (nand[3] == 1);
	}
	if (count_tap == 1) {
		i = (nand[0] == 2) + (nand[1] == 2) + (nand[2] == 2) + (nand[3] == 2);
		i *= 2;
	}
	if (count_tap == 2) {
		i = (nand[0] == 3) + (nand[1] == 3) + (nand[2] == 3) + (nand[3] == 3);
		i *= 3;
	}
	if (count_tap == 3) {
		i = (nand[0] == 4) + (nand[1] == 4) + (nand[2] == 4) + (nand[3] == 4);
		i *= 4;
	}
	if (count_tap == 4) {
		i = (nand[0] == 5) + (nand[1] == 5) + (nand[2] == 5) + (nand[3] == 5);
		i *= 5;
	}
	if (count_tap == 5) {
		i = (nand[0] == 6) + (nand[1] == 6) + (nand[2] == 6) + (nand[3] == 6);
		i *= 6;
	}
	if (count_tap == 6) {
		if (((nand[0] != nand[1]) + (nand[0] != nand[2]) + (nand[0] != nand[3]) + (nand[1] != nand[2]) + (nand[1] != nand[3]) + (nand[2] != nand[3])) == 3) {
			i = (nand[0] + nand[1] + nand[2] + nand[3]);
		}
		else {
			i = 0;
		}
	}
	if (count_tap == 7) {
		if (((nand[0] != nand[1]) + (nand[0] != nand[2]) + (nand[0] != nand[3]) + (nand[1] != nand[2]) + (nand[1] != nand[3]) + (nand[2] != nand[3])) == 4) {
			i = (nand[0] + nand[1] + nand[2] + nand[3]);
		}
		else {
			i = 0;
		}
	}
	if (count_tap == 8) {
		i = nand[0] + nand[1] + nand[2] + nand[3];
	}
	if (count_tap == 9) {
		if (((nand[0] == nand[1]) + (nand[0] == nand[2]) + (nand[0] == nand[3]) + (nand[1] == nand[2]) + (nand[1] == nand[3]) + (nand[2] == nand[3])) == 6) {
			i = 50;
		}
		else {
			i = 0;
		}
	}
	write_back();
	o = 1;
}

void NAND_YA()  // 랜덤 숫자 생성
{
	num0 = rand() % 6 + 1;
}

void NAND()  // 랜덤 숫자 생성
{
	cli();  //전역 인터럽트 비활성화 cli()를 써도 타이머는 작동

	while (1) {
		a2 = 5, b2 = 6, c2 = 5, d2 = 4;
		SEGP();
		if ((PINE & 0x10) == 0x00) {
			break;
		}
	}

	sei();  // 전역 인터럽트 활성화

	while (1) {
		num = rand() % 10000; // 0-9999 난수 설정
		d1 = num % 10;
		c1 = num / 10 % 10;
		b1 = num / 100 % 10;
		a1 = num / 1000;
		j = (a1 == b1) + (a1 == c1) + (a1 == d1) + (b1 == c1) + (b1 == d1) + (c1 == d1) + (a1 == 0);
		if (j == 0) {
			break;
		}
	}

	for (cl = 0; cl < ((3 * cc)); cl++) { // 지난 게임 삭제
		TX0_STR(clr);
	}
	cc = 0;
}

void SEG()  // 7SEGMENT
{
	while (1) {
		SEG_ALL(a, b, c, d);

		if (pos == 4) {
			NOISE();
			if (pos == 4)
				break;
		}
	}
}

void SEG_ALL(unsigned char a, unsigned char b, unsigned char c, unsigned char d)
{
	PORTC = FND_SEGNP[d];
	PORTG = FND_SEGPOS[0];
	_delay_ms(1);

	PORTC = FND_SEGNP[c];
	PORTG = FND_SEGPOS[1];
	_delay_ms(1);

	PORTC = FND_SEGNP[b];
	PORTG = FND_SEGPOS[2];
	_delay_ms(1);

	PORTC = FND_SEGNP[a];
	PORTG = FND_SEGPOS[3];
	_delay_ms(1);
}

void SEG_YA()  // 7SEGMENT
{
	state_j = 0;
	count = 0;
	while (1) {
		SEG_ALL(nand[0], nand[1], nand[2], nand[3]);

		LED();

		if (pos_x == 4) {
			count++;
			for (STA = 0; STA < 4; STA++) {
				LEDNN[STA] = 0;
			}
			pos_x = 0;
			LEDNN[0] = 1;
		}

		if (count == 2) {
			state_j = 2;
			PORTA = 0x00;
			break;
		}
	}
}

void LED()
{
	if (pos_x == 0)
		PORTA = (LEDNP_YA[count] * 64) + (LEDNN[0] * 8);
	if (pos_x == 1)
		PORTA = (LEDNP_YA[count] * 64) + (LEDNN[1] * 4);
	if (pos_x == 2)
		PORTA = (LEDNP_YA[count] * 64) + (LEDNN[2] * 2);
	if (pos_x == 3)
		PORTA = (LEDNP_YA[count] * 64) + LEDNN[3];
}

void SONG_A()
{
	yy = 0;
	do {
		tone = song[yy++]; // 노래 음계
		_delay_ms(200); // 한 계명당 지속 시간
	} while (tone != EOS); // 노래 마지막 음인지 검사
}

void TX0_ch(char data) // PUTTY 입력
{
	while (!(UCSR0A & (1 << UDRE0)));  // 수신 버퍼 UDR0 에 값이 있는지 확인

	UDR0 = data;
}

void TX0_STR(char* str)  // 문자열 출력
{
	while (*str)
	{
		TX0_ch(*str++);
	}
}

void cell()
{
	TX0_STR(A);
	TX0_STR(B);
	TX0_STR(C);
	TX0_STR(D);
	TX0_STR(B);
	TX0_STR(F);
	TX0_STR(B);
	TX0_STR(H);
	TX0_STR(I);
	TX0_STR(J);
	TX0_STR(K);
	TX0_STR(L);
	TX0_STR(M);
	TX0_STR(N);
	TX0_STR(O);
	TX0_STR(P);
	TX0_STR(Q);
}

void hitblow()  // strike, ball 판단
{
	hit = 0, blow = 0;
	hit = (a1 == a) + (b1 == b) + (c1 == c) + (d1 == d);
	blow = (a == b1) + (a == c1) + (a == d1) + (b == a1) + (b == c1) + (b == d1) + (c == a1) + (c == b1) + (c == d1)
		+ (d == a1) + (d == b1) + (d == c1);
	PORTA = (LEDNP[hit] * 16) + LEDNP[blow];

	sprintf(p1, "Player P%d : %d%d%d%d\r\n%d Strike %d Ball\r\n\n", pnum, a, b, c, d, hit, blow);
	TX0_STR(p1);

	cc++;
}

void NOISE()  // 입력한 숫자의 중복 판단
{
	i = (a == b) + (a == c) + (a == d) + (b == a) + (b == c) + (b == d) + (c == a) + (c == b) + (c == d)
		+ (d == a) + (d == b) + (d == c) + (a == 10) + (b == 10) + (c == 10) + (d == 10);

	if (i >= 1) {
		a = 10, b = 10, c = 10, d = 10, pos = 0;
		yy = 0;
		do {
			tone = song_1[yy++]; // 노래 음계
			_delay_ms(200); // 한 계명당 지속 시간
		} while (tone != EOS); // 노래 마지막 음인지 검사
	}
}

void SEGP()     // 점수 표시, P1,P2 표시
{
	PORTC = SEGPLAY[d2];
	PORTG = FND_SEGPOS[0];
	_delay_ms(1);

	PORTC = SEGPLAY[c2];
	PORTG = FND_SEGPOS[1];
	_delay_ms(1);

	PORTC = SEGPLAY[b2];
	PORTG = FND_SEGPOS[2];
	_delay_ms(1);

	PORTC = SEGPLAY[a2];
	PORTG = FND_SEGPOS[3];
	_delay_ms(1);
}

void CLR_YA(int b)
{
	for (g = 0; g < b; g++)
	{
		TX0_STR(clr_YA);
	}
}

void write_back()
{
	if (i < 10) {
		sprintf(write, " %d \r", i);
		TX0_STR(write);
		back = count_tap + 2;
	}
	else {
		a1 = i / 10;
		a2 = i % 10;
		sprintf(write, " %d%d\r", a1, a2);
		TX0_STR(write);
		back = count_tap + 2;
	}
}

void YACHAT()
{
	TX0_STR(game1);
	pos_x = 0, all_count = 0;
	cli();
	while (1) {
		if ((PINE & 0x10) == 0x00) {
			break;
		}
	}
	sei();
	cell();

	while (1) {
		all_count++;
		state_rand = 0;
		nand[0] = 8; nand[1] = 8, nand[2] = 8, nand[3] = 8; pos_x = 0, k = 0, state_z = 0, num1 = 0;
		if (all_count == 21) {
			break;
		}
		SEG_YA();

		back = count_tap + 2;
		if (all_count == 1) {
			back = 12;
		}

		CLR_YA(back);

		if (user1_count != user2_count) {
			TX0_STR(user2);
		}
		sprintf(dice, "%d %d %d %d\b\b\b\b\b\b\b\n\n", nand[0], nand[1], nand[2], nand[3]);
		TX0_STR(dice);
		count_tap = 0;
		o = 0;

		SONG_A();

		while (o == 0) {
			SEG_ALL(nand[0], nand[1], nand[2], nand[3]);
		}
		if (user1_count == user2_count) {
			user1_count++;
			user1_score[count_tap] = i;
		}
		else {
			user2_count++;
			user2_score[count_tap] = i;
		}

		SONG_A();
	} // while

	back = count_tap + 5;
	CLR_YA(back);

	i = user1_score[0] + user1_score[1] + user1_score[2] + user1_score[3] + user1_score[4] + user1_score[5];
	if (i >= 48) {
		user1_score[10] = 35;
	}
	else {
		user1_score[10] = 0;
	}
	user1_score[11] = i + user1_score[6] + user1_score[7] + user1_score[8] + user1_score[9] + user1_score[10];
	i = user2_score[0] + user2_score[1] + user2_score[2] + user2_score[3] + user2_score[4] + user2_score[5];
	if (i >= 48) {
		user2_score[10] = 35;
	}
	else {
		user2_score[10] = 0;
	}
	user2_score[11] = i + user2_score[6] + user2_score[7] + user2_score[8] + user2_score[9] + user2_score[10];

	a1 = user1_score[11] % 10;
	b1 = user1_score[11] / 10 % 10;
	c1 = user1_score[11] / 100;

	a2 = user2_score[11] % 10;
	b2 = user2_score[11] / 10 % 10;
	c2 = user2_score[11] / 100;

	sprintf(C, "  %d%d%d  |  %d%d%d  |TOTAL SCORE     |\r\n", c1, b1, a1, c2, b2, a2);
	TX0_STR(C);

	a1 = user1_score[10] % 10;
	b1 = user1_score[10] / 10;

	a2 = user2_score[10] % 10;
	b2 = user2_score[10] / 10;

	sprintf(D, "  %d%d   |   %d%d  |Bonus           |\r\n", b1, a1, b2, a2);
	TX0_STR(D);
	if (user1_score[11] == user2_score[11]) {
		sprintf(F, "\n    ------ No Winner -------    |\r\n");
		TX0_STR(F);
	}
	else {
		if (user1_score[11] > user2_score[11]) {
			sprintf(F, "\n    ------ Winner P1 -------    |\r\n");
			TX0_STR(F);
		}
		else {
			sprintf(F, "\n    ------ Winner P2 -------    |\r\n");
			TX0_STR(F);
		}
	}
	cli();
}

void BASEBALL()
{
	sei();

	TX0_STR(game2);
	score_user1 = 0, score_user2 = 0;

	NAND();
	TX0_STR(p2);  // game start 표시

	while (1) {  // 게임시작
		a = 10, b = 10, c = 10, d = 10, pos = 0, k = 0;

		if (count_user1 == count_user2)  // P1 순서
		{
			a2 = 0, b2 = 3, c2 = 1, d2 = 0;
			for (i = 0; i < 200; i++) {
				SEGP();  // SEG P1 표시
			}
			SEG();
			count_user1++;
			pnum = 1;
		}
		else // P2 순서
		{
			a2 = 0, b2 = 3, c2 = 2, d2 = 0;
			for (i = 0; i < 200; i++) {
				SEGP();  // SEG P2 표시
			}
			SEG();
			count_user2++;
			pnum = 2;
		}
		hitblow(); // hitblow 판정

		if (hit == 4) {
			SONG_A();

			if (count_user1 == count_user2) { // P2 승리
				score_user2++;
				count_user1 = 0;
				count_user2 = 0;

				NAND();

				sprintf(win, "Winner 2 | %d -- %d\r\n\n", score_user1, score_user2);
				TX0_STR(win);

				TX0_STR(p2);  // game start 표시
			}
			else // P1 승리
			{
				score_user1++;
				count_user1 = 0;
				count_user2 = 0;

				NAND();

				sprintf(win, "Winner 1 | %d -- %d\r\n\n", score_user1, score_user2);
				TX0_STR(win);

				TX0_STR(p2);  // game start 표시
			}
		}

		if (score_user1 == 3) {
			a = 1, b = 1, c = 1, d = 1, pos = 0;
			sprintf(vic, "\b\r--Victory Player : 1--\r\n");
			TX0_STR(vic);
			cli();
			SEG(); // P1이 승리할시 1이 4개 표시
		}
		if (score_user2 == 3) {
			a = 2, b = 2, c = 2, d = 2, pos = 0;
			sprintf(vic, "\b\r--Victory Player : 2--\r\n");
			TX0_STR(vic);
			cli();
			SEG();// P1이 승리할시 2가 4개 표시
		}
	}   //while
}

ISR(INT4_vect)
{
	if (STATE_GAME == 1) {
		if (state_j == 0) {
		}
		if (state_j == 1) {
			state_z = 1;
			sta = 1;
		}
		if (state_j == 2) {
			TX0_STR(TAP);
			count_tap++;
			if (count_tap == 10) {
				if (user1_count == user2_count) {
					back = count_tap;
					CLR_YA(back);
				}
				else {
					back = count_tap + 3;
					CLR_YA(back);
					TX0_STR(user2);
					TX0_STR(TAP);
					TX0_STR(TAP);
				}
				count_tap = 0;
			}
		}
		_delay_ms(100);
	}
	if (STATE_GAME == 2) {
		switch (pos) {
		case 0:
			a++;
			if (a == 11 || a == 10) a = 0;
			_delay_ms(50);
			break;

		case 1:
			b++;
			if (b == 11 || b == 10) b = 0;
			_delay_ms(50);
			break;

		case 2:
			c++;
			if (c == 11 || c == 10) c = 0;
			_delay_ms(50);
			break;

		case 3:
			d++;
			if (d == 11 || d == 10) d = 0;
			_delay_ms(50);
			break;
		}
		k = 1;
	}
}

ISR(INT5_vect)  // sw2 interrupt
{
	if (STATE_GAME == 1) {
		if (state_j == 0) {
		}
		if (state_j == 1) {
			if (sta == 1) {
				pos_x++;
				LEDNN[pos_x] = 1;
				sta = 0;
			}
		}
		if (state_j == 2) {
			if (k == 0) {
				if (user1_count == user2_count) {
					if (user1_score[count_tap] == -1) {
						write_score();
						k = 1;
					}
				}
				if (user1_count != user2_count) {
					if (user2_score[count_tap] == -1) {
						write_score();
						k = 1;
					}
				}
			}
		}
		_delay_ms(100);
	}
	if (STATE_GAME == 2) {
		if ((k == 1) && (a != 0))
		{
			pos++;
			k = 0;
		}
	}
}

ISR(TIMER0_OVF_vect)  // timer interrupt
{
	random1++;
	if (random1 == 10000000) {
		random1 = 0;
	}
	srand(random1);  // srand 설정
	if (yy != 0) {
		if (tone != EOS)  //마지막 음계
		{
			if (state == 0)
			{
				PORTB |= 1 << 4; // 버저 포트 ON
				state = 1;
			}
			else
			{
				PORTB &= ~(1 << 4); // 버저 포트 OFF
				state = 0;
			}
			TCNT0 = f_table[tone];
		}
	}
}

ISR(ADC_vect)  // adc interrupt
{
	adc_low = ADCL;
	adc_high = ADCH;

	value = ((unsigned short)adc_high << 8) | (unsigned short)adc_low;

	if (value < LIGHT_VALUE)
	{
		if (STATE_GAME == 1) {
			if (state_rand == 0) {
				if (state_j == 0) {
					NAND_YA();
					nand[num1] = num0;
					num1++;
					_delay_ms(1000);
					state_rand = 1;
					if (num1 == 4) {
						state_j = 1;
						LEDNN[0] = 1;
					}
				}
				if (state_j == 1) {
					if (state_z == 1) {
						LEDNN[pos_x] = 0;
						NAND_YA();
						nand[pos_x] = num0;
						state_z = 0;
					}

				}
			}
		}

		if (STATE_GAME == 2) {
			for (x = 0; x < 400; x++)
			{
				PORTC = FND_SEGNP[score_user2];
				PORTG = FND_SEGPOS[0];
				_delay_ms(1);

				PORTC = SEGPLAY[0];
				PORTG = FND_SEGPOS[1];
				_delay_ms(1);

				PORTC = SEGPLAY[0];
				PORTG = FND_SEGPOS[2];
				_delay_ms(1);

				PORTC = FND_SEGNP[score_user1];
				PORTG = FND_SEGPOS[3];
				_delay_ms(1);
			}
		}
	}

	if (value > LIGHT_VALUE)
	{
		state_rand = 0;
	}
}
