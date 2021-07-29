#include"Generator/SyntaxGenerator/syntax_generate.h"

//id-> [_a-zA-Z][_0-9a-zA-Z]*
//immediate_value-> -?([1-9][0-9]*|0)(.[0-9]*)?
//
//@@@	// 该行分隔带优先级的运算符与终结符号正则表达式	运算符@优先级（序号低的优先级高）@左结合(L)/右结合(R)@单目(S)/双目(D)/三目(M)
//
//(	@	1	@	L
//[	@	1	@	L
//.	@	1	@	L	@	S
//->	@	1	@	L	@	S
//
//-	@	2	@	R	@	S
//++	@	2	@	R	@	S
//--	@	2	@	R	@	S
//*	@	2	@	R	@	S
//&	@	2	@	R	@	S
//!	@	2	@	R	@	S
//~	@	2	@	R	@	S
//sizeof	@	2	@	R	@	S
//
//*	@	3	@	L	@	D
///	@	3	@	L	@	D
//%	@	3	@	L	@	D
//
//+	@	4	@	L	@	D
//-	@	4	@	L	@	D
//
//<<	@	5	@	L	@	D
//>>	@	5	@	L	@	D
//
//>	@	6	@	L	@	D
//>=	@	6	@	L	@	D
//<	@	6	@	L	@	D
//<=	@	6	@	L	@	D
//
//==	@	7	@	L	@	D
//!=	@	7	@	L	@	D
//
//&	@	8	@	L	@	D
//
//^	@	9	@	L	@	D
//
//|	@	10	@	L	@	D
//
//&&	@	11	@	L	@	D
//
//||	@	12	@	L	@	D
//
//?	@	13	@	R	@	M
//
//=	@	14	@	R	@	D
//*=	@	14	@	R	@	D
///=	@	14	@	R	@	D
//%=	@	14	@	R	@	D
//+=	@	14	@	R	@	D
//-=	@	14	@	R	@	D
//<<=	@	14	@	R	@	D
//>>=	@	14	@	R	@	D
//&=	@	14	@	R	@	D
//^=	@	14	@	R	@	D
//|=	@	14	@	R	@	D
//
//,	@	14	@	L
//
//@@@	// 该行分隔带优先级运算符与产生式，声明过的运算符使用时无需加引号
//
//
//fundamental_data_type-> "bool" | "char" | "short" | "int" | "long" | "float" | "double"
//fundamental_data_type_prefix-> "signed" | "unsigned"
//basic_struct_type-> "struct"
//basic_enum_type-> "enum"
//basic_union_type-> "union"
//
//type_convert-> type '(' return_value_statement ')'
//
//typedef_announce-> typedef basic_var_announce
//
//basic_type-> fundamental_data_type_prefix fundamental_data_type | fundamental_data_type
//const_basic_type-> "const" basic_type | basic_type "const"
//data_type-> basic_type | const_basic_type | "void"
//pointer_tag-> *pointer_tag | @
//type-> data_type pointer_tag
//
//left_value->  id
//			| basic_right_value -> id
//			| basic_right_value . id
//			| basic_right_value [ basic_right_value ]
//			| * basic_right_value
//			| -- basic_right_value
//			| ++ basic_right_value
//
//basic_var_announce-> type id
//basic_var_announce_with_assign-> type assign
//var_id_announce-> id | assign
//advance_var_announce-> advance_var_announce , var_id_announce | var_id_announce
//var_announce-> type advance_var_announce
//
//@@basic_type_announce-> type
//
//@@basic_structg_type-> "struct" '{' announce_statement '}'
//
//@@basic_union_type-> "union" '{' announce_statement '}'
//
//single_function_announce_parameter-> basic_var_announce | type
//nonempty_function_announce_parameter-> single_function_announce_parameter , nonempty_function_announce_parameter | single_function_announce_parameter
//nonempty_function_formal_parameter-> nonempty_function_formal_parameter , basic_right_value | basic_right_value
//nonempty_function_actual_parameter-> nonempty_function_actual_parameter , basic_var_announce | basic_var_announce
//function_announce_parameter-> nonempty_function_formal_parameter | @
//function_formal_parameter-> nonempty_function_formal_parameter | @
//function_actual_parameter-> nonempty_function_formal_parameter | @
//function_call-> id ( function_formal_parameter )
//function_define-> type id ( function_actual_parameter ) '{' statements '}'
//function_announce_statement-> type id ( function_announce_parameter ) ';'
//
//single_statement-> announce_statement | assign_statement | if_else_statement | for_statement | while_statement | do_while_statement | ';' | @
//statements-> statements single_statement | single_statement
//
//basic_right_value->	  left_value
//					| type_covert
//					| sizeof ( basic_right_value )
//					| function_call
//					| basic_right_value + returnvalue_statement
//					| basic_right_value - returnvalue_statement
//					| basic_right_value * basic_right_value
//					| basic_right_value / basic_right_value
//					| basic_right_value & basic_right_value
//					| basic_right_value && basic_right_value
//					| basic_right_value | basic_right_value
//					| basic_right_value || basic_right_value
//					| basic_right_value ^ basic_right_value
//					| basic_right_value == basic_right_value
//					| basic_right_value != basic_right_value
//					| basic_right_value > basic_right_value
//					| basic_right_value >= basic_right_value
//					| basic_right_value < basic_right_value
//					| basic_right_value <= basic_right_value
//					| ! basic_right_value
//					| * basic_right_value
//					| & left_value
//					| - basic_right_value
//					| + basic_right_value
//					| basic_right_value ++
//					| basic_right_value --
//					| basic_right_value -> id
//					| basic_right_value . id
//					| basic_right_value [ basic_right_value ]
//
//assign->  left_value = assign
//		| left_value *= assign
//		| left_value /= assign
//		| left_value %= assign
//		| left_value += assign
//		| left_value -= assign
//		| left_value <<= assign
//		| left_value >>= assign
//		| left_value &= assign
//		| left_value |= assign
//		| left_value ^= assign
//		| basic_right_value
//assign_statement-> assign ';'
//
//announce-> var_announce | function_announce | typedef_announce
//announce_statement-> announce ';'
//
//if_statement-> "if" ( basic_right_value ) single_statement | "if" ( basic_right_value ) '{' statements '}'
//else_statement -> "else" single_statement | "else" '{' statements '}'
//if_else_statement-> if_statement | if_statement else_statement
//
//for_first_parament_statement-> var_announce_statement | assign_statement | ';'
//for_second_parament_statement-> basic_right_value ';' | ';'
//for_third_parament_statement-> single_statement
//for_statement_title-> "for" ( for_first_parament_statement for_second_parament_statement for_third_parament_statement )
//for_statement-> for_statement_title single_statement | for_statement_title '{' statements '}'
//
//while_statement_title-> "while" ( basic_right_value )
//while_statement-> while_statement_title single_statement | while_statement_title '{' statements '}'
//
//do_while_statement-> "do" single_statement while_statement_title ';' | "do" '{' statements '}' while_statement_title ';'