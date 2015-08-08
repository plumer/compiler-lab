#include "intercode.h"


char * instructions[INSTR_COUNT] ;
char label_flags[INSTR_COUNT];

int instruction_count = 0;

void init_label_flags() {
	memset(label_flags, 0, sizeof(label_flags));
}

void preprocess() {
	struct func_entry * rfe = add_func_entry("read");
	rfe -> return_type_flag = TYPE_BASIC;
	rfe -> return_type.basic_type = TYPE_INT;
	rfe -> defined_at = 0;
	
	struct func_entry * wfe = add_func_entry("write");
	wfe -> return_type_flag = TYPE_BASIC;
	wfe -> return_type.basic_type = TYPE_INT;
	wfe -> defined_at = 0;
	struct var_entry * param = add_var_entry("__write_parameter__do_not_use");
	param -> type_flag = TYPE_BASIC;
	param -> t.basic_type = TYPE_INT;
	param -> size = 4;
	param -> defined_at = 0;
	add_func_param(wfe, param);
}

void translate(struct tree_node * ptr) {
	if (ptr -> unit_name == NODE_EXTDEF) {
		if (ptr -> flags == FLAG_EXTDEF_FUNCTION) {
			const char * f_name = ptr -> children[1] -> children[0] -> id_extra -> name;
			struct func_entry * fe = search_func_entry( f_name );
			assert(fe);
			output_code("FUNCTION %s :", f_name);

			struct list_head * param_list = &(fe -> param_list), * temp = NULL;
			struct var_entry * p_ve = NULL;
			list_for_each(temp, param_list) {
				p_ve = list_entry(temp, struct param_entry, list_ptr) -> param_ptr;
				output_code("PARAM %s", p_ve -> name);
			}

			assert(ptr -> children[2] -> unit_name == NODE_COMPST);
			translate(ptr -> children[2]);
		}
	} else if (ptr -> unit_name == NODE_COMPST) {
		struct tree_node * deflist = ptr -> children[1], *def;
		struct tree_node * stmtlist = ptr -> children[2], *stmt;
		while (deflist -> flags == FLAG_DEFLIST_MORE) { 
			translate(deflist -> children[0]);
			deflist = deflist -> children[1];
		}
		while (stmtlist -> flags == FLAG_STMTLIST_MORE) {
			translate(stmtlist -> children[0]);
			backpatch( &(stmtlist -> children[0] -> nextlist), get_next_instr() );
			stmtlist = stmtlist -> children[1];
		}
	} else if (ptr -> unit_name == NODE_DEF) {
		struct tree_node * declist = ptr -> children[1];
		while (1) {
			struct tree_node * vardec = declist -> children[0] -> children[0];
			while (vardec -> flags == FLAG_VARDEC_ARRAY)
				vardec = vardec -> children[0];
			struct var_entry * ve = search_var_entry(vardec -> children[0] -> id_extra -> name);
			if (ve -> type_flag == TYPE_BASIC)
				// nothing happens
				;
			else {
				output_code("DEC __ARRAY_%s %d", ve -> name, ve -> size);
				output_code("%s := &__ARRAY_%s", ve -> name, ve -> name);
			}

			// TODO: variable initialization
			if (declist -> children[0] -> flags == FLAG_DEC_INITIALIZED) {
				struct tree_node * exp = declist -> children[0] -> children[2];
				assert(exp -> unit_name == NODE_EXP);
				translate(exp);
				output_code("%s := %s", ve -> name, exp -> addr);
			}

			if (declist -> flags == FLAG_DECLIST_SINGLE) break;
			else declist = declist -> children[2];
		}
	} else if (ptr -> unit_name == NODE_STMT) {
		if (ptr -> flags == FLAG_STMT_EXP)
			translate(ptr -> children[0]);
		else if (ptr -> flags == FLAG_STMT_COMPST) {
			translate(ptr -> children[0]);
			movelist( &(ptr -> nextlist), &(ptr -> children[0] -> nextlist) );
			//							STMTLIST in COMPST
		} else if (ptr -> flags == FLAG_STMT_RETURN) {
			translate(ptr -> children[1]);
			output_code("RETURN %s", ptr -> children[1] -> addr);
		} else if (ptr -> flags == FLAG_STMT_IF) {
			translate_bool(ptr -> children[2]);
			backpatch( &(ptr -> children[2] -> truelist), get_next_instr());
			translate(ptr -> children[4]);
			mergelist( &(ptr -> nextlist), 
					&(ptr -> children[2] -> falselist), &(ptr -> children[4] -> nextlist) );
		} else if (ptr -> flags == FLAG_STMT_IFELSE) {
			translate_bool(ptr -> children[2]);
			backpatch( &(ptr -> children[2] -> truelist), get_next_instr() );
			translate(ptr -> children[4]);
			// why adding 1 to this works? TODO
			backpatch( &(ptr -> children[2] -> falselist), get_next_instr() + 1 );

			struct list_head temp;
			INIT_LIST_HEAD( &temp );
			struct list_head * new_list = (struct list_head *)malloc( sizeof(struct list_head));
			INIT_LIST_HEAD(new_list);
			makelist(new_list, get_next_instr() );
			output_code("GOTO");
			mergelist( &temp, &(ptr -> children[4] -> nextlist), new_list );

			translate(ptr -> children[6]);
			mergelist( &(ptr -> nextlist), &temp, &(ptr -> children[6] -> nextlist) );
		} else if (ptr -> flags == FLAG_STMT_WHILE) {
			int before_condition = get_next_instr();
			translate_bool(ptr -> children[2]);
			int before_body = get_next_instr();
			translate(ptr -> children[4]);

			backpatch( &(ptr -> children[4] -> nextlist), before_condition);
			backpatch( &(ptr -> children[2] -> truelist), before_body);
			struct list_head * n = &(ptr -> nextlist), 
							 * bf = &(ptr -> children[2] -> falselist);
			n -> next = bf -> next;
			n -> prev = bf -> prev;
			n -> next -> prev = n;
			n -> prev -> next = n;
			INIT_LIST_HEAD(bf);
			output_code("GOTO L__%d", before_condition);
			label_flags[before_condition] = 1;
		}
	} else if (ptr -> unit_name == NODE_EXP) {
		if (ptr -> flags == FLAG_EXP_BINOP) {
			struct tree_node * operator = ptr -> children[1];
			if (operator -> unit_name == TOKEN_AND ||
				operator -> unit_name == TOKEN_OR ||
				operator -> unit_name == TOKEN_RELOP) {
				ptr -> addr = new_temp_name();
				output_code("%s := #0", ptr -> addr);
				translate_bool(ptr);
				backpatch( &(ptr -> truelist), get_next_instr());
				backpatch( &(ptr -> falselist), get_next_instr() + 1);
				output_code("%s := #1", ptr -> addr);
			} else {

				translate(ptr -> children[0]);
				translate(ptr -> children[2]);
				char * temp = NULL;
				if ( is_constant(ptr -> children[0] -> addr) 
						&& is_constant(ptr -> children[2] -> addr) ) {
					int lhs_num = atoi(ptr -> children[0] -> addr + 1);
					int rhs_num = atoi(ptr -> children[2] -> addr + 1);
					ptr -> addr = new_name("#%d", operate(lhs_num, ptr -> children[1], rhs_num));
				} else if (temp = optimize_binop(ptr)) {
					ptr -> addr = temp;
				} else {
					ptr -> addr = new_temp_name();
					output_code("%s := %s %s %s", ptr -> addr, ptr -> children[0] -> addr,
							get_op_str(ptr -> children[1]), ptr -> children[2] -> addr);
				}
				// TODO: improvement
			}
		} else if (ptr -> flags == FLAG_EXP_PAREN) {
			translate(ptr -> children[1]);
			ptr -> addr = ptr -> children[1] -> addr;
		} else if (ptr -> flags == FLAG_EXP_UNOP) {
			struct tree_node * operator = ptr -> children[0];
			if (operator -> unit_name == TOKEN_MINUS) {
				translate(ptr -> children[1]);
				if ( is_constant(ptr -> children[1] -> addr) ) {
					int n = atoi(ptr -> children[1] -> addr + 1);
					ptr -> addr = new_name("#%d", -n);
				} else {
					ptr -> addr = new_temp_name();
					output_code("%s := #0 - %s", ptr -> addr, ptr -> children[1] -> addr);
				}
				// TODO: improvement
			} else {
				assert(operator -> unit_name == TOKEN_NOT);
				ptr -> addr = new_temp_name();
				output_code("%s := #1", ptr -> addr);
				translate_bool(ptr -> children[1]);
				backpatch( &(ptr -> truelist), get_next_instr() );
				backpatch( &(ptr -> falselist), get_next_instr() + 1);
				output_code("%s := #0", ptr -> addr);
			}
		} else if (ptr -> flags == FLAG_EXP_CALL_NOARGS) {
			ptr -> addr = new_temp_name();
			const char * f_name = ptr -> children[0] -> id_extra -> name;
			if ( strlen(f_name) == strlen("read") &&
				strncmp(f_name, "read", strlen("read")) == 0) {
				output_code("READ %s", ptr -> addr);
			} else {
				output_code("%s := CALL %s", ptr -> addr, f_name);
			}
		} else if (ptr -> flags == FLAG_EXP_CALL_ARGS) {
			struct tree_node * args = ptr -> children[2];
			ptr -> addr = new_temp_name();
			const char * f_name = ptr -> children[0] -> id_extra -> name;
			if ( strlen(f_name) == strlen("write") &&
				strncmp(f_name, "write", strlen("write")) == 0) {
				translate(args -> children[0]);
				output_code("WRITE %s", args -> children[0] -> addr);
			} else {
				translate_args(args);
				output_code("%s := CALL %s", ptr -> addr, f_name);
			}
		} else if (ptr -> flags == FLAG_EXP_SUBSCRIPT) {
			translate_subscript(ptr);
			char * address = ptr -> addr;
			ptr -> addr = new_name("*%s", address);
		} else if (ptr -> flags == FLAG_EXP_MEMBER) {
			printf("Sorry, member operation is not supported\n");
		} else if (ptr -> flags == FLAG_EXP_INT) {
			ptr -> addr = (char *)malloc( sizeof(char) * 16 );
			memset(ptr -> addr, 0, sizeof(char) * 16);
			sprintf(ptr -> addr, "#%d", ptr -> children[0] -> i_value);
		} else if (ptr -> flags == FLAG_EXP_ID) {
			ptr -> addr = ptr -> children[0] -> id_extra -> name;
		} else if (ptr -> flags == FLAG_EXP_ASSIGN) {
			translate(ptr -> children[0]);
			translate(ptr -> children[2]);
			output_code("%s := %s", ptr -> children[0] -> addr, ptr -> children[2] -> addr);
			ptr -> addr = ptr -> children[2] -> addr;
		}

	} else {
		int i;
		for (i = 0; i < ptr -> num_of_children; ++i) {
			translate(ptr -> children[i]);
		}
	}
			
}

void translate_bool(struct tree_node * ptr) {
	assert(ptr -> unit_name == NODE_EXP);
	if (ptr -> flags == FLAG_EXP_INT) {
		/*
		if (ptr -> children[0] -> i_value != 0) {
			makelist(&(ptr -> truelist), get_next_instr() );
			makelist(&(ptr -> falselist), get_next_instr() + 1);
			output_code("GOTO");
		}*/
		makelist(&(ptr -> truelist), get_next_instr() );
		makelist(&(ptr -> falselist), get_next_instr() + 1);
		output_code("IF #%d != #0 GOTO", ptr -> children[0] -> i_value);
		output_code("GOTO");
		
	} else if (ptr -> flags == FLAG_EXP_ASSIGN) {
		translate(ptr);
		makelist( &(ptr -> truelist), get_next_instr() );
		makelist( &(ptr -> falselist), get_next_instr() + 1);
		output_code("IF %s != #0 GOTO", ptr -> addr);
		output_code("GOTO");
	} else if (ptr -> flags == FLAG_EXP_BINOP) {
		struct tree_node * operator = ptr -> children[1];
		if (operator -> unit_name == TOKEN_RELOP) {
			translate(ptr -> children[0]);
			translate(ptr -> children[2]);
			makelist( &(ptr -> truelist), get_next_instr() );
			makelist( &(ptr -> falselist), get_next_instr() + 1);
			output_code( "IF %s %s %s GOTO", ptr -> children[0] -> addr,
					get_op_str(operator), ptr -> children[2] -> addr);
			output_code("GOTO");
		} else if (operator -> unit_name == TOKEN_AND) {
			translate_bool(ptr -> children[0]);
			backpatch( &(ptr -> children[0] -> truelist), get_next_instr() );
			translate_bool(ptr -> children[2]);
			movelist( &(ptr -> truelist), &(ptr -> children[2] -> truelist) );
			mergelist( &(ptr -> falselist), 
					&(ptr -> children[0] -> falselist), &(ptr -> children[2] -> falselist) );
		} else if (operator -> unit_name == TOKEN_OR) {
			translate_bool(ptr -> children[0]);
			backpatch( &(ptr -> children[0] -> falselist), get_next_instr() );
			translate_bool(ptr -> children[2]);
			mergelist( &(ptr -> truelist),
					&(ptr -> children[0] -> truelist), &(ptr -> children[2] -> truelist) );
			movelist( &(ptr -> falselist), &(ptr -> children[2] -> falselist) );
		} else {
			translate( ptr );
			makelist(&(ptr -> truelist), get_next_instr() );
			makelist(&(ptr -> falselist), get_next_instr() + 1);
			output_code("IF %s != #0 GOTO", ptr -> addr);
			output_code("GOTO");
		}
	} else if (ptr -> flags == FLAG_EXP_UNOP) {
		struct tree_node * operator = ptr -> children[0];
		if (operator -> unit_name == TOKEN_MINUS) {
			translate( ptr -> children[1] );
			makelist(&(ptr -> truelist), get_next_instr() );
			makelist(&(ptr -> falselist), get_next_instr() + 1);
			output_code("IF %s != #0 GOTO", ptr -> children[1] -> addr);
			output_code("GOTO");
		} else if (operator -> unit_name == TOKEN_NOT) {
			translate_bool(ptr -> children[1]);
			movelist( &(ptr -> truelist), &(ptr -> children[1] -> falselist) );
			movelist( &(ptr -> falselist), &(ptr -> children[1] -> truelist) );
		} else {
			assert(0);
		}
	} else {
		translate(ptr);
		makelist( &(ptr -> truelist), get_next_instr() );
		makelist( &(ptr -> falselist), get_next_instr() + 1);
		output_code("IF %s != #0 GOTO", ptr -> addr);
		output_code("GOTO");
	}
	
}

void translate_args(struct tree_node * args) {
	assert(args -> unit_name == NODE_ARGS);
	if (args -> flags == FLAG_ARGS_MORE) {
		translate_args(args -> children[2]);
	}
	translate(args -> children[0]);
	output_code("ARG %s", args -> children[0] -> addr);

}


void output_code(const char * fmt, ...) {
	static char buf[INSTR_COUNT];
	assert(instruction_count < INSTR_COUNT);
	va_list arg_ptr;
	va_start(arg_ptr, fmt);
	memset(buf, 0, sizeof(buf));
	instructions[instruction_count] = (char *)malloc( sizeof(char) * INSTR_LEN );
	memset(instructions[instruction_count], 0, sizeof(char) * INSTR_LEN );
	vsprintf(instructions[instruction_count], fmt, arg_ptr);
	va_end(arg_ptr);

	// printf("%d ", instruction_count);

	instruction_count ++;
}

void flush_code() {
	FILE * ir_file = fopen(outputfile, "w+");
	int i = 0;
	for (; i < instruction_count; ++i) {
		if (label_flags[i])
			fprintf(ir_file, "LABEL L__%d :\n", i);
		fputs(instructions[i], ir_file);
		fputc('\n', ir_file);
	}
}

// flow-control utility functions

int get_next_instr() {
	return instruction_count;
}

void makelist(struct list_head * target, int instr) {
	struct instr_list_entry * p = 
		(struct instr_list_entry *)malloc( sizeof(struct instr_list_entry) );
	assert( list_empty(target) );
	p -> instr = instr;
	list_add(&(p -> list_ptr), target);
}

void movelist(struct list_head * target, struct list_head * source) {
	assert( list_empty(target) );
	if ( !list_empty(source) ) {
		target -> next = source -> next;
		target -> prev = source -> prev;
		target -> next -> prev = target;
		target -> prev -> next = target;
		INIT_LIST_HEAD(source);
	}
}

void mergelist(struct list_head * target,
		struct list_head * l1, struct list_head * l2) {
	assert( list_empty(target) );
	if ( !list_empty(l1) ) {
		target -> next = l1 -> next;
		target -> prev = l1 -> prev;
		target -> prev -> next = target;
		target -> next -> prev = target;

		INIT_LIST_HEAD( l1 );
	}

	if ( !list_empty(l2) ) {
		struct list_head * head = target;
		struct list_head * tail = target -> prev;
		tail -> next = l2 -> next;
		head -> prev = l2 -> prev;
		tail -> next -> prev = tail;
		head -> prev -> next = head;
		INIT_LIST_HEAD(l2);
	}
}

void backpatch(struct list_head * head, int instr) {
	struct list_head * p, * n;
	list_for_each_safe(p, n, head) {
		struct instr_list_entry * ie = list_entry(p, struct instr_list_entry, list_ptr);
		char * instr_str = instructions[ie -> instr];
		label_flags[instr] = 1;
		int end = strlen(instr_str);
		list_del(p);
		free(ie);
		sprintf(instr_str + end, " L__%d", instr);
	}
}

void printlist(struct list_head *head) {
	printf("current instr: %d ", get_next_instr() -1);
	struct list_head *p;
	putchar('[');
	list_for_each(p, head) {
		struct instr_list_entry *ie = list_entry(p, struct instr_list_entry, list_ptr);
		printf(" %d ", ie -> instr);
	}
	puts("]");
}

// generators of names of operators, params, args, temps and labels

char * new_param_name() {
	static int c = 1;
	static char buf[32];
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "p__%d", c);
	c++;
	return buf;
}

char * new_arg_name() {
	static int c = 1;
	static char buf[32];
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "a__%d", c);
	c++;
	return buf;
}

char * new_temp_name() {
	static int i = 0;
	char *buf = (char *)malloc( sizeof(char) * 32);
	memset(buf, 0, sizeof(char) *32);
	sprintf(buf, "t__%d", i);
	i++;
	return buf;
}

char * new_label_name() {
	static int c = 1;
	char * buf = (char *)malloc(32 * sizeof(char));
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "__l%d", c);
	c++;
	return buf;
}

char * new_name(const char *fmt, ...) {
	char * buf = (char *)malloc( sizeof(char) * 32 );
	memset(buf, 0, sizeof(char) * 32 );

	va_list arg_ptr;
	va_start(arg_ptr, fmt);
	vsprintf(buf, fmt, arg_ptr);
	va_end(arg_ptr);

	return buf;
}

char * get_op_str(struct tree_node * ptr) {
	switch (ptr -> unit_name) {
		case TOKEN_AND:
			return "&&";
		case TOKEN_OR:
			return "||";
		case TOKEN_PLUS:
			return "+";
		case TOKEN_MINUS:
			return "-";
		case TOKEN_STAR:
			return "*";
		case TOKEN_DIV:
			return "/";
		case TOKEN_RELOP:
			switch (ptr -> relop_type) {
				case RELOP_EQ:
					return "==";
				case RELOP_GE:
					return ">=";
				case RELOP_GT:
					return ">";
				case RELOP_LE:
					return "<=";
				case RELOP_LT:
					return "<";
				case RELOP_NE:
					return "!=";
				default:
					assert(0);
			}
		default:
			assert(0);
	}
}

void translate_subscript(struct tree_node * ptr) {
	assert(ptr -> unit_name == NODE_EXP);
	assert(ptr -> flags == FLAG_EXP_SUBSCRIPT);
	translate(ptr -> children[2]);
	ptr -> addr = new_temp_name();
	char * temp = new_temp_name();
	
	const char * lhs = ptr -> children[2] -> addr;
	int rhs_num = ptr -> children[0] -> exp_type.array_type -> element_size;
	if ( lhs[0] == '#') {	// improvement
		free(temp);
		temp = (char *)malloc( sizeof(char) * 32 );
		memset(temp, 0, sizeof(char) * 32);
		sprintf(temp, "#%d", atoi(lhs+1) * rhs_num);
	} else {
		output_code("%s := %s * #%d", temp, ptr -> children[2] -> addr,
			ptr -> children[0] -> exp_type.array_type -> element_size);
	}

	const char * head_addr = NULL;
	if (ptr -> children[0] -> flags == FLAG_EXP_ID) {
		ptr -> array_name = ptr -> children[0] -> children[0] -> id_extra -> name;
		head_addr = ptr -> children[0] -> children[0] -> id_extra -> name;
	} else {
		translate_subscript(ptr -> children[0]);
		ptr -> array_name = ptr -> children[0] -> array_name;
		head_addr = ptr -> children[0] -> addr;
	}
	output_code("%s := %s + %s", ptr -> addr, head_addr, temp);
	// improvement seems infeasible since head_addr unknown.
}

// miscellaneous

int is_constant(const char *str) {
	if (str[0] != '#')
		return 0;
	int i;
	int l = strlen(str);
	if (l < 2) 
		return 0;
	for (i = 1; i < l; ++i) {
		if ( str[i] < '0' && str[i] > '9' )
			return 0;
	}
	return 1;
}

int operate(int l, struct tree_node *op, int r) {
	switch (op -> unit_name) {
		case TOKEN_PLUS:
			return l + r;
		case TOKEN_MINUS:
			return l - r;
		case TOKEN_STAR:
			return l * r;
		case TOKEN_DIV:
			return l / r;
		default:
			printf("panic: %s\n", unit_str[op -> unit_name]);
			assert(0);
	}
}

char * optimize_binop(struct tree_node *ptr) {
	char * lhs_str = ptr -> children[0] -> addr;
	char * rhs_str = ptr -> children[2] -> addr;
	int operator = ptr -> children[1] -> unit_name;
	switch(operator) {
		case TOKEN_PLUS:
			if ( is_constant(lhs_str) && atoi(lhs_str + 1) == 0) {
				return rhs_str;
			} else if ( is_constant(rhs_str) && atoi(rhs_str + 1) == 0) {
				return lhs_str;
			}
			break;
		case TOKEN_MINUS:
			if ( is_constant(rhs_str) && atoi(rhs_str + 1) == 0) {
				return lhs_str;
			} else if (strlen(lhs_str) == strlen(rhs_str) &&
					strncmp(lhs_str, rhs_str, strlen(rhs_str)) == 0) {
				return "#0";
			}
			break;
		case TOKEN_STAR:
			if ( is_constant(lhs_str) && atoi(lhs_str + 1) == 1) {
				return rhs_str;
			} else if ( is_constant(rhs_str) && atoi(rhs_str + 1) == 1) {
				return lhs_str;
			}
			break;
		case TOKEN_DIV:
			if ( is_constant(rhs_str) && atoi(rhs_str + 1) == 1) {
				return lhs_str;
			} else if (strlen(lhs_str) == strlen(rhs_str) &&
					strncmp(lhs_str, rhs_str, strlen(rhs_str)) == 0) {
				return "#1";
			}
			break;
		default:
			break;
	}
	return NULL;
}
