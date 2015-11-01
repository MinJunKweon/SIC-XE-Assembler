#include <stdio.h>
#include <stdlib.h>	
#include <string.h>	// 문자열 관리를 위함
#include <malloc.h>	// 동적할당을 위함

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
int LOCCTR[100];	// 각 명령어들의 메모리를 세기위한 Location Counter
int LocctrCounter = 0;	// LOCCTR의 Index 변수
int Index;
int j;
int ManchineCode;
int SymtabCounter = 0;	// 심볼테이블의 갯수를 세고 가리키기 위한 변수
int start_address;	// 프로그램의 시작 주소
int program_length;	// 프로그램의 총 길이
int ArrayIndex = 0;	// 중간파일을 각각 가리키기 위한 Index 변수

unsigned short int FoundOnSymtab_flag = 0;	// 해당 레이블을 심볼테이블에서 찾았다는 것을 반환하기 위함
unsigned short int FoundOnOptab_flag = 0;	// 해당 Opcode의 Mnemonic을 OP 테이블에서 찾았다는 것을 반환하기 위함

char Buffer[256];	// 각 소스코드를 읽기위한 버퍼 변수
char Label[32];	// 레이블을 임시로 저장하기 위한 변수
char Mnemonic[32];	// Mnemnic을 임시로 저장하기 위한 변수
char Operand[32];	// 피연산자를 임시로 저장하기 위한 변수
// 이 변수들은 모두 소스코드상의 표기법을 가짐

SIC_SYMTAB SYMTAB[20];	// 심볼테이블 변수
IntermediateRec* IMRArray[100];	// 중간파일 변수

// OP 테이블
static SIC_OPTAB OPTAB[] =
{
	/*********Instruction Set I***********/
	{ "ADDR", "2", 0x90 },
	{ "CLEAR", "2", 0xB4 },
	{ "COMPR", "2", 0xA0 },
	{ "DIVR", "2", 0x9C },
	{ "HIO", "1", 0xF4 },
	{ "LDB", "3", 0x68 },
	{ "LDS", "3", 0x6C },
	{ "LDT" , "3", 0x74 },
	{ "LPS", "3", 0xD0 },
	{ "MULR", "2", 0x98 },
	{ "RMO", "2", 0xAC },
	{ "SHIFTL", "2", 0xA4 },
	{ "SHIFTR", "2", 0xA8 },
	{ "SIO", "1", 0xF0 },
	{ "SSK", "3", 0xEC },
	{ "STB", "3", 0x78 },
	{ "STS", "3", 0x7C },
	{ "STT", "3", 0x84 },
	{ "SUBR", "2", 0x94 },
	{ "SVC", "2", 0xB0 },
	{ "TIO", "1", 0xF8 },
	{ "TIXR", "2", 0xB8 },
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
	{ "RSUB",  '3',  0x4C },
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



char* ReadOprator() {	// Mnemonic 읽기
	j = 0;//zeroing
	while (Buffer[Index] != ' ' && Buffer[Index] != '\t' && Buffer[Index] != '\n')
		Mnemonic[j++] = Buffer[Index++];
	Mnemonic[j] = '\0';
	return(Mnemonic);
}

char* ReadOperand() {	// 피연산자 읽기
	j = 0;//zeroing
	while (Buffer[Index] != ' ' && Buffer[Index] != '\t' && Buffer[Index] != '\n')
		Operand[j++] = Buffer[Index++];
	Operand[j] = '\0';
	return(Operand);
}

void RecordSymtab(char* label) {	// 심볼테이블에 해당 레이블의 위치와 레이블 입력
	strcpy(SYMTAB[SymtabCounter].Label, label);
	SYMTAB[SymtabCounter].Address = LOCCTR[LocctrCounter - 1];
	SymtabCounter++;
}

int SearchSymtab(char* label) {	// 심볼테이블에서 레이블 찾기
	FoundOnSymtab_flag = 0;

	for (int k = 0; k <= SymtabCounter; k++) {
		if (!strcmp(SYMTAB[k].Label, label)) {
			FoundOnSymtab_flag = 1;
			return (FoundOnSymtab_flag);
			break;
		}
	}
	return (FoundOnSymtab_flag);	// 없으면 0 반환
}

int SearchOptab(char * Mnemonic) {	// 심볼테이블에서 OP code 찾기
	int size = sizeof(OPTAB) / sizeof(SIC_OPTAB);
	FoundOnOptab_flag = 0;
	for (int i = 0; i<size; i++) {
		if (!strcmp(Mnemonic, OPTAB[i].Mnemonic)) {
			Counter = i;	// 있을 경우 Counter는 해당 OP code가 있는 OPTAB 상 Index 반환하기
			FoundOnOptab_flag = 1;
			break;
		}
	}
	return (FoundOnOptab_flag);
}

int StrToDec(char* c) {	// 10진수를 표현하는 String을 정수형으로 변환해서 반환
	int dec_num = 0;
	char temp[10];
	strcpy(temp, c);

	int len = strlen(c);
	for (int k = len - 1, l = 1; k >= 0; k--)
	{
		dec_num = dec_num + (int)(temp[k] - '0')*l;
		l = l * 10;
	}
	return (dec_num);
}

int StrToHex(char* c)	// 16진수를 표현하는 String을 정수형으로 변환해서 반환
{
	int hex_num = 0;
	char temp[10];
	strcpy(temp, c);

	int len = strlen(temp);
	for (int k = len - 1, l = 1; k >= 0; k--)
	{
		if (temp[k] >= '0' && temp[k] <= '9')
			hex_num = hex_num + (int)(temp[k] - '0')*l;
		else if (temp[k] >= 'A' && temp[k] <= 'F')
			hex_num = hex_num + (int)(temp[k] - 'A' + 10)*l;
		else if (temp[k] >= 'a' && temp[k] >= 'f')
			hex_num = hex_num + (int)(temp[k] - 'a' + 10)*l;
		else;
		l = l * 16;
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
		if (!strcmp(IMRArray[loop]->OperatorField, "START") || !strcmp(IMRArray[loop]->OperatorField, "RESW") || !strcmp(IMRArray[loop]->OperatorField, "RESB") || !strcmp(IMRArray[loop]->OperatorField, "END"))
			fprintf(fptr_list, "%04x\t%-10s%-10s%-10s\n", IMRArray[loop]->Loc, IMRArray[loop]->LabelField, IMRArray[loop]->OperatorField, IMRArray[loop]->OperandField);
		else
			fprintf(fptr_list, "%04x\t%-10s%-10s%-10s\t%06x\n", IMRArray[loop]->Loc, IMRArray[loop]->LabelField, IMRArray[loop]->OperatorField, IMRArray[loop]->OperandField, IMRArray[loop]->ObjectCode);
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
			else if (strcmp(IMRArray[loop]->OperatorField, "RESB") && strcmp(IMRArray[loop]->OperatorField, "RESW"))
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
	printf("x:\\fig2_1.asm\n");
	//scanf("%s", filename);
	strcpy(filename, "x:\\fig2_1.asm");
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
				strcpy(opcode, ReadOprator());
				strcpy(IMRArray[ArrayIndex]->OperatorField, opcode);
				SkipSpace();
				strcpy(operand, ReadOperand());
				strcpy(IMRArray[ArrayIndex]->OperandField, operand);

				if (strcmp(opcode, "END"))
				{
					if (label[0] != '\0')
					{
						if (SearchSymtab(label))
						{
							fclose(fptr);
							printf("ERROR: Duplicate Symbol\n");
							FoundOnSymtab_flag = 0;
							exit(1);
						}
						RecordSymtab(label);
					}

					if (SearchOptab(opcode))
						LOCCTR[LocctrCounter] = loc + (int)(OPTAB[Counter].Format - '0');
					else if (!strcmp(opcode, "WORD"))
						LOCCTR[LocctrCounter] = loc + 3;
					else if (!strcmp(opcode, "RESW"))
						LOCCTR[LocctrCounter] = loc + 3 * StrToDec(operand);
					else if (!strcmp(opcode, "RESB"))
						LOCCTR[LocctrCounter] = loc + StrToDec(operand);
					else if (!strcmp(opcode, "BYTE"))
						LOCCTR[LocctrCounter] = loc + ComputeLen(operand);
					else {
						fclose(fptr);
						printf("ERROR: Invalid Operation Code\n");
						exit(1);
					}
				}
			}
			loc = LOCCTR[LocctrCounter];
			IMRArray[ArrayIndex]->Loc = LOCCTR[LocctrCounter - 1];
			LocctrCounter++;
			ArrayIndex++;
		}
		
		if (is_comment == 1) {	// 첫 줄이 주석일 경우 시작이 되지 않는 오류 수정
			start_line += 1;
		}

		FoundOnOptab_flag = 0;
		line += 1;
	}
	program_length = LOCCTR[LocctrCounter - 2] - LOCCTR[0];

	/********************************** PASS 2 ***********************************/
	printf("Pass 2 Processing...\n");

	unsigned long inst_fmt;//
	unsigned long inst_fmt_opcode;
	unsigned long inst_fmt_index;
	unsigned long inst_fmt_address;


	for (loop = 1; loop<ArrayIndex; loop++) {
		inst_fmt_opcode = 0;
		inst_fmt_index = 0;
		inst_fmt_address = 0;

		strcpy(opcode, IMRArray[loop]->OperatorField);

		if (SearchOptab(opcode)) {
			inst_fmt_opcode = OPTAB[Counter].ManchineCode;
			inst_fmt_opcode <<= 16;
			IMRArray[loop]->ObjectCode = inst_fmt_opcode;
			strcpy(operand, IMRArray[loop]->OperandField);

			if (operand[strlen(operand) - 2] == ',' && operand[strlen(operand) - 1] == 'X') {
				inst_fmt_index = 0x008000;
				operand[strlen(operand) - 2] = '\0';
			}
			else
				inst_fmt_index = 0x000000;


			for (int search_symtab = 0; search_symtab<SymtabCounter; search_symtab++) {
				if (!strcmp(operand, SYMTAB[search_symtab].Label))
					inst_fmt_address = (long)SYMTAB[search_symtab].Address;
			}

			inst_fmt = inst_fmt_opcode + inst_fmt_index + inst_fmt_address;
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
		else
			/* do nothing */;
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