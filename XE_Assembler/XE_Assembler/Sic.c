#include <stdio.h>
#include <stdlib.h>	
#include <string.h>	// 문자열 관리를 위함
#include <malloc.h>	// 동적할당을 위함

#define TEST_FNAME "x:\\2.asm"

/***************************** DECLERATE VARIABLE ****************************/
typedef struct OperationCodeTable
{
	char Mnemonic[8];	// 명령어의 형상 (ex. LDB, LDA, etc...)
	char Format;	// 명령어의 형식 (명령어의 길이)	3/4 형식은 편의상 3형식으로 표현하도록 설계했습니다.
	unsigned short int  ManchineCode;	// 해당 명령어의 목적 코드
}SIC_OPTAB;

typedef struct SymbolTable
{
	char Label[10];	// 레이블의 이름
	int Address;	// 레이블이 가리키는 주소
}SIC_SYMTAB;

// 레지스터 테이블을 구성하는 레지스터 레코드
typedef struct RegisterTable
{
	char name[3];	// 레지스터 Mnemonic
	int id;	// 레지스터의 고유번호
} SIC_XE_REGISTER;

typedef struct IntermediateRecord {	// 중간파일
	unsigned short int LineIndex;	// 소스코드의 행을 저장하는 변수
	unsigned short int Loc;	//  해당 명령어의 메모리상 위치
	unsigned long int ObjectCode;	//  Pass 2를 거쳐 Assemble된 목적코드
	char LabelField[32];	// 소스코드상 표기되어있는 레이블
	char OperatorField[32];	// 소스코드상 표기되어있는 Mnemonic
	char OperandField[32];	// 소스코드상 표기되어있는 피연산자
}IntermediateRec;

// 함수의 결과를 전달하기 위해 임시로 사용하는 전역변수들
int Counter;	// Opcode찾을 때 그 명령어의 위치를 가리키기 위한 변수
int RegIdx;		// 레지스터의 위치를 가리킴. Counter 변수와 역할 비슷
int SymIdx;		// Label의 위치를 가리킴. Counter 변수와 역할 비슷
int LOCCTR[100];	// 각 명령어들의 메모리를 세기위한 Location Counter
int LocctrCounter = 0;	// LOCCTR의 Index 변수
int Flag;
int Index;
int j;
int ManchineCode;
int SymtabCounter = 0;	// 심볼테이블의 갯수를 세고 가리키기 위한 변수
int start_address;	// 프로그램의 시작 주소
int program_length;	// 프로그램의 총 길이
int ArrayIndex = 0;	// 중간파일을 각각 가리키기 위한 Index 변수

unsigned short int FoundOnSymtab_flag = 0;	// 해당 레이블을 심볼테이블에서 찾았다는 것을 반환하기 위함
unsigned short int FoundOnOptab_flag = 0;	// 해당 Opcode의 Mnemonic을 OP 테이블에서 찾았다는 것을 반환하기 위함
unsigned short int FoundOnRegTab_flag = 0;	// 레지스터 테이블에 해당 레지스터의 형상이 있는지 확인

char Buffer[256];	// 각 소스코드를 읽기위한 버퍼 변수
char Label[32];	// 레이블을 임시로 저장하기 위한 변수
char Mnemonic[32];	// Mnemnic을 임시로 저장하기 위한 변수
char Operand[32];	// 피연산자를 임시로 저장하기 위한 변수
// 이 변수들은 모두 소스코드상의 표기법을 가짐

SIC_SYMTAB SYMTAB[20];	// 심볼테이블 변수
IntermediateRec* IMRArray[100];	// 중간파일 변수

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
	{ "JSUB",  '3',  0x4B },
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
		Flag = 1;	// extended instruction
		break;
	case '#':
		Flag = 2;	// immediate addressing mode
		break;
	case '@':
		Flag = 3;	// indirect addressing mode
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
			return 0;
		}
	}
	return 1;	// 모두 숫자이므로 1을 반환
}

unsigned long ConvertDiffForAddress(short diff) {
	if (diff >= 0) { // 양수이므로 그대로 반환
		return diff;
	}
	// 음수일경우 자료형의 크기가 12비트라고 가정하고 2's completion을 진행
	diff ^= 0xF000;	// 12비트 이후의 bit를 제거하기 위해서 총 16비트의 short형의 뒤 4비트를 0으로 만든다
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
		dec_num = dec_num + (int)(temp[k] - '0')*l;
		l = l * 10;	// 자리수를 계산하기위해 앞으로 나올 숫자들이 몇번째 자리인지 나타냄
	}
	return (dec_num);
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

int ComputeLen(char* c) {	// 아스키 코드나 16진수의 길이를 계산
	unsigned int b;
	char len[32];

	strcpy(len, c);
	if (len[0] == 'C' || len[0] == 'c' && len[1] == '\'') {
		for (b = 2; b <= strlen(len); b++) {
			if (len[b] == '\'') {
				b -= 2;
				break;
			}
		}
	}
	if (len[0] == 'X' || len[0] == 'x' && len[1] == '\'')
		b = 1;
	return (b);
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
	fprintf(fptr_list, "%-4s\t%-10s%-10s%-10s\t%s\n", "LOC", "LABEL", "OPERATOR", "OPERAND", "OBJECT CODE");
	for (loop = 0; loop<ArrayIndex; loop++)
	{
		len = 0;
		fprintf(fptr_list, "%04x\t%-10s%-10s%-10s\t", IMRArray[loop]->Loc, IMRArray[loop]->LabelField, IMRArray[loop]->OperatorField, IMRArray[loop]->OperandField);
		if (!strcmp(IMRArray[loop]->OperatorField, "START") || !strcmp(IMRArray[loop]->OperatorField, "RESW") || !strcmp(IMRArray[loop]->OperatorField, "RESB") || !strcmp(IMRArray[loop]->OperatorField, "BASE") || !strcmp(IMRArray[loop]->OperatorField, "END"))
			fprintf(fptr_list, "\n");
		else if (SearchOptab(IMRArray[loop]->OperatorField)) {
			if (OPTAB[Counter].Format == '3') {
				if (ReadFlag(IMRArray[loop]->OperatorField)) {
					fprintf(fptr_list, "%08x\n", IMRArray[loop]->ObjectCode);
				} else {
					fprintf(fptr_list, "%06x\n", IMRArray[loop]->ObjectCode);
				}
			} else if (OPTAB[Counter].Format == '2') {
				fprintf(fptr_list, "%04x\n", IMRArray[loop]->ObjectCode);
			} else if (OPTAB[Counter].Format == '1') {
				fprintf(fptr_list, "%02x\n", IMRArray[loop]->ObjectCode);
			}
		} else {
			len = (strlen(IMRArray[loop]->OperandField)-3)/2;	// C, ', ' 혹은 X, ', '를 제외한 것들이 몇바이트인지 계산하기 위함
			if (len == 1) {
				fprintf(fptr_list, "%02x\n", IMRArray[loop]->ObjectCode);
			} else if (len == 2) {
				fprintf(fptr_list, "%04x\n", IMRArray[loop]->ObjectCode);
			} else if (len == 3) {
				fprintf(fptr_list, "%06x\n", IMRArray[loop]->ObjectCode);
			} else {
				fprintf(fptr_list, "\n");
			}
		}
	}
	fclose(fptr_list);
}

void CreateObjectCode() {	// 목적파일 생성
	int first_address;
	int last_address;
	int temp_address;
	int temp_objectcode[30];
	int first_index;
	int last_index;
	int x, xx;
	int loop;

	char temp_operator[12][10];
	char temp_operand[12][10];

	FILE *fptr_obj;
	fptr_obj = fopen("sic.obj", "w");
	if (fptr_obj == NULL)
	{
		printf("ERROR: Unable to open the sic.obj.\n");	// 목적파일을 쓸 수 없을 경우 예외처리
		exit(1);
	}

	printf("Creating Object Code...\n\n");

	loop = 0;
	if (!strcmp(IMRArray[loop]->OperatorField, "START"))
	{
		printf("H%-6s%06x%06x\n", IMRArray[loop]->LabelField, start_address, program_length);
		fprintf(fptr_obj, "H^%-6s^%06x^%06x\n", IMRArray[loop]->LabelField, start_address, program_length);
		loop++;
	}

	while (1)
	{
		first_address = IMRArray[loop]->Loc;
		last_address = IMRArray[loop]->Loc + 27;
		first_index = loop;

		for (x = 0, temp_address = first_address; temp_address <= last_address; loop++)
		{
			if (!strcmp(IMRArray[loop]->OperatorField, "END"))
				break;
			else if (strcmp(IMRArray[loop]->OperatorField, "RESB") && strcmp(IMRArray[loop]->OperatorField, "RESW") && strcmp(IMRArray[loop]->OperatorField, "BASE"))
			{
				temp_objectcode[x] = IMRArray[loop]->ObjectCode;
				strcpy(temp_operator[x], IMRArray[loop]->OperatorField);
				strcpy(temp_operand[x], IMRArray[loop]->OperandField);
				last_index = loop + 1;
				x++;
			}
			else;
			temp_address = IMRArray[loop + 1]->Loc;
		}

		printf("T%06x%02x", first_address, (IMRArray[last_index]->Loc - IMRArray[first_index]->Loc));
		fprintf(fptr_obj, "T^%06x^%02x", first_address, (IMRArray[last_index]->Loc - IMRArray[first_index]->Loc));

		for (xx = 0; xx<x; xx++)
		{
			if ((strcmp(temp_operator[xx], "BYTE") == 0) && (temp_operand[xx][0] == 'X' || temp_operand[xx][0] == 'x')) {
				printf("%02x", temp_objectcode[xx]);
				fprintf(fptr_obj, "^%02x", temp_objectcode[xx]);
			}
			else {
				printf("%06x", temp_objectcode[xx]);
				fprintf(fptr_obj, "^%06x", temp_objectcode[xx]);
			}
		}

		printf("\n");
		fprintf(fptr_obj, "\n");

		if (!strcmp(IMRArray[loop]->OperatorField, "END"))
			break;
	}

	printf("E%06x\n\n", start_address);
	fprintf(fptr_obj, "E^%06x\n\n", start_address);
	fclose(fptr_obj);
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
	while (fgets(Buffer, 256, fptr) != NULL)	// 소스코드 파일에서 읽기
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
			}
			else
			{

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
					else if (!strcmp(opcode, "WORD"))	// 3바이트(1 WORD) 확보
						LOCCTR[LocctrCounter] = loc + 3;
					else if (!strcmp(opcode, "RESW"))	// 피연산자 갯수의 WORD 만큼 메모리 확보
						LOCCTR[LocctrCounter] = loc + 3 * StrToDec(operand);
					else if (!strcmp(opcode, "RESB"))	// 피연산자 갯수의 바이트만큼 메모리 확보
						LOCCTR[LocctrCounter] = loc + StrToDec(operand);
					else if (!strcmp(opcode, "BYTE"))	// 1바이트 확보
						LOCCTR[LocctrCounter] = loc + ComputeLen(operand);
					else if (!strcmp(opcode, "BASE")) {
							LOCCTR[LocctrCounter] = loc;	// BASE Assembler Directive일 경우 Loc을 대입
					}
					else { // 정의되지 않은 OP code일 경우 경고후 프로그램 종료
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

	/********************************** PASS 2 ***********************************/
	for (int dd = 0; dd < SymtabCounter; dd++) {
		printf("%7s:%8x\n", SYMTAB[dd].Label, SYMTAB[dd].Address);
	}

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
	int base_register = 0;

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
				if (Flag == 2) {	// Immediate Addressing Mode
					inst_fmt_sign = 0x010000;
				}
				else if (Flag == 3) {	// Indirect Addressing Mode
					inst_fmt_sign = 0x020000;
				}
				inst_fmt_sign <<= 8 * (inst_fmt_byte - 3);	// 바이트 수 만큼 왼쪽으로 Shift
			}
			else if (inst_fmt_byte >= 3) {	// 3/4형식 명령어 Simple Addressing Mode
				inst_fmt_sign = 0x030000;
				inst_fmt_sign <<= 8 * (inst_fmt_byte - 3);	// 바이트 수 만큼 왼쪽으로 Shift
			}
			
			if (inst_fmt_byte >= 3) {
				if (operand[strlen(operand) - 2] == ',' && operand[strlen(operand) - 1] == 'X') {	// index addressing Mode
					inst_fmt_index = 0x008000;	// index addressing mode 플래그 비트 x에 1
					inst_fmt_index <<= 8 * (inst_fmt_byte - 3);
					operand[strlen(operand) - 2] = '\0';
				}

				if (SearchSymtab(operand)) {
					if (inst_fmt_byte == 4) {	// extended instruction의 주소 지정
						inst_fmt_address = SYMTAB[SymIdx].Address;
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
							inst_fmt_address += ConvertDiffForAddress(diff);
						}
						else {	// PC relative addressing mode가 실패했을 경우 Base relative addressing mode 시도
							// base relative addressing mode에 기반하여 변위 다시 계산
							diff = SYMTAB[SymIdx].Address - base_register;
							if (diff >= 0 && diff < 4096) {
								inst_fmt_address = 0x004000;
								inst_fmt_address += diff;
							}
							else {
								fclose(fptr);
								printf("ERROR: CANNOT present relative addressing mode[line : %d]\n", IMRArray[loop]->LineIndex);
								exit(1);
							}
						}
					}
				}
				else {
					if (isNum(operand)) {
						inst_fmt_address = StrToDec(operand);
					}
					else {
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

				if (!strcmp(OPTAB[Counter].Mnemonic, "CLEAR") || !strcmp(OPTAB[Counter].Mnemonic, "TIXR") || !strcmp(OPTAB[Counter].Mnemonic, "SVC")) {
					// 피연산자가 1개 일경우 기록후 4비트 왼쪽으로 이동
					inst_fmt_address <<= 4;
				}
			}

			inst_fmt = inst_fmt_opcode + inst_fmt_sign + inst_fmt_index + inst_fmt_relative + inst_fmt_extended + inst_fmt_address;
			IMRArray[loop]->ObjectCode = inst_fmt;
		}
		else if (!strcmp(opcode, "WORD")) {
			strcpy(operand, IMRArray[loop]->OperandField);
			IMRArray[loop]->ObjectCode = StrToDec(operand);
		}
		else if (!strcmp(opcode, "BYTE")) {
			strcpy(operand, IMRArray[loop]->OperandField);
			IMRArray[loop]->ObjectCode = 0;

			if (operand[0] == 'C' || operand[0] == 'c' && operand[1] == '\'') {
				for (int x = 2; x <= (int)(strlen(operand) - 2); x++) {
					IMRArray[loop]->ObjectCode = IMRArray[loop]->ObjectCode + (int)operand[x];
					IMRArray[loop]->ObjectCode <<= 8;
				}
			}

			if (operand[0] == 'X' || operand[0] == 'x' && operand[1] == '\'') {
				char *operand_ptr;
				operand_ptr = &operand[2];
				*(operand_ptr + 2) = '\0';
				for (int x = 2; x <= (int)(strlen(operand) - 2); x++) {
					IMRArray[loop]->ObjectCode = IMRArray[loop]->ObjectCode + StrToHex(operand_ptr);
					IMRArray[loop]->ObjectCode <<= 8;
				}
			}

			IMRArray[loop]->ObjectCode >>= 8;
		}
		else if (!strcmp(opcode, "BASE")) {
			strcpy(operand, IMRArray[loop]->OperandField);
			IMRArray[loop]->ObjectCode = 0;
			if (SearchSymtab(operand)) {
				base_register = SYMTAB[SymIdx].Address;
			}
			else {	// SymTab에 없기 때문에 오류로 처리하고 프로그램 종료 
				fclose(fptr);
				printf("ERROR: No Label in SYMTAB[%s]\n", operand);
				exit(1);
			}
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
	fclose(fptr);

	exit(0);	// exit을 안해서 프로그램이 비정상적으로 종료되는 버그 수정
}