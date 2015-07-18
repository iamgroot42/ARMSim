/* myARMSim.cpp
   Purpose of this file: implementation file for myARMSim
*/

/*
Anshuman Suri : 2014021
Harshvardhan Kalra : 2014043
*/

/*
R13: Stack Pointer
R14: Link Register
R15: Program counter
*/

#include <stdlib.h>
#include "myARMSim.h"
#include <stdio.h>
#include <string.h>

//Indicator for branching
static int indicator=0;
//Destination  Register
static int ans;
//Accumulator
static int acc;
//Opcode
static int opc_g;
//Register file
static unsigned int R[16];
//flags
static int N,C,V,Z;
//memory
static unsigned char MEM[4000];

//intermediate datapath and control path signals
static unsigned int instruction_word;
static unsigned int operand1;
static unsigned int operand2;

int read_word(char *mem, unsigned int address) {
  int *data;
  data =  (int*) (mem + address);
  return *data;
}

void write_word(char *mem, unsigned int address, unsigned int data) {
  int *data_p;
  data_p = (int*) (mem + address);
  *data_p = data;
}

// it is used to set the reset values
//reset all registers and memory content to 0
void reset_proc()
{
    memset(R,0,sizeof(R));
    memset(MEM,0,sizeof(MEM));
}

//load_program_memory reads the input memory, and populates the instruction 
// memory
void load_program_memory(char *file_name) {
  FILE *fp;
  unsigned int address, instruction;
  fp = fopen(file_name, "r");
  if(fp == NULL) {
    printf("Error opening input mem file\n");
    exit(1);
  }
  while(fscanf(fp, "%x %x", &address, &instruction) != EOF) {
    write_word(MEM, address, instruction);
  }
  fclose(fp);
}

//writes the data memory in "data_out.mem" file
void write_data_memory() {
  FILE *fp;
  unsigned int i;
  fp = fopen("data_out.mem", "w");
  if(fp == NULL) {
    printf("Error opening dataout.mem file for writing\n");
    return;
  }
  
  for(i=0; i < 4000; i = i+4){
    fprintf(fp, "%x %x\n", i, read_word(MEM, i));
  }
  fclose(fp);
}

//should be called when instruction is swi_exit
void swi_exit() {
  write_data_memory();
  exit(0);
}

//reads from the instruction memory and updates the instruction register
void fetch() {
	if(R[15]>4000) {printf("PC out of range.\n Exiting!\n"); swi_exit();}
	instruction_word=read_word(MEM,R[15]);
	if(instruction_word==0xEF000011) swi_exit(); //swi 0x11
	printf("FETCH:Fetch instruction 0x%x from address %x\n",instruction_word,R[15]);
}
//reads the instruction register, reads operand1, operand2 from register file, decides the operation to be performed in execute stage
void decode() {
	int cond,p1,p2,extra,imm;
	imm=instruction_word >> 25;
	imm=imm & 1;
	extra=instruction_word>>26;
	extra=extra & 3;
	cond=instruction_word >> 28;
	ans=instruction_word >> 12;
	ans=ans & 15;
	p2=instruction_word & 4095;
	p1=instruction_word >> 16;
	p1=p1 & 15;
	if(extra==1)
	{
		//Data-transfer
		operand1=p1;
		operand2=p2;
		printf("DECODE:");
		opc_g=instruction_word >> 20;
		opc_g=opc_g & 31; //6-bit op-code
		printf("Potato :D %d %d\n",(instruction_word>>25) & 1,opc_g);
		if(!((instruction_word>>25) & 1))
		printf("Data Transfer, first operand R%d,Second operand #%d, destination register R%d\n",p1,p2,ans);
		else
		printf("Data Transfer, first operand R%d,Second operand R%d, destination register R%d\n",p1,p2,ans);			
	}
	else if(extra==2)
	{
		opc_g = opc_g & 3; //2-bit op-code , value '2' for branching
		printf("DECODE:");
		printf("Branch instruction ");
		// Indicator used to check if condition is true
		switch(cond)
		{
			case 0: printf("EQ\n"); break;
			case 1: printf("NE\n"); break;
			case 10: printf("GE\n"); break;
			case 11: printf("LT\n"); break;
			case 12: printf("GT\n"); break;
			case 13: printf("LE\n"); break;
			case 14: printf("AL\n"); break; 
		}
	}
	else
	{
		//Data-Processing
		printf("DECODE: ");
		opc_g=instruction_word >> 21;
		opc_g=opc_g & 15;
		if((instruction_word>>25) & 1) //Immediate 
		{
			operand2=p2;
		}
		else
		{
			operand2=R[p2];
		}
		operand1=R[p1];
		switch(opc_g)
		{
			case 0: printf("AND,"); break;
			case 1: printf("EOR, "); break;
			case 2: printf("SUB, "); break;
			case 4: printf("ADD, "); break;
			case 5: printf("ADC, "); break;
			case 10: printf("CMP, "); break;
			case 12: printf("ORR, "); break;
			case 13: printf("MOV, "); break;
			case 15: printf("MNV, "); break;
		}
		if(opc_g==10)
		{
			if(imm) printf("first operand R%d,Second operand #%d\nDECODE: Read registers R%d = %d,Immediate = %d\n",p1,p2,p1,R[p1],p2);
			else printf("first operand R%d,Second operand R%d\nDECODE: Read registers R%d = %d,R%d = %d\n",p1,p2,p1,R[p1],p2,R[p2]);			
		}
		else if(opc_g!=13)
		{
			if(imm) printf("first operand R%d,Second operand #%d, destination register R%d\nDECODE: Read registers R%d = %d,Immediate = %d\n",p1,p2,ans,p1,R[p1],p2);
			else printf("first operand R%d,Second operand R%d, destination register R%d\nDECODE: Read registers R%d = %d,R%d = %d\n",p1,p2,ans,p1,R[p1],p2,R[p2]);
		}
		else
		{
			if(imm) printf("%d to R%d\n",p2,ans);
			else printf("%d to R%d\n",R[p2],ans);
		}
	}
}
//executes the ALU operation based on ALUop
void execute() {
	int extra,p2,cond;
	cond=instruction_word >> 28;
	extra=instruction_word>>26;
	extra=extra & 3;
	printf("EXECUTE: ");
	int temp;
	if(extra==1)
	{
		//Data-transfer
		if(opc_g==25) //LDR
		{
			if(!((instruction_word>>25) & 1)) //Immediate
			acc=R[operand1]+operand2;
			else
			acc=R[operand1]+R[operand2];
		}
		else if(opc_g==24) //STR
		{
			if(!((instruction_word>>25) & 1)) //Immediate
			acc=R[operand1]+operand2;
			else
			acc=R[operand1]+R[operand2];
		}
		printf("Nothing to execute\n");
	}
	else if(extra==2)
	{
		//Branch ; change value of PC accordingly
		unsigned int branch_p2;
		opc_g=instruction_word >> 24;
		branch_p2= instruction_word & 16777215; //Last 24 bits
		if((branch_p2>>23)&1) //If MSB is one
		{
			p2=(~branch_p2); //Make number -ve appropriately
			p2=p2 & 16777215;
			p2+=1;
			p2*=(-1);
		}
		else
		{
			p2=branch_p2; //Orherwise leav it as it is
		}
		// Indicator used to check if condition is true
		switch(cond)
		{
			case 0: if(Z) indicator=1; break;
			case 1: if(!Z) indicator=1; break;
			case 10: if((!N)||(Z)) indicator=1; break;
			case 11: if(N && (!Z)) indicator=1; break;
			case 12: if(!N && (!Z)) indicator=1; break; 
			case 13: if((Z)||N) indicator=1; break;
			case 14: indicator=1; break; 
		}

		if(indicator) { printf("Condition True\n"); printf("Offset : %d\n",p2);R[15]+=p2*4 + 4; indicator=0;} //PC = PC + SignExt32(offset x 4) + 8 
		else printf("Condition False\n");
	}
	else
	{
		//Data Processing
		switch(opc_g)
		{
			case 0: printf("AND %d and %d\n",operand1,operand2); acc=operand1&operand2; break; //AND
			case 1: printf("EOR %d and %d\n",operand1,operand2);acc=operand1^operand2; break; //EOR
			case 2: printf("SUB %d and %d\n",operand1,operand2);acc=operand1-operand2; break; //SUB
			case 4: printf("ADD %d and %d\n",operand1,operand2);acc=operand1+operand2; break; //ADD
			case 5: printf("ADC %d and %d\n",operand1,operand2);acc=operand1+operand2; C=(operand1>>31)&(operand2>>31); break; //ADC
			case 10: printf("CMP %d and %d\n",operand1,operand2);temp=operand1-operand2; Z=(!temp); N=(temp<0); printf("Flags updated Z :%d , N :%d\n",Z,N);break; //CMP
			case 12: printf("ORR %d and %d\n",operand1,operand2);acc=operand1|operand2; break; //ORR
			case 13: printf("Nothing\n"); acc=operand2; break; //Nothing to do in MOV
			case 15: printf("Nothing\n");acc=~operand2; printf("MNV, "); break; //Nothing to do in MOV
		}
	}
}
//perform the memory operation
void mem() {
	int extra;
	extra=instruction_word>>26;
	extra=extra & 3;
	if(extra==1)
	{
		//Data-transfer
		if(opc_g==25) //LDR
		{
			acc=read_word(MEM,acc);
			printf("MEMORY:Load data %d into  R%d\n",acc,ans);
		}
		else if(opc_g==24) //STR
		{
			printf("MEMORY:Store data %d into memory at %d\n",R[ans],acc);
		}
	}
	else
	{
		//Data processing or Branch;  do nothing
		printf("MEMORY:No memory  operation\n");
	}
}
//writes the results back to register file
void write_back() {
	int extra;
	extra=instruction_word>>26;
	extra=extra & 3;
	if((extra==1) && opc_g==24) //STR 
	{
		printf("WRITEBACK: No writeback.\n\n");
		write_word(MEM,acc,R[ans]); 
	}
	else
	{
		if(extra==0 && opc_g==10) {printf("WRITEBACK: Flags updated\n\n");}
		else if(extra==2) {printf("WRITEBACK: Nothing to write.\n\n");}
		else
		{
			R[ans]=acc;
			printf("WRITEBACK: Write %d to R%d\n\n",acc,ans);
		}
	}
	R[15]+=4;
}

void run_armsim() {
  R[15]=0;
  while(1) {
    fetch();
    decode();
    execute();
    mem();
    write_back();
  }
}
