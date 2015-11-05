#include <stdio.h>
#include <stdlib.h>	
#include <string.h>	// 문자열 관리를 위함
#include <math.h>	// log 함수를 사용하기 위함 (자리수 구하기)
#include <malloc.h>	// 동적할당을 위함

// 명령어, 피연산자 앞에 붙어있는 기호들을 알아보기 쉽도록 define
#define PLUS 1
#define SHARP 2
#define AT 3

// 각각의 상수들을 명확하게 의미하기 위해서 define
#define BUFFER_SIZE 256	// 버퍼의 크기
#define LABEL_LENGTH 32	// 소스코드상 라벨들의 최대 길이
#define SYMTAB_SIZE 20	// 심볼테이블의 레코드 최대 개수
#define IMR_SIZE 100	// 중간파일의 레코드 최대 개수
#define RLD_SIZE 20		// Relocation DIctionary의 레코드 최대 개수

#define TEST_FNAME "x:\\3.asm"

/***************************** DECLERATE VARIABLE ****************************/
typedef struct OperationCodeTable {	// OP 테이블의 각각의 레코드 구조체
	char Mnemonic[LABEL_LENGTH];	// 명령어의 형상 (ex. LDB, LDA, etc...)
	char Format;	// 명령어의 형식 (명령어의 길이)	3/4 형식은 편의상 3형식으로 표현하도록 설계했습니다.
	unsigned short int  ManchineCode;	// 해당 명령어의 목적 코드
}SIC_OPTAB;

typedef struct SymbolTable { // 심볼테이블의 각각의 레코드 구조체
	char Label[LABEL_LENGTH];	// 레이블의 이름
	int Address;	// 레이블이 가리키는 주소
}SIC_SYMTAB;

// 레지스터 테이블을 구성하는 레지스터 레코드
typedef struct RegisterTable {
	char name[LABEL_LENGTH];	// 레지스터 Mnemonic
	int id;	// 레지스터의 고유번호
} SIC_XE_REGISTER;

// 수정레코드 작성을 위해 재배치가 필요한 부분을 담는 Dictionary 레코드
typedef struct RelocationDictionary {
	int Address;	// 재배치가 필요한 위치
	int Nibbles;	// 재배치가 필요한 길이
} RLD;

typedef struct IntermediateRecord {	// 중간파일 구조
	unsigned short int LineIndex;	// 소스코드의 행을 저장하는 변수
	unsigned short int Loc;	//  해당 명령어의 메모리상 위치
	unsigned long long int ObjectCode;	//  Pass 2를 거쳐 Assemble된 목적코드
	char LabelField[LABEL_LENGTH];	// 소스코드상 표기되어있는 레이블
	char OperatorField[LABEL_LENGTH];	// 소스코드상 표기되어있는 Mnemonic
	char OperandField[LABEL_LENGTH];	// 소스코드상 표기되어있는 피연산자
}IntermediateRec;

// 함수의 결과를 전달하기 위해 임시로 사용하는 전역변수들
int Counter;	// Opcode찾을 때 그 명령어의 위치를 가리키기 위한 변수
int RegIdx;		// 레지스터의 위치를 가리킴. Counter 변수와 역할 비슷
int SymIdx;		// Label의 위치를 가리킴. Counter 변수와 역할 비슷
int LOCCTR[IMR_SIZE];	// 각 명령어들의 메모리를 세기위한 Location Counter. 중간파일의 개수와 동일하다
int LocctrCounter = 0;	// LOCCTR의 Index 변수
int Flag;
int Index;
int j;
int ManchineCode;
int SymtabCounter = 0;	// 심볼테이블의 개수를 세고 가리키기 위한 변수
int start_address;	// 프로그램의 시작 주소
int program_length;	// 프로그램의 총 길이
int ArrayIndex = 0;	// 중간파일을 각각 가리키기 위한 Index 변수
int RLDCounter = 0;	// 재배치가 필요한 부분의 개수를 세고 가리키기 위한 변수 (수정 레코드)

unsigned short int FoundOnSymtab_flag = 0;	// 해당 레이블을 심볼테이블에서 찾았다는 것을 반환하기 위함
unsigned short int FoundOnOptab_flag = 0;	// 해당 Opcode의 Mnemonic을 OP 테이블에서 찾았다는 것을 반환하기 위함
unsigned short int FoundOnRegTab_flag = 0;	// 레지스터 테이블에 해당 레지스터의 형상이 있는지 확인

// 이 변수들은 모두 소스코드상의 표기법을 가짐
char Buffer[BUFFER_SIZE];	// 각 소스코드를 읽기위한 버퍼 변수
char Label[LABEL_LENGTH];	// 레이블을 임시로 저장하기 위한 변수
char Mnemonic[LABEL_LENGTH];	// Mnemnic을 임시로 저장하기 위한 변수
char Operand[LABEL_LENGTH];	// 피연산자를 임시로 저장하기 위한 변수

// 각각 프로그램당 1개씩 필요하므로 2차원 배열로 생성
SIC_SYMTAB SYMTAB[SYMTAB_SIZE];	// 심볼테이블 변수
IntermediateRec* IMRArray[IMR_SIZE];	// 중간파일 변수
RLD RLDArray[RLD_SIZE];	// 재배치가 필요한 부분을 저장하기 위한 변수

static SIC_XE_REGISTER REG_TAB[] =
{
	{ "A", 0 },
	{ "X", 1 },
	{ "L", 2 },
	{ "B", 3 },
	{ "S", 4 },
	{ "T", 5 },
	{ "F", 6 },
	{ "PC", 8 },
	{ "SW", 9 }
};

// OP 테이블
static SIC_OPTAB OPTAB[] =
{
	/*********Instruction Set II***********/
	{ "ADDF", '3', 0x58 },
	{ "COMPF", '3', 0x88 },
	{ "DIVF", '3', 0x64 },
	{ "FIX", '1', 0xC4 },
	{ "FLOAT", '1', 0xC0 },
	{ "LDF", '3', 0x70 },
	{ "MULF", '3', 0x60 },
	{ "NORM", '1', 0xC8 },
	{ "STF", '3', 0x80 },
	{ "SUBF", '3', 0x5C },
	/*********Instruction Set I***********/
	{ "ADDR", '2', 0x90 },
	{ "CLEAR", '2', 0xB4 },
	{ "COMPR", '2', 0xA0 },
	{ "DIVR", '2', 0x9C },
	{ "HIO", '1', 0xF4 },
	{ "LDB", '3', 0x68 },
	{ "LDS", '3', 0x6C },
	{ "LDT" , '3', 0x74 },
	{ "LPS", '3', 0xD0 },
	{ "MULR", '2', 0x98 },
	{ "RMO", '2', 0xAC },
	{ "SHIFTL", '2', 0xA4 },
	{ "SHIFTR", '2', 0xA8 },
	{ "SIO", '1', 0xF0 },
	{ "SSK", '3', 0xEC },
	{ "STB", '3', 0x78 },
	{ "STS", '3', 0x7C },
	{ "STT", '3', 0x84 },
	{ "SUBR", '2', 0x94 },
	{ "SVC", '2', 0xB0 },
	{ "TIO", '1', 0xF8 },
	{ "TIXR", '2', 0xB8 },
	/**********SIC Instruction Set*********/
	{ "ADD",  '3',  0x18 },
	{ "AND",  '3',  0x40 },
	{ "COMP",  '3',  0x28 },
	{ "DIV",  '3',  0x24 },
	{ "J",  '3',  0x3C },
	{ "JEQ",  '3',  0x30 },
	{ "JGT",  '3',  0x34 },
	{ "JLT",  '3',  0x38 },
	{ "JSUB",  '3',  0x48 },
	{ "LDA",  '3',  0x00 },
	{ "LDCH",  '3',  0x50 },
	{ "LDL",  '3',  0x08 },
	{ "LDX",  '3',  0x04 },
	{ "MUL",  '3',  0x20 },
	{ "OR",  '3',  0x44 },
	{ "RD",  '3',  0xD8 },
	{ "RSUB",  '3',  0x4F },
	{ "STA",  '3',  0x0C },
	{ "STCH",  '3',  0x54 },
	{ "STL",  '3',  0x14 },
	{ "STSW",  '3',  0xE8 },
	{ "STX",  '3',  0x10 },
	{ "SUB",  '3',  0x1C },
	{ "TD",  '3',  0xE0 },
	{ "TIX",  '3',  0x2C },
	{ "WD",  '3',  0xDC },
};


/****************************** DFINATE FUNCTION *****************************/
char* ReadLabel() {	// 레이블 읽기
	j = 0;//zeroing
	while (Buffer[Index] != ' ' && Buffer[Index] != '\t' && Buffer[Index] != '\n')
		Label[j++] = Buffer[Index++];
	Label[j] = '\0';
	return(Label);
}

void SkipSpace() {	// 공백 스킵하기 (Index를 뒤로 옮김)
	while (Buffer[Index] == ' ' || Buffer[Index] == '\t')
		Index++;
}

int ReadFlag(char *Mnemonic) {	// Mnemonic에서 플래그 비트 읽기
	Flag = 0;
	switch (Mnemonic[0]) {	// Mnemonic의 첫번째 글자가 특수문자일경우
	case '+':
		Flag = PLUS;	// extended instruction
		break;
	case '#':
		Flag = SHARP;	// immediate addressing mode
		break;
	case '@':
		Flag = AT;	// indirect addressing mode
		break;
	default:
		Flag = 0;	// default (아무런 표시가 없을 경우)
	}
	return Flag;
}

char* ReadOprator() {	// Mnemonic 읽기
	j = 0;//zeroing
	while (Buffer[Index] != ' ' && Buffer[Index] != '\t' && Buffer[Index] != '\n')	// 공백이 나올때까지 Mnemonic 읽기
		Mnemonic[j++] = Buffer[Index++];
	Mnemonic[j] = '\0';	// 단순 문자배열을 문자열로 인식하도록 널 문자 추가
	return(Mnemonic);	// 문자열 반환
}

char* ReadOperand() {	// 피연산자 읽기
	j = 0;//zeroing
	while (Buffer[Index] != ' ' && Buffer[Index] != '\t' && Buffer[Index] != '\n')	// 공백이 나올때까지 Operand 읽기
		Operand[j++] = Buffer[Index++];
	Operand[j] = '\0';	// 단순 문자배열을 문자열로 인식하도록 널 문자 추가
	return(Operand);	// 문자열 반환
}

void RecordSymtab(char* label) {	// 심볼테이블에 해당 레이블의 위치와 레이블 입력
	if (ReadFlag(label)) { // Immediate or Indirect Addressing Mode 예외 처리
		label = label + 1;
	}
	strcpy(SYMTAB[SymtabCounter].Label, label);	// Symbol 테이블에 Label 추가
	SYMTAB[SymtabCounter].Address = LOCCTR[LocctrCounter - 1];	// 해당 Label의 메모리 위치도 기록
	SymtabCounter++;	// 1개 추가되었으므로 카운트 1증가
}

void RecordRLD(char* Mnemonic, int loc) {	// 재배치가 필요한 부분 RLDArray에 추가
	RLDArray[RLDCounter].Address = loc + 1;	// 명령어 시작 위치에서 OP code와 플래그비트 부분을 제외한 시작 위치 저장
	RLDArray[RLDCounter].Nibbles = 3;	// 재배치가 필요한 부분 길이 저장 (3형식일 경우 3 니블)
	if (ReadFlag(Mnemonic)) {	// 4형식일 경우 1바이트(2 니블)만큼 피연산자가 늘어나므로 추가
		RLDArray[RLDCounter].Nibbles += 2;
	}
	RLDCounter++;	// RLDCounter에 개수 1증가
}

int SearchSymtab(char* label) {	// 심볼테이블에서 레이블 찾기
	FoundOnSymtab_flag = 0;
	if (ReadFlag(label)) { // Immediate Addressing Mode이거나 Indirect Addressing Mode일 경우 예외처리
		label = label + 1;
	}

	for (int k = 0; k <= SymtabCounter; k++) {	// 반복을 통해 찾기
		if (!strcmp(SYMTAB[k].Label, label)) {	// label이 심볼테이블에 있을 경우
			FoundOnSymtab_flag = 1;	// 찾았다는 의미를 표현하기위한 플래그
			SymIdx = k;	// SymIdx는 심볼테이블에서 어느 위치에 있는지 가리킨다.
			return (FoundOnSymtab_flag);
		}
	}
	return (FoundOnSymtab_flag);	// 없으면 0 반환
}

int SearchOptab(char * Mnemonic) {	// 심볼테이블에서 OP code 찾기
	int size = sizeof(OPTAB) / sizeof(SIC_OPTAB);
	FoundOnOptab_flag = 0;
	if (ReadFlag(Mnemonic)) { // Extended Instruction일 경우 예외처리
		Mnemonic = Mnemonic + 1;	// 문자열포인터를 1증가시킴. 맨앞자리 생략
	}
	for (int i = 0; i<size; i++) {
		if (!strcmp(Mnemonic, OPTAB[i].Mnemonic)) {	// OP 테이블에 해당 Mnemonic이 있을 경우
			Counter = i;	// 있을 경우 Counter는 해당 OP code가 있는 OPTAB 상 Index 반환하기
			FoundOnOptab_flag = 1;	// 찾았다는 의미를 표현하기위한 플래그
			break;
		}
	}
	return (FoundOnOptab_flag);	// 없으면 0 반환
}

int SearchRegTab(char * Mnemonic) {	// 미리 정의된 레지스터 테이블에서 해당 레지스터를 읽는다
	int size = sizeof(REG_TAB) / sizeof(SIC_XE_REGISTER);
	FoundOnRegTab_flag = 0;
	for (int i = 0; i < size; i++) {
		if (!strcmp(Mnemonic, REG_TAB[i].name)) {	// 레지스터 테이블에 해당 레지스터의 기호가 있을 경우
			RegIdx = i;	// 레지스터 테이블에서 어느 위치에 있는지 가리킴
			FoundOnRegTab_flag = 1;	// 찾았다는 의미를 표현하기 위한 플래그
			break;
		}
	}
	return (FoundOnRegTab_flag);	// 없으면 0 반환
}

int isNum(char * str) {	// 문자열을 이루고 있는 모든 원소들이 숫자로 이루어져있는지 확인
	if (ReadFlag(str)) {	// 문자열 맨 앞에 플래그비트가 있을 경우 이를 생략하기 위함.
		str += 1;
	}
	int i, len = strlen(str);
	for (i = 0; i < len; ++i) {	// 문자열 길이만큼 반복해서 모두 숫자인지 판단
		if ('0' > str[i] || '9' < str[i]) {	// 숫자가 아닐 경우 0을 반환
			if (str[i] == '-') continue;	// 음수를 나타내는 '-'가 나왔을 경우는 생략
			return 0;
		}
	}
	return 1;	// 모두 숫자이므로 1을 반환
}

int isFloatNum(char * str) {	// Floating Number인지 확인하기 위한 함수
	if (ReadFlag(str)) {	// 문자열 맨 앞에 플래그비트가 있을 경우 이를 생략하기 위함.
		str += 1;
	}
	int i, len = strlen(str), f = 0;	// 소수점이 나오지 않을 경우 Float Num이 아니기 때문에 분기처리하기 위한 변수 f
	for (i = 0; i < len; ++i) {	// 문자열 길이만큼 반복해서 모두 숫자인지 판단
		if ('0' > str[i] || '9' < str[i]) {	// 숫자가 아닐 경우 0을 반환
			if (str[i] == '.' && f == 0) {
				f = 1;
				continue;
			}
			if (str[i] == '-') continue;	// 소수점임을 나타내는 '.'과 음수를 나타내는 '-'가 나왔을 경우는 생략
			return 0;
		}
	}
	return (f != 0) ? 1 : 0;	// 모두 숫자이므로 1을 반환
}

unsigned long ConvertNumber(int diff, int nibble) {
	// 비트에 맞춰 음수를 계산하기 위한 함수 (12 bit이므로 음수 계산을 한다.)
	if (diff >= 0) { // 양수이므로 그대로 반환
		return diff;
	}
	// 음수일경우 자료형의 크기가 12비트라고 가정하고 2's completion을 진행
	// 표현하는 nibble에 따라 뒤에 잘라내는 비트 수를 다르게한다.
	if (nibble == 5) {
		diff ^= 0xFFF00000;	// 20비트 이후의 bit를 제거하기 위해서 총 32비트의 int형의 뒤 12비트를 0으로 만든다
	} else {
		diff ^= 0xFFFFF000;	// 12비트 이후의 bit를 제거하기 위해서 총 32비트의 int형의 뒤 20비트를 0으로 만든다
	}
	return diff;
}

int StrToDec(char* c) {	// 10진수를 표현하는 String을 정수형으로 변환해서 반환
	if (ReadFlag(c)) {	// 플래그비트를 생략하기위함.
		c += 1;
	}
	int dec_num = 0;
	char temp[10];
	strcpy(temp, c);	// temp에 문자열 복사

	int len = strlen(c);
	for (int k = len - 1, l = 1; k >= 0; k--)	// 각 문자열을 거꾸로 읽어서 dec_num에 계산
	{
		if (temp[0] == '-') { // 음수를 나타내는 '-'를 생략하기 위함
			continue;
		}
		dec_num = dec_num + (int)(temp[k] - '0')*l;
		l = l * 10;	// 자리수를 계산하기위해 앞으로 나올 숫자들이 몇번째 자리인지 나타냄
	}
	return (temp[0] == '-') ? (-dec_num) : (dec_num);	// 음수일 경우 음수를 반환
}

int StrToHex(char* c)	// 16진수를 표현하는 String을 정수형으로 변환해서 반환
{
	int hex_num = 0;
	char temp[10];
	strcpy(temp, c);	// temp에 문자열 복사

	int len = strlen(temp);
	for (int k = len - 1, l = 1; k >= 0; k--)	// 각 문자열을 거꾸로 읽어 16진수를 정수형으로 변환
	{
		if (temp[k] >= '0' && temp[k] <= '9')
			hex_num = hex_num + (int)(temp[k] - '0')*l;
		else if (temp[k] >= 'A' && temp[k] <= 'F')	// 대문자 일경우
			hex_num = hex_num + (int)(temp[k] - 'A' + 10)*l;
		else if (temp[k] >= 'a' && temp[k] >= 'f')	// 소문자 일경우
			hex_num = hex_num + (int)(temp[k] - 'a' + 10)*l;
		else;
		l = l * 16; // 자리수를 계산하기위해 앞으로 나올 숫자들이 몇번째 자리인지 나타냄
	}
	return (hex_num);
}

double StrToFloat(char* c) {
	double float_num = 0;
	int len = strlen(c);
	for (int i = len - 1; i >= 0; i--) {
		float_num /= 10.0;
		float_num += (c[i] - '0')/10.0;
	}
	return float_num;
}

unsigned long long ConvertFloatNum(char * operand) {
	// Floating Number로 바꾸기 위한 함수
	int dec_size = 0, b = 0, k = 0;
	// dec_size : 정수부가 차지하는 비트수
	// b : 소수부가 차지하는 비트수
	// k : 만약 정수부가 0이라면 소수부 시작지점까지의 지수 
	int i = 0, j = 0;	// 인덱스 변수
	unsigned long long int s = 0, dec = 0, f = 0, e = 0x400;
	// 48비트는 unsigned long long int로 표현할 수 있기 때문에 변경
	// s : 음수, 양수를 표현하기 위한 변수 (음수 : 1, 양수 : 0)
	// dec : 정수부를 표현하는 부분
	// f : 소수부를 표현하는 부분
	// e : 지수부를 표현하는 부분
	double frac = 0;	// 연산을 위해 fraction을 표현하는 부분
	char temp[1000];	// 정수부, 소수부를 잘라내기 위한 임시 변수

	if (ReadFlag(operand)) {	// 문자열 맨 앞에 플래그비트가 있을 경우 이를 생략하기 위함.
		operand += 1;
	}
	if (operand[0] == '-') {	// 음수일 때 분기처리
		s = 1;	// 음수임을 표현
		operand += 1;	// '-' 제거
	}

	// 지수를 계산하기 위한 자리수 재기와 정수부, 소수부 나눠서 int형에 저장
	do {
		if (operand[i] == '.') {	// 정수부 잘라서 저장
			temp[j] = '\0';
			dec = StrToDec(temp);	// 정수부 계산
			if (dec > 0) {	// 0일 경우 분기처리 (무한대)
				dec_size = log2(dec) + 1;
			}
			j = 0;
		}
		else if (operand[i] == '\0') {
			// 소수부 계산
			temp[j] = '\0';
			frac = StrToFloat(temp);	// 소수부 double형으로 저장
			while (frac == 0 || 36 - dec_size > b) {
				// 더이상 곱할 소수부가 없을 경우나 표현가능한 비트 수가 넘어갈 경우 반복문 탈출
				frac *= 2; // 소수를 2 곱함
				f <<= 1;	// 표현할 소수부에 1비트의 자리를 만듦
				if (f != 0 || dec_size != 0) {
					// f가 어떤 비트라도 들어가 있거나 정수부의 길이가 0이 아닐때 b 증가
					// 즉, 0.00357 같은 경우 3이 나타날 때부터 표현할 비트에 저장하기 위함
					b++;	// 표현된 비트 수 1증가
				}

				if ((int)frac >= 1) {	// 곱한 소수가 1.xxx형태일 경우
					frac -= 1;	// 0.xxx형태로 변경
					f += 1;	// f의 맨 오른쪽 비트에 1증가
				} else if (f == 0) {	// f가 여전히 0일 경우 정수부가 0일때 지수표현을 위해 k에 1증가 
					k += 1;
				}
			}
		}
		else {
			temp[j++] = operand[i];
		}
	} while (operand[i++] != '\0');

	e += (dec_size > 0) ? (dec_size - 1) : (-k);	// 지수 부분 표현

	if ((dec + frac) == 0) {
		// 정수부와 소수부 모두 0일 경우 0을 반환
		return 0;
	}

	if (dec_size > 36) {	// 정수부가 36비트가 넘어갈 경우 넘는 비트 잘라내기
		dec >>= (dec_size - 36);
		dec_size = 36;
	}
	else {	// 정수부가 36비트가 안될 경우 소수부의 맨앞으로 당기기
		dec <<= (36 - dec_size);
	}

	if (b >= (36 - dec_size)) {	// 정수부가 차지한 부분을 제외한 비트에 소수가 다 들어가지 않을 경우 비트 잘라내기
		f >>= (b - (36 - dec_size));
	}
	else {	// 정수부가 차지한 부분을 제외한 비트에 소수가 들어가고도 남을 경우 비트 당기기
		f <<= ((36 - dec_size) - b);
	}
	return (s << 47) + (e << 36) + dec + f;	// 표현된 부동소수점 반환
}

int ComputeLen(char* c) {	// 아스키 코드나 16진수의 길이를 계산
	unsigned int b;	// 길이를 저장하기위한 변수 (byte 단위)
	char len[32];

	strcpy(len, c);
	if (len[0] == 'C' || len[0] == 'c' && len[1] == '\'') {	// C'로 시작할 경우
		for (b = 2; b <= strlen(len); b++) {
			// 글자 읽기
			if (len[b] == '\'') {
				b -= 2;	// 마지막 '를 만났을 경우 그 길이를 맨앞 C'의 두글자를 제외한 길이를 가지고 반복문 탈출
				break;
			}
		}
	}
	if (len[0] == 'X' || len[0] == 'x' && len[1] == '\'')	// X'로 시작할 경우
		b = 1;	// 무조건 1바이트
	return (b);
}

void CreateSymbolTable() {	// 심볼테이블 파일 생성
	int loop;
	FILE *fptr_sym;
	fptr_sym = fopen("symtab.list", "w");	// 심볼테이블 파일을 쓰기 형태로 엶

	if (fptr_sym == NULL)
	{
		printf("ERROR: Unable to open the symtab.list.\n");	// 심볼테이블 파일을 쓸 수 없을 경우 예외처리
		exit(1);
	}
	
	// 각 Column 들의 제목을 출력
	// 콘솔창과 파일에 모두 출력
	printf("%-10s\t%-4s\n", "LABEL", "LOC");
	fprintf(fptr_sym, "%-10s\t%-4s\n", "LABEL", "LOC");
	for (loop = 0; loop < SymtabCounter; loop++) {
		// 심볼테이블의 레코드들을 각각 출력
		// 콘솔창과 파일에 모두 출력
		printf("%-10s\t%04X\n", SYMTAB[loop].Label, SYMTAB[loop].Address);
		fprintf(fptr_sym, "%-10s\t%04X\n", SYMTAB[loop].Label, SYMTAB[loop].Address);
	}
	fclose(fptr_sym);	// 파일 출력이 끝났으므로 close
}

void CreateProgramList() {	// 리스트 파일 생성
	int loop;
	int len;	// 문자나 16진수일 경우 길이 계산을 위한 변수
	FILE *fptr_list;

	fptr_list = fopen("sic.list", "w");

	if (fptr_list == NULL)
	{
		printf("ERROR: Unable to open the sic.list.\n");	// 리스트 파일을 쓸 수 없을 경우 예외처리
		exit(1);
	}

	// 리스트 파일 내용 출력
	fprintf(fptr_list, "%-4s\t%-10s%-10s%-10s\t%s\n", "LOC", "LABEL", "OPERATOR", "OPERAND", "OBJECT CODE");	// 각 Column 들의 제목
	for (loop = 0; loop<ArrayIndex; loop++)
	{
		len = 0;
		fprintf(fptr_list, "%04X\t%-10s%-10s%-10s\t", IMRArray[loop]->Loc, IMRArray[loop]->LabelField, IMRArray[loop]->OperatorField, IMRArray[loop]->OperandField);	// 모든 코드들의 공통되는 부분
		if (!strcmp(IMRArray[loop]->OperatorField, "START")
			|| !strcmp(IMRArray[loop]->OperatorField, "RESW")
			|| !strcmp(IMRArray[loop]->OperatorField, "RESB")
			|| !strcmp(IMRArray[loop]->OperatorField, "BASE")
			|| !strcmp(IMRArray[loop]->OperatorField, "END")
			|| !strcmp(IMRArray[loop]->OperatorField, "EXTREF")
			|| !strcmp(IMRArray[loop]->OperatorField, "EXTDEF"))
			// Object code 출력이 필요없는 부분들 object code 생략
			fprintf(fptr_list, "\n");
		else if (SearchOptab(IMRArray[loop]->OperatorField)) {
			// operator가 OPTAB에 존재할 경우
			if (OPTAB[Counter].Format == '3') {	// 해당 명령어가 3/4형식 명령어일 경우
				if (ReadFlag(IMRArray[loop]->OperatorField)) {	// 명령어에 '+'가 붙은 경우 (4형식 명령어인지 판단하기 위함)
					fprintf(fptr_list, "%08X\n", IMRArray[loop]->ObjectCode);	// 4형식일 때 4바이트 출력
				} else {
					fprintf(fptr_list, "%06X\n", IMRArray[loop]->ObjectCode);	// 3형식일 때 3바이트 출력
				}
			} else if (OPTAB[Counter].Format == '2') {	// 해당 명령어가 2형식 명령어일 경우
				fprintf(fptr_list, "%04X\n", IMRArray[loop]->ObjectCode);	// 2바이트 출력
			} else if (OPTAB[Counter].Format == '1') {	// 해당 명령어가 1형식 명령어일 경우
				fprintf(fptr_list, "%02X\n", IMRArray[loop]->ObjectCode);	// 1바이트 출력
			}
		} else {
			if (isFloatNum(IMRArray[loop]->OperandField)) {
				// 부동소수점이므로 6바이트로 표현
				fprintf(fptr_list, "%012llX\n", IMRArray[loop]->ObjectCode);
			} else {
				// C'XX' 혹은 X'XX' 일때 예외처리
				len = ComputeLen(IMRArray[loop]->OperandField);	// C, ', ' 혹은 X, ', '를 제외한 원소들이 몇바이트인지 계산하기 위함
				if (len == 1) {	// 1바이트의 경우
					fprintf(fptr_list, "%02X\n", IMRArray[loop]->ObjectCode);	// 1바이트 출력
				}
				else if (len == 2) {	// 2바이트의 경우
					fprintf(fptr_list, "%04X\n", IMRArray[loop]->ObjectCode);	// 2바이트 출력
				}
				else if (len == 3) {	// 3바이트의 경우
					fprintf(fptr_list, "%06X\n", IMRArray[loop]->ObjectCode);	// 3바이트 출력
				}
				else {
					fprintf(fptr_list, "\n");	// 그외의 경우 object code 생략
				}
			}
		}
	}
	fclose(fptr_list);	// 리스트 파일 출력이 종료되어서 파일 저장
}

void CreateObjectCode() {	// 목적파일 생성
	// 목적파일 생성에 사용할 임시변수 선언
	int first_address;
	int last_address;
	int temp_address;
	unsigned long long int temp_objectcode[30];	// 6바이트 목적코드를 담기위해
	int first_index;
	int last_index;
	int x, xx;
	int loop;

	char temp_operator[12][10];
	char temp_operand[12][10];

	FILE *fptr_obj;
	fptr_obj = fopen("sic.obj", "w");	// sic.obj 파일을 쓰기형태로 선언
	if (fptr_obj == NULL)
	{
		printf("ERROR: Unable to open the sic.obj.\n");	// 목적파일을 쓸 수 없을 경우 예외처리
		exit(1);
	}

	printf("Creating Object Code...\n\n");

	loop = 0;	// 반복을 위한 인덱스 변수
	if (!strcmp(IMRArray[loop]->OperatorField, "START"))	// 중간파일의 첫번째 원소가 START일 때
	{
		// 헤더 레코드 작성 (프로그램 이름, 시작주소, 프로그램 길이)
		// 콘솔창과 파일 둘다 출력
		printf("H%-6s%06X%06X\n", IMRArray[loop]->LabelField, start_address, program_length);
		fprintf(fptr_obj, "H^%-6s^%06X^%06X\n", IMRArray[loop]->LabelField, start_address, program_length);
		loop++;
	}

	while (1)	// 무한루프 시작
	{
		first_address = IMRArray[loop]->Loc;	// 한줄의 시작주소를 저장
		last_address = IMRArray[loop]->Loc + 29;	// 1D개의 바이트를 출력할 수 있으므로 최대 29 바이트 출력하는 한계 설정
		first_index = loop;	// 반복문을 돌기위한 첫번째 인덱스값을 초기화

		// 출력할 수 있는 길이 계산
		for (x = 0, temp_address = first_address; temp_address <= last_address; loop++) {
			// END 어셈블러 지시자가 나오거나 한줄에 출력할 수 있는 양의 한계에 도달했을 때 for문 종료
			// x : 한 줄안에 출력할 수 있는 목적코드 최대 개수

			if (!strcmp(IMRArray[loop]->OperatorField, "END"))	// END 어셈블러 지시자를 만나면 for문 탈출
				break;
			else if (strcmp(IMRArray[loop]->OperatorField, "RESB")
				&& strcmp(IMRArray[loop]->OperatorField, "RESW")
				&& strcmp(IMRArray[loop]->OperatorField, "BASE")
				&& strcmp(IMRArray[loop]->OperatorField, "NOBASE")
				&& strcmp(IMRArray[loop]->OperatorField, "EXTREF")
				&& strcmp(IMRArray[loop]->OperatorField, "EXTDEF")) {
				// 목적코드가 없는 어셈블러 지시자를 제외한 나머지들을 출력하기 위해 저장
				// temp_objectcode : 목적코드를 저장
				// temp_operator : Operator Mnemonic 저장
				// temp_operand : Operand Mnemonic 저장
				temp_objectcode[x] = IMRArray[loop]->ObjectCode;
				strcpy(temp_operator[x], IMRArray[loop]->OperatorField);
				strcpy(temp_operand[x], IMRArray[loop]->OperandField);
				last_index = loop + 1;	// 한줄에 표현할 수 있는 목적코드의 길이를 계산하기 위해 저장
				x++; // 명령어 갯수 1 증가
			}
			// 다음번 명령어의 시작점이 한계점인지 검사하기 위한 저장
			temp_address = IMRArray[loop + 1]->Loc;
			// 다음번 명령어가 출력되어도 출력할 수 있는 양의 한계를 넘지 않는 지 검사 (형식이 다를 수 있기 때문)
			if (SearchOptab(IMRArray[loop + 1]->OperatorField)) {
				if (ReadFlag(IMRArray[loop + 1]->OperatorField)) {	// 4형식 명령어일 경우
					temp_address += 1;	// 1바이트 추가
				}
				temp_address += OPTAB[Counter].Format - '0';	// 각각 명령어 형식만큼 추가
			} else {
				if (!strcmp(IMRArray[loop + 1]->OperatorField, "WORD")
					|| !strcmp(IMRArray[loop + 1]->OperatorField, "BYTE")) {
					if (isFloatNum(IMRArray[loop + 1]->OperandField)) {
						// 부동소수점일 경우 6바이트 추가
						temp_address += 6;
					} else {
						// C'XX' 혹은 X'XX' 일 경우 출력되어도 가능한 지 검사하기 위함
						temp_address += ComputeLen(IMRArray[loop + 1]->OperandField);
					}
				}
			}
		}

		// 텍스트 레코드로 한줄의 시작주소와 목적코드의 길이 계산해서 출력
		// 콘솔창과 파일에 모두 출력
		if ((IMRArray[last_index]->Loc - IMRArray[first_index]->Loc) == 0) {
			if (!strcmp(IMRArray[loop]->OperatorField, "END"))	// END 어셈블러 지시자를 만났을 경우 while문 탈출
				break;
			else
				continue;
		}
		printf("T%06X%02X", first_address, (IMRArray[last_index]->Loc - IMRArray[first_index]->Loc));
		fprintf(fptr_obj, "T^%06X^%02X", first_address, (IMRArray[last_index]->Loc - IMRArray[first_index]->Loc));

		for (xx = 0; xx<x; xx++) {
			// 한줄에 들어갈 수 있는 최대의 목적코드를 출력하기 위한 반복문
			// 콘솔창과 파일에 모두 출력
			if ((strcmp(temp_operator[xx], "BYTE") == 0)) {
				if (temp_operand[xx][0] == 'X' || temp_operand[xx][0] == 'x') {
					// 16진수로 표현한 1바이트의 값일 경우 1바이트의 형식에 맞춰 출력
					printf("%02X", temp_objectcode[xx]);
					fprintf(fptr_obj, "^%02X", temp_objectcode[xx]);
				} else if (isFloatNum(temp_operand[xx])) {
					// 부동소수점일 경우 6바이트의 형식에 맞춰 출력
					printf("%012llX", temp_objectcode[xx]);
					fprintf(fptr_obj, "^%012llX", temp_objectcode[xx]);
				}
			}
			else {
				// 명령어의 형식에 따라 달라지는 길이에 맞춰서 출력
				if (SearchOptab(temp_operator[xx])) {
					// operator가 OPTAB에 존재할 경우
					if (OPTAB[Counter].Format == '3') {	// 해당 명령어가 3/4형식 명령어일 경우
						if (ReadFlag(temp_operator[xx])) {	// 명령어에 '+'가 붙은 경우 (4형식 명령어인지 판단하기 위함)
							// 4형식일 때 4바이트에 출력
							printf("%08X", temp_objectcode[xx]);
							fprintf(fptr_obj, "^%08X", temp_objectcode[xx]);
						}
						else {
							// 3형식일 때 3바이트에 출력
							printf("%06X", temp_objectcode[xx]);
							fprintf(fptr_obj, "^%06X", temp_objectcode[xx]);
						}
					}
					else if (OPTAB[Counter].Format == '2') {
						// 2형식 명령어의 경우 2바이트에 출력
						printf("%04X", temp_objectcode[xx]);
						fprintf(fptr_obj, "^%04X", temp_objectcode[xx]);
					}
					else if (OPTAB[Counter].Format == '1') {
						// 1형식 명령어의 경우 1바이트에 출력
						printf("%02X", temp_objectcode[xx]);
						fprintf(fptr_obj, "^%02X", temp_objectcode[xx]);
					}
				} else {
					if (isFloatNum(temp_operand[xx])) {
						// 부동소수점일 경우 6바이트의 형식에 맞춰 출력
						printf("%012llX", temp_objectcode[xx]);
						fprintf(fptr_obj, "^%012llX", temp_objectcode[xx]);
					} else {
						// 명령어가 아닐 경우 기본 3바이트 형식에 맞춰 출력
						printf("%06X", temp_objectcode[xx]);
						fprintf(fptr_obj, "^%06X", temp_objectcode[xx]);
					}
				}
			}
		}

		// 한줄 출력이 끝난 후 개행
		// 콘솔창과 파일에 모두 출력
		printf("\n");
		fprintf(fptr_obj, "\n");

		if (!strcmp(IMRArray[loop]->OperatorField, "END"))	// END 어셈블러 지시자를 만났을 경우 while문 탈출
			break;
	}

	// 수정레코드 출력부분
	for (loop = 0; loop < RLDCounter; loop++) {
		// RLD의 모든 레코드들을 수정레코드로 출력
		// 재배치가 필요함을 로더에게 알리기위함
		printf("M%06X%02X\n", RLDArray[loop].Address, RLDArray[loop].Nibbles);
		fprintf(fptr_obj, "M^%06X^%02X\n", RLDArray[loop].Address, RLDArray[loop].Nibbles);
	}

	// 엔드 레코드를 통해 프로그램의 시작주소를 출력
	// 콘솔창과 파일에 모두 출력
	printf("E%06X\n\n", start_address);
	fprintf(fptr_obj, "E^%06X\n\n", start_address);

	fclose(fptr_obj);	// obj 파일 쓰기 종료
}

/******************************* MAIN FUNCTION *******************************/
void main(void)
{
	FILE* fptr;

	char filename[10];
	char label[32];
	char opcode[32];
	char operand[32];

	int loc = 0;
	int line = 0;
	int loop;
	int is_empty_line;
	int is_comment;
	int loader_flag = 0;
	int start_line = 0;

	// Intro Part
	printf(" ******************************************************************************\n");
	printf(" * Program: SIC ASSEMBYER                                                     *\n");
	printf(" *                                                                            *\n");
	printf(" * Procedure:                                                                 *\n");
	printf(" *   - Enter file name of source code.                                        *\n");
	printf(" *   - Do pass 1 process.                                                     *\n");
	printf(" *   - Do pass 2 process.                                                     *\n");
	printf(" *   - Create \"program list\" data on sic.list.(Use Notepad to read this file) *\n");
	printf(" *   - Create \"object code\" data on sic.obj.(Use Notepad to read this file)   *\n");
	printf(" *   - Also output object code to standard output device.                     *\n");
	printf(" ******************************************************************************\n");


	printf("\nEnter the file name you want to assembly (sic.asm):");
	
	/******************************TEST INPUT********************************************/
	printf("%s\n", TEST_FNAME);
	//scanf("%s", filename);
	strcpy(filename, TEST_FNAME);
	/******************************TEST INPUT********************************************/
	
	fptr = fopen(filename, "r");
	if (fptr == NULL)	// 소스코드 파일 읽기 실패했을 경우 예외처리
	{
		printf("ERROR: Unable to open the %s file.\n", filename);
		exit(1);
	}

	/********************************** PASS 1 ***********************************/
	printf("Pass 1 Processing...\n");
	while (fgets(Buffer, 256, fptr) != NULL)	// 소스코드 파일에서 코드 읽기
	{
		is_empty_line = strlen(Buffer);

		Index = 0;
		j = 0;
		strcpy(label, ReadLabel());
		if (Label[0] == '.')	// 해당 소스코드가 주석인지 아닌지 확인
			is_comment = 1;
		else
			is_comment = 0;

		if (is_empty_line>1 && is_comment != 1)
		{
			// 인덱스 변수들 초기화
			Index = 0;
			j = 0;

			IMRArray[ArrayIndex] = (IntermediateRec*)malloc(sizeof(IntermediateRec));/* [A] */	// 중간파일 동적할당
			IMRArray[ArrayIndex]->LineIndex = ArrayIndex;	// 소스코드 상의 행 삽입
			strcpy(label, ReadLabel());	// 레이블을 읽어 Label에 저장
			strcpy(IMRArray[ArrayIndex]->LabelField, label);	// 레이블을 중간파일에 저장
			SkipSpace();	// 공백 제거

			if (line == start_line)	// 프로그램의 시작 지점이 첫줄이 아닐 경우 (첫번째 줄이 주석일 경우) 예외처리
			{
				strcpy(opcode, ReadOprator());	// Mnemonic 읽기
				strcpy(IMRArray[ArrayIndex]->OperatorField, opcode); /* [A] */	// 읽은 Mnemonic을 중간파일에 저장
				if (!strcmp(opcode, "START")) {	// 시작주소 초기화
					SkipSpace();
					strcpy(operand, ReadOperand());
					strcpy(IMRArray[ArrayIndex]->OperandField, operand);/* [A] */
					LOCCTR[LocctrCounter] = StrToHex(operand);
					start_address = LOCCTR[LocctrCounter];
				} else {	// 시작 주소가 명시되어있지 않을 경우 0으로 초기화
					LOCCTR[LocctrCounter] = 0;
					start_address = LOCCTR[LocctrCounter];
				}
			} else {
				strcpy(opcode, ReadOprator());	// OP Code 읽기
				strcpy(IMRArray[ArrayIndex]->OperatorField, opcode);	// 중간파일에 OP code 복사
				SkipSpace();	// OP code와 피연산자 사이의 공백 제거
				strcpy(operand, ReadOperand());	// 피연산자 부분 읽기
				strcpy(IMRArray[ArrayIndex]->OperandField, operand);	// 중간파일에 피연산자 복사

				if (strcmp(opcode, "END"))	// OP code가 END 어셈블러 지시자일 경우
				{
					if (label[0] != '\0')	// 레이블이 있을 경우
					{
						if (SearchSymtab(label))	// 같은 이름의 레이블이 있는지 찾음
						{
							// 만약 같은 이름의 레이블이 있을 경우 Alert하고 프로그램 종료
							fclose(fptr);
							printf("ERROR: Duplicate Symbol\n");
							FoundOnSymtab_flag = 0;
							exit(1);
						}
						RecordSymtab(label);	// 같은이름이 없으므로 심볼테이블에 추가
					}

					if (SearchOptab(opcode)) {	// OP Code가 OPTAB에 있을 경우 명령어 형식만큼 메모리 확보
						LOCCTR[LocctrCounter] = loc + (int)(OPTAB[Counter].Format - '0');
						if (ReadFlag(opcode)) {
							// 4형식 명령어일 경우
							LOCCTR[LocctrCounter] += 1;	// 기존에 1바이트 더 추가
						}
					}
					else if (!strcmp(opcode, "WORD")) {	// 3바이트(1 WORD) 확보
						if (isFloatNum(operand)) {
							// 부동소수점은 6바이트 사용하므로
							LOCCTR[LocctrCounter] = loc + 6;
						} else {
							LOCCTR[LocctrCounter] = loc + 3;
						}
					} else if (!strcmp(opcode, "RESW"))	// 피연산자 갯수의 WORD 만큼 메모리 확보
						LOCCTR[LocctrCounter] = loc + 3 * StrToDec(operand);
					else if (!strcmp(opcode, "RESB"))	// 피연산자 갯수의 바이트만큼 메모리 확보
						LOCCTR[LocctrCounter] = loc + StrToDec(operand);
					else if (!strcmp(opcode, "BYTE")) {	// 1바이트 확보
						if (isFloatNum(operand)) {
							// 부동소수점은 6바이트 사용하므로
							LOCCTR[LocctrCounter] = loc + 6;
						} else {
							LOCCTR[LocctrCounter] = loc + ComputeLen(operand);
						}
					} else if (!strcmp(opcode, "BASE")
						|| !strcmp(opcode, "NOBASE")
						|| !strcmp(opcode, "EXTDEF")
						|| !strcmp(opcode, "EXTREF")) {
						// 별달리 처리가 필요한 Assembler Directive가 아닐 경우 Loc을 대입
						LOCCTR[LocctrCounter] = loc;
					}
					else { // 정의되지 않은 OP code이므로 경고후 프로그램 종료
						fclose(fptr);
						printf("ERROR: Invalid Operation Code\n");
						exit(1);
					}
				}
			}
			loc = LOCCTR[LocctrCounter];	// loc을 다시 설정하고 다음 루프를 준비
			IMRArray[ArrayIndex]->Loc = LOCCTR[LocctrCounter - 1];	// 중간파일에 해당 코드의 메모리 번지 기록
			LocctrCounter++;	// LOCCTR를 접근하는 인덱스 변수 값 증가
			ArrayIndex++;	// 다음 코드를 읽기 위한 중간파일의 인덱스 변수 값 증가
		}
		
		if (is_comment == 1) {	// 첫 줄이 주석일 경우 시작이 되지 않는 오류 수정
			start_line += 1;
		}

		FoundOnOptab_flag = 0;	// flag 변수 초기화
		line += 1;	// 소스 행 1 증가
	}
	program_length = LOCCTR[LocctrCounter - 2] - LOCCTR[0];
	// END 지시자를 만났을 경우 END 지시자 바로 이전 소스코드의 메모리 위치와 시작주소를 빼서 총 프로그램 길이 계산

	// Pass 1에서 생성된 심볼테이블 출력
	CreateSymbolTable();

	/********************************** PASS 2 ***********************************/
	printf("Pass 2 Processing...\n");

	unsigned long inst_fmt;		// 최종 목적 코드
	unsigned long inst_fmt_opcode;	// 목적코드의 op code 부분을 나타내는 변수
	unsigned long inst_fmt_sign;	// Immediate, Indirect Addressing Mode를 나타내는 플래그비트 변수 
	unsigned long inst_fmt_relative;	// relative addressing mode를 나타내는 플래그비트 변수
	unsigned long inst_fmt_index;	// index Addressing Mode를 나타내는 변수
	unsigned long inst_fmt_extended;	// 플래그비트 e를 나타내는 플래그비트 변수
	unsigned long inst_fmt_address;	// 피연산자 부분을 나타내는 변수 (필요에 의해서 직접적인 상수가 들어갈 수도 있다. ex. Immediate Addressing Mode)
	int inst_fmt_byte;		// 몇형식 명령어인지 나타내는 변수 (바이트 수)
	int i, regCharIdx;
	char regName[3];	// 레지스터 이름을 비교하기위해 담아놓는 임시변수
	
	int diff = 0;	// 주소에 들어갈 변위를 저장하는 변수 (음수가 나올 수 있으므로 unsigned 가 아니다)
	int base_register = -1;	// BASE 어셈블러 지시자가 나오지 않을 경우 base relative addressing mode를 사용하지 못하게 하도록 하기 위한 기본값 -1

	for (loop = 1; loop<ArrayIndex; loop++) {	// 중간파일을 순차적으로 읽음
		// 각 변수들 초기화
		inst_fmt_opcode = 0;
		inst_fmt_sign = 0;
		inst_fmt_relative = 0;
		inst_fmt_index = 0;
		inst_fmt_extended = 0;
		inst_fmt_address = 0;
		inst_fmt_byte = 0;
		regName[0] = '\0';

		strcpy(opcode, IMRArray[loop]->OperatorField);	// op code 부분 복사

		if (SearchOptab(opcode)) {	// opcode 찾기
			if (!strcmp(OPTAB[Counter].Mnemonic, "RSUB")) {
				// RSUB는 3형식 명령어이지만 상관이 없으므로
				IMRArray[loop]->ObjectCode = (OPTAB[Counter].ManchineCode << 16);
				continue;
			}
			inst_fmt_opcode = OPTAB[Counter].ManchineCode;	// opcode의 목적코드 복사
			inst_fmt_byte = OPTAB[Counter].Format - '0';	// 해당 명령어가 몇 바이트를 사용하는 지 저장
			if (inst_fmt_byte == 3 && ReadFlag(opcode)) {	// 만약 4형식 명령어일 경우 분기처리
				inst_fmt_byte = 4;	// 4형식 명령어
				inst_fmt_extended = 0x00100000;	// 플래그 비트 e가 1임.
			}
			inst_fmt_opcode <<= (8 * (inst_fmt_byte - 1));	// 각 명령어 형식에 맞게 왼쪽으로 Shift
			IMRArray[loop]->ObjectCode = inst_fmt_opcode;
			strcpy(operand, IMRArray[loop]->OperandField);
			
			if (ReadFlag(operand)) {
				if (inst_fmt_byte <= 2) {
					fclose(fptr);
					printf("ERROR: Invalid Addressing Mode\n");
					exit(1);
				}
				if (Flag == SHARP) {	// Immediate Addressing Mode
					inst_fmt_sign = 0x010000;
				}
				else if (Flag == AT) {	// Indirect Addressing Mode
					inst_fmt_sign = 0x020000;
				}
				inst_fmt_sign <<= 8 * (inst_fmt_byte - 3);	// 바이트 수 만큼 왼쪽으로 Shift
			}
			else if (inst_fmt_byte >= 3) {	// 3/4형식 명령어 Simple Addressing Mode
				inst_fmt_sign = 0x030000;
				inst_fmt_sign <<= 8 * (inst_fmt_byte - 3);	// 바이트 수 만큼 왼쪽으로 Shift
			}
			
			if (inst_fmt_byte >= 3) {
				// 3/4형식 명령어 일경우
				if (operand[strlen(operand) - 2] == ',' && operand[strlen(operand) - 1] == 'X') {	// index addressing Mode
					inst_fmt_index = 0x008000;	// index addressing mode 플래그 비트 x에 1
					inst_fmt_index <<= 8 * (inst_fmt_byte - 3);	// 4형식 명령어의 경우 8비트씩 왼쪽으로 이동
					operand[strlen(operand) - 2] = '\0';	// ,X 부분 삭제
				}

				if (SearchSymtab(operand)) {
					// 심볼테이블에서 해당 피연산자를 찾을 수 있을 경우
					if (inst_fmt_byte == 4) {	// extended instruction의 주소 지정
						inst_fmt_address = SYMTAB[SymIdx].Address;
						RecordRLD(IMRArray[loop]->OperatorField, IMRArray[loop]->Loc);	// 재배치가 필요하므로 RLD에 추가
					}
					else {	// relative Addressing mode
						// PC의 값 저장
						// 명령어 실행 시점에서 PC의 값 계산 (다음 명령어의 메모리 위치)
						diff = SYMTAB[SymIdx].Address - IMRArray[loop]->Loc - inst_fmt_byte;
						if (diff >= -2048 && diff < 2048) {
							// pc relative일 경우
							// 음수면 12비트로 표현해야하므로 12비트의 음수로 변환해주기 위한 함수 호출
							// 음수가 아니라면 함수 로직 내에서 그대로 반환되므로 음수가 아니어도 함수는 호출
							inst_fmt_address = 0x002000;
							inst_fmt_address += ConvertNumber(diff, 3);
						}
						else {	// PC relative addressing mode가 실패했을 경우 Base relative addressing mode 시도
							// base relative addressing mode에 기반하여 변위 다시 계산
							diff = SYMTAB[SymIdx].Address - base_register;
							if (base_register != -1 && diff >= 0 && diff < 4096) {	// Base relative addressing mode가 가능할경우
								// base relative addressing mode로 어셈블
								inst_fmt_address = 0x004000;
								inst_fmt_address += diff;
							}
							else {
								// pc 혹은 base relative addressing mode로 어셈블 불가할 경우 예외처리
								fclose(fptr);
								printf("ERROR: CANNOT present relative addressing mode[line : %d]\n", IMRArray[loop]->LineIndex);
								exit(1);
							}
						}
					}
				}
				else {
					// 심볼테이블에서 피연산자를 찾을 수 없을 때
					ReadFlag(operand);	// 피연산자에 붙어있는 플래그가 #인지 검사하기위함 
					if (Flag == SHARP && isNum(operand)) {	// 피연산자가 숫자로 이루어져있고, immediate addressing mode인지 검사
						inst_fmt_address = ConvertNumber(StrToDec(operand), (inst_fmt_byte == 4) ? 5 : 3);	// 피연산자가 숫자(십진수)로 이루어져 있으므로 그 값을 주소에 대입
					}
					else {
						// 심볼테이블에서 피연산자를 찾을 수 없고 숫자로 이루어져있지도 않기 때문에
						fclose(fptr);
						printf("ERROR: Label isn't exist [%s]\n", operand);
						exit(1);
					}
				}
			}
			else if (inst_fmt_byte == 2) {	// 2형식 명령어일 경우
				i = 0; regCharIdx = 0;	// 인덱스 변수들 초기화
				do {	// 피연산자를 읽어 레지스터들에 맞는 목적코드 작성
					if (operand[i] == ',' || operand[i] == '\0') {	// 앞서 나온 레지스터를 읽을 준비가 되었을 경우
						regName[regCharIdx] = '\0';	// 단순 문자배열을 문자열로 끊고
						if (operand[i] == ',') {	// 기존에 기록된 레지스터의 아이디가 존재할경우 4비트를 왼쪽으로 밀고 기록
							inst_fmt_address <<= 4;
						}

						if (SearchRegTab(regName)) {	// 미리 정의된 레지스터 테이블에서 읽음
							inst_fmt_address += REG_TAB[RegIdx].id;	// 레지스터 테이블에 해당 레지스터가 있을 경우 그 아이디를 목적코드에 추가 
						}
						else { 
							if (!strcmp(OPTAB[Counter].Mnemonic, "SVC") || !strcmp(OPTAB[Counter].Mnemonic, "SHIFTL") || !strcmp(OPTAB[Counter].Mnemonic, "SHIFTR")) {
								// 피연산자로 레지스터를 사용하지않고 숫자를 사용하는 경우
								if (isNum(regName)) {	// 피연산자가 숫자라면
									inst_fmt_address += StrToDec(regName);	// 추가
								}
							}
							else { // RegTab에 없기 때문에 오류로 처리하고 프로그램 종료
								fclose(fptr);
								printf("ERROR: Invalid Register\n");
								exit(1);
							}
						}
						regCharIdx = 0;	// 인덱스 변수 초기화
					}
					else if (operand[i] != ' ') {	// 공백일 경우 스킵하도록
						regName[regCharIdx++] = operand[i];	// 레지스터 이름 저장
					}
				} while (operand[i++] != '\0');

				if (!strcmp(OPTAB[Counter].Mnemonic, "CLEAR")
					|| !strcmp(OPTAB[Counter].Mnemonic, "TIXR")
					|| !strcmp(OPTAB[Counter].Mnemonic, "SVC")) {
					// 피연산자 형식이 다른 특정 명령어들에 한해 예외처리
					// 피연산자가 1개 일경우 기록후 4비트 왼쪽으로 이동
					inst_fmt_address <<= 4;
				}
			}

			// Object Code 병합
			inst_fmt = inst_fmt_opcode + inst_fmt_sign + inst_fmt_index + inst_fmt_relative + inst_fmt_extended + inst_fmt_address;
			IMRArray[loop]->ObjectCode = inst_fmt;
		}
		else if (!strcmp(opcode, "WORD")) {
			// 1 WORD의 크기에 10진수 대입
			strcpy(operand, IMRArray[loop]->OperandField);
			if (isFloatNum(operand)) {
				// 부동소수점이므로 부동소수점 계산
				IMRArray[loop]->ObjectCode = ConvertFloatNum(operand);
			} else {
				IMRArray[loop]->ObjectCode = StrToDec(operand);
			}
		}
		else if (!strcmp(opcode, "BYTE")) {
			// 1 Byte의 값을 여러개 혹은 한개 대입 (ASCII code 혹은 16진수)
			strcpy(operand, IMRArray[loop]->OperandField);
			IMRArray[loop]->ObjectCode = 0;

			if (isFloatNum(operand)) {
				// 부동소수점이므로 부동소수점 계산
				IMRArray[loop]->ObjectCode = ConvertFloatNum(operand);
			}
			else {
				// 값이 ASCII code일 경우 값 계산후 objectcode에 대입
				if (operand[0] == 'C' || operand[0] == 'c' && operand[1] == '\'') {
					for (int x = 2; x <= (int)(strlen(operand) - 2); x++) {
						IMRArray[loop]->ObjectCode = IMRArray[loop]->ObjectCode + (int)operand[x];
						IMRArray[loop]->ObjectCode <<= 8;
					}
				}

				// 값이 16진수일 경우 값 계산후 objectcode에 대입
				if (operand[0] == 'X' || operand[0] == 'x' && operand[1] == '\'') {
					char *operand_ptr;
					operand_ptr = &operand[2];
					*(operand_ptr + 2) = '\0';
					for (int x = 2; x <= (int)(strlen(operand) - 2); x++) {
						IMRArray[loop]->ObjectCode = IMRArray[loop]->ObjectCode + StrToHex(operand_ptr);
						IMRArray[loop]->ObjectCode <<= 8;
					}
				}

				IMRArray[loop]->ObjectCode >>= 8;	// 각 반복문 맨마지막에 1바이트 밀었던 것을 다시 원위치로 변경
			}
		}
		else if (!strcmp(opcode, "BASE")) {
			// BASE 어셈블러 지시자일 경우 해당 위치를 base 레지스터에 넣고 base relative addressing mode의 기준점으로 삼음
			strcpy(operand, IMRArray[loop]->OperandField);
			IMRArray[loop]->ObjectCode = 0;
			if (SearchSymtab(operand)) {
				base_register = SYMTAB[SymIdx].Address;
			} else {
				// 피연산자가 SymTab에 없기 때문에 오류로 처리하고 프로그램 종료 
				fclose(fptr);
				printf("ERROR: No Label in SYMTAB[%s]\n", operand);
				exit(1);
			}
		}
		else if (!strcmp(opcode, "NOBASE")) {
			strcpy(operand, IMRArray[loop]->OperandField);
			IMRArray[loop]->ObjectCode = 0;
			// base register 해제
			base_register = -1;
		}
		else if (!strcmp(opcode, "EXTDEF")) {
			
		}
		else if (!strcmp(opcode, "EXTREF")) {

		}
	}

	// 리스트 파일과 목적 파일 생성
	CreateProgramList();
	CreateObjectCode();

	// 메모리 동적할당 해제
	for (loop = 0; loop<ArrayIndex; loop++)
		free(IMRArray[loop]);

	printf("Compeleted Assembly\n");
	fclose(fptr); // 소스코드 파일 읽기 종료

	exit(0);	// exit을 안해서 프로그램이 비정상적으로 종료되는 버그 수정
}