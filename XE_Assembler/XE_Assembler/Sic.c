#include <stdio.h>
#include <stdlib.h>	
#include <string.h>	// ���ڿ� ������ ����
#include <malloc.h>	// �����Ҵ��� ����

#define TEST_FNAME "x:\\2.asm"

/***************************** DECLERATE VARIABLE ****************************/
typedef struct OperationCodeTable
{
	char Mnemonic[8];	// ��ɾ��� ���� (ex. LDB, LDA, etc...)
	char Format;	// ��ɾ��� ���� (��ɾ��� ����)	3/4 ������ ���ǻ� 3�������� ǥ���ϵ��� �����߽��ϴ�.
	unsigned short int  ManchineCode;	// �ش� ��ɾ��� ���� �ڵ�
}SIC_OPTAB;

typedef struct SymbolTable
{
	char Label[10];	// ���̺��� �̸�
	int Address;	// ���̺��� ����Ű�� �ּ�
}SIC_SYMTAB;

// �������� ���̺��� �����ϴ� �������� ���ڵ�
typedef struct RegisterTable
{
	char name[3];	// �������� Mnemonic
	int id;	// ���������� ������ȣ
} SIC_XE_REGISTER;

typedef struct IntermediateRecord {	// �߰�����
	unsigned short int LineIndex;	// �ҽ��ڵ��� ���� �����ϴ� ����
	unsigned short int Loc;	//  �ش� ��ɾ��� �޸𸮻� ��ġ
	unsigned long int ObjectCode;	//  Pass 2�� ���� Assemble�� �����ڵ�
	char LabelField[32];	// �ҽ��ڵ�� ǥ��Ǿ��ִ� ���̺�
	char OperatorField[32];	// �ҽ��ڵ�� ǥ��Ǿ��ִ� Mnemonic
	char OperandField[32];	// �ҽ��ڵ�� ǥ��Ǿ��ִ� �ǿ�����
}IntermediateRec;

// �Լ��� ����� �����ϱ� ���� �ӽ÷� ����ϴ� ����������
int Counter;	// Opcodeã�� �� �� ��ɾ��� ��ġ�� ����Ű�� ���� ����
int RegIdx;		// ���������� ��ġ�� ����Ŵ. Counter ������ ���� ���
int SymIdx;		// Label�� ��ġ�� ����Ŵ. Counter ������ ���� ���
int LOCCTR[100];	// �� ��ɾ���� �޸𸮸� �������� Location Counter
int LocctrCounter = 0;	// LOCCTR�� Index ����
int Flag;
int Index;
int j;
int ManchineCode;
int SymtabCounter = 0;	// �ɺ����̺��� ������ ���� ����Ű�� ���� ����
int start_address;	// ���α׷��� ���� �ּ�
int program_length;	// ���α׷��� �� ����
int ArrayIndex = 0;	// �߰������� ���� ����Ű�� ���� Index ����

unsigned short int FoundOnSymtab_flag = 0;	// �ش� ���̺��� �ɺ����̺��� ã�Ҵٴ� ���� ��ȯ�ϱ� ����
unsigned short int FoundOnOptab_flag = 0;	// �ش� Opcode�� Mnemonic�� OP ���̺��� ã�Ҵٴ� ���� ��ȯ�ϱ� ����
unsigned short int FoundOnRegTab_flag = 0;	// �������� ���̺� �ش� ���������� ������ �ִ��� Ȯ��

char Buffer[256];	// �� �ҽ��ڵ带 �б����� ���� ����
char Label[32];	// ���̺��� �ӽ÷� �����ϱ� ���� ����
char Mnemonic[32];	// Mnemnic�� �ӽ÷� �����ϱ� ���� ����
char Operand[32];	// �ǿ����ڸ� �ӽ÷� �����ϱ� ���� ����
// �� �������� ��� �ҽ��ڵ���� ǥ����� ����

SIC_SYMTAB SYMTAB[20];	// �ɺ����̺� ����
IntermediateRec* IMRArray[100];	// �߰����� ����

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

// OP ���̺�
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
char* ReadLabel() {	// ���̺� �б�
	j = 0;//zeroing
	while (Buffer[Index] != ' ' && Buffer[Index] != '\t' && Buffer[Index] != '\n')
		Label[j++] = Buffer[Index++];
	Label[j] = '\0';
	return(Label);
}

void SkipSpace() {	// ���� ��ŵ�ϱ� (Index�� �ڷ� �ű�)
	while (Buffer[Index] == ' ' || Buffer[Index] == '\t')
		Index++;
}

int ReadFlag(char *Mnemonic) {	// Mnemonic���� �÷��� ��Ʈ �б�
	Flag = 0;
	switch (Mnemonic[0]) {	// Mnemonic�� ù��° ���ڰ� Ư�������ϰ��
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
		Flag = 0;	// default (�ƹ��� ǥ�ð� ���� ���)
	}
	return Flag;
}

char* ReadOprator() {	// Mnemonic �б�
	j = 0;//zeroing
	while (Buffer[Index] != ' ' && Buffer[Index] != '\t' && Buffer[Index] != '\n')	// ������ ���ö����� Mnemonic �б�
		Mnemonic[j++] = Buffer[Index++];
	Mnemonic[j] = '\0';	// �ܼ� ���ڹ迭�� ���ڿ��� �ν��ϵ��� �� ���� �߰�
	return(Mnemonic);	// ���ڿ� ��ȯ
}

char* ReadOperand() {	// �ǿ����� �б�
	j = 0;//zeroing
	while (Buffer[Index] != ' ' && Buffer[Index] != '\t' && Buffer[Index] != '\n')	// ������ ���ö����� Operand �б�
		Operand[j++] = Buffer[Index++];
	Operand[j] = '\0';	// �ܼ� ���ڹ迭�� ���ڿ��� �ν��ϵ��� �� ���� �߰�
	return(Operand);	// ���ڿ� ��ȯ
}

void RecordSymtab(char* label) {	// �ɺ����̺� �ش� ���̺��� ��ġ�� ���̺� �Է�
	if (ReadFlag(label)) { // Immediate or Indirect Addressing Mode ���� ó��
		label = label + 1;
	}
	strcpy(SYMTAB[SymtabCounter].Label, label);	// Symbol ���̺� Label �߰�
	SYMTAB[SymtabCounter].Address = LOCCTR[LocctrCounter - 1];	// �ش� Label�� �޸� ��ġ�� ���
	SymtabCounter++;	// 1�� �߰��Ǿ����Ƿ� ī��Ʈ 1����
}

int SearchSymtab(char* label) {	// �ɺ����̺��� ���̺� ã��
	FoundOnSymtab_flag = 0;
	if (ReadFlag(label)) { // Immediate Addressing Mode�̰ų� Indirect Addressing Mode�� ��� ����ó��
		label = label + 1;
	}

	for (int k = 0; k <= SymtabCounter; k++) {	// �ݺ��� ���� ã��
		if (!strcmp(SYMTAB[k].Label, label)) {	// label�� �ɺ����̺� ���� ���
			FoundOnSymtab_flag = 1;	// ã�Ҵٴ� �ǹ̸� ǥ���ϱ����� �÷���
			SymIdx = k;	// SymIdx�� �ɺ����̺��� ��� ��ġ�� �ִ��� ����Ų��.
			return (FoundOnSymtab_flag);
		}
	}
	return (FoundOnSymtab_flag);	// ������ 0 ��ȯ
}

int SearchOptab(char * Mnemonic) {	// �ɺ����̺��� OP code ã��
	int size = sizeof(OPTAB) / sizeof(SIC_OPTAB);
	FoundOnOptab_flag = 0;
	if (ReadFlag(Mnemonic)) { // Extended Instruction�� ��� ����ó��
		Mnemonic = Mnemonic + 1;	// ���ڿ������͸� 1������Ŵ. �Ǿ��ڸ� ����
	}
	for (int i = 0; i<size; i++) {
		if (!strcmp(Mnemonic, OPTAB[i].Mnemonic)) {	// OP ���̺� �ش� Mnemonic�� ���� ���
			Counter = i;	// ���� ��� Counter�� �ش� OP code�� �ִ� OPTAB �� Index ��ȯ�ϱ�
			FoundOnOptab_flag = 1;	// ã�Ҵٴ� �ǹ̸� ǥ���ϱ����� �÷���
			break;
		}
	}
	return (FoundOnOptab_flag);	// ������ 0 ��ȯ
}

int SearchRegTab(char * Mnemonic) {	// �̸� ���ǵ� �������� ���̺��� �ش� �������͸� �д´�
	int size = sizeof(REG_TAB) / sizeof(SIC_XE_REGISTER);
	FoundOnRegTab_flag = 0;
	for (int i = 0; i < size; i++) {
		if (!strcmp(Mnemonic, REG_TAB[i].name)) {	// �������� ���̺� �ش� ���������� ��ȣ�� ���� ���
			RegIdx = i;	// �������� ���̺��� ��� ��ġ�� �ִ��� ����Ŵ
			FoundOnRegTab_flag = 1;	// ã�Ҵٴ� �ǹ̸� ǥ���ϱ� ���� �÷���
			break;
		}
	}
	return (FoundOnRegTab_flag);	// ������ 0 ��ȯ
}

int isNum(char * str) {	// ���ڿ��� �̷�� �ִ� ��� ���ҵ��� ���ڷ� �̷�����ִ��� Ȯ��
	if (ReadFlag(str)) {	// ���ڿ� �� �տ� �÷��׺�Ʈ�� ���� ��� �̸� �����ϱ� ����.
		str += 1;
	}
	int i, len = strlen(str);
	for (i = 0; i < len; ++i) {	// ���ڿ� ���̸�ŭ �ݺ��ؼ� ��� �������� �Ǵ�
		if ('0' > str[i] || '9' < str[i]) {	// ���ڰ� �ƴ� ��� 0�� ��ȯ
			return 0;
		}
	}
	return 1;	// ��� �����̹Ƿ� 1�� ��ȯ
}

unsigned long ConvertDiffForAddress(short diff) {
	if (diff >= 0) { // ����̹Ƿ� �״�� ��ȯ
		return diff;
	}
	// �����ϰ�� �ڷ����� ũ�Ⱑ 12��Ʈ��� �����ϰ� 2's completion�� ����
	diff ^= 0xF000;	// 12��Ʈ ������ bit�� �����ϱ� ���ؼ� �� 16��Ʈ�� short���� �� 4��Ʈ�� 0���� �����
	return diff;
}

int StrToDec(char* c) {	// 10������ ǥ���ϴ� String�� ���������� ��ȯ�ؼ� ��ȯ
	if (ReadFlag(c)) {	// �÷��׺�Ʈ�� �����ϱ�����.
		c += 1;
	}
	int dec_num = 0;
	char temp[10];
	strcpy(temp, c);	// temp�� ���ڿ� ����

	int len = strlen(c);
	for (int k = len - 1, l = 1; k >= 0; k--)	// �� ���ڿ��� �Ųٷ� �о dec_num�� ���
	{
		dec_num = dec_num + (int)(temp[k] - '0')*l;
		l = l * 10;	// �ڸ����� ����ϱ����� ������ ���� ���ڵ��� ���° �ڸ����� ��Ÿ��
	}
	return (dec_num);
}

int StrToHex(char* c)	// 16������ ǥ���ϴ� String�� ���������� ��ȯ�ؼ� ��ȯ
{
	int hex_num = 0;
	char temp[10];
	strcpy(temp, c);	// temp�� ���ڿ� ����

	int len = strlen(temp);
	for (int k = len - 1, l = 1; k >= 0; k--)	// �� ���ڿ��� �Ųٷ� �о� 16������ ���������� ��ȯ
	{
		if (temp[k] >= '0' && temp[k] <= '9')
			hex_num = hex_num + (int)(temp[k] - '0')*l;
		else if (temp[k] >= 'A' && temp[k] <= 'F')	// �빮�� �ϰ��
			hex_num = hex_num + (int)(temp[k] - 'A' + 10)*l;
		else if (temp[k] >= 'a' && temp[k] >= 'f')	// �ҹ��� �ϰ��
			hex_num = hex_num + (int)(temp[k] - 'a' + 10)*l;
		else;
		l = l * 16; // �ڸ����� ����ϱ����� ������ ���� ���ڵ��� ���° �ڸ����� ��Ÿ��
	}
	return (hex_num);
}

int ComputeLen(char* c) {	// �ƽ�Ű �ڵ峪 16������ ���̸� ���
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

void CreateProgramList() {	// ����Ʈ ���� ����
	int loop;
	int len;	// ���ڳ� 16������ ��� ���� ����� ���� ����
	FILE *fptr_list;

	fptr_list = fopen("sic.list", "w");

	if (fptr_list == NULL)
	{
		printf("ERROR: Unable to open the sic.list.\n");	// ����Ʈ ������ �� �� ���� ��� ����ó��
		exit(1);
	}

	// ����Ʈ ���� ���� ���
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
			len = (strlen(IMRArray[loop]->OperandField)-3)/2;	// C, ', ' Ȥ�� X, ', '�� ������ �͵��� �����Ʈ���� ����ϱ� ����
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

void CreateObjectCode() {	// �������� ����
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
		printf("ERROR: Unable to open the sic.obj.\n");	// ���������� �� �� ���� ��� ����ó��
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
	if (fptr == NULL)	// �ҽ��ڵ� ���� �б� �������� ��� ����ó��
	{
		printf("ERROR: Unable to open the %s file.\n", filename);
		exit(1);
	}

	/********************************** PASS 1 ***********************************/
	printf("Pass 1 Processing...\n");
	while (fgets(Buffer, 256, fptr) != NULL)	// �ҽ��ڵ� ���Ͽ��� �б�
	{
		is_empty_line = strlen(Buffer);

		Index = 0;
		j = 0;
		strcpy(label, ReadLabel());
		if (Label[0] == '.')	// �ش� �ҽ��ڵ尡 �ּ����� �ƴ��� Ȯ��
			is_comment = 1;
		else
			is_comment = 0;

		if (is_empty_line>1 && is_comment != 1)
		{
			Index = 0;
			j = 0;

			IMRArray[ArrayIndex] = (IntermediateRec*)malloc(sizeof(IntermediateRec));/* [A] */	// �߰����� �����Ҵ�

			IMRArray[ArrayIndex]->LineIndex = ArrayIndex;	// �ҽ��ڵ� ���� �� ����
			strcpy(label, ReadLabel());	// ���̺��� �о� Label�� ����
			strcpy(IMRArray[ArrayIndex]->LabelField, label);	// ���̺��� �߰����Ͽ� ����
			SkipSpace();	// ���� ����

			if (line == start_line)	// ���α׷��� ���� ������ ù���� �ƴ� ��� (ù��° ���� �ּ��� ���) ����ó��
			{
				strcpy(opcode, ReadOprator());	// Mnemonic �б�
				strcpy(IMRArray[ArrayIndex]->OperatorField, opcode); /* [A] */	// ���� Mnemonic�� �߰����Ͽ� ����
				if (!strcmp(opcode, "START")) {	// �����ּ� �ʱ�ȭ
					SkipSpace();
					strcpy(operand, ReadOperand());
					strcpy(IMRArray[ArrayIndex]->OperandField, operand);/* [A] */
					LOCCTR[LocctrCounter] = StrToHex(operand);
					start_address = LOCCTR[LocctrCounter];
				} else {	// ���� �ּҰ� ��õǾ����� ���� ��� 0���� �ʱ�ȭ
					LOCCTR[LocctrCounter] = 0;
					start_address = LOCCTR[LocctrCounter];
				}
			}
			else
			{

				strcpy(opcode, ReadOprator());	// OP Code �б�
				strcpy(IMRArray[ArrayIndex]->OperatorField, opcode);	// �߰����Ͽ� OP code ����
				SkipSpace();	// OP code�� �ǿ����� ������ ���� ����
				strcpy(operand, ReadOperand());	// �ǿ����� �κ� �б�
				strcpy(IMRArray[ArrayIndex]->OperandField, operand);	// �߰����Ͽ� �ǿ����� ����

				if (strcmp(opcode, "END"))	// OP code�� END ����� �������� ���
				{
					if (label[0] != '\0')	// ���̺��� ���� ���
					{
						if (SearchSymtab(label))	// ���� �̸��� ���̺��� �ִ��� ã��
						{
							// ���� ���� �̸��� ���̺��� ���� ��� Alert�ϰ� ���α׷� ����
							fclose(fptr);
							printf("ERROR: Duplicate Symbol\n");
							FoundOnSymtab_flag = 0;
							exit(1);
						}
						RecordSymtab(label);	// �����̸��� �����Ƿ� �ɺ����̺� �߰�
					}

					if (SearchOptab(opcode)) {	// OP Code�� OPTAB�� ���� ��� ��ɾ� ���ĸ�ŭ �޸� Ȯ��
						LOCCTR[LocctrCounter] = loc + (int)(OPTAB[Counter].Format - '0');
						if (ReadFlag(opcode)) {
							// 4���� ��ɾ��� ���
							LOCCTR[LocctrCounter] += 1;	// ������ 1����Ʈ �� �߰�
						}
					}
					else if (!strcmp(opcode, "WORD"))	// 3����Ʈ(1 WORD) Ȯ��
						LOCCTR[LocctrCounter] = loc + 3;
					else if (!strcmp(opcode, "RESW"))	// �ǿ����� ������ WORD ��ŭ �޸� Ȯ��
						LOCCTR[LocctrCounter] = loc + 3 * StrToDec(operand);
					else if (!strcmp(opcode, "RESB"))	// �ǿ����� ������ ����Ʈ��ŭ �޸� Ȯ��
						LOCCTR[LocctrCounter] = loc + StrToDec(operand);
					else if (!strcmp(opcode, "BYTE"))	// 1����Ʈ Ȯ��
						LOCCTR[LocctrCounter] = loc + ComputeLen(operand);
					else if (!strcmp(opcode, "BASE")) {
							LOCCTR[LocctrCounter] = loc;	// BASE Assembler Directive�� ��� Loc�� ����
					}
					else { // ���ǵ��� ���� OP code�� ��� ����� ���α׷� ����
						fclose(fptr);
						printf("ERROR: Invalid Operation Code\n");
						exit(1);
					}
				}
			}
			loc = LOCCTR[LocctrCounter];	// loc�� �ٽ� �����ϰ� ���� ������ �غ�
			IMRArray[ArrayIndex]->Loc = LOCCTR[LocctrCounter - 1];	// �߰����Ͽ� �ش� �ڵ��� �޸� ���� ���
			LocctrCounter++;	// LOCCTR�� �����ϴ� �ε��� ���� �� ����
			ArrayIndex++;	// ���� �ڵ带 �б� ���� �߰������� �ε��� ���� �� ����
		}
		
		if (is_comment == 1) {	// ù ���� �ּ��� ��� ������ ���� �ʴ� ���� ����
			start_line += 1;
		}

		FoundOnOptab_flag = 0;	// flag ���� �ʱ�ȭ
		line += 1;	// �ҽ� �� 1 ����
	}
	program_length = LOCCTR[LocctrCounter - 2] - LOCCTR[0];
	// END �����ڸ� ������ ��� END ������ �ٷ� ���� �ҽ��ڵ��� �޸� ��ġ�� �����ּҸ� ���� �� ���α׷� ���� ���

	/********************************** PASS 2 ***********************************/
	for (int dd = 0; dd < SymtabCounter; dd++) {
		printf("%7s:%8x\n", SYMTAB[dd].Label, SYMTAB[dd].Address);
	}

	printf("Pass 2 Processing...\n");

	unsigned long inst_fmt;		// ���� ���� �ڵ�
	unsigned long inst_fmt_opcode;	// �����ڵ��� op code �κ��� ��Ÿ���� ����
	unsigned long inst_fmt_sign;	// Immediate, Indirect Addressing Mode�� ��Ÿ���� �÷��׺�Ʈ ���� 
	unsigned long inst_fmt_relative;	// relative addressing mode�� ��Ÿ���� �÷��׺�Ʈ ����
	unsigned long inst_fmt_index;	// index Addressing Mode�� ��Ÿ���� ����
	unsigned long inst_fmt_extended;	// �÷��׺�Ʈ e�� ��Ÿ���� �÷��׺�Ʈ ����
	unsigned long inst_fmt_address;	// �ǿ����� �κ��� ��Ÿ���� ���� (�ʿ信 ���ؼ� �������� ����� �� ���� �ִ�. ex. Immediate Addressing Mode)
	int inst_fmt_byte;		// ������ ��ɾ����� ��Ÿ���� ���� (����Ʈ ��)
	int i, regCharIdx;
	char regName[3];	// �������� �̸��� ���ϱ����� ��Ƴ��� �ӽú���
	
	int diff = 0;	// �ּҿ� �� ������ �����ϴ� ���� (������ ���� �� �����Ƿ� unsigned �� �ƴϴ�)
	int base_register = 0;

	for (loop = 1; loop<ArrayIndex; loop++) {	// �߰������� ���������� ����
		// �� ������ �ʱ�ȭ
		inst_fmt_opcode = 0;
		inst_fmt_sign = 0;
		inst_fmt_relative = 0;
		inst_fmt_index = 0;
		inst_fmt_extended = 0;
		inst_fmt_address = 0;
		inst_fmt_byte = 0;
		regName[0] = '\0';

		strcpy(opcode, IMRArray[loop]->OperatorField);	// op code �κ� ����

		if (SearchOptab(opcode)) {	// opcode ã��
			if (!strcmp(OPTAB[Counter].Mnemonic, "RSUB")) {
				// RSUB�� 3���� ��ɾ������� ����� �����Ƿ�
				IMRArray[loop]->ObjectCode = (OPTAB[Counter].ManchineCode << 16);
				continue;
			}
			inst_fmt_opcode = OPTAB[Counter].ManchineCode;	// opcode�� �����ڵ� ����
			inst_fmt_byte = OPTAB[Counter].Format - '0';	// �ش� ��ɾ �� ����Ʈ�� ����ϴ� �� ����
			if (inst_fmt_byte == 3 && ReadFlag(opcode)) {	// ���� 4���� ��ɾ��� ��� �б�ó��
				inst_fmt_byte = 4;	// 4���� ��ɾ�
				inst_fmt_extended = 0x00100000;	// �÷��� ��Ʈ e�� 1��.
			}
			inst_fmt_opcode <<= (8 * (inst_fmt_byte - 1));	// �� ��ɾ� ���Ŀ� �°� �������� Shift
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
				inst_fmt_sign <<= 8 * (inst_fmt_byte - 3);	// ����Ʈ �� ��ŭ �������� Shift
			}
			else if (inst_fmt_byte >= 3) {	// 3/4���� ��ɾ� Simple Addressing Mode
				inst_fmt_sign = 0x030000;
				inst_fmt_sign <<= 8 * (inst_fmt_byte - 3);	// ����Ʈ �� ��ŭ �������� Shift
			}
			
			if (inst_fmt_byte >= 3) {
				if (operand[strlen(operand) - 2] == ',' && operand[strlen(operand) - 1] == 'X') {	// index addressing Mode
					inst_fmt_index = 0x008000;	// index addressing mode �÷��� ��Ʈ x�� 1
					inst_fmt_index <<= 8 * (inst_fmt_byte - 3);
					operand[strlen(operand) - 2] = '\0';
				}

				if (SearchSymtab(operand)) {
					if (inst_fmt_byte == 4) {	// extended instruction�� �ּ� ����
						inst_fmt_address = SYMTAB[SymIdx].Address;
					}
					else {	// relative Addressing mode
						// PC�� �� ����
						// ��ɾ� ���� �������� PC�� �� ��� (���� ��ɾ��� �޸� ��ġ)
						diff = SYMTAB[SymIdx].Address - IMRArray[loop]->Loc - inst_fmt_byte;
						if (diff >= -2048 && diff < 2048) {
							// pc relative�� ���
							// ������ 12��Ʈ�� ǥ���ؾ��ϹǷ� 12��Ʈ�� ������ ��ȯ���ֱ� ���� �Լ� ȣ��
							// ������ �ƴ϶�� �Լ� ���� ������ �״�� ��ȯ�ǹǷ� ������ �ƴϾ �Լ��� ȣ��
							inst_fmt_address = 0x002000;
							inst_fmt_address += ConvertDiffForAddress(diff);
						}
						else {	// PC relative addressing mode�� �������� ��� Base relative addressing mode �õ�
							// base relative addressing mode�� ����Ͽ� ���� �ٽ� ���
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
			else if (inst_fmt_byte == 2) {	// 2���� ��ɾ��� ���
				i = 0; regCharIdx = 0;	// �ε��� ������ �ʱ�ȭ
				do {	// �ǿ����ڸ� �о� �������͵鿡 �´� �����ڵ� �ۼ�
					if (operand[i] == ',' || operand[i] == '\0') {	// �ռ� ���� �������͸� ���� �غ� �Ǿ��� ���
						regName[regCharIdx] = '\0';	// �ܼ� ���ڹ迭�� ���ڿ��� ����
						if (operand[i] == ',') {	// ������ ��ϵ� ���������� ���̵� �����Ұ�� 4��Ʈ�� �������� �а� ���
							inst_fmt_address <<= 4;
						}

						if (SearchRegTab(regName)) {	// �̸� ���ǵ� �������� ���̺��� ����
							inst_fmt_address += REG_TAB[RegIdx].id;	// �������� ���̺� �ش� �������Ͱ� ���� ��� �� ���̵� �����ڵ忡 �߰� 
						}
						else { 
							if (!strcmp(OPTAB[Counter].Mnemonic, "SVC") || !strcmp(OPTAB[Counter].Mnemonic, "SHIFTL") || !strcmp(OPTAB[Counter].Mnemonic, "SHIFTR")) {
								// �ǿ����ڷ� �������͸� ��������ʰ� ���ڸ� ����ϴ� ���
								if (isNum(regName)) {	// �ǿ����ڰ� ���ڶ��
									inst_fmt_address += StrToDec(regName);	// �߰�
								}
							}
							else { // RegTab�� ���� ������ ������ ó���ϰ� ���α׷� ����
								fclose(fptr);
								printf("ERROR: Invalid Register\n");
								exit(1);
							}
						}
						regCharIdx = 0;	// �ε��� ���� �ʱ�ȭ
					}
					else if (operand[i] != ' ') {	// ������ ��� ��ŵ�ϵ���
						regName[regCharIdx++] = operand[i];	// �������� �̸� ����
					}
				} while (operand[i++] != '\0');

				if (!strcmp(OPTAB[Counter].Mnemonic, "CLEAR") || !strcmp(OPTAB[Counter].Mnemonic, "TIXR") || !strcmp(OPTAB[Counter].Mnemonic, "SVC")) {
					// �ǿ����ڰ� 1�� �ϰ�� ����� 4��Ʈ �������� �̵�
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
			else {	// SymTab�� ���� ������ ������ ó���ϰ� ���α׷� ���� 
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

	// ����Ʈ ���ϰ� ���� ���� ����
	CreateProgramList();
	CreateObjectCode();

	// �޸� �����Ҵ� ����
	for (loop = 0; loop<ArrayIndex; loop++)
		free(IMRArray[loop]);

	printf("Compeleted Assembly\n");
	fclose(fptr);

	exit(0);	// exit�� ���ؼ� ���α׷��� ������������ ����Ǵ� ���� ����
}