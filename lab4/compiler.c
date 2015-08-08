#include "utilities/memory_table.h"
#include "intercode.h"

extern void yyrestart(FILE * input_file);

struct intercode * 
parse_single_intercode(const char *);

struct intercode * intercodes[INSTR_COUNT];

int get_intercodes();

char *
strcopy(const char *);

FILE * asm_file = NULL;

int
streq(const char *s1, const char * s2) {
	return (
		strlen(s1) == strlen(s2) &&
		strncmp(s1, s2, strlen(s2)) == 0
	);
}

void make_assembly(int);

void test_intercodes(int);

int main(int argc, char ** argv) {
	FILE * f;
	if (argc == 1) {
		printf("Usage: %s [input.cmm] [output.ir] or %s [input.cmm]\n",
				argv[0], argv[0]);
	}
	int i = 0;
	if (argc == 3) {
		outputfile = argv[2];
	} else if (argc == 2) {
		outputfile = strcopy(argv[1]);
		char * p = strstr(outputfile, "cmm");
		if (!p || *(p-1) != '.') {
			puts("input file name format invalid");
			return -1;
		}
		p[0] = 'a';
		p[1] = 's';
	}	
	f = fopen(argv[1], "r");
	if (!f) {
		printf("File \'%s\' not exist\n", argv[1]);
		return 1;
	}
	yyrestart(f);
	lineno = 1;
	error = 0;
	semerror = 0;
	current_file = argv[i];
	//:yydebug=1;
	yyparse();
	fclose(f);
	if (root && !error) {
		// print_tree(root);
		init_var_table();
		init_func_table();
		init_struct_table();
		preprocess();
		function_field = NULL;
		//printf("now analyse\n");
		semantic_analysis(root);
		if ( !semerror ) {
			memset(label_flags, 0, sizeof(label_flags));
			translate(root);
			// flush_code();
			int ic_count = get_intercodes();
			make_assembly(ic_count);
		}
		clear_var_table();
		clear_func_table();
		clear_struct_table();
	}

	return 0;
}

char *
strcopy(const char *str) {
	char * copy = (char *)malloc( sizeof(char) * ( strlen(str) + 1 ) );
	memset(copy, 0, sizeof(char) * (strlen(str)+1));
	strncpy(copy, str, strlen(str));
	return copy;
}

void 
parse_operand(const char * operand_str, struct operand * opr_ptr) {
	memset(opr_ptr, 0, sizeof(struct operand));
	if (operand_str[0] == '#') {
		opr_ptr -> type = OPR_CONSTANT;
		opr_ptr -> opr_value = atoi( operand_str + 1 );
	} else if (operand_str[0] == '&') {
		opr_ptr -> type = OPR_GETADDR;
		opr_ptr -> addr_variable = strcopy( operand_str + 1 );
	} else if (operand_str[0] == '*') {
		opr_ptr -> type = OPR_DEREFERENCE;
		opr_ptr -> deref_address = strcopy( operand_str + 1 );
	} else {
		opr_ptr -> type = OPR_VARIABLE;
		opr_ptr -> opr_variable = strcopy( operand_str );
	}
}

const char *
operand_to_str(struct operand * opr) {
	static char buf[64];
	memset(buf, 0, sizeof(buf));
	switch (opr -> type) {
		case OPR_VARIABLE:
			return opr -> opr_variable;
		case OPR_CONSTANT:
			sprintf(buf, "#%d", opr -> opr_value);
			return buf;
		case OPR_GETADDR:
			sprintf(buf, "&%s", opr -> addr_variable);
			return buf;
		case OPR_DEREFERENCE:
			sprintf(buf, "*%s", opr -> deref_address);
			return buf;
		default:
			return "ERROR";
	}
}


struct intercode *
parse_single_intercode(const char *instr_str) {
	char * instr_str_copy = strcopy(instr_str);
	
	char * lexeme[6] = {NULL};
	int lexeme_count = 0;

	char * p = strtok(instr_str_copy, " ");
	while (p) {
		assert( lexeme_count < 6 );
		lexeme[ lexeme_count ] = p;
		lexeme_count ++;
		p = strtok(NULL, " ");
	}
	
	struct intercode * ic = (struct intercode *) malloc ( sizeof(struct intercode) );
	memset(ic, 0, sizeof(struct intercode));
	
	switch (lexeme_count) {
		case 2:
			// GOTO, READ, WRITE, RETURN, ARG, PARAM
			if ( streq( lexeme[0], "GOTO" ) ) {
				ic -> type = IR_GOTO;
				ic -> goto_target = strcopy( lexeme[1] );
			} else if ( streq( lexeme[0], "READ" ) ) {
				ic -> type = IR_READ;
				parse_operand( lexeme[1], &(ic -> read_name) );
			} else if ( streq( lexeme[0], "WRITE" ) ) {
				ic -> type = IR_WRITE;
				parse_operand( lexeme[1], &(ic -> write_name) );
			} else if ( streq( lexeme[0], "RETURN" ) ) {
				ic -> type = IR_RETURN;
				parse_operand( lexeme[1], &(ic -> return_target));
			} else if ( streq( lexeme[0], "ARG" ) ) {
				ic -> type = IR_ARG;
				parse_operand( lexeme[1], &(ic -> argument) );
			} else if ( streq( lexeme[0], "PARAM" ) ) {
				ic -> type = IR_PARAM;
				ic -> parameter = strcopy(lexeme[1]);
			} else {
				assert(0);
			}
			break;
		case 3:
			if ( streq( lexeme[0], "FUNCTION" ) ) {
				ic -> type = IR_FUNCTION;
				ic -> function_name = strcopy(lexeme[1]);
			} else if ( streq( lexeme[1], ":=" ) ) {
				// assign
				ic -> type = IR_ASSIGN;
				parse_operand( lexeme[0], &(ic -> assign.target) );
				parse_operand( lexeme[2], &(ic -> assign.rhs) );
			} else if ( streq( lexeme[0], "DEC" ) ) {
				ic -> type = IR_DEC;
				ic -> dec.variable = strcopy(lexeme[1]);
				ic -> dec.size = atoi( lexeme[2] );
			} else if ( streq( lexeme[0], "LABEL" ) ) {
				printf("panic: LABEL intercode");
			} else {
				assert(0);
			}
			break;
		case 4:
			assert( streq( lexeme[2], "CALL" ) );
			ic -> type = IR_CALL;
			parse_operand( lexeme[0], &(ic -> call.target) );
			ic -> call.call_function_name = strcopy( lexeme[3] );
			break;
		case 5:
			ic -> type = IR_ARITH;
			parse_operand( lexeme[0], &(ic -> arith_operation.target) );
			parse_operand( lexeme[2], &(ic -> arith_operation.operand1) );
			ic -> arith_operation.arith_operator = strcopy( lexeme[3] );
			parse_operand( lexeme[4], &(ic -> arith_operation.operand2) );
			break;
		case 6:
			ic -> type = IR_IFGOTO;
			parse_operand( lexeme[1], &(ic -> if_goto.operand1) );
			ic -> if_goto.bool_operator = strcopy( lexeme[2] );
			parse_operand( lexeme[3], &(ic -> if_goto.operand2) );
			ic -> if_goto.if_goto_target = strcopy( lexeme[5] );
			break;
		default:
			printf(" * WTF is \'%s\' ?\n", instr_str);
			break;
	}
		
	free(instr_str_copy);
	return ic;
}


int get_intercodes() {
	memset(intercodes, 0, sizeof(intercodes));
	int i = 0;
	int ic_count = 0;
	for (; i < instruction_count; ++i) {
		if (label_flags[i]) {
			struct intercode * new_intercode = 
				(struct intercode *) malloc (sizeof(struct intercode));
			memset( new_intercode, 0, sizeof(struct intercode) );
			new_intercode -> type = IR_LABEL;
			char * lname = (char *) malloc ( sizeof(char) * 16 );
			memset(lname, 0, sizeof(char) * 16 );
			sprintf(lname, "L__%d", i);
			new_intercode -> label_name = lname;
			intercodes[ ic_count ] = new_intercode;
			ic_count ++;
		}
		intercodes[ ic_count ] = parse_single_intercode( instructions[i] );
		ic_count ++;
	}

	return ic_count;
}

void test_intercodes(int ic_count) {
	int i;
	for (i = 0; i < ic_count; ++i) {
		struct intercode * ic = intercodes[i];
		switch (ic -> type) {
			case IR_LABEL:
				printf("LABEL %s :\n", ic -> label_name);
				break;
			case IR_FUNCTION:
				printf("FUNCTION %s :\n", ic -> function_name);
				break;
			case IR_ASSIGN:
				printf("%s := %s\n", 
						operand_to_str(&ic -> assign.target), 
						operand_to_str(&ic -> assign.rhs));
				break;
			case IR_ARITH:
				printf("%s := %s %s %s\n",
						operand_to_str(&ic -> arith_operation.target),
						operand_to_str(&ic -> arith_operation.operand1),
						ic -> arith_operation.arith_operator,
						operand_to_str(&ic -> arith_operation.operand2));
				break;
			case IR_GOTO:
				printf("GOTO %s\n", ic -> goto_target);
				break;
			case IR_IFGOTO:
				printf("IF %s %s %s GOTO %s\n", 
						operand_to_str(&ic -> if_goto.operand1), 
						ic -> if_goto.bool_operator,
						operand_to_str(&ic -> if_goto.operand2), 
						ic -> if_goto.if_goto_target);
				break;
			case IR_RETURN:
				printf("RETURN %s\n", 
						operand_to_str(&ic -> return_target));
				break;
			case IR_DEC:
				printf("DEC %s %d\n", ic -> dec.variable, ic -> dec.size);
				break;
			case IR_ARG:
				printf("ARG %s\n", operand_to_str(&ic -> argument));
				break;
			case IR_CALL:
				printf("%s := CALL %s\n", 
						operand_to_str(&ic -> call.target), ic -> call.call_function_name);
				break;
			case IR_PARAM:
				printf("PARAM %s\n", ic -> parameter);
				break;
			case IR_READ:
				printf("READ %s\n", operand_to_str(&ic -> read_name));
				break;
			case IR_WRITE:
				printf("WRITE %s\n", operand_to_str(&ic -> write_name));
				break;
			default:
				printf("**** WTF is type %d?\n", ic -> type);
				break;
		}
	}
}

void load_to_reg(struct memory_table * mt, const char * temp_reg, struct operand * opr) {
	if (opr -> type == OPR_VARIABLE) {
		int offset = get_position(mt, opr -> opr_variable);
		fprintf(asm_file, "  lw %s, %d($fp)\t\t# var:%s\n", temp_reg, -offset, opr -> opr_variable);
	} else if (opr -> type == OPR_CONSTANT) {
		fprintf(asm_file, "  addi %s, $0, %d\t\t# constant\n", temp_reg, opr -> opr_value);
	} else if (opr -> type == OPR_GETADDR) {
		int offset = get_position(mt, opr -> addr_variable);
		fprintf(asm_file, "  addi %s, $fp, %d\t\t# get address of %s\n", temp_reg, -offset,
				opr -> addr_variable);
	} else if (opr -> type == OPR_DEREFERENCE) {
		int offset = get_position(mt, opr -> deref_address);
		fprintf(asm_file, "  lw %s, %d($fp)\t\t# dereference %s\n", temp_reg, -offset,
				opr -> deref_address);
		fprintf(asm_file, "  lw %s, 0(%s)\n", temp_reg, temp_reg);
	} else {
		assert(0);
	}
}

void store_to_mem(struct memory_table * mt, const char * source_reg, struct operand * opr) {
	if (opr -> type == OPR_VARIABLE) {
		int offset = get_position(mt, opr -> opr_variable);
		fprintf(asm_file, "  sw %s, %d($fp)\t\t# store to regular mem %s\n", source_reg, -offset,
				opr -> opr_variable);
	} else if (opr -> type == OPR_DEREFERENCE) {
		int offset = get_position(mt, opr -> deref_address);
		fprintf(asm_file, "  lw $t3, %d($fp)\t\t# store to mem at %s\n", -offset, opr -> deref_address);
		fprintf(asm_file, "  sw %s, 0($t3)\n", source_reg);
	} else {
		assert(0);
	}
}

void make_assembly_function(struct memory_table * mt, int fstart, int fend) {
	struct intercode * ic = intercodes[fstart];
	if (ic -> type != IR_FUNCTION) {
		printf("panic: not a function\n");
		return;
	}
	fprintf(asm_file, "%s:\n", ic -> function_name);
	int i = fstart + 1;
	// store frame
	fprintf(asm_file, "  addi $sp, $sp, -4\t\t# push old fp on stack\n");
	fprintf(asm_file, "  sw $fp, 0($sp)\n");
	fprintf(asm_file, "  move $fp, $sp\t\t# update fp\n");
	// allocate space in stack
	int allocate_space = mt -> offsets[mt -> num_entry - 1];
	fprintf(asm_file, "  addi $sp, $sp, %d\n", -allocate_space);
	int arg_count = 0;
	for(; i < fend; ++i) {
		ic = intercodes[i];
		switch(ic -> type) {
			case IR_LABEL:
				fprintf(asm_file, "%s:\n", ic -> label_name);
				break;
			case IR_ASSIGN: 
				load_to_reg(mt, "$t1", &(ic -> assign.rhs) );
				store_to_mem(mt, "$t1", &(ic -> assign.target) );
				break;
			case IR_FUNCTION:
				fprintf(asm_file, "%s:\n", ic -> function_name);
				break;
			case IR_ARITH:
				load_to_reg(mt, "$t1", &(ic -> arith_operation.operand1));
				load_to_reg(mt, "$t2", &(ic -> arith_operation.operand2));
				if ( streq("+", ic -> arith_operation.arith_operator) ) {
					fprintf(asm_file, "  add $t0, $t1, $t2\n");
				} else if ( streq("-", ic -> arith_operation.arith_operator) ) {
					fprintf(asm_file, "  sub $t0, $t1, $t2\n");
				} else if ( streq("*", ic -> arith_operation.arith_operator) ) {
					fprintf(asm_file, "  mul $t0, $t1, $t2\n");
				} else if ( streq("/", ic -> arith_operation.arith_operator) ) {
					fprintf(asm_file, "  div $t1, $t2\n");
					fprintf(asm_file, "  mflo $t0\n");
				}
				store_to_mem(mt, "$t0", &(ic -> arith_operation.target) );
				break;
			case IR_GOTO:
				fprintf(asm_file, "  j %s\n", ic -> goto_target);
				break;
			case IR_IFGOTO:
				{
					load_to_reg(mt, "$t1", &(ic -> if_goto.operand1));
					load_to_reg(mt, "$t2", &(ic -> if_goto.operand2));
					const char * branch_command = NULL;
					const char * bop = ic -> if_goto.bool_operator;
					if ( streq(bop, ">") )
						branch_command = "bgt";
					else if ( streq(bop, ">=") )
						branch_command = "bge";
					else if ( streq(bop, "<") )
						branch_command = "blt";
					else if ( streq(bop, "<=") )
						branch_command = "ble";
					else if ( streq(bop, "==") )
						branch_command = "beq";
					else if ( streq(bop, "!=") )
						branch_command = "bne";
					else
						assert(0);
					fprintf(asm_file, "  %s $t1, $t2, %s\n", branch_command, ic -> if_goto.if_goto_target);
				}
				break;
			case IR_RETURN:
				load_to_reg(mt, "$v0", &(ic -> return_target));
				fprintf(asm_file, "  addi $sp, $sp, %d\t\t# release space for local var\n", allocate_space);
				fprintf(asm_file, "  lw $fp, 0($sp)\t\t# restore old fp\n");
				fprintf(asm_file, "  addi $sp, $sp, 4\t\t# pop\n");
				fprintf(asm_file, "  jr $ra\n");
				break;
			case IR_DEC:
				break;
			case IR_ARG:
				load_to_reg(mt, "$t0", &(ic -> argument));
				fprintf(asm_file, "  addi $sp, $sp, -4\n");
				fprintf(asm_file, "  sw $t0, 0($sp)\t\t# push argument onto stack\n");
				arg_count ++;
				break;
			case IR_CALL:
				fprintf(asm_file, "  addi $sp, $sp, -4\n");
				fprintf(asm_file, "  sw $ra, 0($sp)\t\t# push $ra on stack\n");
				fprintf(asm_file, "  jal %s\n", ic -> call.call_function_name);
				fprintf(asm_file, "  lw $ra, 0($sp)\t\t# pop $ra from stack\n");
				fprintf(asm_file, "  addi $sp, $sp, 4\n");
				store_to_mem(mt, "$v0", &(ic -> call.target));
				// release mem for arguments
				fprintf(asm_file, "  addi $sp, $sp, %d\t\t# release space for arguments\n", 4 * arg_count);
				arg_count = 0;
				
				break;
			case IR_PARAM:
				break;
			case IR_READ:
				fprintf(asm_file, "  addi $sp, $sp, -4\n");
				fprintf(asm_file, "  sw $ra, 0($sp)\n");
				fprintf(asm_file, "  jal read\n");
				fprintf(asm_file, "  lw $ra, 0($sp)\n");
				fprintf(asm_file, "  addi $sp, $sp, 4\n");
				store_to_mem(mt, "$v0", &(ic -> read_name));
				break;
				
			case IR_WRITE:
				load_to_reg(mt, "$a0", &(ic -> write_name));
				fprintf(asm_file, "  addi $sp, $sp, -4\n");
				fprintf(asm_file, "  sw $ra, 0($sp)\n");
				fprintf(asm_file, "  jal write\n");
				fprintf(asm_file, "  lw $ra, 0($sp)\n");
				fprintf(asm_file, "  addi $sp, $sp, 4\n");
				break;
			default:
				break;
		}
	}
}

void make_assembly(int ic_count) {
	// preamble
	asm_file = fopen(outputfile, "w+");
	if (!asm_file) {
		puts("file open failed");
		exit(-2);
	}
	fputs(".data\n", asm_file);
	fputs("_prompt: .asciiz \"Enter an integer:\"\n", asm_file);
	fputs("_ret: .asciiz \"\\n\"\n", asm_file);
	fputs(".globl main\n", asm_file);
	fputs(".text\n", asm_file);

	fputs("read:\n", asm_file);
	fputs("  li $v0, 4\n", asm_file);
	fputs("  la $a0, _prompt\n", asm_file);
	fputs("  syscall\n", asm_file);
	fputs("  li $v0, 5\n", asm_file);
	fputs("  syscall\n", asm_file);
	fputs("  jr $ra\n", asm_file);

	fputs("write:\n", asm_file);
	fputs("  li $v0, 1\n", asm_file);
	fputs("  syscall\n", asm_file);
	fputs("  li $v0, 4\n", asm_file);
	fputs("  la $a0, _ret\n", asm_file);
	fputs("  syscall\n", asm_file);
	fputs("  move $v0, $0\n", asm_file);
	fputs("  jr $ra\n", asm_file);
			   
	// differ various functions
	int fstart = 0, fend = 0;
	while (fend < ic_count) {
		while ( intercodes[fstart] -> type != IR_FUNCTION )
			fstart++;
		fend = fstart + 1;
		while ( fend < ic_count && intercodes[fend] -> type != IR_FUNCTION )
			fend++;

		struct memory_table * mt = make_memory_table(fstart, fend);

		// generate assembly code for this function
		
		make_assembly_function(mt, fstart, fend);

		fstart = fend;
	}
	fclose(asm_file);
}
