/// 该文件仅且必须最终被Generator/SyntaxGenerator下的
/// process_functions_classes.h
/// process_functions_classes_register.h
/// config_construct.cpp所包含
#include "Generator/SyntaxGenerator/syntax_generate.h"

/// 用户可修改部分
/// 用户定义头文件请在user_defined_functions.h中添加
/// 该文件不可以添加用户定义头文件

/// 终结产生式
GENERATOR_DEFINE_TERMINAL_PRODUCTION("Id", R"([a-zA-Z_][a-zA-Z0-9_]*)")
GENERATOR_DEFINE_TERMINAL_PRODUCTION("Num", R"([+-]?[0-9]+(\.[0-9]*)?)")
GENERATOR_DEFINE_TERMINAL_PRODUCTION("Str", R"(".*")")
GENERATOR_DEFINE_TERMINAL_PRODUCTION("Character", R"('..?')")
GENERATOR_DEFINE_TERMINAL_PRODUCTION("]", R"(\])")
GENERATOR_DEFINE_TERMINAL_PRODUCTION(")", R"(\))")
GENERATOR_DEFINE_TERMINAL_PRODUCTION("{", R"({)")
GENERATOR_DEFINE_TERMINAL_PRODUCTION("}", R"(})")
GENERATOR_DEFINE_TERMINAL_PRODUCTION(";", R"(;)")
GENERATOR_DEFINE_TERMINAL_PRODUCTION(":", R"(:)")

/// 关键字
/// 定义时请当做单词定义，不允许使用正则
GENERATOR_DEFINE_KEY_WORD("char")
GENERATOR_DEFINE_KEY_WORD("short")
GENERATOR_DEFINE_KEY_WORD("int")
GENERATOR_DEFINE_KEY_WORD("long")
GENERATOR_DEFINE_KEY_WORD("float")
GENERATOR_DEFINE_KEY_WORD("double")
GENERATOR_DEFINE_KEY_WORD("void")
GENERATOR_DEFINE_KEY_WORD("signed")
GENERATOR_DEFINE_KEY_WORD("unsigned")
GENERATOR_DEFINE_KEY_WORD("const")
GENERATOR_DEFINE_KEY_WORD("enum")
GENERATOR_DEFINE_KEY_WORD("struct")
GENERATOR_DEFINE_KEY_WORD("union")
GENERATOR_DEFINE_KEY_WORD("typedef")
GENERATOR_DEFINE_KEY_WORD("return")
GENERATOR_DEFINE_KEY_WORD("break")
GENERATOR_DEFINE_KEY_WORD("continue")
GENERATOR_DEFINE_KEY_WORD("if")
GENERATOR_DEFINE_KEY_WORD("else")
GENERATOR_DEFINE_KEY_WORD("for")
GENERATOR_DEFINE_KEY_WORD("do")
GENERATOR_DEFINE_KEY_WORD("while")
GENERATOR_DEFINE_KEY_WORD("switch")
GENERATOR_DEFINE_KEY_WORD("case")
GENERATOR_DEFINE_KEY_WORD("default")

/// 运算符
GENERATOR_DEFINE_BINARY_OPERATOR(",", OperatorAssociatityType::kLeftToRight, 1)
GENERATOR_DEFINE_BINARY_OPERATOR("=", OperatorAssociatityType::kRightToLeft, 2)
GENERATOR_DEFINE_BINARY_OPERATOR("+=", OperatorAssociatityType::kRightToLeft, 2)
GENERATOR_DEFINE_BINARY_OPERATOR("-=", OperatorAssociatityType::kRightToLeft, 2)
GENERATOR_DEFINE_BINARY_OPERATOR("*=", OperatorAssociatityType::kRightToLeft, 2)
GENERATOR_DEFINE_BINARY_OPERATOR("/=", OperatorAssociatityType::kRightToLeft, 2)
GENERATOR_DEFINE_BINARY_OPERATOR("%=", OperatorAssociatityType::kRightToLeft, 2)
GENERATOR_DEFINE_BINARY_OPERATOR("<<=", OperatorAssociatityType::kRightToLeft,
                                 2)
GENERATOR_DEFINE_BINARY_OPERATOR(">>=", OperatorAssociatityType::kRightToLeft,
                                 2)
GENERATOR_DEFINE_BINARY_OPERATOR("&=", OperatorAssociatityType::kRightToLeft, 2)
GENERATOR_DEFINE_BINARY_OPERATOR("|=", OperatorAssociatityType::kRightToLeft, 2)
GENERATOR_DEFINE_BINARY_OPERATOR("^=", OperatorAssociatityType::kRightToLeft, 2)
GENERATOR_DEFINE_BINARY_OPERATOR("?", OperatorAssociatityType::kRightToLeft, 3)
GENERATOR_DEFINE_BINARY_OPERATOR("||", OperatorAssociatityType::kLeftToRight, 4)
GENERATOR_DEFINE_BINARY_OPERATOR("&&", OperatorAssociatityType::kLeftToRight, 5)
GENERATOR_DEFINE_BINARY_OPERATOR("|", OperatorAssociatityType::kLeftToRight, 6)
GENERATOR_DEFINE_BINARY_OPERATOR("^", OperatorAssociatityType::kLeftToRight, 7)
GENERATOR_DEFINE_BINARY_UNARY_OPERATOR("&",
                                       OperatorAssociatityType::kLeftToRight, 8,
                                       OperatorAssociatityType::kRightToLeft,
                                       14)
GENERATOR_DEFINE_BINARY_OPERATOR("!=", OperatorAssociatityType::kLeftToRight, 9)
GENERATOR_DEFINE_BINARY_OPERATOR("==", OperatorAssociatityType::kLeftToRight, 9)
GENERATOR_DEFINE_BINARY_OPERATOR(">", OperatorAssociatityType::kLeftToRight, 10)
GENERATOR_DEFINE_BINARY_OPERATOR(">=", OperatorAssociatityType::kLeftToRight,
                                 10)
GENERATOR_DEFINE_BINARY_OPERATOR("<", OperatorAssociatityType::kLeftToRight, 10)
GENERATOR_DEFINE_BINARY_OPERATOR("<=", OperatorAssociatityType::kLeftToRight,
                                 10)
GENERATOR_DEFINE_BINARY_OPERATOR("<<", OperatorAssociatityType::kLeftToRight,
                                 11)
GENERATOR_DEFINE_BINARY_OPERATOR(">>", OperatorAssociatityType::kLeftToRight,
                                 11)
GENERATOR_DEFINE_BINARY_OPERATOR("+", OperatorAssociatityType::kLeftToRight, 12)
GENERATOR_DEFINE_BINARY_UNARY_OPERATOR("-",
                                       OperatorAssociatityType::kLeftToRight,
                                       12,
                                       OperatorAssociatityType::kRightToLeft,
                                       14)
GENERATOR_DEFINE_BINARY_UNARY_OPERATOR("*",
                                       OperatorAssociatityType::kLeftToRight,
                                       13,
                                       OperatorAssociatityType::kRightToLeft,
                                       14)
GENERATOR_DEFINE_BINARY_OPERATOR("/", OperatorAssociatityType::kLeftToRight, 13)
GENERATOR_DEFINE_BINARY_OPERATOR("%", OperatorAssociatityType::kLeftToRight, 13)
GENERATOR_DEFINE_BINARY_UNARY_OPERATOR("++",
                                       OperatorAssociatityType::kRightToLeft,
                                       14,
                                       OperatorAssociatityType::kRightToLeft,
                                       14)
GENERATOR_DEFINE_BINARY_UNARY_OPERATOR("--",
                                       OperatorAssociatityType::kRightToLeft,
                                       14,
                                       OperatorAssociatityType::kRightToLeft,
                                       14)
GENERATOR_DEFINE_UNARY_OPERATOR("!", OperatorAssociatityType::kRightToLeft, 14)
GENERATOR_DEFINE_UNARY_OPERATOR("~", OperatorAssociatityType::kRightToLeft, 14)
GENERATOR_DEFINE_UNARY_OPERATOR("sizeof", OperatorAssociatityType::kRightToLeft,
                                14)
GENERATOR_DEFINE_BINARY_OPERATOR(".", OperatorAssociatityType::kRightToLeft, 15)
GENERATOR_DEFINE_BINARY_OPERATOR("->", OperatorAssociatityType::kRightToLeft,
                                 15)
GENERATOR_DEFINE_BINARY_OPERATOR("[", OperatorAssociatityType::kRightToLeft, 15)
GENERATOR_DEFINE_BINARY_OPERATOR("(", OperatorAssociatityType::kRightToLeft, 15)

/// 非终结产生式
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleConstexprValue,
    c_parser_frontend::parse_functions::SingleConstexprValueChar, 0,
    {"Character"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleConstexprValue,
    c_parser_frontend::parse_functions::SingleConstexprValueIndexedString, 1,
    {"Str", "[", "Num", "]"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleConstexprValue,
    c_parser_frontend::parse_functions::SingleConstexprValueNum, 2, {"Num"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleConstexprValue,
    c_parser_frontend::parse_functions::SingleConstexprValueString, 3, {"Str"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FundamentalType, c_parser_frontend::parse_functions::FundamentalTypeChar, 0,
    {"char"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FundamentalType, c_parser_frontend::parse_functions::FundamentalTypeShort,
    1, {"short"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FundamentalType, c_parser_frontend::parse_functions::FundamentalTypeInt, 2,
    {"int"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FundamentalType, c_parser_frontend::parse_functions::FundamentalTypeLong, 3,
    {"long"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FundamentalType, c_parser_frontend::parse_functions::FundamentalTypeFloat,
    4, {"float"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FundamentalType, c_parser_frontend::parse_functions::FundamentalTypeDouble,
    5, {"double"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FundamentalType, c_parser_frontend::parse_functions::FundamentalTypeVoid, 6,
    {"void"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SignTag, c_parser_frontend::parse_functions::SignTagSigned, 0, {"signed"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SignTag, c_parser_frontend::parse_functions::SignTagUnSigned, 1,
    {"unsigned"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    ConstTag, c_parser_frontend::parse_functions::ConstTagConst, 0, {"const"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    IdOrEquivence, c_parser_frontend::parse_functions::IdOrEquivenceConstTagId,
    0, {"ConstTag", "Id"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    IdOrEquivence,
    c_parser_frontend::parse_functions::IdOrEquivenceNumAddressing, 2,
    {"IdOrEquivence", "[", "Num", "]"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    IdOrEquivence,
    c_parser_frontend::parse_functions::IdOrEquivenceAnonymousAddressing, 3,
    {"IdOrEquivence", "[", "]"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    IdOrEquivence,
    c_parser_frontend::parse_functions::IdOrEquivencePointerAnnounce, 4,
    {"ConstTag", "*", "IdOrEquivence"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    IdOrEquivence, c_parser_frontend::parse_functions::IdOrEquivenceInBrackets,
    5, {"(", "IdOrEquivence", ")"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    AnonymousIdOrEquivence,
    c_parser_frontend::parse_functions::AnonymousIdOrEquivenceConst, 0,
    {"const"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    AnonymousIdOrEquivence,
    c_parser_frontend::parse_functions::AnonymousIdOrEquivenceNumAddressing, 1,
    {"AnonymousIdOrEquivence", "[", "Num", "]"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    AnonymousIdOrEquivence,
    c_parser_frontend::parse_functions::
        AnonymousIdOrEquivenceAnonymousAddressing,
    2, {"AnonymousIdOrEquivence", "[", "]"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    AnonymousIdOrEquivence,
    c_parser_frontend::parse_functions::AnonymousIdOrEquivencePointerAnnounce,
    3, {"ConstTag", "*", "AnonymousIdOrEquivence"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    AnonymousIdOrEquivence,
    c_parser_frontend::parse_functions::AnonymousIdOrEquivenceInBrackets, 4,
    {"(", "AnonymousIdOrEquivence", ")"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    NotEmptyEnumArguments,
    c_parser_frontend::parse_functions::NotEmptyEnumArgumentsIdBase, 0, {"Id"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    NotEmptyEnumArguments,
    c_parser_frontend::parse_functions::NotEmptyEnumArgumentsIdAssignNumBase, 1,
    {"Id", "=", "Num"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    NotEmptyEnumArguments,
    c_parser_frontend::parse_functions::NotEmptyEnumArgumentsIdExtend, 2,
    {"NotEmptyEnumArguments", ",", "Id"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    NotEmptyEnumArguments,
    c_parser_frontend::parse_functions::NotEmptyEnumArgumentsIdAssignNumExtend,
    3, {"NotEmptyEnumArguments", ",", "Id", "=", "Num"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    EnumArguments,
    c_parser_frontend::parse_functions::EnumArgumentsNotEmptyEnumArguments, 0,
    {"NotEmptyEnumArguments"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Enum, c_parser_frontend::parse_functions::EnumDefine, 0,
    {"enum", "Id", "{", "EnumArguments", "}"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Enum, c_parser_frontend::parse_functions::EnumAnonymousDefine, 1,
    {"enum", "{", "EnumArguments", "}"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    EnumAnnounce, c_parser_frontend::parse_functions::EnumAnnounce, 0,
    {"enum", "Id"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    StructureAnnounce,
    c_parser_frontend::parse_functions::StructureAnnounceStructId, 0,
    {"struct", "Id"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    StructureAnnounce,
    c_parser_frontend::parse_functions::StructureAnnounceUnionId, 1,
    {"union", "Id"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    StructureDefineHead,
    c_parser_frontend::parse_functions::StructureDefineHeadStruct, 0,
    {"struct"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    StructureDefineHead,
    c_parser_frontend::parse_functions::StructureDefineHeadUnion, 1, {"union"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    StructureDefineHead,
    c_parser_frontend::parse_functions::StructureDefineHeadStructureAnnounce, 2,
    {"StructureAnnounce"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    StructureDefineInitHead,
    c_parser_frontend::parse_functions::StructureDefineInitHead, 0,
    {"StructureDefineHead", "{"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    StructureDefine, c_parser_frontend::parse_functions::StructureDefine, 0,
    {"StructureDefineInitHead", "StructureBody", "}"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    StructType, c_parser_frontend::parse_functions::StructTypeStructDefine, 0,
    {"StructureDefine"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    StructType, c_parser_frontend::parse_functions::StructTypeStructAnnounce, 1,
    {"StructureAnnounce"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    BasicType, c_parser_frontend::parse_functions::BasicTypeFundamental, 0,
    {"ConstTag", "SignTag", "FundamentalType"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    BasicType, c_parser_frontend::parse_functions::BasicTypeStructType, 1,
    {"ConstTag", "StructType"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    BasicType, c_parser_frontend::parse_functions::BasicTypeEnumAnnounce, 3,
    {"ConstTag", "EnumAnnounce"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FunctionRelaventBasePartFunctionInit,
    c_parser_frontend::parse_functions::
        FunctionRelaventBasePartFunctionInitBase,
    0, {"IdOrEquivence", "("})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FunctionRelaventBasePartFunctionInit,
    c_parser_frontend::parse_functions::
        FunctionRelaventBasePartFunctionInitExtend,
    1, {"FunctionRelaventBasePart", "("})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FunctionRelaventBasePart,
    c_parser_frontend::parse_functions::FunctionRelaventBasePartFunction, 0,
    {"FunctionRelaventBasePartFunctionInit", "FunctionRelaventArguments", ")"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FunctionRelaventBasePart,
    c_parser_frontend::parse_functions::FunctionRelaventBasePartPointer, 1,
    {"ConstTag", "*", "FunctionRelaventBasePart"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FunctionRelaventBasePart,
    c_parser_frontend::parse_functions::FunctionRelaventBasePartBranckets, 2,
    {"(", "FunctionRelaventBasePart", ")"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FunctionRelavent, c_parser_frontend::parse_functions::FunctionRelavent, 0,
    {"BasicType", "FunctionRelaventBasePart"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleAnnounceNoAssign,
    c_parser_frontend::parse_functions::SingleAnnounceNoAssignVariety<false>, 0,
    {"BasicType", "IdOrEquivence"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleAnnounceNoAssign,
    c_parser_frontend::parse_functions::SingleAnnounceNoAssignNotPodVariety, 1,
    {"ConstTag", "Id", "IdOrEquivence"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleAnnounceNoAssign,
    c_parser_frontend::parse_functions::SingleAnnounceNoAssignFunctionRelavent<
        false>,
    2, {"FunctionRelavent"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    AnonymousSingleAnnounceNoAssign,
    c_parser_frontend::parse_functions::SingleAnnounceNoAssignVariety<true>, 0,
    {"BasicType", "AnonymousIdOrEquivence"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    TypeDef, c_parser_frontend::parse_functions::TypeDef, 0,
    {"typedef", "SingleAnnounceNoAssign"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    NotEmptyFunctionPointerArguments,
    c_parser_frontend::parse_functions::NotEmptyFunctionRelaventArgumentsBase,
    0, {"SingleAnnounceNoAssign"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    NotEmptyFunctionPointerArguments,
    c_parser_frontend::parse_functions::
        NotEmptyFunctionRelaventArgumentsAnonymousBase,
    1, {"AnonymousSingleAnnounceNoAssign"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    NotEmptyFunctionPointerArguments,
    c_parser_frontend::parse_functions::NotEmptyFunctionRelaventArgumentsExtend,
    2, {"NotEmptyFunctionPointerArguments", ",", "SingleAnnounceNoAssign"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    NotEmptyFunctionPointerArguments,
    c_parser_frontend::parse_functions::
        NotEmptyFunctionRelaventArgumentsAnonymousExtend,
    3,
    {"NotEmptyFunctionPointerArguments", ",",
     "AnonymousSingleAnnounceNoAssign"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FunctionRelaventArguments,
    c_parser_frontend::parse_functions::FunctionRelaventArguments, 0,
    {"NotEmptyFunctionPointerArguments"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FunctionDefineHead, c_parser_frontend::parse_functions::FunctionDefineHead,
    0, {"FunctionRelavent", "{"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FunctionDefine, c_parser_frontend::parse_functions::FunctionDefine, 0,
    {"FunctionDefineHead", "Statements", "}"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStructureBody,
    c_parser_frontend::parse_functions::SingleStructureBodyBase, 0,
    {"SingleAnnounceNoAssign"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStructureBody,
    c_parser_frontend::parse_functions::SingleStructureBodyExtend, 1,
    {"SingleStructureBody", "Id"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    NotEmptyStructureBody,
    c_parser_frontend::parse_functions::NotEmptyStructureBodyBase, 0,
    {"SingleStructureBody"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    NotEmptyStructureBody,
    c_parser_frontend::parse_functions::NotEmptyStructureBodyExtend, 1,
    {"NotEmptyStructureBody", "SingleStructureBody", ";"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    StructureBody, c_parser_frontend::parse_functions::StructureBody, 0,
    {"NotEmptyStructureBody"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    InitializeList, c_parser_frontend::parse_functions::InitializeList, 0,
    {"{", "InitializeListArguments", "}"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleInitializeListArgument,
    c_parser_frontend::parse_functions::
        SingleInitializeListArgumentConstexprValue,
    0, {"SingleConstexprValue"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleInitializeListArgument,
    c_parser_frontend::parse_functions::SingleInitializeListArgumentList, 1,
    {"InitializeList"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    InitializeListArguments,
    c_parser_frontend::parse_functions::InitializeListArgumentsBase, 0,
    {"SingleInitializeListArgument"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    InitializeListArguments,
    c_parser_frontend::parse_functions::InitializeListArgumentsExtend, 1,
    {"InitializeListArguments", ",", "SingleInitializeListArgument"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    AnnounceAssignable,
    c_parser_frontend::parse_functions::AnnounceAssignableAssignable, 0,
    {"Assignable"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    AnnounceAssignable,
    c_parser_frontend::parse_functions::AnnounceAssignableInitializeList, 1,
    {"InitializeList"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleAnnounceAndAssign,
    c_parser_frontend::parse_functions::SingleAnnounceAndAssignNoAssignBase, 0,
    {"SingleAnnounceNoAssign"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleAnnounceAndAssign,
    c_parser_frontend::parse_functions::SingleAnnounceAndAssignWithAssignBase,
    1, {"SingleAnnounceNoAssign", "=", "AnnounceAssignable"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleAnnounceAndAssign,
    c_parser_frontend::parse_functions::SingleAnnounceAndAssignNoAssignExtend,
    2, {"SingleAnnounceAndAssign", ",", "Id"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleAnnounceAndAssign,
    c_parser_frontend::parse_functions::SingleAnnounceAndAssignWithAssignExtend,
    3, {"SingleAnnounceAndAssign", ",", "Id", "=", "AnnounceAssignable"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Type, c_parser_frontend::parse_functions::TypeBasicType, 0, {"BasicType"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Type, c_parser_frontend::parse_functions::TypeFunctionRelavent, 1,
    {"FunctionRelavent"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalOperator,
    c_parser_frontend::parse_functions::MathematicalOperatorPlus, 0, {"+"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalOperator,
    c_parser_frontend::parse_functions::MathematicalOperatorMinus, 1, {"-"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalOperator,
    c_parser_frontend::parse_functions::MathematicalOperatorMultiple, 2, {"*"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalOperator,
    c_parser_frontend::parse_functions::MathematicalOperatorDivide, 3, {"/"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalOperator,
    c_parser_frontend::parse_functions::MathematicalOperatorMod, 4, {"%"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalOperator,
    c_parser_frontend::parse_functions::MathematicalOperatorLeftShift, 5,
    {"<<"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalOperator,
    c_parser_frontend::parse_functions::MathematicalOperatorRightShift, 6,
    {">>"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalOperator,
    c_parser_frontend::parse_functions::MathematicalOperatorAnd, 7, {"&"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalOperator,
    c_parser_frontend::parse_functions::MathematicalOperatorOr, 8, {"|"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalOperator,
    c_parser_frontend::parse_functions::MathematicalOperatorXor, 9, {"^"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalOperator,
    c_parser_frontend::parse_functions::MathematicalOperatorNot, 10, {"!"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalAndAssignOperator,
    c_parser_frontend::parse_functions::MathematicalAndAssignOperatorPlusAssign,
    0, {"+="})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalAndAssignOperator,
    c_parser_frontend::parse_functions::
        MathematicalAndAssignOperatorMinusAssign,
    1, {"-="})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalAndAssignOperator,
    c_parser_frontend::parse_functions::
        MathematicalAndAssignOperatorMultipleAssign,
    2, {"*="})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalAndAssignOperator,
    c_parser_frontend::parse_functions::
        MathematicalAndAssignOperatorDivideAssign,
    3, {"/="})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalAndAssignOperator,
    c_parser_frontend::parse_functions::MathematicalAndAssignOperatorModAssign,
    4, {"%="})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalAndAssignOperator,
    c_parser_frontend::parse_functions::
        MathematicalAndAssignOperatorLeftShiftAssign,
    5, {"<<="})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalAndAssignOperator,
    c_parser_frontend::parse_functions::
        MathematicalAndAssignOperatorRightShiftAssign,
    6, {">>="})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalAndAssignOperator,
    c_parser_frontend::parse_functions::MathematicalAndAssignOperatorAndAssign,
    7, {"&="})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalAndAssignOperator,
    c_parser_frontend::parse_functions::MathematicalAndAssignOperatorOrAssign,
    8, {"|="})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalAndAssignOperator,
    c_parser_frontend::parse_functions::MathematicalAndAssignOperatorXorAssign,
    9, {"^="})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    LogicalOperator, c_parser_frontend::parse_functions::LogicalOperatorAndAnd,
    0, {"&&"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    LogicalOperator, c_parser_frontend::parse_functions::LogicalOperatorOrOr, 1,
    {"||"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    LogicalOperator, c_parser_frontend::parse_functions::LogicalOperatorGreater,
    2, {">"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    LogicalOperator,
    c_parser_frontend::parse_functions::LogicalOperatorGreaterEqual, 3, {">="})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    LogicalOperator, c_parser_frontend::parse_functions::LogicalOperatorLess, 4,
    {"<"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    LogicalOperator,
    c_parser_frontend::parse_functions::LogicalOperatorLessEqual, 5, {"<="})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    LogicalOperator, c_parser_frontend::parse_functions::LogicalOperatorEqual,
    6, {"=="})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    LogicalOperator,
    c_parser_frontend::parse_functions::LogicalOperatorNotEqual, 7, {"!="})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableConstexprValue, 0,
    {"SingleConstexprValue"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableId, 1, {"Id"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableTemaryOperator, 2,
    {"TemaryOperator"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableFunctionCall, 3,
    {"FunctionCall"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableSizeOfType, 4,
    {"sizeof", "(", "Type", ")"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableSizeOfAssignable,
    5, {"sizeof", "(", "Assignable", ")"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableMemberAccess, 6,
    {"Assignable", ",", "Id"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable,
    c_parser_frontend::parse_functions::AssignablePointerMemberAccess, 7,
    {"Assignable", "->", "Id"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableBracket, 8,
    {"(", "Assignable", ")"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableTypeConvert, 9,
    {"(", "Type", ")", "Assignable"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableAssign, 10,
    {"Assignable", "=", "Assignable"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable,
    c_parser_frontend::parse_functions::AssignableMathematicalOperate, 11,
    {"Assignable", "MathematicalOperator", "Assignable"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable,
    c_parser_frontend::parse_functions::AssignableMathematicalAndAssignOperate,
    12, {"Assignable", "MathematicalAndAssignOperator", "Assignable"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableLogicalOperate,
    13, {"Assignable", "LogicalOperator", "Assignable"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableNot, 14,
    {"!", "Assignable"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableLogicalNegative,
    15, {"~", "Assignable"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable,
    c_parser_frontend::parse_functions::AssignableMathematicalNegative, 16,
    {"-", "Assignable"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableObtainAddress, 17,
    {"&", "Assignable"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableDereference, 18,
    {"*", "Assignable"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableArrayAccess, 19,
    {"Assignable", "[", "Assignable", "]"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignablePrefixPlus, 20,
    {"++", "Assignable"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignablePrefixMinus, 21,
    {"--", "Assignable"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableSuffixPlus, 22,
    {"Assignable", "++"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableSuffixMinus, 23,
    {"Assignable", "--"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Return, c_parser_frontend::parse_functions::ReturnWithValue, 0,
    {"return", "Assignable", ";"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Return, c_parser_frontend::parse_functions::ReturnWithoutValue, 1,
    {"return", ";"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    TemaryOperator, c_parser_frontend::parse_functions::TemaryOperator, 0,
    {"Assignable", "?", "Assignable", ":", "Assignable"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    NotEmptyFunctionCallArguments,
    c_parser_frontend::parse_functions::NotEmptyFunctionCallArgumentsBase, 0,
    {"Assignable"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    NotEmptyFunctionCallArguments,
    c_parser_frontend::parse_functions::NotEmptyFunctionCallArgumentsExtend, 1,
    {"NotEmptyFunctionCallArguments", ",", "Assignable"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FunctionCallArguments,
    c_parser_frontend::parse_functions::FunctionCallArguments, 0,
    {"NotEmptyFunctionCallArguments"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FunctionCallInit, c_parser_frontend::parse_functions::FunctionCallInit, 0,
    {"Assignable", "("})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FunctionCall, c_parser_frontend::parse_functions::FunctionCall, 0,
    {"FunctionCallInit", "FunctionCallArguments", ")"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignables, c_parser_frontend::parse_functions::AssignablesBase, 0,
    {"Assignable"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignables, c_parser_frontend::parse_functions::AssignablesExtend, 1,
    {"Assignables", ",", "Assignable"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Break, c_parser_frontend::parse_functions::Break, 0, {"break", ";"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Continue, c_parser_frontend::parse_functions::Continue, 0,
    {"continue", ";"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStatement, c_parser_frontend::parse_functions::SingleStatementIf, 0,
    {"If"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStatement, c_parser_frontend::parse_functions::SingleStatementDoWhile,
    1, {"DoWhile"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStatement, c_parser_frontend::parse_functions::SingleStatementWhile,
    2, {"While"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStatement, c_parser_frontend::parse_functions::SingleStatementFor, 3,
    {"For"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStatement, c_parser_frontend::parse_functions::SingleStatementSwitch,
    4, {"Switch"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStatement,
    c_parser_frontend::parse_functions::SingleStatementAssignable, 5,
    {"Assignable", ";"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStatement,
    c_parser_frontend::parse_functions::SingleStatementAnnounce, 6,
    {"SingleAnnounceAndAssign", ";"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStatement, c_parser_frontend::parse_functions::SingleStatementReturn,
    7, {"Return"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStatement, c_parser_frontend::parse_functions::SingleStatementBreak,
    8, {"Break"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStatement,
    c_parser_frontend::parse_functions::SingleStatementContinue, 9,
    {"Continue"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStatement,
    c_parser_frontend::parse_functions::SingleStatementEmptyStatement, 10,
    {";"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    IfCondition, c_parser_frontend::parse_functions::IfCondition, 0,
    {"if", "(", "Assignable", ")"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    IfWithElse, c_parser_frontend::parse_functions::IfWithElse, 0,
    {"IfCondition", "ProcessControlSentenceBody", "else"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    If, c_parser_frontend::parse_functions::IfElseSence, 0,
    {"IfWithElse", "ProcessControlSentenceBody"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    If, c_parser_frontend::parse_functions::IfIfSentence, 1,
    {"IfCondition", "ProcessControlSentenceBody"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    ForRenewSentence, c_parser_frontend::parse_functions::ForRenewSentence, 0,
    {"Assignables"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    ForInitSentence,
    c_parser_frontend::parse_functions::ForInitSentenceAssignables, 0,
    {"Assignables"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    ForInitSentence,
    c_parser_frontend::parse_functions::ForInitSentenceAnnounce, 1,
    {"SingleAnnounceAndAssign"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    ForInitHead, c_parser_frontend::parse_functions::ForInitHead, 0, {"for"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    ForHead, c_parser_frontend::parse_functions::ForHead, 0,
    {"ForInitHead", "(", "ForInitSentence", ";", "Assignable", ";",
     "ForRenewSentence", ")"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    For, c_parser_frontend::parse_functions::For, 0,
    {"ForHead", "ProcessControlSentenceBody"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    WhileInitHead, c_parser_frontend::parse_functions::WhileInitHead, 0,
    {"while", "(", "Assignable", ")"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    While, c_parser_frontend::parse_functions::While, 0,
    {"WhileInitHead", "ProcessControlSentenceBody"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    DoWhileInitHead, c_parser_frontend::parse_functions::DoWhileInitHead, 0,
    {"do"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    DoWhile, c_parser_frontend::parse_functions::DoWhile, 0,
    {"DoWhileInitHead", "ProcessControlSentenceBody", "while", "(",
     "Assignable", ")", ";"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SwitchCase, c_parser_frontend::parse_functions::SwitchCaseSimple, 0,
    {"case", "SingleConstexprValue", ":"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SwitchCase, c_parser_frontend::parse_functions::SwitchCaseDefault, 1,
    {"default", ":"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleSwitchStatement,
    c_parser_frontend::parse_functions::SingleSwitchStatementCase, 0,
    {"SwitchCase"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleSiwtchStatement,
    c_parser_frontend::parse_functions::SingleSwitchStatementStatements, 1,
    {"Statements"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SwitchStatements, c_parser_frontend::parse_functions::SwitchStatements, 0,
    {"SwitchStatements", "SingleSwitchStatement"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SwitchCondition, c_parser_frontend::parse_functions::SwitchCondition, 0,
    {"switch", "(", "Assignable", ")"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Switch, c_parser_frontend::parse_functions::Switch, 0,
    {"SwitchCondition", "{", "SwitchStatements", "}"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    StatementsLeftBrace,
    c_parser_frontend::parse_functions::StatementsLeftBrace, 0, {"{"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Statements, c_parser_frontend::parse_functions::StatementsSingleStatement,
    0, {"Statements", "SingleStatement"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Statements, c_parser_frontend::parse_functions::StatementsBrace, 1,
    {"StatementsLeftBrace", "Statements", "}"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    ProcessControlSentenceBody,
    c_parser_frontend::parse_functions::
        ProcessControlSentenceBodySingleStatement,
    0, {"SingleStatement"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    ProcessControlSentenceBody,
    c_parser_frontend::parse_functions::ProcessControlSentenceBodyStatements, 1,
    {"{", "Statements", "}"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Root, c_parser_frontend::parse_functions::RootFunctionDefine, 0,
    {"Root", "FunctionDefine"})
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Root, c_parser_frontend::parse_functions::RootAnnounce, 1,
    {"Root", "SingleAnnounceNoAssign", ";"})

/// 设置可以空规约的非终结节点
GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT(EnumArguments)
GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT(SignTag)
GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT(ConstTag)
GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT(EnumArguments)
GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT(AnonymousIdOrEquivence)
GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT(
    FunctionRelaventArguments)
GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT(StructureBody)
GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT(FunctionCallArguments)
GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT(ForRenewSentences)
GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT(ForInitSentence)
GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT(SwitchStatements)
GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT(Statements)
GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT(Root)

/// 设置产生式根节点
/// 仅允许设置一个
GENERATOR_DEFINE_ROOT_PRODUCTION(Root)