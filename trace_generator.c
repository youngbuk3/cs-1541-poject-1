/**************************************************************/
/* CS/COE 1541				 			
This program allows the user to interactively input the instructions for a trace 
and produce a trace file with these instructions readable by five_stage.c.
The program takes the name of the file to be generated as an argument.
***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "CPU.h" 

int main(int argc, char **argv)
{
  struct instruction *tr_entry=malloc(sizeof(struct instruction));
  size_t size;
  char *trace_file_name;
  
  unsigned char t_type ;
  unsigned int t_sReg_a;
  unsigned int t_sReg_b;
  unsigned int t_dReg;
  unsigned int t_PC ;
  unsigned int t_Addr ;

  unsigned int cycle_number = 0;

  if (argc == 1) {
    fprintf(stdout, "\nMissing argument: the name of the file to be generated\n");
    exit(0);
  }
  trace_file_name = argv[1];
  fopen(trace_file_name, "w");
  int check;
  int trcount, i, repeat;
  char itype ;

  printf("\nEnter the number of instructions you want to have in the file \"%s\": ", trace_file_name);
  scanf("%d", &trcount );
  printf("\n") ;
  printf("For each instruction, enter PC, itype, sReg_a, sReg_b, d_Reg and addr, \n");
  printf("where PC, sReg_a, sReg_b, d_Reg and addr are integers and \n");
  printf("itype is a character: R for R-type, L for load, S for store, B for branch or N for No-op \n");

  for (i = 0 ; i < trcount ; i++) {
    printf("\nEnter exactly 6 fields for instruction %d: ", i);
    scanf("%d %c %d %d %d %d",  &tr_entry->PC, &itype, &t_sReg_a, &t_sReg_b, &t_dReg, &tr_entry->Addr) ;

    //set the register values after casting
    tr_entry->sReg_a = (char) t_sReg_a;
    tr_entry->sReg_b = (char) t_sReg_b;
    tr_entry->dReg = (char) t_dReg;

    repeat = 0 ;
    if(itype == 'R') {tr_entry->type = ti_RTYPE ;} 
      else if (itype == 'L') {tr_entry->type = ti_LOAD;} 
      else if (itype == 'S') {tr_entry->type = ti_STORE;} 
      else if (itype == 'B') {tr_entry->type = ti_BRANCH ;} 
      else if (itype == 'N') {tr_entry->type = ti_NOP ;}
      else {printf("unrecognized instruction type -- try again ") ; repeat = 1;  i-- ; }

    //write the instruction into the trace file
    if (repeat == 0) write_trace(*tr_entry, trace_file_name);
  }
  printf("Now, the file \"%s\" contains the following instructions: \n", trace_file_name);
  trace_fd = fopen(trace_file_name, "rb");
  trace_init();
  while(1) {
    size = trace_get_item(&tr_entry);
   
    if (!size) 
      break; 

    // Display the generated trace 
    switch(tr_entry->type) {
        case ti_NOP:
          printf("NOP: \n") ;
          break;
        case ti_RTYPE: /* registers are translated for printing by subtracting offset  */
          printf("RTYPE: ");
		  printf("(PC: %d)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->dReg);
          break;
        case ti_LOAD:      
          printf("LOAD: ");
		  printf("(PC: %d)(sReg_a: %d)(dReg: %d)(addr: %d)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->dReg, tr_entry->Addr);
          break;
        case ti_STORE:    
          printf("STORE: ");
		  printf("(PC: %d)(sReg_a: %d)(sReg_b: %d)(addr: %d)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->Addr);
          break;
        case ti_BRANCH:
          printf("BRANCH: ");
		  printf("(PC: %d)(sReg_a: %d)(sReg_b: %d)(addr: %d)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->Addr);
          break;
    }
  }

  trace_uninit();

  exit(0);
}