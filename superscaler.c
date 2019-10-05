/**************************************************************/
/* CS/COE 1541
compile with gcc -o superscaler superscaler.c
and execute using
./Superscaler  /afs/cs.pitt.edu/courses/1541/short_traces/sample.tr	0
***************************************************************/

#include <stdio.h>

#include <stdlib.h>

#include <inttypes.h>

#include <stdbool.h>

//Linux requires a file arpa/inet.h where as windows requires winsock2, so check the platform and adjust accoridingly
#ifdef _WIN32
	#include <winsock2.h>

#else
	#include <arpa/inet.h>

# endif
#include "CPU.h"

int main(int argc, char ** argv) {
	struct instruction * tr_entry, *tr_entry2;
	struct instruction PCregister, PCregister_B, IF_ID_A, IF_ID_B, ID_EX1, ID_EX2, EX_MEM1, EX_MEM2, MEM_WB1, MEM_WB2;
	size_t size, size2;
	char * trace_file_name;
	int trace_view_on = 0;
	int flush_counter = 4; //5 stage pipeline, so we have to execute 4 instructions once trace is done
	bool replaceA = false;
	unsigned int cycle_number = 0;

	if (argc == 1) {
		fprintf(stdout, "\nUSAGE: tv <trace_file> <switch - any character>\n");
		fprintf(stdout, "\n(switch) to turn on or off individual item view.\n\n");
		exit(0);
	}

	trace_file_name = argv[1];
	if (argc == 3) trace_view_on = atoi(argv[2]);

	fprintf(stdout, "\n ** opening file %s\n", trace_file_name);

	trace_fd = fopen(trace_file_name, "rb");

	if (!trace_fd) {
		fprintf(stdout, "\ntrace file %s not opened.\n\n", trace_file_name);
		exit(0);
	}

	trace_init();

	while (1) {
		size = trace_get_item(&tr_entry); /* put the ALU/Branch instruction into a buffer */
		size2 = trace_get_item(&tr_entry2); /* put the LW/SW instruction into another buffer */

		if (!size && !size2 && flush_counter == 0) {
			/* no more instructions to simulate */
			printf("+ Simulation terminates at cycle : %u\n", cycle_number);
			break;
		} else {
			/* move the pipeline forward */
			cycle_number++;

			/* move instructions one stage ahead */
			MEM_WB1 = EX_MEM1;
			MEM_WB2 = EX_MEM2;
			EX_MEM1 = ID_EX1;
			EX_MEM2 = ID_EX2;
			ID_EX1 = IF_ID_A;
			ID_EX2 = IF_ID_B;
			
			//If the last instruction was unable to be pushed into the pipeline, we must wait to use it next cycle instead
			//This ensures that the next instruction is only put into the pipe if the 2 previous ones were pushed through the pipe
			if (!replaceA) {
				IF_ID_A = PCregister;
			}

			IF_ID_B = PCregister_B;

			replaceA = false;

			if (!size && !size2) {
				/* if no more instructions in trace, reduce flush_counter */
				flush_counter--;
			} else {
				/* copy trace entry into IF stage */
				if ((tr_entry -> type != ti_STORE || tr_entry -> type != ti_LOAD) && (tr_entry2 -> type != ti_STORE || tr_entry2 -> type != ti_LOAD)) {
					//Both are ALU/Branch instructions, only the first can go through
					memcpy(&PCregister, tr_entry, sizeof(PCregister));
					tr_entry = tr_entry2;
					tr_entry2 -> type = ti_NOP;
					memcpy(&PCregister_B, tr_entry2, sizeof(PCregister_B));
					replaceA = true;
				} else if ((tr_entry -> type != ti_STORE || tr_entry -> type != ti_LOAD) && (tr_entry2 -> type == ti_STORE || tr_entry2 -> type == ti_LOAD)) {
					//Instruction 1 is ALU/Branch and instruction 2 is LW/SW, set instruction 1 to go through pipeline A and instruction 2 to go through pipeline B
					memcpy(&PCregister, tr_entry, sizeof(PCregister));
					memcpy(&PCregister_B, tr_entry2, sizeof(PCregister_B));
				} else if ((tr_entry -> type == ti_STORE || tr_entry -> type == ti_LOAD) && (tr_entry2 -> type != ti_STORE || tr_entry2 -> type != ti_LOAD)) {
					//Instruction 1 is LW/SW and instruction 2 is ALU/Branch type; put instruction 1 through pipeline B and instruction 2 through pipeline A
					memcpy(&PCregister, tr_entry2, sizeof(PCregister));
					memcpy(&PCregister_B, tr_entry, sizeof(PCregister_B));
				} else if ((tr_entry -> type == ti_STORE || tr_entry -> type == ti_LOAD) && (tr_entry2 -> type == ti_STORE || tr_entry2 -> type == ti_LOAD)) {
					//Instruction 1 and instruction 2 are both LW/SW, only 1 can go through...Set instruction 1 to go through pipeline B and Hold instruction 2 for next cycle
					memcpy(&PCregister, tr_entry, sizeof(PCregister));
					tr_entry = tr_entry2;
					tr_entry2 -> type = ti_NOP;
					memcpy(&PCregister_B, tr_entry2, sizeof(PCregister_B));
					replaceA = true;
				} 
				//-----------------------------------------WHAT ABOUT THE OTHER INSTRUCTIONS LIKE J, THEY'RE NOT MENTIONED IN THE PDF, WHICH PIPE DO THEY GO DOWN?----------

				//memcpy(&PCregister, tr_entry , sizeof(PCregister));
			}

			//printf("==============================================================================\n");
		}

		if (trace_view_on && cycle_number >= 5) {
			/* print the instruction exiting the pipeline if trace_view_on=1 */

			//Print Instruction exiting Pipeline A
			switch (MEM_WB1.type) {
			case ti_NOP:
				printf("[cycle %d] NOP:\n", cycle_number);
				break;
			case ti_RTYPE:
				/* registers are translated for printing by subtracting offset  */
				printf("[cycle %d] RTYPE:", cycle_number);
				printf(" (PC: %d)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", MEM_WB1.PC, MEM_WB1.sReg_a, MEM_WB1.sReg_b, MEM_WB1.dReg);
				break;
			case ti_ITYPE:
				printf("[cycle %d] ITYPE:", cycle_number);
				printf(" (PC: %d)(sReg_a: %d)(dReg: %d)(addr: %d)\n", MEM_WB1.PC, MEM_WB1.sReg_a, MEM_WB1.dReg, MEM_WB1.Addr);
				break;
			case ti_LOAD:
				printf("[cycle %d] LOAD:", cycle_number);
				printf(" (PC: %d)(sReg_a: %d)(dReg: %d)(addr: %d)\n", MEM_WB1.PC, MEM_WB1.sReg_a, MEM_WB1.dReg, MEM_WB1.Addr);
				break;
			case ti_STORE:
				printf("[cycle %d] STORE:", cycle_number);
				printf(" (PC: %d)(sReg_a: %d)(sReg_b: %d)(addr: %d)\n", MEM_WB1.PC, MEM_WB1.sReg_a, MEM_WB1.sReg_b, MEM_WB1.Addr);
				break;
			case ti_BRANCH:
				printf("[cycle %d] BRANCH:", cycle_number);
				printf(" (PC: %d)(sReg_a: %d)(sReg_b: %d)(addr: %d)\n", MEM_WB1.PC, MEM_WB1.sReg_a, MEM_WB1.sReg_b, MEM_WB1.Addr);
				break;
			case ti_JTYPE:
				printf("[cycle %d] JTYPE:", cycle_number);
				printf(" (PC: %d)(addr: %d)\n", MEM_WB1.PC, MEM_WB1.Addr);
				break;
			case ti_SPECIAL:
				printf("[cycle %d] SPECIAL:\n", cycle_number);
				break;
			case ti_JRTYPE:
				printf("[cycle %d] JRTYPE:", cycle_number);
				printf(" (PC: %d) (sReg_a: %d)(addr: %d)\n", MEM_WB1.PC, MEM_WB1.dReg, MEM_WB1.Addr);
				break;
			}

			//Print instruction exiting Pipeline B
			switch (MEM_WB2.type) {
			case ti_NOP:
				printf("[cycle %d] NOP:\n", cycle_number);
				break;
			case ti_RTYPE:
				/* registers are translated for printing by subtracting offset  */
				printf("[cycle %d] RTYPE:", cycle_number);
				printf(" (PC: %d)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", MEM_WB2.PC, MEM_WB2.sReg_a, MEM_WB2.sReg_b, MEM_WB2.dReg);
				break;
			case ti_ITYPE:
				printf("[cycle %d] ITYPE:", cycle_number);
				printf(" (PC: %d)(sReg_a: %d)(dReg: %d)(addr: %d)\n", MEM_WB2.PC, MEM_WB2.sReg_a, MEM_WB2.dReg, MEM_WB2.Addr);
				break;
			case ti_LOAD:
				printf("[cycle %d] LOAD:", cycle_number);
				printf(" (PC: %d)(sReg_a: %d)(dReg: %d)(addr: %d)\n", MEM_WB2.PC, MEM_WB2.sReg_a, MEM_WB2.dReg, MEM_WB2.Addr);
				break;
			case ti_STORE:
				printf("[cycle %d] STORE:", cycle_number);
				printf(" (PC: %d)(sReg_a: %d)(sReg_b: %d)(addr: %d)\n", MEM_WB2.PC, MEM_WB2.sReg_a, MEM_WB2.sReg_b, MEM_WB2.Addr);
				break;
			case ti_BRANCH:
				printf("[cycle %d] BRANCH:", cycle_number);
				printf(" (PC: %d)(sReg_a: %d)(sReg_b: %d)(addr: %d)\n", MEM_WB2.PC, MEM_WB2.sReg_a, MEM_WB2.sReg_b, MEM_WB2.Addr);
				break;
			case ti_JTYPE:
				printf("[cycle %d] JTYPE:", cycle_number);
				printf(" (PC: %d)(addr: %d)\n", MEM_WB2.PC, MEM_WB2.Addr);
				break;
			case ti_SPECIAL:
				printf("[cycle %d] SPECIAL:\n", cycle_number);
				break;
			case ti_JRTYPE:
				printf("[cycle %d] JRTYPE:", cycle_number);
				printf(" (PC: %d) (sReg_a: %d)(addr: %d)\n", MEM_WB2.PC, MEM_WB2.dReg, MEM_WB2.Addr);
				break;
			}
		}
	}

	trace_uninit();

	exit(0);
}