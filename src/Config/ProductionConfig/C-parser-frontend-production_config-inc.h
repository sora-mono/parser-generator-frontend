/// 该文件仅且必须最终被Generator/SyntaxGenerator下的
/// process_functions_classes.h
/// process_functions_classes_register.h
/// reduct_type_register.h
/// config_construct.cpp所包含
#include "Generator/SyntaxGenerator/syntax_generate.h"

/// 用户可修改部分
/// 用户定义头文件请在user_defined_functions.h中添加
/// 该文件不可以添加用户定义头文件

/// 终结产生式
/// 使用正则语法定义
GENERATOR_DEFINE_TERMINAL_PRODUCTION(Id, R"([a-zA-Z_][a-zA-Z0-9_]*)")
GENERATOR_DEFINE_TERMINAL_PRODUCTION(Num, R"([+-]?[0-9]+(\.[0-9]*)?)")
GENERATOR_DEFINE_TERMINAL_PRODUCTION(Str, R"(".*")")
GENERATOR_DEFINE_TERMINAL_PRODUCTION(Character, R"('..?')")
GENERATOR_DEFINE_TERMINAL_PRODUCTION(RightSquareBracket, R"(\])")
GENERATOR_DEFINE_TERMINAL_PRODUCTION(RightParenthesis, R"(\))")
GENERATOR_DEFINE_TERMINAL_PRODUCTION(LeftCurlyBracket, R"({)")
GENERATOR_DEFINE_TERMINAL_PRODUCTION(RightCurlyBracket, R"(})")
GENERATOR_DEFINE_TERMINAL_PRODUCTION(Semicolon, R"(;)")
GENERATOR_DEFINE_TERMINAL_PRODUCTION(Colon, R"(:)")

/// 关键字
/// 定义时不允许使用正则
GENERATOR_DEFINE_KEY_WORD(KeyWordChar, "char")
GENERATOR_DEFINE_KEY_WORD(KeyWordShort, "short")
GENERATOR_DEFINE_KEY_WORD(KeyWordInt, "int")
GENERATOR_DEFINE_KEY_WORD(KeyWordLong, "long")
GENERATOR_DEFINE_KEY_WORD(KeyWordFloat, "float")
GENERATOR_DEFINE_KEY_WORD(KeyWordDouble, "double")
GENERATOR_DEFINE_KEY_WORD(KeyWordVoid, "void")
GENERATOR_DEFINE_KEY_WORD(KeyWordSigned, "signed")
GENERATOR_DEFINE_KEY_WORD(KeyWordUnsigned, "unsigned")
GENERATOR_DEFINE_KEY_WORD(KeyWordConst, "const")
GENERATOR_DEFINE_KEY_WORD(KeyWordEnum, "enum")
GENERATOR_DEFINE_KEY_WORD(KeyWordStruct, "struct")
GENERATOR_DEFINE_KEY_WORD(KeyWordUnion, "union")
GENERATOR_DEFINE_KEY_WORD(KeyWordTypedef, "typedef")
GENERATOR_DEFINE_KEY_WORD(KeyWordReturn, "return")
GENERATOR_DEFINE_KEY_WORD(KeyWordBreak, "break")
GENERATOR_DEFINE_KEY_WORD(KeyWordContinue, "continue")
GENERATOR_DEFINE_KEY_WORD(KeyWordIf, "if")
GENERATOR_DEFINE_KEY_WORD(KeyWordElse, "else")
GENERATOR_DEFINE_KEY_WORD(KeyWordFor, "for")
GENERATOR_DEFINE_KEY_WORD(KeyWordDo, "do")
GENERATOR_DEFINE_KEY_WORD(KeyWordWhile, "while")
GENERATOR_DEFINE_KEY_WORD(KeyWordSwitch, "switch")
GENERATOR_DEFINE_KEY_WORD(KeyWordCase, "case")
GENERATOR_DEFINE_KEY_WORD(KeyWordDefault, "default")

/// 运算符
/// 定义时不允许使用正则
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorComma, ",",
                                 OperatorAssociatityType::kLeftToRight, 1)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorAssign, "=",
                                 OperatorAssociatityType::kRightToLeft, 2)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorPlusAssign,
                                 "+=", OperatorAssociatityType::kRightToLeft, 2)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorMinusAssign,
                                 "-=", OperatorAssociatityType::kRightToLeft, 2)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorMultiAssign,
                                 "*=", OperatorAssociatityType::kRightToLeft, 2)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorDivAssign,
                                 "/=", OperatorAssociatityType::kRightToLeft, 2)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorModAssign,
                                 "%=", OperatorAssociatityType::kRightToLeft, 2)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorLeftShiftAssign,
                                 "<<=", OperatorAssociatityType::kRightToLeft,
                                 2)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorRightShiftAssign,
                                 ">>=", OperatorAssociatityType::kRightToLeft,
                                 2)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorAndAssign,
                                 "&=", OperatorAssociatityType::kRightToLeft, 2)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorOrAssign,
                                 "|=", OperatorAssociatityType::kRightToLeft, 2)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorXORAssign,
                                 "^=", OperatorAssociatityType::kRightToLeft, 2)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorQuestionMark, "?",
                                 OperatorAssociatityType::kRightToLeft, 3)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorLogicOr, "||",
                                 OperatorAssociatityType::kLeftToRight, 4)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorLogicAnd, "&&",
                                 OperatorAssociatityType::kLeftToRight, 5)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorOr, "|",
                                 OperatorAssociatityType::kLeftToRight, 6)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorXOR, "^",
                                 OperatorAssociatityType::kLeftToRight, 7)
GENERATOR_DEFINE_BINARY_UNARY_OPERATOR(OperatorAndGetAddress, "&",
                                       OperatorAssociatityType::kLeftToRight, 8,
                                       OperatorAssociatityType::kRightToLeft,
                                       14)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorNotEqual,
                                 "!=", OperatorAssociatityType::kLeftToRight, 9)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorEqual,
                                 "==", OperatorAssociatityType::kLeftToRight, 9)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorGreater, ">",
                                 OperatorAssociatityType::kLeftToRight, 10)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorGreaterEqual,
                                 ">=", OperatorAssociatityType::kLeftToRight,
                                 10)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorLess, "<",
                                 OperatorAssociatityType::kLeftToRight, 10)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorLessEqual,
                                 "<=", OperatorAssociatityType::kLeftToRight,
                                 10)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorLeftShift, "<<",
                                 OperatorAssociatityType::kLeftToRight, 11)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorRightShift, ">>",
                                 OperatorAssociatityType::kLeftToRight, 11)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorPlus, "+",
                                 OperatorAssociatityType::kLeftToRight, 12)
GENERATOR_DEFINE_BINARY_UNARY_OPERATOR(OperatorMinusNegative, "-",
                                       OperatorAssociatityType::kLeftToRight,
                                       12,
                                       OperatorAssociatityType::kRightToLeft,
                                       14)
GENERATOR_DEFINE_BINARY_UNARY_OPERATOR(OperatorMultiResolveRef, "*",
                                       OperatorAssociatityType::kLeftToRight,
                                       13,
                                       OperatorAssociatityType::kRightToLeft,
                                       14)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorDiv, "/",
                                 OperatorAssociatityType::kLeftToRight, 13)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorMod, "%",
                                 OperatorAssociatityType::kLeftToRight, 13)
GENERATOR_DEFINE_BINARY_UNARY_OPERATOR(OperatorPlusOne, "++",
                                       OperatorAssociatityType::kRightToLeft,
                                       14,
                                       OperatorAssociatityType::kRightToLeft,
                                       14)
GENERATOR_DEFINE_BINARY_UNARY_OPERATOR(OperatorMinusOne, "--",
                                       OperatorAssociatityType::kRightToLeft,
                                       14,
                                       OperatorAssociatityType::kRightToLeft,
                                       14)
GENERATOR_DEFINE_UNARY_OPERATOR(OperatorNot, "!",
                                OperatorAssociatityType::kRightToLeft, 14)
GENERATOR_DEFINE_UNARY_OPERATOR(OperatorBitwiseNegation, "~",
                                OperatorAssociatityType::kRightToLeft, 14)
GENERATOR_DEFINE_UNARY_OPERATOR(OperatorSizeof, "sizeof",
                                OperatorAssociatityType::kRightToLeft, 14)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorMemberAccess, ".",
                                 OperatorAssociatityType::kRightToLeft, 15)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorPointerMemberAccess, "->",
                                 OperatorAssociatityType::kRightToLeft, 15)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorLeftSquareBracket, "[",
                                 OperatorAssociatityType::kRightToLeft, 15)
GENERATOR_DEFINE_BINARY_OPERATOR(OperatorLeftBracket, "(",
                                 OperatorAssociatityType::kRightToLeft, 15)

/// 非终结产生式
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleConstexprValue,
    c_parser_frontend::parse_functions::SingleConstexprValueChar, Character)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleConstexprValue,
    c_parser_frontend::parse_functions::SingleConstexprValueIndexedString, Str,
    OperatorLeftSquareBracket, Num, RightSquareBracket)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleConstexprValue,
    c_parser_frontend::parse_functions::SingleConstexprValueNum, Num)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleConstexprValue,
    c_parser_frontend::parse_functions::SingleConstexprValueString, Str)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FundamentalType, c_parser_frontend::parse_functions::FundamentalTypeChar,
    KeyWordChar)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FundamentalType, c_parser_frontend::parse_functions::FundamentalTypeShort,
    KeyWordShort)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FundamentalType, c_parser_frontend::parse_functions::FundamentalTypeInt,
    KeyWordInt)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FundamentalType, c_parser_frontend::parse_functions::FundamentalTypeLong,
    KeyWordLong)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FundamentalType, c_parser_frontend::parse_functions::FundamentalTypeFloat,
    KeyWordFloat)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FundamentalType, c_parser_frontend::parse_functions::FundamentalTypeDouble,
    KeyWordDouble)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FundamentalType, c_parser_frontend::parse_functions::FundamentalTypeVoid,
    KeyWordVoid)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SignTag, c_parser_frontend::parse_functions::SignTagSigned, KeyWordSigned)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SignTag, c_parser_frontend::parse_functions::SignTagUnSigned,
    KeyWordUnsigned)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    ConstTag, c_parser_frontend::parse_functions::ConstTagConst, KeyWordConst)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    IdOrEquivence, c_parser_frontend::parse_functions::IdOrEquivenceConstTagId,
    ConstTag, Id)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    IdOrEquivence,
    c_parser_frontend::parse_functions::IdOrEquivenceNumAddressing,
    IdOrEquivence, OperatorLeftSquareBracket, Num, RightSquareBracket)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    IdOrEquivence,
    c_parser_frontend::parse_functions::IdOrEquivenceAnonymousAddressing,
    IdOrEquivence, OperatorLeftSquareBracket, RightSquareBracket)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    IdOrEquivence,
    c_parser_frontend::parse_functions::IdOrEquivencePointerAnnounce, ConstTag,
    OperatorMultiResolveRef, IdOrEquivence)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    IdOrEquivence, c_parser_frontend::parse_functions::IdOrEquivenceInBrackets,
    OperatorLeftBracket, IdOrEquivence, RightParenthesis)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    AnonymousIdOrEquivence,
    c_parser_frontend::parse_functions::AnonymousIdOrEquivenceConst,
    KeyWordConst)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    AnonymousIdOrEquivence,
    c_parser_frontend::parse_functions::AnonymousIdOrEquivenceNumAddressing,
    AnonymousIdOrEquivence, OperatorLeftSquareBracket, Num, RightSquareBracket)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    AnonymousIdOrEquivence,
    c_parser_frontend::parse_functions::
        AnonymousIdOrEquivenceAnonymousAddressing,
    AnonymousIdOrEquivence, OperatorLeftSquareBracket, RightSquareBracket)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    AnonymousIdOrEquivence,
    c_parser_frontend::parse_functions::AnonymousIdOrEquivencePointerAnnounce,
    ConstTag, OperatorMultiResolveRef, AnonymousIdOrEquivence)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    AnonymousIdOrEquivence,
    c_parser_frontend::parse_functions::AnonymousIdOrEquivenceInBrackets,
    OperatorLeftBracket, AnonymousIdOrEquivence, RightParenthesis)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    NotEmptyEnumArguments,
    c_parser_frontend::parse_functions::NotEmptyEnumArgumentsIdBase, Id)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    NotEmptyEnumArguments,
    c_parser_frontend::parse_functions::NotEmptyEnumArgumentsIdAssignNumBase,
    Id, OperatorAssign, Num)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    NotEmptyEnumArguments,
    c_parser_frontend::parse_functions::NotEmptyEnumArgumentsIdExtend,
    NotEmptyEnumArguments, OperatorComma, Id)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    NotEmptyEnumArguments,
    c_parser_frontend::parse_functions::NotEmptyEnumArgumentsIdAssignNumExtend,
    NotEmptyEnumArguments, OperatorComma, Id, OperatorAssign, Num)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    EnumArguments,
    c_parser_frontend::parse_functions::EnumArgumentsNotEmptyEnumArguments,
    NotEmptyEnumArguments)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Enum, c_parser_frontend::parse_functions::EnumDefine, KeyWordEnum, Id,
    LeftCurlyBracket, EnumArguments, RightCurlyBracket)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Enum, c_parser_frontend::parse_functions::EnumAnonymousDefine, KeyWordEnum,
    LeftCurlyBracket, EnumArguments, RightCurlyBracket)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    EnumAnnounce, c_parser_frontend::parse_functions::EnumAnnounce, KeyWordEnum,
    Id)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    StructureAnnounce,
    c_parser_frontend::parse_functions::StructureAnnounceStructId,
    KeyWordStruct, Id)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    StructureAnnounce,
    c_parser_frontend::parse_functions::StructureAnnounceUnionId, KeyWordUnion,
    Id)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    StructureDefineHead,
    c_parser_frontend::parse_functions::StructureDefineHeadStruct,
    KeyWordStruct)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    StructureDefineHead,
    c_parser_frontend::parse_functions::StructureDefineHeadUnion, KeyWordUnion)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    StructureDefineHead,
    c_parser_frontend::parse_functions::StructureDefineHeadStructureAnnounce,
    StructureAnnounce)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    StructureDefineInitHead,
    c_parser_frontend::parse_functions::StructureDefineInitHead,
    StructureDefineHead, LeftCurlyBracket)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    StructureDefine, c_parser_frontend::parse_functions::StructureDefine,
    StructureDefineInitHead, StructureBody, RightCurlyBracket)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    StructType, c_parser_frontend::parse_functions::StructTypeStructDefine,
    StructureDefine)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    StructType, c_parser_frontend::parse_functions::StructTypeStructAnnounce,
    StructureAnnounce)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    BasicType, c_parser_frontend::parse_functions::BasicTypeFundamental,
    ConstTag, SignTag, FundamentalType)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    BasicType, c_parser_frontend::parse_functions::BasicTypeStructType,
    ConstTag, StructType)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    BasicType, c_parser_frontend::parse_functions::BasicTypeEnumAnnounce,
    ConstTag, EnumAnnounce)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FunctionRelaventBasePartFunctionInit,
    c_parser_frontend::parse_functions::
        FunctionRelaventBasePartFunctionInitBase,
    IdOrEquivence, OperatorLeftBracket)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FunctionRelaventBasePartFunctionInit,
    c_parser_frontend::parse_functions::
        FunctionRelaventBasePartFunctionInitExtend,
    FunctionRelaventBasePart, OperatorLeftBracket)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FunctionRelaventBasePart,
    c_parser_frontend::parse_functions::FunctionRelaventBasePartFunction,
    FunctionRelaventBasePartFunctionInit, FunctionRelaventArguments,
    RightParenthesis)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FunctionRelaventBasePart,
    c_parser_frontend::parse_functions::FunctionRelaventBasePartPointer,
    ConstTag, OperatorMultiResolveRef, FunctionRelaventBasePart)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FunctionRelaventBasePart,
    c_parser_frontend::parse_functions::FunctionRelaventBasePartBranckets,
    OperatorLeftBracket, FunctionRelaventBasePart, RightParenthesis)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FunctionRelavent, c_parser_frontend::parse_functions::FunctionRelavent,
    BasicType, FunctionRelaventBasePart)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleAnnounceNoAssign,
    c_parser_frontend::parse_functions::SingleAnnounceNoAssignVariety<false>,
    BasicType, IdOrEquivence)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleAnnounceNoAssign,
    c_parser_frontend::parse_functions::SingleAnnounceNoAssignNotPodVariety,
    ConstTag, Id, IdOrEquivence)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleAnnounceNoAssign,
    c_parser_frontend::parse_functions::SingleAnnounceNoAssignFunctionRelavent<
        false>,
    FunctionRelavent)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    AnonymousSingleAnnounceNoAssign,
    c_parser_frontend::parse_functions::SingleAnnounceNoAssignVariety<true>,
    BasicType, AnonymousIdOrEquivence)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    TypeDef, c_parser_frontend::parse_functions::TypeDef, KeyWordTypedef,
    SingleAnnounceNoAssign)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    NotEmptyFunctionPointerArguments,
    c_parser_frontend::parse_functions::NotEmptyFunctionRelaventArgumentsBase,
    SingleAnnounceNoAssign)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    NotEmptyFunctionPointerArguments,
    c_parser_frontend::parse_functions::
        NotEmptyFunctionRelaventArgumentsAnonymousBase,
    AnonymousSingleAnnounceNoAssign)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    NotEmptyFunctionPointerArguments,
    c_parser_frontend::parse_functions::NotEmptyFunctionRelaventArgumentsExtend,
    NotEmptyFunctionPointerArguments, OperatorComma, SingleAnnounceNoAssign)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    NotEmptyFunctionPointerArguments,
    c_parser_frontend::parse_functions::
        NotEmptyFunctionRelaventArgumentsAnonymousExtend,
    NotEmptyFunctionPointerArguments, OperatorComma,
    AnonymousSingleAnnounceNoAssign)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FunctionRelaventArguments,
    c_parser_frontend::parse_functions::FunctionRelaventArguments,
    NotEmptyFunctionPointerArguments)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FunctionDefineHead, c_parser_frontend::parse_functions::FunctionDefineHead,
    FunctionRelavent, LeftCurlyBracket)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FunctionDefine, c_parser_frontend::parse_functions::FunctionDefine,
    FunctionDefineHead, Statements, RightCurlyBracket)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStructureBody,
    c_parser_frontend::parse_functions::SingleStructureBodyBase,
    SingleAnnounceNoAssign)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStructureBody,
    c_parser_frontend::parse_functions::SingleStructureBodyExtend,
    SingleStructureBody, OperatorComma, Id)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    NotEmptyStructureBody,
    c_parser_frontend::parse_functions::NotEmptyStructureBodyBase,
    SingleStructureBody)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    NotEmptyStructureBody,
    c_parser_frontend::parse_functions::NotEmptyStructureBodyExtend,
    NotEmptyStructureBody, SingleStructureBody, Semicolon)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    StructureBody, c_parser_frontend::parse_functions::StructureBody,
    NotEmptyStructureBody)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    InitializeList, c_parser_frontend::parse_functions::InitializeList,
    LeftCurlyBracket, InitializeListArguments, RightCurlyBracket)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleInitializeListArgument,
    c_parser_frontend::parse_functions::
        SingleInitializeListArgumentConstexprValue,
    SingleConstexprValue)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleInitializeListArgument,
    c_parser_frontend::parse_functions::SingleInitializeListArgumentList,
    InitializeList)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    InitializeListArguments,
    c_parser_frontend::parse_functions::InitializeListArgumentsBase,
    SingleInitializeListArgument)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    InitializeListArguments,
    c_parser_frontend::parse_functions::InitializeListArgumentsExtend,
    InitializeListArguments, OperatorComma, SingleInitializeListArgument)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    AnnounceAssignable,
    c_parser_frontend::parse_functions::AnnounceAssignableAssignable,
    Assignable)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    AnnounceAssignable,
    c_parser_frontend::parse_functions::AnnounceAssignableInitializeList,
    InitializeList)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleAnnounceAndAssign,
    c_parser_frontend::parse_functions::SingleAnnounceAndAssignNoAssignBase,
    SingleAnnounceNoAssign)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleAnnounceAndAssign,
    c_parser_frontend::parse_functions::SingleAnnounceAndAssignWithAssignBase,
    SingleAnnounceNoAssign, OperatorAssign, AnnounceAssignable)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleAnnounceAndAssign,
    c_parser_frontend::parse_functions::SingleAnnounceAndAssignNoAssignExtend,
    SingleAnnounceAndAssign, OperatorComma, Id)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleAnnounceAndAssign,
    c_parser_frontend::parse_functions::SingleAnnounceAndAssignWithAssignExtend,
    SingleAnnounceAndAssign, OperatorComma, Id, OperatorAssign,
    AnnounceAssignable)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Type, c_parser_frontend::parse_functions::TypeBasicType, BasicType)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Type, c_parser_frontend::parse_functions::TypeFunctionRelavent,
    FunctionRelavent)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalOperator,
    c_parser_frontend::parse_functions::MathematicalOperatorPlus, OperatorPlus)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalOperator,
    c_parser_frontend::parse_functions::MathematicalOperatorMinus,
    OperatorMinusNegative)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalOperator,
    c_parser_frontend::parse_functions::MathematicalOperatorMultiple,
    OperatorMultiResolveRef)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalOperator,
    c_parser_frontend::parse_functions::MathematicalOperatorDivide, OperatorDiv)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalOperator,
    c_parser_frontend::parse_functions::MathematicalOperatorMod, OperatorMod)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalOperator,
    c_parser_frontend::parse_functions::MathematicalOperatorLeftShift,
    OperatorLeftShift)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalOperator,
    c_parser_frontend::parse_functions::MathematicalOperatorRightShift,
    OperatorRightShift)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalOperator,
    c_parser_frontend::parse_functions::MathematicalOperatorAnd,
    OperatorAndGetAddress)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalOperator,
    c_parser_frontend::parse_functions::MathematicalOperatorOr, OperatorOr)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalOperator,
    c_parser_frontend::parse_functions::MathematicalOperatorXor, OperatorXOR)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalOperator,
    c_parser_frontend::parse_functions::MathematicalOperatorNot, OperatorNot)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalAndAssignOperator,
    c_parser_frontend::parse_functions::MathematicalAndAssignOperatorPlusAssign,
    OperatorPlusAssign)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalAndAssignOperator,
    c_parser_frontend::parse_functions::
        MathematicalAndAssignOperatorMinusAssign,
    OperatorMinusAssign)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalAndAssignOperator,
    c_parser_frontend::parse_functions::
        MathematicalAndAssignOperatorMultipleAssign,
    OperatorMultiAssign)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalAndAssignOperator,
    c_parser_frontend::parse_functions::
        MathematicalAndAssignOperatorDivideAssign,
    OperatorDivAssign)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalAndAssignOperator,
    c_parser_frontend::parse_functions::MathematicalAndAssignOperatorModAssign,
    OperatorModAssign)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalAndAssignOperator,
    c_parser_frontend::parse_functions::
        MathematicalAndAssignOperatorLeftShiftAssign,
    OperatorLeftShiftAssign)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalAndAssignOperator,
    c_parser_frontend::parse_functions::
        MathematicalAndAssignOperatorRightShiftAssign,
    OperatorRightShiftAssign)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalAndAssignOperator,
    c_parser_frontend::parse_functions::MathematicalAndAssignOperatorAndAssign,
    OperatorAndAssign)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalAndAssignOperator,
    c_parser_frontend::parse_functions::MathematicalAndAssignOperatorOrAssign,
    OperatorOrAssign)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    MathematicalAndAssignOperator,
    c_parser_frontend::parse_functions::MathematicalAndAssignOperatorXorAssign,
    OperatorXORAssign)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    LogicalOperator, c_parser_frontend::parse_functions::LogicalOperatorAndAnd,
    OperatorLogicAnd)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    LogicalOperator, c_parser_frontend::parse_functions::LogicalOperatorOrOr,
    OperatorLogicOr)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    LogicalOperator, c_parser_frontend::parse_functions::LogicalOperatorGreater,
    OperatorGreater)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    LogicalOperator,
    c_parser_frontend::parse_functions::LogicalOperatorGreaterEqual,
    OperatorGreaterEqual)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    LogicalOperator, c_parser_frontend::parse_functions::LogicalOperatorLess,
    OperatorLess)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    LogicalOperator,
    c_parser_frontend::parse_functions::LogicalOperatorLessEqual,
    OperatorLessEqual)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    LogicalOperator, c_parser_frontend::parse_functions::LogicalOperatorEqual,
    OperatorEqual)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    LogicalOperator,
    c_parser_frontend::parse_functions::LogicalOperatorNotEqual,
    OperatorNotEqual)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableConstexprValue,
    SingleConstexprValue)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableId, Id)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableTemaryOperator,
    TemaryOperator)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableFunctionCall,
    FunctionCall)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableSizeOfType,
    OperatorSizeof, OperatorLeftBracket, Type, RightParenthesis)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableSizeOfAssignable,
    OperatorSizeof, OperatorLeftBracket, Assignable, RightParenthesis)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableMemberAccess,
    Assignable, OperatorMemberAccess, Id)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable,
    c_parser_frontend::parse_functions::AssignablePointerMemberAccess,
    Assignable, OperatorPointerMemberAccess, Id)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableBracket,
    OperatorLeftBracket, Assignable, RightParenthesis)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableTypeConvert,
    OperatorLeftBracket, Type, RightParenthesis, Assignable)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableAssign,
    Assignable, OperatorAssign, Assignable)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable,
    c_parser_frontend::parse_functions::AssignableMathematicalOperate,
    Assignable, MathematicalOperator, Assignable)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable,
    c_parser_frontend::parse_functions::AssignableMathematicalAndAssignOperate,
    Assignable, MathematicalAndAssignOperator, Assignable)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableLogicalOperate,
    Assignable, LogicalOperator, Assignable)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableNot, OperatorNot,
    Assignable)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableLogicalNegative,
    OperatorBitwiseNegation, Assignable)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable,
    c_parser_frontend::parse_functions::AssignableMathematicalNegative,
    OperatorMinusNegative, Assignable)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableObtainAddress,
    OperatorAndGetAddress, Assignable)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableDereference,
    OperatorMultiResolveRef, Assignable)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableArrayAccess,
    Assignable, OperatorLeftSquareBracket, Assignable, RightSquareBracket)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignablePrefixPlus,
    OperatorPlusOne, Assignable)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignablePrefixMinus,
    OperatorMinusOne, Assignable)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableSuffixPlus,
    Assignable, OperatorPlusOne)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignable, c_parser_frontend::parse_functions::AssignableSuffixMinus,
    Assignable, OperatorMinusOne)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Return, c_parser_frontend::parse_functions::ReturnWithValue, KeyWordReturn,
    Assignable, Semicolon)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Return, c_parser_frontend::parse_functions::ReturnWithoutValue,
    KeyWordReturn, Semicolon)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    TemaryOperator, c_parser_frontend::parse_functions::TemaryOperator,
    Assignable, OperatorQuestionMark, Assignable, Colon, Assignable)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    NotEmptyFunctionCallArguments,
    c_parser_frontend::parse_functions::NotEmptyFunctionCallArgumentsBase,
    Assignable)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    NotEmptyFunctionCallArguments,
    c_parser_frontend::parse_functions::NotEmptyFunctionCallArgumentsExtend,
    NotEmptyFunctionCallArguments, OperatorComma, Assignable)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FunctionCallArguments,
    c_parser_frontend::parse_functions::FunctionCallArguments,
    NotEmptyFunctionCallArguments)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FunctionCallInit,
    c_parser_frontend::parse_functions::FunctionCallInitAssignable, Assignable,
    OperatorLeftBracket)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FunctionCallInit, c_parser_frontend::parse_functions::FunctionCallInitId,
    Id, OperatorLeftBracket)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    FunctionCall, c_parser_frontend::parse_functions::FunctionCall,
    FunctionCallInit, FunctionCallArguments, RightParenthesis)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignables, c_parser_frontend::parse_functions::AssignablesBase,
    Assignable)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Assignables, c_parser_frontend::parse_functions::AssignablesExtend,
    Assignables, OperatorComma, Assignable)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Break, c_parser_frontend::parse_functions::Break, KeyWordBreak, Semicolon)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Continue, c_parser_frontend::parse_functions::Continue, KeyWordContinue,
    Semicolon)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStatement, c_parser_frontend::parse_functions::SingleStatementIf, If)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStatement, c_parser_frontend::parse_functions::SingleStatementDoWhile,
    DoWhile)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStatement, c_parser_frontend::parse_functions::SingleStatementWhile,
    While)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStatement, c_parser_frontend::parse_functions::SingleStatementFor,
    For)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStatement, c_parser_frontend::parse_functions::SingleStatementSwitch,
    Switch)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStatement,
    c_parser_frontend::parse_functions::SingleStatementAssignable, Assignable,
    Semicolon)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStatement,
    c_parser_frontend::parse_functions::SingleStatementAnnounce,
    SingleAnnounceAndAssign, Semicolon)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStatement, c_parser_frontend::parse_functions::SingleStatementReturn,
    Return)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStatement, c_parser_frontend::parse_functions::SingleStatementBreak,
    Break)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStatement,
    c_parser_frontend::parse_functions::SingleStatementContinue, Continue)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleStatement,
    c_parser_frontend::parse_functions::SingleStatementEmptyStatement,
    Semicolon)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    IfCondition, c_parser_frontend::parse_functions::IfCondition, KeyWordIf,
    OperatorLeftBracket, Assignable, RightParenthesis)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    IfWithElse, c_parser_frontend::parse_functions::IfWithElse, IfCondition,
    ProcessControlSentenceBody, KeyWordElse)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    If, c_parser_frontend::parse_functions::IfElseSence, IfWithElse,
    ProcessControlSentenceBody)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    If, c_parser_frontend::parse_functions::IfIfSentence, IfCondition,
    ProcessControlSentenceBody)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    ForRenewSentence, c_parser_frontend::parse_functions::ForRenewSentence,
    Assignables)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    ForInitSentence,
    c_parser_frontend::parse_functions::ForInitSentenceAssignables, Assignables)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    ForInitSentence,
    c_parser_frontend::parse_functions::ForInitSentenceAnnounce,
    SingleAnnounceAndAssign)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    ForInitHead, c_parser_frontend::parse_functions::ForInitHead, KeyWordFor)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    ForHead, c_parser_frontend::parse_functions::ForHead, ForInitHead,
    OperatorLeftBracket, ForInitSentence, Semicolon, Assignable, Semicolon,
    ForRenewSentence, RightParenthesis)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(For,
                                        c_parser_frontend::parse_functions::For,
                                        ForHead, ProcessControlSentenceBody)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    WhileInitHead, c_parser_frontend::parse_functions::WhileInitHead,
    KeyWordWhile, OperatorLeftBracket, Assignable, RightParenthesis)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    While, c_parser_frontend::parse_functions::While, WhileInitHead,
    ProcessControlSentenceBody)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    DoWhileInitHead, c_parser_frontend::parse_functions::DoWhileInitHead,
    KeyWordDo)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    DoWhile, c_parser_frontend::parse_functions::DoWhile, DoWhileInitHead,
    ProcessControlSentenceBody, KeyWordWhile, OperatorLeftBracket, Assignable,
    RightParenthesis, Semicolon)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SwitchCase, c_parser_frontend::parse_functions::SwitchCaseSimple,
    KeyWordCase, SingleConstexprValue, Colon)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SwitchCase, c_parser_frontend::parse_functions::SwitchCaseDefault,
    KeyWordDefault, Colon)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleSwitchStatement,
    c_parser_frontend::parse_functions::SingleSwitchStatementCase, SwitchCase)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SingleSiwtchStatement,
    c_parser_frontend::parse_functions::SingleSwitchStatementStatements,
    Statements)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SwitchStatements, c_parser_frontend::parse_functions::SwitchStatements,
    SwitchStatements, SingleSwitchStatement)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    SwitchCondition, c_parser_frontend::parse_functions::SwitchCondition,
    KeyWordSwitch, OperatorLeftBracket, Assignable, RightParenthesis)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Switch, c_parser_frontend::parse_functions::Switch, SwitchCondition,
    LeftCurlyBracket, SwitchStatements, RightCurlyBracket)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    StatementsLeftBrace,
    c_parser_frontend::parse_functions::StatementsLeftBrace, Statements,
    LeftCurlyBracket)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Statements, c_parser_frontend::parse_functions::StatementsSingleStatement,
    Statements, SingleStatement)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Statements, c_parser_frontend::parse_functions::StatementsBrace,
    StatementsLeftBrace, Statements, RightCurlyBracket)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    ProcessControlSentenceBody,
    c_parser_frontend::parse_functions::
        ProcessControlSentenceBodySingleStatement,
    SingleStatement)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    ProcessControlSentenceBody,
    c_parser_frontend::parse_functions::ProcessControlSentenceBodyStatements,
    LeftCurlyBracket, Statements, RightCurlyBracket)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Root, c_parser_frontend::parse_functions::RootFunctionDefine, Root,
    FunctionDefine)
GENERATOR_DEFINE_NONTERMINAL_PRODUCTION(
    Root, c_parser_frontend::parse_functions::RootAnnounce, Root,
    SingleAnnounceNoAssign, Semicolon)

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
GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT(ForRenewSentence)
GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT(ForInitSentence)
GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT(SwitchStatements)
GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT(Statements)
GENERATOR_SET_NONTERMINAL_PRODUCTION_COULD_EMPTY_REDUCT(Root)

/// 设置产生式根节点
/// 仅允许设置一个
GENERATOR_DEFINE_ROOT_PRODUCTION(Root)