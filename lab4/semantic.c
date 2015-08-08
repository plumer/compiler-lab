#include "semantic.h"
#include "lex.yy.c"

struct struct_entry * anonymous_struct[32];


void semantic_analysis(struct tree_node * ptr) {
	if (!ptr) return;
	int i;

	if (ptr -> unit_name == NODE_EXTDEF) {
		if (ptr -> flags == FLAG_EXTDEF_VARIABLES) {

			struct tree_node * spec = ptr -> children[0];
			struct tree_node * declist = ptr -> children[1];

            push_anonymous_struct();
			// handle type first: if define struct, add struct; if basic, just use it, skip
			if (spec -> flags == FLAG_SPECIFIER_STRUCT &&
				spec -> children[0] -> flags == FLAG_STRUCTSPECIFIER_DEF) {
				symtab_add_struct(spec);
			}
			// whether or not spec is successfully added, just use them.
			while (declist -> flags == FLAG_EXTDECLIST_MORE) {
				symtab_add_variable(spec, declist -> children[0]);
				declist = declist -> children[2];
			}
			symtab_add_variable(spec, declist -> children[0]);

			pop_anonymous_struct();

			return;
			// end of EXTDEF_VARIABLES
		} else if (ptr -> flags == FLAG_EXTDEF_FUNCTION) {

			if (function_field != NULL) {
				semerrorpos(ERROR_OTHER, ptr->line);
				printf("nested function definition \'%s\' inside another function definition\'%s\'\n",
						ptr -> children[1] -> children[0] -> id_extra -> name,
						function_field -> name);
				return;
			}

			// register in the symbol table
			//   all the names in parameter list
			struct tree_node * spec = ptr -> children[0];
			struct tree_node * id = ptr -> children[1] -> children[0];
			struct func_entry * f = symtab_add_function(ptr -> children[1]);
			if (!f) {
				f = search_func_entry(id -> id_extra -> name);
				semerrorpos(ERROR_FUNCTION_CONFLICT, ptr -> line);
				printf("function \'%s\' already defined at %d\n",
					id -> id_extra -> name, f -> defined_at);
			}
			function_field = f;
			// check return type: if spec is a struct definition, add that.
            if (spec -> flags == FLAG_SPECIFIER_STRUCT) {
        	    struct struct_entry * se = NULL;
        	    push_anonymous_struct();
        		se = symtab_add_struct(spec -> children[0]);
        		f -> return_type_flag = TYPE_STRUCT;
        		f -> return_type.struct_type = se;
        		// return type may be NULL, checked by expression analysis
        		pop_anonymous_struct();
        	} else {
        	    assert(spec -> flags == FLAG_SPECIFIER_BASIC);
        	    f -> return_type_flag = TYPE_BASIC;
        	    f -> return_type.basic_type = spec -> children[0] -> type_name;
        	}
			// recursive analyze the definitions and statements in the compst
			struct tree_node * deflist = ptr -> children[2] -> children[1];
			while (deflist -> flags == FLAG_DEFLIST_MORE) {
				semantic_analysis(deflist -> children[0]);
				deflist = deflist -> children[1];
			}
			struct tree_node * stmtlist = ptr -> children[2] -> children[2];
			while (stmtlist -> flags == FLAG_STMTLIST_MORE) {
				semantic_analysis(stmtlist -> children[0]);
				stmtlist = stmtlist -> children[1];
			}
			function_field = NULL;
			// end of EXTDEF_FUNCTION
		} else if (ptr -> flags == FLAG_EXTDEF_TYPE) {
			if (ptr -> children[0] -> flags == FLAG_SPECIFIER_BASIC) return;
			else if (ptr -> children[0] -> flags == FLAG_SPECIFIER_STRUCT)
				symtab_add_struct(ptr -> children[0] -> children[0]);
			else assert(0);
		} else {
			assert(0);
		}
	} else if (ptr -> unit_name == NODE_EXP) {
		check_expression(ptr);
	} else if (ptr -> unit_name == NODE_DEF) {
		struct tree_node * spec = ptr -> children[0];
		struct tree_node * declist = ptr -> children[1];
		while (declist -> flags == FLAG_DECLIST_MORE) {
			symtab_add_variable(spec, declist -> children[0] -> children[0]);
			if (declist -> children[0] -> flags == FLAG_DEC_INITIALIZED) {
				check_expression(declist -> children[0] -> children[2]);
			}
			declist = declist -> children[2];
		}
		symtab_add_variable(spec, declist -> children[0] -> children[0]);
		if (declist -> children[0] -> flags == FLAG_DEC_INITIALIZED) {
			check_expression(declist -> children[0] -> children[2]);
		}
	} else if (ptr -> unit_name == NODE_STMT) {
		if (ptr -> flags == FLAG_STMT_EXP) {
			check_expression(ptr -> children[0]);
		} else if (ptr -> flags == FLAG_STMT_COMPST) {
			struct tree_node * deflist = ptr -> children[0] -> children[1];
			while (deflist -> flags == FLAG_DEFLIST_MORE) {
				semantic_analysis(deflist -> children[0]);
				deflist = deflist -> children[1];
			}
			struct tree_node * stmtlist = ptr -> children[0] -> children[2];
			while (stmtlist -> flags == FLAG_STMTLIST_MORE) {
				semantic_analysis(stmtlist -> children[0]);
				stmtlist = stmtlist -> children[1];
			}
		} else if (ptr -> flags == FLAG_STMT_IF) {
			check_expression(ptr -> children[2]);
			if (ptr -> children[2] -> exp_type_flag != TYPE_BASIC ||
				ptr -> children[2] -> exp_type.basic_type != TYPE_INT) {
				semerrorpos(ERROR_OTHER, ptr -> line);
				printf("non-integer in condition expression\n");
			}
			semantic_analysis(ptr -> children[4]);
		} else if (ptr -> flags == FLAG_STMT_IFELSE) {
			check_expression(ptr -> children[2]);
			if (ptr -> children[2] -> exp_type_flag != TYPE_BASIC ||
				ptr -> children[2] -> exp_type.basic_type != TYPE_INT) {
				semerrorpos(ERROR_OTHER, ptr -> line);
				printf("non-integer in condition expression\n");
			}
			semantic_analysis(ptr -> children[4]);
			semantic_analysis(ptr -> children[6]);
		} else if (ptr -> flags == FLAG_STMT_WHILE) {
			check_expression(ptr -> children[2]);
			if (ptr -> children[2] -> exp_type_flag != TYPE_BASIC ||
				ptr -> children[2] -> exp_type.basic_type != TYPE_INT) {
				semerrorpos(ERROR_OTHER, ptr -> line);
				printf("non-integer in condition expression\n");
			}
			semantic_analysis(ptr -> children[4]);
		} else if (ptr -> flags == FLAG_STMT_RETURN) {
			// done
			check_expression(ptr -> children[1]);
			if (function_field == NULL) {
				semerrorpos(ERROR_OTHER, ptr -> line);
				printf("no corresponding block to return operation\n");
				return;
			}
			int c = compare_type(ptr -> children[1] -> exp_type_flag, &(ptr -> children[1] -> exp_type),
				function_field -> return_type_flag, &(function_field -> return_type));
			if (c == COMPARE_GOOD) {
				// good
			} else {
				semerrorpos(ERROR_RETURN_MISMATCH, ptr -> line);
				printf("return type error: %s\n", compare_str[c]);
			}
			// end of return
		} else {
			assert(0);
		}
	} else {
		for (i = 0; i < ptr -> num_of_children; ++i) {
			assert(ptr -> children[i]);
			semantic_analysis(ptr -> children[i]);
		}
	}

}

void reorder(struct array * ptr) {
	int dimension = ptr -> dimension;
	int *lengths = (int *)malloc( sizeof(int) * dimension);
	struct array * p = ptr;
	int i = 0;
	for (; i < dimension; ++i) {
		lengths[dimension - i - 1] = p -> num_of_elements;
		p = p -> elem.array_type;
	}
	int *sizes = (int *)malloc( sizeof(int) * dimension);
	sizes[dimension-1] = 4;
	for (i = dimension-2; i >= 0; --i) {
		sizes[i] = sizes[i+1] * lengths[i+1];

	}
	p  = ptr;
	for (i = 0; i < dimension; ++i) {
		p -> num_of_elements = lengths[i];
		p -> element_size = sizes[i];
		p = p -> elem.array_type;
	}
	free(lengths);
	free(sizes);
}


// return a pointer to a struct array.
// array.num_of_elements and TODO element_width will be correctly placed.
// if spec is basic, elem.basic_type is set.
struct array *
construct_basic_array_type(struct tree_node *spec, struct tree_node * vardec) {
	// assert(spec -> flags == FLAG_SPECIFIER_BASIC);
	assert(vardec -> unit_name == NODE_VARDEC);
	assert(vardec -> flags == FLAG_VARDEC_ARRAY);
	assert(spec -> flags == FLAG_SPECIFIER_BASIC);

	struct array * p = (struct array *)malloc(sizeof(struct array));
	memset(p, 0, sizeof(struct array));
	p -> num_of_elements = vardec -> children[2] -> i_value;
	//TODO p -> element_width =
	if (vardec -> children[0] -> flags == FLAG_VARDEC_SINGLE) {
		// the last dimension of the array
		p -> type_flag = TYPE_BASIC;
		p -> elem.basic_type = spec -> children[0] -> type_name;
		p -> dimension = 1;
		p -> element_size = 4;
	} else {
		// more dimensions on the way
		assert(vardec -> children[0] -> flags == FLAG_VARDEC_ARRAY);
		p -> type_flag = TYPE_ARRAY;
		p -> elem.array_type = construct_basic_array_type(spec, vardec -> children[0]);
		if (p -> elem.array_type == NULL) {
			free(p);
			return NULL;
		}
		p -> dimension = p -> elem.array_type -> dimension + 1;
		p -> element_size = p -> elem.array_type -> element_size * p -> num_of_elements;
	}
	return p;
}

// if spec is struct, struct should always exist, and elem.struct_type will be set
struct array * construct_struct_array_type(struct struct_entry *se, struct tree_node *vardec) {
	assert(vardec -> unit_name == NODE_VARDEC);
	assert(vardec -> flags == FLAG_VARDEC_ARRAY);

	struct array *p = (struct array *)malloc(sizeof(struct array));
	memset(p, 0, sizeof(struct array));
	p -> num_of_elements = vardec -> children[2] -> i_value;

	if (vardec -> children[0] -> flags == FLAG_VARDEC_SINGLE) {
		p -> type_flag = TYPE_STRUCT;
		p -> elem.struct_type = se;
		p -> dimension = 1;
		p -> element_size = se -> size;
	} else {
		assert(vardec -> children[0] -> flags == FLAG_VARDEC_ARRAY);
		p -> type_flag = TYPE_ARRAY;
		p -> elem.array_type = construct_struct_array_type(se, vardec -> children[0]);
		if (p -> elem.array_type == NULL) {
			free(p);
			return NULL;
		}
		p -> dimension = p -> elem.array_type -> dimension + 1;
		p -> element_size = p -> elem.array_type -> element_size * p -> num_of_elements;
	}
	return p;
}

// adds an entry in the variable and handles arrays,
// if spec is a struct specifier definition,
//   it will only check if struct is declared
// if spec is a struct specifier anonymous definition,
//   anonymous_struct is used.
struct var_entry *
symtab_add_variable(struct tree_node * spec, struct tree_node * vardec) {
	assert(spec -> unit_name == NODE_SPECIFIER);
	assert(vardec -> unit_name == NODE_VARDEC);

	// if variable with same name exists, just return NULL

	// getting name requires a deep-in digging.
    struct tree_node * identifier = vardec;
    while (identifier -> flags == FLAG_VARDEC_ARRAY) identifier = identifier -> children[0];
    identifier = identifier -> children[0];
    assert(identifier -> unit_name == TOKEN_IDENTIFIER);
	struct var_entry * e = add_var_entry(identifier -> id_extra -> name);
	if (e == NULL) {
		e = search_var_entry(identifier -> id_extra -> name);
		if (e != NULL) {
			semerrorpos(ERROR_VARIABLE_CONFLICT, vardec -> line);
			printf("variable \'%s\' already declared at line %d\n",
					e -> name, e -> defined_at);
		} else {
			struct struct_entry * se = search_struct_entry(identifier -> id_extra -> name);
			semerrorpos(ERROR_OTHER, vardec -> line);
			printf("variable \'%s\' name conflicts with struct declared at line %d\n",
				se -> name, se -> defined_at);
		}
		return NULL;
	}
	// if spec is basic type, just next
	if (spec -> flags == FLAG_SPECIFIER_BASIC) ;
	// if spec is struct, check if that struct exists
	struct struct_entry * strent = NULL;
	if (spec -> flags == FLAG_SPECIFIER_STRUCT) {

		const char *s_name;
		if (spec -> children[0] -> flags == FLAG_STRUCTSPECIFIER_DEC) {
			s_name = spec -> children[0] -> children[1] -> children[0] -> id_extra -> name;
			strent = search_struct_entry(s_name);
			if (strent == NULL) {
				semerrorpos(ERROR_STRUCT_UNDEFINED, vardec -> line);
				printf("struct \'%s\' not declared\n", s_name);
				return NULL;
			} else if (strent -> is_defined == 0) {
				semerrorpos(ERROR_STRUCT_UNDEFINED, vardec -> line);
				printf("struct \'%s\' is not defined\n", s_name);
				return NULL;
			}
		} else if (spec -> children[0] -> flags == FLAG_STRUCTSPECIFIER_DEF) {
		    // a variable declared simultaneously with struct definition
			if (spec -> children[0] -> children[1] -> flags == FLAG_OPTTAG_TAGGED) {
				s_name = spec -> children[0] -> children[1] -> children[0] -> id_extra -> name;
				strent = search_struct_entry(s_name);
				if (strent == NULL) {
					semerrorpos(ERROR_STRUCT_UNDEFINED, vardec -> line);
					printf("struct \'%s\' is not defined\n", s_name);
					return NULL;
				}
			} else if (spec -> children[0]-> children[1] -> flags == FLAG_OPTTAG_EMPTY) {
			    // an anonymous struct
				strent = get_anonymous_struct();
				if (strent == NULL) {
					semerrorpos(ERROR_OTHER, vardec -> line);
					printf("anonymous struct not set\n");
					return NULL;
				}
			} else assert(0);
		} else assert(0);
	} // end if (spec -> flags == FLAG_SPECIFIER_STRUCT)
	// now e points to a var_entry, filled with name
	// spec points to a valid spec
	e -> defined_at = identifier -> line;
	if (vardec -> flags == FLAG_VARDEC_ARRAY) {
		e -> type_flag = TYPE_ARRAY;
		if (spec -> flags == FLAG_SPECIFIER_BASIC) {
			e -> t.array_type = construct_basic_array_type(spec, vardec);
			reorder(e -> t.array_type);
		}
		else if (spec -> flags == FLAG_SPECIFIER_STRUCT)
			e -> t.array_type = construct_struct_array_type(strent, vardec);
		e -> size = e -> t.array_type -> element_size * e -> t.array_type -> num_of_elements;

	} else if (spec -> flags == FLAG_SPECIFIER_BASIC) {
		e -> type_flag = TYPE_BASIC;
		e -> t.basic_type = spec -> children[0] -> type_name;
		e -> size = 4;
	} else {
		assert(spec -> flags == FLAG_SPECIFIER_STRUCT);
		e -> type_flag = TYPE_STRUCT;
		e -> t.struct_type = strent;
		e -> size = strent -> size;
	}
	return e;
} // end of function symtab_add_variable

// assumption: the function is always a definition
// adds an entry in the function table, and add the parameters
// if the return type is a struct definition, adds the definition
struct func_entry *
symtab_add_function(struct tree_node *fundec) {

	struct tree_node * identifier = fundec -> children[0];
	struct func_entry * f = add_func_entry(identifier -> id_extra -> name);
	if (f == NULL) {
	    //semerrorpos(identifier -> line);
		//printf("function name already defined\n");
		return NULL;
	}

	if (fundec -> flags == FLAG_FUNDEC_NOARGS)
	    return f;
	// done : add parameters
	assert(fundec -> flags == FLAG_FUNDEC_WITHARGS);
	struct tree_node * varlist = fundec -> children[2];
	while (1) {

	    push_anonymous_struct();
	    struct tree_node * paramdec = varlist -> children[0];
	    if (paramdec -> children[0] -> flags == FLAG_SPECIFIER_STRUCT) {
	        struct struct_entry *se = symtab_add_struct(paramdec -> children[0] -> children[0]);
	        if (se == NULL) {
	            // printf("current param not added\n");
	        }
	    }
	    struct var_entry * ve = symtab_add_variable(paramdec -> children[0], paramdec -> children[1]);
	    if (ve) {
	        add_func_param(f, ve);
	    } else {
	        // printf("current param not added\n");
	    }
	    pop_anonymous_struct();
	    if (varlist -> flags != FLAG_VARLIST_MORE) break;
	    varlist = varlist -> children[2];
	}
	return f;
}

int check_expression(struct tree_node * exp) {
	assert(exp -> unit_name == NODE_EXP);

	if (exp -> flags == FLAG_EXP_ID) {					// terminal
		const char * id_name = exp -> children[0] -> id_extra -> name;
		struct var_entry * ve = search_var_entry(id_name);
		if (ve == NULL) {
			semerrorpos(ERROR_VARIABLE_UNDEFINED, exp -> children[0] -> line);
			printf("identifier \'%s\' not exist\n", id_name);
		} else {
			// check the ve's type and its flags
			exp -> exp_type_flag = ve -> type_flag;
			exp -> exp_type = ve -> t;
			exp -> exp_is_lvalue = 1;
		}
	} else if (exp -> flags == FLAG_EXP_INT) {			// terminal
		assert(exp -> children[0] -> unit_name == TOKEN_INTEGER);
		exp -> exp_type_flag = TYPE_BASIC;
		exp -> exp_type.basic_type = TYPE_INT;
		exp -> exp_is_lvalue = 0;
	} else if (exp -> flags == FLAG_EXP_FLOAT) {		// terminal
	    assert(exp -> children[0] -> unit_name == TOKEN_FLOAT);
	    exp -> exp_type_flag = TYPE_BASIC;
		exp -> exp_type.basic_type = TYPE_FLOAT;
		exp -> exp_is_lvalue = 0;
	} else if (exp -> flags == FLAG_EXP_ASSIGN) {		// NON-terminal
		// TODO: check left-value, then check type confliction
		check_expression(exp -> children[0]);
		check_expression(exp -> children[2]);
		if (exp -> children[0] -> exp_is_lvalue == 0) {
			semerrorpos(ERROR_ASSIGN_NONLEFT, exp -> children[0] -> line);
			printf("expression on the left side is not left-value\n");
		}
		exp -> exp_is_lvalue = exp -> children[2] -> exp_is_lvalue;
		// whether or not the left side is left-value or not, better to check type first.
		int c = compare_type(
			exp -> children[0] -> exp_type_flag, &(exp -> children[0] -> exp_type),
			exp -> children[2] -> exp_type_flag, &(exp -> children[2] -> exp_type));
		if (c != COMPARE_GOOD) {
			semerrorpos(ERROR_ASSIGN_MISMATCH, exp -> children[1] -> line);
			printf("type mismatch in assignment: %s\n", compare_str[c]);
		} else {
			exp -> exp_type_flag = exp -> children[0] -> exp_type_flag;
			exp -> exp_type = exp -> children[0] -> exp_type;
		}

		// end of assignment
	} else if (exp -> flags == FLAG_EXP_BINOP) {		// NON-terminal
		check_expression(exp -> children[0]);
		check_expression(exp -> children[2]);

		// prune: non-basic types cannot take binary operations whatsoever
		if (exp -> children[0] -> exp_type_flag == TYPE_ARRAY  ||
			exp -> children[0] -> exp_type_flag == TYPE_STRUCT) {
			semerrorpos(ERROR_OTHER, exp -> children[1] -> line);
			printf("%s (on the left side) cannot take binary operation\n",
				type_str[exp -> children[0] -> exp_type_flag]);
			exp -> exp_type_flag = exp -> children[0] -> exp_type_flag;
			return -1;
		}
		if (exp -> children[2] -> exp_type_flag == TYPE_ARRAY  ||
			exp -> children[2] -> exp_type_flag == TYPE_STRUCT) {
			semerrorpos(ERROR_OTHER, exp -> children[1] -> line);
			printf("%s (on the right side) cannot take binary operation\n",
				type_str[exp -> children[2] -> exp_type_flag]);
			exp -> exp_type_flag = exp -> children[2] -> exp_type_flag;
			return -1;
		}

		// now they are both basic types.
		exp -> exp_type_flag = TYPE_BASIC;
		exp -> exp_type.basic_type = exp -> children[0] -> exp_type.basic_type;
		if (exp -> children[1] -> unit_name == TOKEN_RELOP)
			exp -> exp_type.basic_type = TYPE_INT;
		exp -> exp_is_lvalue = 0;
		int type_left = exp -> children[0] -> exp_type.basic_type;
		int type_right = exp -> children[2] -> exp_type.basic_type;
		// assert(type_left == TYPE_INT || type_left == TYPE_FLOAT);
		// assert(type_right == TYPE_INT || type_right == TYPE_FLOAT);
		if (type_left == type_right) {
			// printf("types: %d, %d\n", type_left, type_right);
		} else {
			semerrorpos(ERROR_OPERAND_MISMATCH, exp->children[1] -> line);
			printf("expression type not match: %d and %d\n", type_left, type_right);
		}
		// end of binary operators: + - * / && || relop
	} else if (exp -> flags == FLAG_EXP_PAREN) {		// NON-terminal
		check_expression(exp -> children[1]);
		exp -> exp_type_flag = exp -> children[1] -> exp_type_flag;
		exp -> exp_type = exp -> children[1] -> exp_type;
		exp -> exp_is_lvalue = exp -> children[1] -> exp_is_lvalue;
		// end of parentheses
	} else if (exp -> flags == FLAG_EXP_UNOP) {			// NON-terminal
		check_expression(exp -> children[1]);

		if (exp -> exp_type_flag == TYPE_ARRAY || exp -> exp_type_flag == TYPE_STRUCT) {
			semerrorpos(ERROR_OTHER, exp -> line);
			printf("%s cannot take unary operator\n", type_str[exp -> exp_type_flag]);
		}

		exp -> exp_is_lvalue = 0;
		exp -> exp_type_flag = exp -> children[1] -> exp_type_flag;
		exp -> exp_type = exp -> children[1] -> exp_type;
		// end of unary operator
	} else if (exp -> flags == FLAG_EXP_CALL_ARGS) {	// NON-self-recursive
		// DONE: check arguments
		const char * f_name = exp -> children[0] -> id_extra -> name;
		struct func_entry * f = search_func_entry(f_name);
		struct tree_node * arguments = exp -> children[2];
		struct list_head * parameters_head = NULL;
		if (f == NULL) {
			semerrorpos(ERROR_FUNCTION_UNDEFINED, exp -> children[0] -> line);
			printf("function \'%s\' not defined\n", f_name);
			exp -> exp_type_flag = TYPE_ERROR;
			return -1;
		} else { // checking parameters_head
			parameters_head = &(f -> param_list);
			struct list_head * param_l = NULL;
			struct var_entry * param_ve = NULL;
/*for loop*/list_for_each(param_l, parameters_head) {
				struct tree_node * a = arguments -> children[0];
				param_ve = list_entry(param_l, struct param_entry, list_ptr) -> param_ptr;
				check_expression(a);
				int c = compare_type(param_ve -> type_flag, &(param_ve -> t),
					a -> exp_type_flag, &(a -> exp_type));
				if (c != COMPARE_GOOD) {
					semerrorpos(ERROR_PARAM_MISMATCH, a -> line);
					printf("argument and parameter type mismatch: %s\n",
						compare_str[c]);
				}
				if (arguments -> flags == FLAG_ARGS_SINGLE) {
					// param_ve should be the last parameter
					if (param_l -> next != parameters_head) {
						semerrorpos(ERROR_PARAM_MISMATCH, a -> line);
						printf("more params not matched\n");
					}
					break;
				} else {
					assert(arguments -> flags == FLAG_ARGS_MORE);
					// but if parameters has reached its end...
					if (param_l -> next == parameters_head) {
						semerrorpos(ERROR_PARAM_MISMATCH, a -> line);
						printf("more arguments not matched\n");
						break;
					}
				}
				arguments = arguments -> children[2];
			}
		}
		while (arguments -> flags == FLAG_ARGS_MORE) {
			check_expression(arguments -> children[0]);
			arguments = arguments -> children[2];
		}

		exp -> exp_type_flag = f -> return_type_flag;
		exp -> exp_type = f -> return_type;
		// end of function calling with args
	} else if (exp -> flags == FLAG_EXP_CALL_NOARGS) {	// NON-self-recursive
		// done : get return type from function
		const char * f_name = exp -> children[0] -> id_extra -> name;
		struct func_entry * f = search_func_entry(f_name);
		if (f == NULL) {
			semerrorpos(ERROR_FUNCTION_UNDEFINED, exp -> children[0] -> line);
			printf("function \'%s\' not defined\n", f_name);
		} else {
			exp -> exp_type_flag = f -> return_type_flag;
			exp -> exp_type = f -> return_type;
		}
	} else if (exp -> flags == FLAG_EXP_SUBSCRIPT) {	// NON-terminal
		// done : subscription
		check_expression(exp -> children[0]);
		check_expression(exp -> children[2]);
		if ((exp -> children[2] -> exp_type_flag != TYPE_BASIC) ||
			(exp -> children[2] -> exp_type.basic_type != TYPE_INT) ) {
			semerrorpos(ERROR_SUBSCRIPT_NONINT, exp -> children[2] -> line);
			printf("subscriptor non-integer");
		}
		if (exp -> children[0] -> exp_type_flag != TYPE_ARRAY) {
			semerrorpos(ERROR_ARRAY_MISUSE, exp -> line);
			printf("array dimension exhausted\n");
			exp -> exp_type_flag = exp -> children[0] -> exp_type_flag;
			exp -> exp_type = exp -> children[0] -> exp_type;
		} else {
			struct array * a = exp -> children[0] -> exp_type.array_type;
			exp -> exp_type_flag = a -> type_flag;
			exp -> exp_type = a -> elem;
		}

		exp -> exp_is_lvalue = 1;
		// end of exp_subscription
	} else if (exp -> flags == FLAG_EXP_MEMBER) {		// NON-self-recursive
		// done : get type from struct
		check_expression(exp -> children[0]);
		if (exp -> children[0] -> exp_type_flag != TYPE_STRUCT) {
			semerrorpos(ERROR_STRUCT_MISUSE, exp -> children[1] -> line);
			printf("cannot take members from a non-struct variable\n");
		} else {
			struct struct_entry *se = exp -> children[0] -> exp_type.struct_type;
			const char * m_name = exp -> children[2] -> id_extra -> name;
			struct var_entry *me = search_struct_member(se, m_name);
			if (me == NULL) {
				semerrorpos(ERROR_STRUCT_MEMBER_NOTFOUND, exp -> children[1] -> line);
				printf("no member \'%s\' in struct type \'%s\'\n", m_name, se -> name);
			} else {
				exp -> exp_type_flag = me -> type_flag;
				exp -> exp_type = me -> t;
				exp -> exp_is_lvalue = 1;
			}
		}
		// end of exp_member
	}
	return 0;
}

// adds an entry in the struct table, and
// if strspec contains a definition of the struct, add all the members
// if strspec is an anonymous definition, anonymous_struct will be set.
struct struct_entry *
symtab_add_struct(struct tree_node * strspec) {
	static int anonymouscount = 0;
	assert(strspec -> unit_name == NODE_STRUCTSPECIFIER);

	struct struct_entry * se;
	if (strspec -> flags == FLAG_STRUCTSPECIFIER_DEC) {
		const char * s_name = strspec -> children[1] -> children[0] -> id_extra -> name;
		se = add_struct_entry(s_name);
		if (se == NULL)
			se = search_struct_entry(s_name);
		if (se == NULL)
			return NULL;
		if (se -> declared_at == 0)
			se -> declared_at = strspec -> line;
		return se;
	}
	assert(strspec -> flags == FLAG_STRUCTSPECIFIER_DEF);

	// TODO
	struct tree_node * opttag = strspec -> children[1];
	if (opttag -> flags == FLAG_OPTTAG_TAGGED) {
		se = add_struct_entry(opttag -> children[0] -> id_extra -> name);
		if (se == NULL) {
			se = search_struct_entry(opttag -> children[0] -> id_extra -> name);
			semerrorpos(ERROR_STRUCT_NAME_CONFLICT, opttag -> line);
			if (se != NULL) {
				printf("struct \'%s\' already defined at line %d\n",
						opttag -> children[0] -> id_extra -> name, se -> defined_at);
			} else {
				struct var_entry * ve = search_var_entry(opttag -> children[0] -> id_extra -> name);
				assert(ve);
				printf("struct \'%s\' conflicts with variable defined at %d\n",
					ve -> name, ve -> defined_at);
			}
			return NULL;
		}
	} else {
		assert(opttag -> flags == FLAG_OPTTAG_EMPTY);
		char anonyname[32] = {0};
		sprintf(anonyname, "__anony%d", anonymouscount);
		se = add_struct_entry(anonyname);
		assert(se);
		// printf("%d, line = %d", __LINE__, opttag -> line);
		set_anonymous_struct(se);
		se -> declared_at = strspec -> line;
	}
	// now the name has been added and pointed to by *se*.
	//   and we should add its members
	struct tree_node *deflist = strspec -> children[3];
	struct tree_node *def;
	se -> size = 0;
	while (deflist -> flags == FLAG_DEFLIST_MORE) {
		def = deflist -> children[0];
		push_anonymous_struct();

		struct tree_node *spec = def -> children[0];
		if (spec -> flags == FLAG_SPECIFIER_STRUCT) {
			if (spec -> children[0] -> flags == FLAG_STRUCTSPECIFIER_DEC) {
				const char * s_name = spec -> children[0] -> children[1] -> children[0] -> id_extra -> name;
				struct struct_entry * def_se = search_struct_entry(s_name);
				if (def_se == NULL) {
					semerrorpos( ERROR_STRUCT_UNDEFINED, def -> line );
					printf("struct \'%s\' not defined\n", s_name);
					continue;
				}
			} else {
				// def. done: maybe a nested anonymous struct def
				symtab_add_struct(def -> children[0] -> children[0]);
			}
		}

		// add members to the variable table
		struct tree_node *declist = def -> children[1];
		struct var_entry *ve = NULL;
		while (1) {
			ve = symtab_add_variable(spec, declist -> children[0] -> children[0]);
			if (ve) {
				// printf("ve -> name, \'%s\'\n", ve -> name);
				add_struct_member(se, ve);
				se -> size += ve -> size;
			} else {
				semerrorpos(ERROR_OTHER, declist -> line);
				printf("member name conflict\n");
			}
			if (declist -> children[0] -> flags == FLAG_DEC_INITIALIZED) {
				semerrorpos(ERROR_STRUCT_MEMBER_CONFLICT, declist -> children[0] -> line);
				printf("no initialization to members in a struct definition\n");
			}
			if (declist -> flags == FLAG_DECLIST_SINGLE) break;
			declist = declist -> children[2];
		}

		deflist = deflist -> children[1];
		pop_anonymous_struct();
	}
	se -> is_defined = 1;
	se -> defined_at = strspec -> line;
	return se;
}


int anonymous_struct_depth = 0;

int push_anonymous_struct() {
	// printf("pushing, anonymous_struct_depth = %d\n", anonymous_struct_depth);
	anonymous_struct_depth ++;
	anonymous_struct[anonymous_struct_depth] = NULL;
	assert(anonymous_struct_depth <= 32);
	return 0;
}

void set_anonymous_struct(struct struct_entry *se) {
	// printf("setting, anonymous_struct_depth = %d, name = %s\n",
	//	anonymous_struct_depth, se -> name);
	anonymous_struct[anonymous_struct_depth - 1] = se;
}

struct struct_entry *
get_anonymous_struct() {
	// printf("getting, anonymous_struct_depth = %d\n", anonymous_struct_depth);
	return anonymous_struct[anonymous_struct_depth - 1];
}

int pop_anonymous_struct() {
	// printf("popping, anonymous_struct_depth = %d\n", anonymous_struct_depth);
	anonymous_struct_depth --;
	anonymous_struct[anonymous_struct_depth] = NULL;
	assert(anonymous_struct_depth >= 0);
	return 0;
}

int compare_type(unsigned short ltf, union var_type * lt,
	unsigned short rtf, union var_type * rt) {
	if (ltf != rtf)
		return COMPARE_FLAG_MISMATCH;
	if (ltf == TYPE_BASIC) {
		if (lt -> basic_type != rt -> basic_type)
			return COMPARE_BASIC_TYPE_MISMATCH;
		else
			return COMPARE_GOOD;
	} else if (ltf == TYPE_ARRAY) {
		return compare_array(lt -> array_type, rt -> array_type);
	} else if (ltf == TYPE_STRUCT) {
		return compare_struct(lt -> struct_type, rt -> struct_type);
	}
	assert(0);
	//return COMPARE_UNKNOWN;
}

int compare_array(struct array * la, struct array *ra) {
	if (la -> dimension != ra -> dimension)
		return COMPARE_ARRAY_DIMENSION_MISMATCH;
	while (la -> type_flag == TYPE_ARRAY)
		la = la -> elem.array_type;
	while (ra -> type_flag == TYPE_ARRAY)
		ra = ra -> elem.array_type;
	return compare_type(la -> type_flag, &(la -> elem),
		ra -> type_flag, &(ra -> elem));
}

int compare_struct(struct struct_entry * lse, struct struct_entry * rse) {
	if (strlen(lse -> name) == strlen(rse -> name) &&
		(strncmp(lse -> name, rse -> name, strlen(lse -> name)) == 0)) {
		return COMPARE_GOOD;
	}
	struct var_entry * l_member, *r_member;
	struct list_head * l_list, *r_list;
	for (
		l_list = lse -> member_list.next, r_list = rse -> member_list.next;
		l_list != &(lse -> member_list) && r_list != &(rse -> member_list);
		l_list = l_list -> next, r_list = r_list -> next) {
		l_member = list_entry(l_list, struct member_entry, list_ptr) -> member_ptr;
		r_member = list_entry(r_list, struct member_entry, list_ptr) -> member_ptr;
		int c = compare_type(
			l_member -> type_flag, &(l_member -> t),
			r_member -> type_flag, &(r_member -> t));
		if (c != COMPARE_GOOD)
			return COMPARE_STRUCT_MISMATCH;
	}
	if (r_list != &(rse -> member_list)) return COMPARE_STRUCT_MISMATCH;
	if (l_list != &(lse -> member_list)) return COMPARE_STRUCT_MISMATCH;
	return COMPARE_GOOD;
}
