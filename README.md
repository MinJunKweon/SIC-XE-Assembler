# SIC/XE Machine Assembler
--------------------------

## Author
[MinJunKweon](http://github.com/MinJunKweon)

## What is SIC/XE Machhine
* [SIC Machine](https://en.wikipedia.org/wiki/Simplified_Instructional_Computer)'s eXension Edition
* you can read article at [SIC/XE](https://en.wikipedia.org/wiki/SIC/XE) on wikipedia

## Function
* Control Section
* Addressing Mode (Flag bit)
	* Direct Addressing Mode
	* Indirect Addressing Mode
	* Simple Addressing Mode
	* Immediate Addressing Mode
	* Relative Addressing Mode
		* Program Counter (PC Register)
		* Base	(Base Register)
* Extended Instruction (4bit Instruction)
* Floating Point Number
* External Symbol Reference & Define

## Sample Source
**All of Source files MUST HAVE '.asm' extension**
```
	COPY     START   0              
	FIRST    STL     RETADR            
	         LDB    #LENGTH             
	         BASE    LENGTH             
	CLOOP   +JSUB    RDREC              
	         LDA     LENGTH           
	         COMPF	 ZEROF			 Float Type Zero           
	         JEQ     ENDFIL              
	        +JSUB    WRREC
			 LDF	 FLOAT1			 Floating Point Immdidate
			 ADDF	 FLOAT2			 for Instruction Set II Test
			 FIX					 1 byte instruction
	         J       CLOOP 
	ENDFIL   LDA     EOF             
	         STA     BUFFER 
	         LDA    #3            
	         STA     LENGTH           
	        +JSUB    WRREC
	         J      @RETADR            Indirect Addressing Mode
	EOF      BYTE    C'EOF' 
	RETADR   RESW    1
	LENGTH   RESW    1
	ZEROF	 WORD	 0.0				for floating point type number (6 byte)
	FLOAT1	 WORD	 15.357
	FLOAT2	 WORD	 14.643				for floating point type number
	BUFFER   RESB    4096
	RDREC    CLEAR   X
	         CLEAR   A
	         CLEAR   S
	        +LDT    #4096
	RLOOP    TD      INPUT    
	         JEQ     RLOOP          
	         RD      INPUT
	         COMPR   A,S             
	         JEQ     EXIT       
	         STCH    BUFFER,X              
	         TIXR    T                          
	         JLT     RLOOP            
	EXIT     STX     LENGTH         
	         RSUB             
	INPUT    BYTE    X'F1'                
	WRREC    CLEAR   X         
	         LDT     LENGTH            
	WLOOP    TD      OUTPUT           
	         JEQ     WLOOP              
	         LDCH    BUFFER,X 
	         WD      OUTPUT
	         TIXR    T
	         JLT     WLOOP
	         RSUB
	OUTPUT   BYTE    X'05'
	COPY2	 CSECT						 for Control Section Test
			 EXTDEF	 BUFFER,BUFEND,LENGTH	 for D record Test
			 EXTREF	 RDREC2,WRREC2			 for R record Test
	FIRST2	 STL	 RETADR
	CLOOP	+JSUB	 RDREC2
			 LDA	 LENGTH
			 COMP	#0
			 JEQ	 ENDFIL
			+JSUB	 WRREC2
			 J		 CLOOP
	ENDFIL	 LDA	 EOF
			 STA	 BUFFER
			 LDA	#3
			 STA	 LENGTH
			 +JSUB	 WRREC2
			 J		@RETADR
	RETADR	 RESW	 1
	LENGTH	 RESW	 1
	EOF		 BYTE	 C'EOF'
	BUFFER	 RESB	 4096
	BUFEND	 WORD	 4147
	MAXLEN	 WORD	 4096
	RDREC2	 CSECT
			 EXTREF	 BUFFER,LENGTH,BUFEND
		 	 CLEAR	 X
			 CLEAR	 A
			 CLEAR	 S
			 LDT	 MAXLEN
	RLOOP	 TD		 INPUT
			 JEQ	 RLOOP
			 COMPR	 A,S
			 JEQ	 EXIT
			 +STCH	 BUFFER,X
			 TIXR	 T
			 JLT	 RLOOP
	EXIT	 +STX	 LENGTH
			 RSUB
	INPUT	 BYTE	 X'F1'
	MAXLEN	 WORD	 4096
	WRREC2	 CSECT
			 EXTREF	 LENGTH,BUFFER
			 CLEAR	 X
			 +LDT	 LENGTH
	WLOOP	 TD		#5
			 JEQ	 WLOOP
			 +LDCH	 BUFFER,X
			 WD		#5
			 TIXR	 T
			 JLT	 WLOOP
			 RSUB
			 END	 FIRST 
```

### Question?
* Please create issue on this repository

## Please create Pull Request if you want
