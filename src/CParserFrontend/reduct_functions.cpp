#include "reduct_functions.h"

#include <optional>
#include <tuple>
namespace c_parser_frontend::parse_functions {

std::pair<EnumType::EnumContainerType::iterator, bool>
EnumReturnData::AddMember(std::string&& member_name, long long value) {
  auto return_result = enum_container_.emplace(std::move(member_name), value);
  if (return_result.second == true) {
    // 成功插入，更新相关数据
    SetLastValue(value);
    if (GetMaxValue() < value) {
      SetMaxValue(value);
    } else if (GetMinValue() > value) {
      SetMinValue(value);
    }
  }
  return return_result;
}

void VarietyOrFunctionConstructError(
    ObjectConstructData::CheckResult check_result,
    const std::string& object_name) {
  switch (check_result) {
    [[likely]] case ObjectConstructData::CheckResult::kSuccess:
      break;
    case ObjectConstructData::CheckResult::kAttachToTerminalType:
      OutputError(std::format(
          "待构建对象{:}已存在终结类型，不可声明更多类型结构", object_name));
      exit(-1);
      break;
    case ObjectConstructData::CheckResult::kReturnFunction:
      OutputError(std::format("函数{:}返回值不能为函数", object_name));
      exit(-1);
      break;
    case ObjectConstructData::CheckResult::kPointerEnd:
      OutputError(std::format("待构建对象{:}缺少具体类型", object_name));
      exit(-1);
      break;
    case ObjectConstructData::CheckResult::kEmptyChain:
      OutputError(std::format("没有为待构建对象{:}提供任何类型", object_name));
      exit(-1);
      break;
    default:
      assert(false);
      break;
  }
}

// 根据声明时的初始类型获取一次声明中非第一个变量的类型
std::shared_ptr<const TypeInterface> GetExtendAnnounceType(
    const std::shared_ptr<const TypeInterface>& source_type) {
  if (source_type->GetType() == StructOrBasicType::kPointer) [[unlikely]] {
    // 第一个声明的变量使用完全类型，其余的变量使用去掉一重指针的类型
    return source_type->GetNextNodePointer();
  } else {
    // 第一个声明的变量不是指针，其余变量类型与第一个相同
    return source_type;
  }
}

void CheckAssignableCheckResult(AssignableCheckResult assignable_check_result) {
  switch (assignable_check_result) {
    case AssignableCheckResult::kNonConvert:
    case AssignableCheckResult::kUpperConvert:
    case AssignableCheckResult::kConvertToVoidPointer:
    case AssignableCheckResult::kZeroConvertToPointer:
      // 正常可以赋值的情况
      break;
    case AssignableCheckResult::kUnsignedToSigned:
      // 使用警告
      OutputWarning(std::format("将无符号类型赋值给有符号类型"));
      break;
    case AssignableCheckResult::kSignedToUnsigned:
      // 使用警告
      OutputWarning(std::format("将有符号类型赋值给无符号类型"));
      break;
    case AssignableCheckResult::kLowerConvert:
      OutputError(std::format("在赋值时发生缩窄转换"));
      exit(-1);
      break;
    case AssignableCheckResult::kCanNotConvert:
      OutputError(std::format("无法转换类型"));
      exit(-1);
      break;
    case AssignableCheckResult::kAssignedNodeIsConst:
      OutputError(std::format("非声明时无法给const对象赋值"));
      exit(-1);
      break;
    case AssignableCheckResult::kAssignToRightValue:
      OutputError(std::format("无法给右值对象赋值"));
      exit(-1);
      break;
    case AssignableCheckResult::kArgumentsFull:
      // TODO 支持可变参数函数后修改该报错进行更具体的判断
      OutputError(std::format("实参数量超出形参数量"));
      exit(-1);
      break;
    case AssignableCheckResult::kMayBeZeroToPointer:
      // 该类型在返回前应被具体确认并替换成更精确的结果
    default:
      assert(false);
      break;
  }
}

void CheckMathematicalComputeTypeResult(
    DeclineMathematicalComputeTypeResult
        decline_mathematical_compute_type_result) {
  switch (decline_mathematical_compute_type_result) {
    case DeclineMathematicalComputeTypeResult::kComputable:
    case DeclineMathematicalComputeTypeResult::kConvertToLeft:
    case DeclineMathematicalComputeTypeResult::kConvertToRight:
    case DeclineMathematicalComputeTypeResult::kLeftOffsetRightPointer:
    case DeclineMathematicalComputeTypeResult::kLeftPointerRightOffset:
      // 可以运算
      break;
    case DeclineMathematicalComputeTypeResult::kLeftNotComputableType:
      OutputError(std::format("左运算数不是可运算类型"));
      exit(-1);
      break;
    case DeclineMathematicalComputeTypeResult::kLeftNotIntger:
      OutputError(std::format("左运算数作为指针偏移量时不是整型"));
      exit(-1);
      break;
    case DeclineMathematicalComputeTypeResult::kLeftRightBothPointer:
      OutputError(std::format("两个指针类型无法进行数学运算"));
      exit(-1);
      break;
    case DeclineMathematicalComputeTypeResult::kRightNotComputableType:
      OutputError(std::format("右运算数不是可运算类型"));
      exit(-1);
      break;
    case DeclineMathematicalComputeTypeResult::kRightNotIntger:
      OutputError(std::format("右运算数作为指针偏移量时不是整型"));
      exit(-1);
      break;
    default:
      assert(false);
      break;
  }
}

void CheckAddTypeResult(AddTypeResult add_type_result) {
  switch (add_type_result) {
    case c_parser_frontend::type_system::AddTypeResult::kAbleToAdd:
    case c_parser_frontend::type_system::AddTypeResult::kNew:
    case c_parser_frontend::type_system::AddTypeResult::kFunctionDefine:
    case c_parser_frontend::type_system::AddTypeResult::kShiftToVector:
    case c_parser_frontend::type_system::AddTypeResult::kAddToVector:
      break;
    case c_parser_frontend::type_system::AddTypeResult::
        kAnnounceOrDefineBeforeFunctionAnnounce:
      OutputWarning(std::format("重复声明函数"));
      break;
    case c_parser_frontend::type_system::AddTypeResult::kTypeAlreadyIn:
      OutputError(std::format("已存在同名类型"));
      exit(-1);
      break;
    case c_parser_frontend::type_system::AddTypeResult::kDoubleAnnounceFunction:
      OutputError(std::format("重定义函数"));
      exit(-1);
      break;
    default:
      assert(false);
      break;
  }
}

void OutputError(const std::string& error) {
  std::cerr << std::format("Compline Error: 行数:{:} 列数:{:} ", GetLine() + 1,
                           GetColumn() + 1)
            << error << std::endl;
}

void OutputWarning(const std::string& warning) {
  std::cerr << std::format("Compline Warning: 行数:{:} 列数:{:} ",
                           GetLine() + 1, GetColumn() + 1)
            << warning << std::endl;
}

void OutputInfo(const std::string& info) {
  std::cout << std::format("Compline Info: 行数:{:} 列数:{:} ", GetLine() + 1,
                           GetColumn() + 1)
            << info << std::endl;
}

std::shared_ptr<BasicTypeInitializeOperatorNode> SingleConstexprValueChar(
    std::string&& word_data) {
  // ''中夹一个字符或夹一个带转义字符的字符
  // 提取单个字符作为值
  std::string character;
  if (word_data.size() == 3) [[likely]] {
    // 不带转义字符
    character = std::to_string(word_data[1]);
  } else {
    // 带转义字符
    character = std::to_string(word_data[2]);
  }
  auto initialize_node = std::make_shared<BasicTypeInitializeOperatorNode>(
      InitializeType::kBasic, std::move(character));
  initialize_node->SetInitDataType(
      CommonlyUsedTypeGenerator::GetBasicType<BuiltInType::kInt8,
                                              SignTag::kSigned>());
  return initialize_node;
}
std::shared_ptr<BasicTypeInitializeOperatorNode>
SingleConstexprValueIndexedString(std::string&& str,
                                  std::string&& left_square_bracket,
                                  std::string&& num,
                                  std::string&& right_square_bracket) {
  assert(left_square_bracket == "[");
  assert(right_square_bracket == "]");

  size_t converted_num;
  for (auto iter = str.begin(); iter < str.end(); iter++) {
    // 删除转义字符
    if (*iter == '\\') [[unlikely]] {
      iter = str.erase(iter);
      if (*iter == '\\') [[unlikely]] {
        ++iter;
      }
    }
  }
  size_t index = std::stoull(num, &converted_num);
  // 检查使用的下标是否全部转换
  if (converted_num != num.size()) [[unlikely]] {
    OutputError(std::format("无法转换下标，可能为浮点数或超出可用下标范围"));
    exit(-1);
  }
  // 检查下标是否越界
  char result;
  if (index > str.size()) [[unlikely]] {
    OutputError(std::format("使用的下标大于字符串大小"));
    exit(-1);
  } else if (index == str.size()) [[unlikely]] {
    result = '\0';
  } else {
    // +1以跳过开头的"
    result = str[index + 1];
  }
  auto initialize_node = std::make_shared<BasicTypeInitializeOperatorNode>(
      InitializeType::kBasic, std::to_string(result));
  initialize_node->SetInitDataType(
      CommonlyUsedTypeGenerator::GetBasicType<BuiltInType::kInt8,
                                              SignTag::kSigned>());
  return initialize_node;
}
std::shared_ptr<BasicTypeInitializeOperatorNode> SingleConstexprValueNum(
    std::string&& num) {
  auto initialize_node = std::make_shared<BasicTypeInitializeOperatorNode>(
      InitializeType::kBasic, std::move(num));
  initialize_node->SetInitDataType(
      type_system::CommonlyUsedTypeGenerator::GetBasicTypeNotTemplate(
          type_system::CalculateBuiltInType(initialize_node->GetValue()),
          SignTag::kSigned));
  return initialize_node;
}
std::shared_ptr<BasicTypeInitializeOperatorNode> SingleConstexprValueString(
    std::string&& str) {
  // 字符串使用""作为分隔标记
  assert(str.front() == '"');
  assert(str.back() == '"');
  // 删除转义字符
  for (auto iter = str.begin(); iter < str.end(); iter++) {
    if (*iter == '\\') [[unlikely]] {
      iter = str.erase(iter);
      if (*iter == '\\') [[unlikely]] {
        ++iter;
      }
    }
  }
  // 删除尾部的"
  str.pop_back();
  // 删除头部的"
  str.erase(str.begin());
  auto initialize_node = std::make_shared<BasicTypeInitializeOperatorNode>(
      InitializeType::kString, std::move(str));
  initialize_node->SetInitDataType(
      CommonlyUsedTypeGenerator::GetConstExprStringType());
  return initialize_node;
}

BuiltInType FundamentalTypeChar(std::string&& str) {
  return BuiltInType::kInt8;
}

BuiltInType FundamentalTypeShort(std::string&& str) {
  assert(str == "short");
  return BuiltInType::kInt16;
}

BuiltInType FundamentalTypeInt(std::string&& str) {
  assert(str == "int");
  return BuiltInType::kInt32;
}

BuiltInType FundamentalTypeLong(std::string&& str) {
  assert(str == "long");
  return BuiltInType::kInt32;
}

BuiltInType FundamentalTypeFloat(std::string&& str) {
  assert(str == "float");
  return BuiltInType::kFloat32;
}

BuiltInType FundamentalTypeDouble(std::string&& str) {
  assert(str == "double");
  return BuiltInType::kFloat64;
}

BuiltInType FundamentalTypeVoid(std::string&& str) {
  assert(str == "void");
  return BuiltInType::kVoid;
}

SignTag SignTagSigned(std::string&& str) {
  assert(str == "signed");
  return SignTag::kSigned;
}

SignTag SignTagUnSigned(std::string&& str) {
  assert(str == "unsigned");
  return SignTag::kUnsigned;
}

ConstTag ConstTagConst(std::string&& str) {
  assert(str == "const");
  return ConstTag::kConst;
}
std::shared_ptr<ObjectConstructData> IdOrEquivenceConstTagId(ConstTag const_tag,
                                                             std::string&& id) {
  std::shared_ptr<ObjectConstructData> construct_data =
      std::make_shared<ObjectConstructData>(std::move(id));
  construct_data->ConstructBasicObjectPart<VarietyOperatorNode>(
      std::string(), const_tag, LeftRightValueTag::kLeftValue);
  return construct_data;
}

std::shared_ptr<ObjectConstructData>&& IdOrEquivenceNumAddressing(
    std::shared_ptr<ObjectConstructData>&& sub_reduct_result,
    std::string&& left_square_bracket, std::string&& num,
    std::string&& right_square_bracket) {
  assert(left_square_bracket == "[");
  assert(right_square_bracket == "]");

  // 转换为long long的字符数
  size_t chars_converted_to_longlong;
  size_t array_size = std::stoull(num, &chars_converted_to_longlong);
  // 检查是否所有的数字均参与转换
  if (chars_converted_to_longlong != num.size()) [[unlikely]] {
    // 不是所有的数字都参与了转换，说明使用了浮点数
    OutputError(std::format(
        "{:}无法作为数组大小或偏移量，原因可能为浮点数、数值过大、负数", num));
    exit(-1);
  }
  // 添加一层指针和指针指向的数组大小
  auto result =
      sub_reduct_result->AttachSingleNodeToTailNodeEmplace<PointerType>(
          ConstTag::kNonConst, array_size);
  // 检查添加结果
  if (result != ObjectConstructData::CheckResult::kSuccess) [[unlikely]] {
    VarietyOrFunctionConstructError(result, sub_reduct_result->GetObjectName());
  }
  return std::move(sub_reduct_result);
}
std::shared_ptr<ObjectConstructData>&& IdOrEquivenceAnonymousAddressing(
    std::shared_ptr<ObjectConstructData>&& sub_reduct_result,
    std::string&& left_square_bracket, std::string&& right_square_bracket) {
  assert(left_square_bracket == "[");
  assert(right_square_bracket == "]");

  // 添加一层指针和指针指向的数组大小
  // -1代表待推断大小
  auto result =
      sub_reduct_result->AttachSingleNodeToTailNodeEmplace<PointerType>(
          ConstTag::kNonConst, -1);
  // 检查是否成功添加
  if (result != ObjectConstructData::CheckResult::kSuccess) [[unlikely]] {
    VarietyOrFunctionConstructError(result, sub_reduct_result->GetObjectName());
  }
  return std::move(sub_reduct_result);
}
std::shared_ptr<ObjectConstructData>&& IdOrEquivencePointerAnnounce(
    ConstTag const_tag, std::string&& operator_pointer,
    std::shared_ptr<ObjectConstructData>&& sub_reduct_result) {
  assert(operator_pointer == "*");

  // 添加一层指针和指针指向的数组大小
  // 0代表纯指针
  auto result =
      sub_reduct_result->AttachSingleNodeToTailNodeEmplace<PointerType>(
          const_tag, 0);
  // 检查添加结果
  if (result != ObjectConstructData::CheckResult::kSuccess) [[unlikely]] {
    VarietyOrFunctionConstructError(result, sub_reduct_result->GetObjectName());
  }
  return std::move(sub_reduct_result);
}
std::shared_ptr<ObjectConstructData>&& IdOrEquivenceInBrackets(
    std::string&& left_bracket,
    std::shared_ptr<ObjectConstructData>&& sub_reduct_result,
    std::string&& right_bracket) {
  assert(left_bracket == "(");
  assert(right_bracket == ")");

  return std::move(sub_reduct_result);
}

std::shared_ptr<ObjectConstructData> AnonymousIdOrEquivenceConst(
    std::string&& str_const) {
  assert(str_const == "const");

  std::shared_ptr<ObjectConstructData> construct_data =
      std::make_shared<ObjectConstructData>(std::string());
  construct_data->ConstructBasicObjectPart<VarietyOperatorNode>(
      std::string(), ConstTag::kConst, LeftRightValueTag::kLeftValue);
  return construct_data;
}

std::shared_ptr<ObjectConstructData>&& AnonymousIdOrEquivenceNumAddressing(
    std::shared_ptr<ObjectConstructData>&& sub_reduct_result,
    std::string&& left_square_bracket, std::string&& num,
    std::string&& right_square_bracket) {
  return IdOrEquivenceNumAddressing(
      std::move(sub_reduct_result), std::move(left_square_bracket),
      std::move(num), std::move(right_square_bracket));
}

std::shared_ptr<ObjectConstructData>&&
AnonymousIdOrEquivenceAnonymousAddressing(
    std::shared_ptr<ObjectConstructData>&& sub_reduct_result,
    std::string&& left_square_bracket, std::string&& right_square_bracket) {
  return IdOrEquivenceAnonymousAddressing(std::move(sub_reduct_result),
                                          std::move(left_square_bracket),
                                          std::move(right_square_bracket));
}

std::shared_ptr<ObjectConstructData>&& AnonymousIdOrEquivencePointerAnnounce(
    ConstTag const_tag, std::string&& operator_pointer,
    std::shared_ptr<ObjectConstructData>&& sub_reduct_result) {
  return IdOrEquivencePointerAnnounce(std::move(const_tag),
                                      std::move(operator_pointer),
                                      std::move(sub_reduct_result));
}

std::shared_ptr<ObjectConstructData>&& AnonymousIdOrEquivenceInBrackets(
    std::string&& left_bracket,
    std::shared_ptr<ObjectConstructData>&& sub_reduct_result,
    std::string&& right_bracket) {
  return IdOrEquivenceInBrackets(std::move(left_bracket),
                                 std::move(sub_reduct_result),
                                 std::move(right_bracket));
}

std::shared_ptr<EnumReturnData> NotEmptyEnumArgumentsIdBase(std::string&& id) {
  auto enum_return_data = std::make_shared<EnumReturnData>();
  // 第一个枚举值默认从0开始
  auto [iter, inserted] = enum_return_data->AddMember(std::move(id), 0);
  assert(inserted);
  return enum_return_data;
}
std::shared_ptr<EnumReturnData> NotEmptyEnumArgumentsIdAssignNumBase(
    std::string&& id, std::string&& operator_assign, std::string&& num) {
  assert(operator_assign == "=");

  auto enum_return_data = std::make_shared<EnumReturnData>();
  size_t chars_converted_to_longlong;
  // 枚举项的值
  long long enum_member_value = std::stoll(num, &chars_converted_to_longlong);
  if (chars_converted_to_longlong != num.size()) [[unlikely]] {
    // 存在浮点数，不是所有的值都可以作为枚举项的值
    OutputError(std::format("{:}不能作为枚举项的值，原因可能为浮点数", num));
    exit(-1);
  }
  // 添加给定值的枚举值
  auto [iter, inserted] =
      enum_return_data->AddMember(std::move(id), enum_member_value);
  if (!inserted) [[unlikely]] {
    // 插入失败，存在同名成员
    OutputError(std::format("枚举项：{:}已定义", id));
    exit(-1);
  }
  return enum_return_data;
}
std::shared_ptr<EnumReturnData>&& NotEmptyEnumArgumentsIdExtend(
    std::shared_ptr<EnumReturnData>&& enum_data, std::string&& str_comma,
    std::string&& id) {
  assert(str_comma == ",");

  // 未指定项的值则顺延上一个定义的项的值
  long long enum_member_value = enum_data->GetLastValue() + 1;
  auto [iter, inserted] =
      enum_data->AddMember(std::move(id), enum_member_value);
  if (!inserted) [[unlikely]] {
    // 待添加的项的ID已经存在
    OutputError(std::format("枚举项：{:}已定义", id));
    exit(-1);
  }
  return std::move(enum_data);
}
std::shared_ptr<EnumReturnData>&& NotEmptyEnumArgumentsIdAssignNumExtend(
    std::shared_ptr<EnumReturnData>&& enum_data, std::string&& str_comma,
    std::string&& id, std::string&& operator_assign, std::string&& num) {
  assert(str_comma == ",");
  assert(operator_assign == "=");

  size_t chars_converted_to_longlong;
  // 未指定项的值则顺延上一个定义的项的值
  long long enum_member_value = std::stoll(num, &chars_converted_to_longlong);
  if (chars_converted_to_longlong != num.size()) [[unlikely]] {
    // 给定项的值不能完全转化为long long
    OutputError(
        std::format("枚举项{0:} = {1:}中 "
                    "{1:}不能作为枚举项的值，可能原因为浮点数",
                    id, num));
    exit(-1);
  }
  auto [iter, inserted] =
      enum_data->AddMember(std::move(id), enum_member_value);
  if (!inserted) [[unlikely]] {
    // 待添加的项的ID已经存在
    OutputError(std::format("枚举项{0:} = {1:}中 枚举项：{0:}已定义", id, num));
    exit(-1);
  }
  return std::move(enum_data);
}

std::shared_ptr<EnumReturnData>&& EnumArgumentsNotEmptyEnumArguments(
    std::shared_ptr<EnumReturnData>&& enum_data) {
  return std::move(enum_data);
}
std::shared_ptr<EnumType> EnumDefine(
    std::string&& str_enum, std::string&& id, std::string&& left_curly_bracket,
    std::shared_ptr<EnumReturnData>&& enum_data,
    std::string&& right_curly_bracket) {
  assert(str_enum == "enum");
  assert(left_curly_bracket == "{");
  assert(right_curly_bracket == "}");

  std::shared_ptr<EnumType> enum_type =
      std::make_shared<EnumType>(id, std::move(enum_data->GetContainer()));
  // 添加该枚举的定义
  auto [insert_iter, result] =
      c_parser_controller.DefineType(std::move(id), enum_type);
  CheckAddTypeResult(result);
  return enum_type;
}
std::shared_ptr<EnumType> EnumAnonymousDefine(
    std::string&& str_enum, std::string&& left_curly_bracket,
    std::shared_ptr<EnumReturnData>&& enum_data,
    std::string&& right_curly_bracket) {
  assert(str_enum == "enum");
  assert(left_curly_bracket == "{");
  assert(right_curly_bracket == "}");

  std::shared_ptr<EnumType> enum_type = std::make_shared<EnumType>(
      std::string(), std::move(enum_data->GetContainer()));
  ;
  // 匿名枚举无需定义
  return enum_type;
}

std::pair<std::string, StructOrBasicType> EnumAnnounce(std::string&& str_enum,
                                                       std::string&& id) {
  assert(str_enum == "enum");

  return std::make_pair(std::move(id), StructOrBasicType::kEnum);
}

std::pair<std::string, StructOrBasicType> StructureAnnounceStructId(
    std::string&& str_struct, std::string&& id) {
  assert(str_struct == "struct");

  return std::make_pair(std::move(id), StructOrBasicType::kStruct);
}

std::pair<std::string, StructOrBasicType> StructureAnnounceUnionId(
    std::string&& str_union, std::string&& id) {
  assert(str_union == "union");

  return std::make_pair(std::move(id), StructOrBasicType::kUnion);
}

std::pair<std::string, StructOrBasicType> StructureDefineHeadStruct(
    std::string&& str_struct) {
  assert(str_struct == "struct");

  return std::make_pair(std::string(), StructOrBasicType::kStruct);
}

std::pair<std::string, StructOrBasicType> StructureDefineHeadUnion(
    std::string&& str_union) {
  assert(str_union == "union");

  return std::make_pair(std::string(), StructOrBasicType::kUnion);
}

std::pair<std::string, StructOrBasicType>&&
StructureDefineHeadStructureAnnounce(
    std::pair<std::string, StructOrBasicType>&& struct_data) {
  return std::move(struct_data);
}

std::shared_ptr<StructureTypeInterface> StructureDefineInitHead(
    std::pair<std::string, StructOrBasicType>&& struct_data,
    std::string&& left_purly_bracket) {
  auto& [struct_name, struct_type] = struct_data;
  switch (struct_type) {
    case StructOrBasicType::kStruct:
      structure_type_constructuring = std::make_shared<StructType>(struct_name);
      break;
    case StructOrBasicType::kUnion:
      structure_type_constructuring = std::make_shared<UnionType>(struct_name);
      break;
    default:
      assert(false);
      break;
  }
  // 如果是非匿名结构则注册结构体/共用体类型
  // 不使用struct_name因为已经被移动构造
  if (!struct_name.empty()) [[likely]] {
    auto [iter, result] = c_parser_controller.DefineType(
        std::move(struct_name), structure_type_constructuring);
    CheckAddTypeResult(result);
  }
  return structure_type_constructuring;
}
std::shared_ptr<StructureTypeInterface>&& StructureDefine(
    std::shared_ptr<StructureTypeInterface>&& struct_data, std::nullptr_t,
    std::string&& right_curly_bracket) {
  assert(right_curly_bracket == "}");

  return std::move(struct_data);
}

std::shared_ptr<const StructureTypeInterface> StructTypeStructDefine(
    std::shared_ptr<StructureTypeInterface>&& struct_data) {
  return std::move(struct_data);
}
std::shared_ptr<const StructureTypeInterface> StructTypeStructAnnounce(
    std::pair<std::string, StructOrBasicType>&& struct_data) {
  std::string& struct_name = struct_data.first;
  StructOrBasicType struct_type = struct_data.second;
  auto [type_pointer, get_type_result] =
      c_parser_controller.GetType(struct_name, struct_type);
  switch (get_type_result) {
    case GetTypeResult::kSuccess:
      // 成功获取
      break;
    case GetTypeResult::kTypeNameNotFound:
    case GetTypeResult::kNoMatchTypePrefer: {
      // 找不到给定名称的类型
      const char* type_name =
          struct_type == StructOrBasicType::kStruct ? "结构体" : "共用体";
      OutputError(std::format("{:}{:}未定义", type_name, struct_name));
      exit(-1);
    } break;
    case GetTypeResult::kSeveralSameLevelMatches:
      // 该情况仅在无类型选择偏好时有效
    default:
      assert(false);
      break;
  }
  return std::static_pointer_cast<const StructureTypeInterface>(type_pointer);
}
std::pair<std::shared_ptr<const TypeInterface>, ConstTag> BasicTypeFundamental(
    ConstTag const_tag, SignTag sign_tag, BuiltInType builtin_type) {
  switch (builtin_type) {
      // 这两种类型只能使用无符号
    case c_parser_frontend::type_system::BuiltInType::kVoid:
    case c_parser_frontend::type_system::BuiltInType::kInt1:
      assert(sign_tag == SignTag::kSigned);
      break;
    default:
      break;
  }
  return std::make_pair(CommonlyUsedTypeGenerator::GetBasicTypeNotTemplate(
                            builtin_type, sign_tag),
                        const_tag);
}

std::pair<std::shared_ptr<const TypeInterface>, ConstTag> BasicTypeStructType(
    ConstTag const_tag,
    std::shared_ptr<const StructureTypeInterface>&& struct_data) {
  return std::make_pair(std::move(struct_data), const_tag);
}

//// BasicType->ConstTag Id
//// 返回获取到的类型与ConstTag
// std::pair<std::shared_ptr<const TypeInterface>, ConstTag> BasicTypeId(
//    std::vector<WordDataToUser>&& word_data) {
//  assert(word_data.size() == 2);
//  ConstTag const_tag = GetConstTag(word_data[0]);
//  std::string& type_name = word_data[1].GetTerminalWordData().word;
//  assert(!type_name.empty());
//  // 获取ID对应的类型
//  auto [type_pointer, result] =
//      c_parser_frontend.GetType(type_name, StructOrBasicType::kNotSpecified);
//  switch (result) {
//    case GetTypeResult::kSuccess:
//      // 成功获取变量类型
//      break;
//    case GetTypeResult::kSeveralSameLevelMatches:
//      OutputError(std::format("类型名{:}对应多个同级类型", type_name));
//      exit(-1);
//      break;
//    case GetTypeResult::kTypeNameNotFound:
//      OutputError(std::format("类型{:}不存在", type_name));
//      exit(-1);
//      break;
//    case GetTypeResult::kNoMatchTypePrefer:
//      // 无类型选择倾向时不能返回该结果
//    default:
//      assert(false);
//      break;
//  }
//  return std::make_pair(std::move(type_pointer), const_tag);
//}

std::pair<std::shared_ptr<const TypeInterface>, ConstTag> BasicTypeEnumAnnounce(
    ConstTag const_tag, std::pair<std::string, StructOrBasicType>&& enum_data) {
  std::string& enum_name = enum_data.first;
  auto [type_pointer, result] =
      c_parser_controller.GetType(enum_name, StructOrBasicType::kNotSpecified);
  switch (result) {
    case GetTypeResult::kSuccess:
      // 成功获取变量类型
      break;
    case GetTypeResult::kSeveralSameLevelMatches:
      OutputError(std::format("类型名{:}对应多个同级类型", enum_name));
      exit(-1);
      break;
    case GetTypeResult::kTypeNameNotFound:
      OutputError(std::format("类型{:}不存在", enum_name));
      exit(-1);
      break;
    case GetTypeResult::kNoMatchTypePrefer:
      // 无类型选择倾向时不能返回该结果
    default:
      assert(false);
      break;
  }
  return std::make_pair(std::move(type_pointer), const_tag);
}

std::shared_ptr<ObjectConstructData>&& FunctionRelaventBasePartFunctionInitBase(
    std::shared_ptr<ObjectConstructData>&& construct_data,
    std::string&& left_bracket) {
  assert(left_bracket == "(");

  // 设置构建数据，在构建函数参数时使用
  function_type_construct_data = construct_data;
  // 创建匿名函数类型并设置函数参数
  auto result =
      function_type_construct_data
          ->AttachSingleNodeToTailNodeEmplace<FunctionType>(std::string());
  // 检查添加结果
  if (result != ObjectConstructData::CheckResult::kSuccess) [[unlikely]] {
    VarietyOrFunctionConstructError(
        result, function_type_construct_data->GetObjectName());
  }
  return std::move(construct_data);
}

std::shared_ptr<ObjectConstructData>&&
FunctionRelaventBasePartFunctionInitExtend(
    std::shared_ptr<ObjectConstructData>&& construct_data,
    std::string&& left_bracket) {
  return FunctionRelaventBasePartFunctionInitBase(std::move(construct_data),
                                                  std::move(left_bracket));
}

std::shared_ptr<ObjectConstructData>&& FunctionRelaventBasePartFunction(
    std::shared_ptr<ObjectConstructData>&& construct_data, std::nullptr_t&&,
    std::string&& right_bracket) {
  assert(right_bracket == ")");

  return std::move(construct_data);
}

std::shared_ptr<ObjectConstructData>&& FunctionRelaventBasePartPointer(
    ConstTag const_tag, std::string&& str_pointer,
    std::shared_ptr<ObjectConstructData>&& construct_data) {
  assert(str_pointer == "*");

  // 添加一个指针节点
  auto result = construct_data->AttachSingleNodeToTailNodeEmplace<PointerType>(
      const_tag, 0);
  // 检查添加结果
  if (result != ObjectConstructData::CheckResult::kSuccess) [[unlikely]] {
    VarietyOrFunctionConstructError(result, construct_data->GetObjectName());
  }
  return std::move(construct_data);
}

std::shared_ptr<ObjectConstructData>&& FunctionRelaventBasePartBranckets(
    std::string&& left_bracket,
    std::shared_ptr<ObjectConstructData>&& construct_data,
    std::string&& right_bracket) {
  assert(left_bracket == "(");
  assert(right_bracket == ")");

  return std::move(construct_data);
}

std::shared_ptr<FlowInterface> FunctionRelavent(
    std::pair<std::shared_ptr<const TypeInterface>, ConstTag>&& basic_type_data,
    std::shared_ptr<ObjectConstructData>&& construct_data) {
  auto& [final_type, const_tag_before_final_type] = basic_type_data;
  auto [flow_control_node, construct_result] = construct_data->ConstructObject(
      const_tag_before_final_type, std::move(final_type));
  // 检查节点构建是否成功
  if (construct_result != ObjectConstructData::CheckResult::kSuccess)
      [[unlikely]] {
    VarietyOrFunctionConstructError(construct_result,
                                    construct_data->GetObjectName());
  }
  return std::move(flow_control_node);
}

std::shared_ptr<FlowInterface> SingleAnnounceNoAssignNotPodVariety(
    ConstTag const_tag, std::string&& id,
    std::shared_ptr<ObjectConstructData>&& construct_data) {
  if (construct_data->GetObjectName().empty()) [[unlikely]] {
    OutputError(std::format("声明的变量必须有名"));
    exit(-1);
  }
  auto [final_type, result] =
      c_parser_controller.GetType(id, StructOrBasicType::kNotSpecified);
  switch (result) {
    case GetTypeResult::kSeveralSameLevelMatches:
      OutputError(
          std::format("类型名 {:} 对应多种类型，无法确定要使用的类型", id));
      exit(-1);
      break;
    case GetTypeResult::kTypeNameNotFound:
      OutputError(std::format("不存在类型 {:}", id));
      exit(-1);
      break;
    case GetTypeResult::kSuccess:
      break;
    case GetTypeResult::kNoMatchTypePrefer:
    default:
      assert(false);
      break;
  }
  auto [flow_control_node, construct_result] =
      construct_data->ConstructObject(const_tag, std::move(final_type));
  if (construct_result != ObjectConstructData::CheckResult::kSuccess)
      [[unlikely]] {
    VarietyOrFunctionConstructError(construct_result,
                                    construct_data->GetObjectName());
  }
  return std::move(flow_control_node);
}

std::nullptr_t TypeDef(std::string&& str_typedef,
                       std::shared_ptr<FlowInterface>&& flow_control_node) {
  assert(str_typedef == "typedef");

  switch (flow_control_node->GetFlowType()) {
    case FlowType::kSimpleSentence:
      break;
    case FlowType::kFunctionDefine:
      // 不能给函数起别名
      OutputError(std::format("无法给函数起别名"));
      exit(-1);
      break;
    default:
      break;
  }
  auto variety_pointer = static_pointer_cast<VarietyOperatorNode>(
      static_cast<SimpleSentence&>(*flow_control_node)
          .GetSentenceOperateNodePointer());
  auto type_pointer = variety_pointer->GetVarietyTypePointer();
  const std::string& type_name = variety_pointer->GetVarietyName();
  auto [ignore_iter, result] =
      c_parser_controller.DefineType(type_name, type_pointer);
  if (result == AddTypeResult::kTypeAlreadyIn) [[unlikely]] {
    OutputError(std::format(
        "使用typedef定义别名时使用的名字{:}已存在相同类型的定义", type_name));
    exit(-1);
  }
  return nullptr;
}

std::nullptr_t NotEmptyFunctionRelaventArgumentsBase(
    std::shared_ptr<FlowInterface>&& flow_control_node) {
  switch (flow_control_node->GetFlowType()) {
    case FlowType::kSimpleSentence:
      // 只有变量可以作为函数参数
      break;
    case FlowType::kFunctionDefine:
      // 函数不能作为函数声明时的参数
      OutputError(std::format("函数不能作为声明时函数参数,请使用函数指针"));
      OutputError(std::format(
          "此诊断出现在函数{:}处",
          static_cast<c_parser_frontend::flow_control::FunctionDefine&>(
              *flow_control_node)
              .GetFunctionTypeReference()
              .GetFunctionName()));
      exit(-1);
      break;
    default:
      assert(false);
      break;
  }
  auto variety_pointer = std::static_pointer_cast<VarietyOperatorNode>(
      static_cast<SimpleSentence&>(*flow_control_node)
          .GetSentenceOperateNodePointer());
  // 添加函数参数
  function_type_construct_data->AddFunctionTypeArgument(variety_pointer);
  return nullptr;
}

std::nullptr_t NotEmptyFunctionRelaventArgumentsAnonymousBase(
    std::shared_ptr<FlowInterface>&& flow_control_node) {
  return NotEmptyFunctionRelaventArgumentsBase(std::move(flow_control_node));
}

std::nullptr_t NotEmptyFunctionRelaventArgumentsExtend(
    std::nullptr_t&&, std::string&& str_comma,
    std::shared_ptr<FlowInterface>&& flow_control_node) {
  assert(str_comma == ",");

  switch (flow_control_node->GetFlowType()) {
    case FlowType::kSimpleSentence:
      // 只有变量可以作为函数参数
      break;
    case FlowType::kFunctionDefine:
      // 函数不能作为函数的参数
      OutputError(std::format("函数不能作为声明时函数参数,请使用函数指针"));
      OutputError(std::format(
          "此诊断出现在函数{:}处",
          static_cast<c_parser_frontend::flow_control::FunctionDefine&>(
              *flow_control_node)
              .GetFunctionTypeReference()
              .GetFunctionName()));
      exit(-1);
      break;
    default:
      assert(false);
      break;
  }
  auto variety_pointer = std::static_pointer_cast<VarietyOperatorNode>(
      static_cast<SimpleSentence&>(*flow_control_node)
          .GetSentenceOperateNodePointer());
  // 添加函数参数
  function_type_construct_data->AddFunctionTypeArgument(variety_pointer);
  return nullptr;
}

std::nullptr_t NotEmptyFunctionRelaventArgumentsAnonymousExtend(
    std::nullptr_t&&, std::string&& str_comma,
    std::shared_ptr<FlowInterface>&& flow_control_node) {
  return NotEmptyFunctionRelaventArgumentsExtend(nullptr, std::move(str_comma),
                                                 std::move(flow_control_node));
}

std::nullptr_t FunctionRelaventArguments(std::nullptr_t) { return nullptr; }

std::shared_ptr<c_parser_frontend::flow_control::FunctionDefine>
FunctionDefineHead(std::shared_ptr<FlowInterface>&& function_head,
                   std::string&& left_curly_bracket) {
  assert(left_curly_bracket == "{");

  // 检查是否为函数头
  switch (function_head->GetFlowType()) {
    case FlowType::kFunctionDefine:
      break;
    case FlowType::kSimpleSentence:
      OutputError(std::format("函数声明语法错误"));
      exit(-1);
      break;
    default:
      assert(false);
      break;
  }
  std::shared_ptr<const FunctionType> function_type =
      static_cast<c_parser_frontend::flow_control::FunctionDefine&>(
          *function_head)
          .GetFunctionTypePointer();
  // 设置当前待构建函数
  bool set_result = c_parser_controller.SetFunctionToConstruct(
      std::shared_ptr<const FunctionType>(function_type));
  if (!set_result) [[unlikely]] {
    std::cerr << std::format(
                     "行数{:} 列数{:} "
                     "无法声明/定义函数，可能原因为：函数内部声明/"
                     "定义函数、定义与已声明的同名函数签名不同的函数",
                     GetLine(), GetColumn())
              << std::endl;
    exit(-1);
  }
  return std::static_pointer_cast<
      c_parser_frontend::flow_control::FunctionDefine>(
      std::move(function_head));
}

std::nullptr_t FunctionDefine(
    std::shared_ptr<c_parser_frontend::flow_control::FunctionDefine>&&,
    std::nullptr_t, std::string&& right_curly_bracket) {
  assert(right_curly_bracket == "}");

  // 检查函数体是否为空，如果为空则添加无返回值返回语句或报错
  auto& active_function = c_parser_controller.GetActiveFunctionReference();
  if (active_function.GetSentences().empty()) [[unlikely]] {
    auto& function_return_type =
        active_function.GetFunctionTypeReference().GetReturnTypeReference();
    if (function_return_type ==
        *c_parser_frontend::type_system::CommonlyUsedTypeGenerator::
            GetBasicType<c_parser_frontend::type_system::BuiltInType::kVoid,
                         c_parser_frontend::type_system::SignTag::kUnsigned>())
        [[likely]] {
      // 函数无返回值
      bool result = c_parser_controller.AddSentence(std::make_unique<Return>());
      assert(result);
    } else {
      OutputError(std::format("函数必须返回一个值"));
      exit(-1);
    }
  }
  // 添加函数内语句已经在Sentences构建过程中完成
  // 只需清理作用域
  c_parser_controller.PopActionScope();
  return nullptr;
}

std::pair<std::shared_ptr<const TypeInterface>, ConstTag>
SingleStructureBodyBase(std::shared_ptr<FlowInterface>&& flow_control_node) {
  // 检查是否为变量声明
  switch (flow_control_node->GetFlowType()) {
    case FlowType::kSimpleSentence:
      break;
    case FlowType::kFunctionDefine:
      OutputError(std::format("函数声明不能作为结构数据成员"));
      exit(-1);
      break;
    default:
      break;
  }
  std::shared_ptr<VarietyOperatorNode> variety_pointer =
      std::static_pointer_cast<VarietyOperatorNode>(
          static_cast<SimpleSentence&>(*flow_control_node)
              .GetSentenceOperateNodePointer());
  // 扩展声明时使用的类型（如果有则去除第一重指针）
  std::shared_ptr<const TypeInterface> extend_type =
      GetExtendAnnounceType(variety_pointer->GetVarietyTypePointer());
  // 获取扩展声明时的变量ConstTag
  ConstTag extend_const_tag = variety_pointer->GetConstTag();
  // 向结构数据节点中添加成员
  auto structure_member_index =
      structure_type_constructuring->AddStructureMember(
          variety_pointer->GetVarietyName(),
          variety_pointer->GetVarietyTypePointer(),
          variety_pointer->GetConstTag());
  assert(structure_member_index.IsValid());
  return std::make_pair(std::move(extend_type), extend_const_tag);
}

std::pair<std::shared_ptr<const TypeInterface>, ConstTag>&&
SingleStructureBodyExtend(
    std::pair<std::shared_ptr<const TypeInterface>, ConstTag>&& struct_data,
    std::string&& str_comma, std::string&& new_member_name) {
  assert(str_comma == ",");

  auto& [extend_type, extend_const_tag] = struct_data;
  auto structure_member_index =
      structure_type_constructuring->AddStructureMember(
          std::move(new_member_name), extend_type, extend_const_tag);
  // 检查待添加成员名是否重复
  if (!structure_member_index.IsValid()) {
    OutputError(std::format("重定义成员{:}", new_member_name));
    exit(-1);
  }
  return std::move(struct_data);
}

std::nullptr_t NotEmptyStructureBodyBase(
    std::pair<std::shared_ptr<const TypeInterface>, ConstTag>&&) {
  return nullptr;
}

std::nullptr_t NotEmptyStructureBodyExtend(
    std::nullptr_t, std::pair<std::shared_ptr<const TypeInterface>, ConstTag>&&,
    std::string&& semicolon) {
  assert(semicolon == ";");

  return nullptr;
}

std::nullptr_t StructureBody(std::nullptr_t) { return nullptr; }

std::shared_ptr<ListInitializeOperatorNode> InitializeList(
    std::string&& left_curly_bracket,
    std::shared_ptr<
        std::list<std::shared_ptr<InitializeOperatorNodeInterface>>>&&
        list_arguments,
    std::string&& right_curly_bracket) {
  auto initialize_list = std::make_shared<ListInitializeOperatorNode>();
  // 获取初始化列表参数
  std::list<std::shared_ptr<const InitializeOperatorNodeInterface>>
      argument_pointers_to_add;
  for (auto& pointer : *list_arguments) {
    argument_pointers_to_add.emplace_back(std::move(pointer));
  }
  // 将初始化列表的所有参数添加到容器中
  bool result =
      initialize_list->SetListValues(std::move(argument_pointers_to_add));
  return std::move(initialize_list);
}

std::shared_ptr<InitializeOperatorNodeInterface>&&
SingleInitializeListArgumentConstexprValue(
    std::shared_ptr<BasicTypeInitializeOperatorNode>&& value) {
  return std::move(value);
}

std::shared_ptr<InitializeOperatorNodeInterface>&&
SingleInitializeListArgumentList(
    std::shared_ptr<ListInitializeOperatorNode>&& value) {
  return std::move(value);
}

std::shared_ptr<std::list<std::shared_ptr<InitializeOperatorNodeInterface>>>
InitializeListArgumentsBase(
    std::shared_ptr<InitializeOperatorNodeInterface>&& init_data_pointer) {
  auto list_pointer = std::make_shared<
      std::list<std::shared_ptr<InitializeOperatorNodeInterface>>>();
  list_pointer->emplace_back(std::move(init_data_pointer));
  return list_pointer;
}

std::shared_ptr<std::list<std::shared_ptr<InitializeOperatorNodeInterface>>>&&
InitializeListArgumentsExtend(
    std::shared_ptr<std::list<
        std::shared_ptr<InitializeOperatorNodeInterface>>>&& list_pointer,
    std::string&& str_comma,
    std::shared_ptr<InitializeOperatorNodeInterface>&& init_data_pointer) {
  assert(str_comma == ",");

  list_pointer->emplace_back(std::move(init_data_pointer));
  return std::move(list_pointer);
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
AnnounceAssignableAssignable(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        value) {
  return std::move(value);
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AnnounceAssignableInitializeList(
    std::shared_ptr<ListInitializeOperatorNode>&& initialize_list) {
  return std::make_pair(
      std::move(initialize_list),
      std::make_shared<std::list<std::unique_ptr<FlowInterface>>>());
}

std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
           std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
VarietyAnnounceNoAssign(std::shared_ptr<VarietyOperatorNode>&& variety_node) {
  // 获取原始声明使用的类型
  auto raw_variety_type = variety_node->GetVarietyTypePointer();
  // 获取原始声明的ConstTag
  ConstTag extend_const_tag = variety_node->GetConstTag();
  // 存储获取变量过程操作的容器
  auto flow_control_node_container =
      std::make_shared<std::list<std::unique_ptr<FlowInterface>>>();
  // 创建并添加空间分配节点
  auto allocate_space_node = std::make_shared<AllocateOperatorNode>();
  bool result = allocate_space_node->SetTargetVariety(
      std::shared_ptr<VarietyOperatorNode>(variety_node));
  assert(result);
  auto allocate_flow_control_node = std::make_unique<SimpleSentence>();
  result = allocate_flow_control_node->SetSentenceOperateNode(
      std::move(allocate_space_node));
  assert(result);
  flow_control_node_container->emplace_back(
      std::move(allocate_flow_control_node));
  // 定义变量
  c_parser_controller.DefineVariety(variety_node);
  // 获取扩展声明时的类型
  if (raw_variety_type->GetType() == StructOrBasicType::kPointer) {
    // 剥离数组维数层指针
    while (raw_variety_type->GetType() == StructOrBasicType::kPointer &&
           std::static_pointer_cast<const PointerType>(raw_variety_type)
                   ->GetArraySize() != 0) {
      raw_variety_type = raw_variety_type->GetNextNodePointer();
    }
    // 如果是指针类型则在原始声明类型基础上去除一重指针
    return std::make_tuple(
        raw_variety_type->GetType() == StructOrBasicType::kPointer
            ? raw_variety_type->GetNextNodePointer()
            : raw_variety_type,
        extend_const_tag, std::move(flow_control_node_container));
  } else {
    // 不是指针类型，直接返回
    return std::make_tuple(raw_variety_type, extend_const_tag,
                           std::move(flow_control_node_container));
  }
}

std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
           std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
VarietyAnnounceWithAssign(
    std::shared_ptr<VarietyOperatorNode>&& variety_node,
    const std::shared_ptr<const OperatorNodeInterface>& node_for_assign) {
  // 获取原始声明使用的类型
  auto raw_variety_type = variety_node->GetVarietyTypePointer();
  // 获取原始声明的ConstTag
  ConstTag extend_const_tag = variety_node->GetConstTag();
  // 存储获取变量过程操作的容器
  auto flow_control_node_container =
      std::make_shared<std::list<std::unique_ptr<FlowInterface>>>();
  // 创建并添加空间分配节点
  auto allocate_node = std::make_shared<AllocateOperatorNode>();
  // 设置要分配空间的节点
  bool result = allocate_node->SetTargetVariety(variety_node);
  assert(result);
  // 将分配空间的节点包装成流程节点
  auto allocate_flow_control_node = std::make_unique<SimpleSentence>();
  result = allocate_flow_control_node->SetSentenceOperateNode(allocate_node);
  assert(result);
  // 添加流程语句
  flow_control_node_container->emplace_back(
      std::move(allocate_flow_control_node));
  // 创建并添加赋值节点
  auto assign_node = std::make_shared<AssignOperatorNode>();
  // 设置被赋值的节点
  assign_node->SetNodeToBeAssigned(variety_node);
  // 设置用来赋值的节点
  // 当前处于声明状态
  auto assignable_check_result =
      assign_node->SetNodeForAssign(node_for_assign, true);
  CheckAssignableCheckResult(assignable_check_result);
  // 将赋值节点包装成流程节点
  auto assign_flow_control_node = std::make_unique<SimpleSentence>();
  result =
      assign_flow_control_node->SetSentenceOperateNode(std::move(assign_node));
  assert(result);
  // 添加流程语句
  flow_control_node_container->emplace_back(
      std::move(assign_flow_control_node));
  // 定义变量
  c_parser_controller.DefineVariety(variety_node);
  // 获取扩展声明时的类型
  if (raw_variety_type->GetType() == StructOrBasicType::kPointer) {
    // 剥离数组维数层指针
    while (raw_variety_type->GetType() == StructOrBasicType::kPointer &&
           std::static_pointer_cast<const PointerType>(raw_variety_type)
                   ->GetArraySize() != 0) {
      raw_variety_type = raw_variety_type->GetNextNodePointer();
    }
    // 如果是指针类型则在原始声明类型基础上去除一重指针
    return std::make_tuple(raw_variety_type->GetNextNodePointer(),
                           extend_const_tag,
                           std::move(flow_control_node_container));
  } else {
    // 不是指针类型，直接返回
    return std::make_tuple(raw_variety_type, extend_const_tag,
                           std::move(flow_control_node_container));
  }
}

std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
           std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
SingleAnnounceAndAssignNoAssignBase(
    std::shared_ptr<FlowInterface>&& flow_control_node) {
  switch (flow_control_node->GetFlowType()) {
    case FlowType::kSimpleSentence:
      break;
    case FlowType::kFunctionDefine:
      OutputError(std::format("不支持的扩展：函数内声明函数"));
      exit(-1);
      break;
    default:
      assert(false);
      break;
  }
  return VarietyAnnounceNoAssign(std::static_pointer_cast<VarietyOperatorNode>(
      static_cast<SimpleSentence&>(*flow_control_node)
          .GetSentenceOperateNodePointer()));
}

std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
           std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
SingleAnnounceAndAssignWithAssignBase(
    std::shared_ptr<FlowInterface>&& flow_control_node,
    std::string&& str_assign,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        container) {
  assert(str_assign == "=");

  // 检查是否为变量声明
  switch (flow_control_node->GetFlowType()) {
    case FlowType::kSimpleSentence:
      break;
    case FlowType::kFunctionDefine:
      OutputError(std::format("不支持的扩展：函数内声明函数"));
      exit(-1);
      break;
    default:
      assert(false);
      break;
  }
  auto& [node_for_assign, statements_to_get_node_for_assign] = container;
  c_parser_controller.AddSentences(
      std::move(*statements_to_get_node_for_assign));
  return VarietyAnnounceWithAssign(
      std::static_pointer_cast<VarietyOperatorNode>(
          static_cast<SimpleSentence&>(*flow_control_node)
              .GetSentenceOperateNodePointer()),
      std::move(node_for_assign));
}

std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
           std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
SingleAnnounceAndAssignNoAssignExtend(
    std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
               std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        container,
    std::string&& str_comma, std::string&& variety_name) {
  assert(str_comma == ",");

  auto& [extend_announce_type, extend_const_tag, flow_control_node_container] =
      container;
  // 构建变量节点
  auto variety_node = std::make_shared<VarietyOperatorNode>(
      std::move(variety_name), extend_const_tag, LeftRightValueTag::kLeftValue);
  variety_node->SetVarietyType(
      std::shared_ptr<const TypeInterface>(extend_announce_type));
  auto [ignore_type, ignore_const_tag, sub_flow_control_node_container] =
      VarietyAnnounceNoAssign(std::move(variety_node));
  // 将获取变量的操作合并到主容器中
  flow_control_node_container->splice(
      flow_control_node_container->end(),
      std::move(*sub_flow_control_node_container));
  return std::move(container);
}

std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
           std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
SingleAnnounceAndAssignWithAssignExtend(
    std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
               std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        container,
    std::string&& str_comma, std::string&& variety_name,
    std::string&& str_assign,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        assigned_container) {
  assert(str_comma == ",");
  assert(str_assign == "=");

  auto& [extend_announce_type, extend_const_tag, flow_control_node_container] =
      container;
  auto& [node_for_assign, flow_control_nodes_to_get_node_for_assign] =
      assigned_container;
  // 将获取待赋值变量的操作合并到主容器中
  flow_control_node_container->splice(
      flow_control_node_container->end(),
      std::move(*flow_control_nodes_to_get_node_for_assign));
  // 构建变量节点
  auto variety_node = std::make_shared<VarietyOperatorNode>(
      std::move(variety_name), extend_const_tag, LeftRightValueTag::kLeftValue);
  variety_node->SetVarietyType(
      std::shared_ptr<const TypeInterface>(extend_announce_type));
  auto [ignore_type, ignore_const_tag, sub_flow_control_node_container] =
      VarietyAnnounceWithAssign(std::move(variety_node),
                                std::move(node_for_assign));
  // 将初始化变量的操作合并到主容器中
  flow_control_node_container->splice(
      flow_control_node_container->end(),
      std::move(*sub_flow_control_node_container));
  return std::move(container);
}

std::pair<std::shared_ptr<const TypeInterface>, ConstTag>&& TypeBasicType(
    std::pair<std::shared_ptr<const TypeInterface>, ConstTag>&& type_data) {
  return std::move(type_data);
}

std::pair<std::shared_ptr<const TypeInterface>, ConstTag> TypeFunctionRelavent(
    std::shared_ptr<FlowInterface>&& flow_control_node) {
  // 检查类型是否为函数声明
  if (flow_control_node->GetFlowType() == FlowType::kFunctionDefine)
      [[unlikely]] {
    // 函数不能作为类型使用
    OutputError(std::format("函数声明/定义不能作为类型"));
    exit(-1);
  }
  auto operator_node = std::static_pointer_cast<VarietyOperatorNode>(
      static_cast<SimpleSentence&>(*flow_control_node)
          .GetSentenceOperateNodePointer());
  // 检查是否为匿名指针声明
  if (!operator_node->GetVarietyName().empty()) [[unlikely]] {
    OutputError(std::format("变量声明不是类型"));
    exit(-1);
  }
  return std::make_pair(operator_node->GetVarietyTypePointer(),
                        operator_node->GetConstTag());
}

MathematicalOperation MathematicalOperatorPlus(std::string&& str_operator) {
  assert(str_operator == "+");
  return MathematicalOperation::kPlus;
}

MathematicalOperation MathematicalOperatorMinus(std::string&& str_operator) {
  assert(str_operator == "-");
  return MathematicalOperation::kMinus;
}

MathematicalOperation MathematicalOperatorMultiple(std::string&& str_operator) {
  assert(str_operator == "*");
  return MathematicalOperation::kMultiple;
}

MathematicalOperation MathematicalOperatorDivide(std::string&& str_operator) {
  assert(str_operator == "/");
  return MathematicalOperation::kDivide;
}

MathematicalOperation MathematicalOperatorMod(std::string&& str_operator) {
  assert(str_operator == "%");
  return MathematicalOperation::kMod;
}

MathematicalOperation MathematicalOperatorLeftShift(
    std::string&& str_operator) {
  assert(str_operator == "<<");
  return MathematicalOperation::kLeftShift;
}

MathematicalOperation MathematicalOperatorRightShift(
    std::string&& str_operator) {
  assert(str_operator == ">>");
  return MathematicalOperation::kRightShift;
}

MathematicalOperation MathematicalOperatorAnd(std::string&& str_operator) {
  assert(str_operator == "&");
  return MathematicalOperation::kAnd;
}

MathematicalOperation MathematicalOperatorOr(std::string&& str_operator) {
  assert(str_operator == "|");
  return MathematicalOperation::kOr;
}

MathematicalOperation MathematicalOperatorXor(std::string&& str_operator) {
  assert(str_operator == "^");
  return MathematicalOperation::kXor;
}

MathematicalOperation MathematicalOperatorNot(std::string&& str_operator) {
  assert(str_operator == "!");
  return MathematicalOperation::kNot;
}

MathematicalAndAssignOperation MathematicalAndAssignOperatorPlusAssign(
    std::string&& str_operator) {
  assert(str_operator == "+=");
  return MathematicalAndAssignOperation::kPlusAssign;
}

MathematicalAndAssignOperation MathematicalAndAssignOperatorMinusAssign(
    std::string&& str_operator) {
  assert(str_operator == "-=");
  return MathematicalAndAssignOperation::kMinusAssign;
}

MathematicalAndAssignOperation MathematicalAndAssignOperatorMultipleAssign(
    std::string&& str_operator) {
  assert(str_operator == "*=");
  return MathematicalAndAssignOperation::kMultipleAssign;
}

MathematicalAndAssignOperation MathematicalAndAssignOperatorDivideAssign(
    std::string&& str_operator) {
  assert(str_operator == "/=");
  return MathematicalAndAssignOperation::kDivideAssign;
}

MathematicalAndAssignOperation MathematicalAndAssignOperatorModAssign(
    std::string&& str_operator) {
  assert(str_operator == "%=");
  return MathematicalAndAssignOperation::kModAssign;
}

MathematicalAndAssignOperation MathematicalAndAssignOperatorLeftShiftAssign(
    std::string&& str_operator) {
  assert(str_operator == "<<=");
  return MathematicalAndAssignOperation::kLeftShiftAssign;
}

MathematicalAndAssignOperation MathematicalAndAssignOperatorRightShiftAssign(
    std::string&& str_operator) {
  assert(str_operator == ">>=");
  return MathematicalAndAssignOperation::kRightShiftAssign;
}

MathematicalAndAssignOperation MathematicalAndAssignOperatorAndAssign(
    std::string&& str_operator) {
  assert(str_operator == "&=");
  return MathematicalAndAssignOperation::kAndAssign;
}

MathematicalAndAssignOperation MathematicalAndAssignOperatorOrAssign(
    std::string&& str_operator) {
  assert(str_operator == "|=");
  return MathematicalAndAssignOperation::kOrAssign;
}

MathematicalAndAssignOperation MathematicalAndAssignOperatorXorAssign(
    std::string&& str_operator) {
  assert(str_operator == "^=");
  return MathematicalAndAssignOperation::kXorAssign;
}

LogicalOperation LogicalOperatorAndAnd(std::string&& str_operator) {
  assert(str_operator == "&&");
  return LogicalOperation::kAndAnd;
}

LogicalOperation LogicalOperatorOrOr(std::string&& str_operator) {
  assert(str_operator == "||");
  return LogicalOperation::kOrOr;
}

LogicalOperation LogicalOperatorGreater(std::string&& str_operator) {
  assert(str_operator == ">");
  return LogicalOperation::kGreater;
}

LogicalOperation LogicalOperatorGreaterEqual(std::string&& str_operator) {
  assert(str_operator == ">=");
  return LogicalOperation::kGreaterEqual;
}

LogicalOperation LogicalOperatorLess(std::string&& str_operator) {
  assert(str_operator == "<");
  return LogicalOperation::kLess;
}

LogicalOperation LogicalOperatorLessEqual(std::string&& str_operator) {
  assert(str_operator == "<=");
  return LogicalOperation::kLessEqual;
}

LogicalOperation LogicalOperatorEqual(std::string&& str_operator) {
  assert(str_operator == "==");
  return LogicalOperation::kEqual;
}

LogicalOperation LogicalOperatorNotEqual(std::string&& str_operator) {
  assert(str_operator == "!=");
  return LogicalOperation::kNotEqual;
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableConstexprValue(
    std::shared_ptr<BasicTypeInitializeOperatorNode>&& value) {
  // 构建存储获取常量/变量的操作的容器
  return std::make_pair(
      std::move(value),
      std::make_shared<std::list<std::unique_ptr<FlowInterface>>>());
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableId(std::string&& variety_name) {
  auto [variety_operator_node, found] =
      c_parser_controller.GetVarietyOrFunction(variety_name);
  if (!found) [[unlikely]] {
    // 未找到给定名称的变量
    OutputError(std::format("找不到对象{:}", variety_name));
    exit(-1);
  }
  return std::make_pair(
      std::move(variety_operator_node),
      std::make_shared<std::list<std::unique_ptr<FlowInterface>>>());
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
AssignableTemaryOperator(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        value) {
  return std::move(value);
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
AssignableBracket(
    std::string&& left_bracket,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        value,
    std::string&& right_bracket) {
  assert(left_bracket == "(");
  assert(right_bracket == ")");

  return std::move(value);
}
std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableTypeConvert(
    std::string&& left_bracket,
    std::pair<std::shared_ptr<const TypeInterface>, ConstTag>&& type_data,
    std::string&& right_bracket,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        old_variety_data) {
  assert(left_bracket == "(");
  assert(right_bracket == ")");

  auto& [new_type, new_const_tag] = type_data;
  auto& [old_operator_node, flow_control_node_container] = old_variety_data;
  if (old_operator_node->GetGeneralOperatorType() ==
          GeneralOperationType::kInitValue &&
      new_const_tag == ConstTag::kNonConst) [[unlikely]] {
    OutputError("初始化数据不能转化为非const类型");
    exit(-1);
  }
  // 转换节点
  auto convert_operator_node =
      std::make_shared<TypeConvert>(old_operator_node, new_type, new_const_tag);
  // 经过类型转换得到的节点
  auto converted_operator_node =
      convert_operator_node->GetDestinationNodePointer();
  // 添加转换流程控制节点
  auto flow_control_node = std::make_unique<SimpleSentence>();
  bool result =
      flow_control_node->SetSentenceOperateNode(convert_operator_node);
  assert(result);
  flow_control_node_container->emplace_back(std::move(flow_control_node));
  // 返回经过转换的节点而不是转换节点
  return std::make_pair(std::move(converted_operator_node),
                        std::move(flow_control_node_container));
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableAssign(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        destination_variety_data,
    std::string&& str_assign,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        source_variety_data) {
  assert(str_assign == "=");

  auto& [node_to_be_assigned, statements_to_get_node_to_be_assigned] =
      destination_variety_data;
  auto& [node_for_assign, statements_to_get_node_for_assign] =
      source_variety_data;
  // 获取真正的待赋值对象
  auto real_node_to_be_assigned = node_to_be_assigned->GetResultOperatorNode();
  if (real_node_to_be_assigned == nullptr) {
    real_node_to_be_assigned = node_to_be_assigned;
  }
  // 获取真正的待赋值对象
  auto real_node_for_assign = node_for_assign->GetResultOperatorNode();
  if (real_node_for_assign == nullptr) {
    real_node_for_assign = node_for_assign;
  }
  if (real_node_to_be_assigned->GetGeneralOperatorType() !=
      GeneralOperationType::kVariety) [[unlikely]] {
    OutputError(std::format("无法对非变量赋值"));
    exit(-1);
  } else if (static_cast<const VarietyOperatorNode&>(*real_node_to_be_assigned)
                 .GetLeftRightValueTag() != LeftRightValueTag::kLeftValue)
      [[unlikely]] {
    OutputError(std::format("无法对右值赋值"));
    exit(-1);
  }
  auto assign_node = std::make_shared<AssignOperatorNode>();
  assign_node->SetNodeToBeAssigned(real_node_to_be_assigned);
  auto assignable_check_result =
      assign_node->SetNodeForAssign(real_node_for_assign, false);
  CheckAssignableCheckResult(assignable_check_result);
  // 合并获取赋值用节点和被赋值的节点过程中产生的语句
  statements_to_get_node_to_be_assigned->splice(
      statements_to_get_node_to_be_assigned->end(),
      std::move(*statements_to_get_node_for_assign));
  auto flow_control_sentence = std::make_unique<SimpleSentence>();
  bool result = flow_control_sentence->SetSentenceOperateNode(assign_node);
  assert(result);
  statements_to_get_node_to_be_assigned->emplace_back(
      std::move(flow_control_sentence));
  assert(statements_to_get_node_for_assign->empty());
  return std::make_pair(std::move(assign_node),
                        std::move(statements_to_get_node_to_be_assigned));
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
AssignableFunctionCall(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        value) {
  return std::move(value);
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableSizeOfType(
    std::string&& str_sizeof, std::string&& str_left_bracket,
    std::pair<std::shared_ptr<const TypeInterface>, ConstTag>&& type_data,
    std::string&& str_right_bracket) {
  assert(str_sizeof == "sizeof");
  assert(str_left_bracket == "(");
  assert(str_right_bracket == ")");

  auto& [type_pointer, variety_const_tag] = type_data;
  size_t type_size = type_pointer->TypeSizeOf();
  auto return_constexpr_value =
      std::make_shared<BasicTypeInitializeOperatorNode>(
          InitializeType::kBasic, std::to_string(type_size));
  BuiltInType return_constexpr_value_type =
      c_parser_frontend::type_system::CalculateBuiltInType(
          return_constexpr_value->GetValue());
  assert(return_constexpr_value_type < BuiltInType::kFloat32);
  bool result = return_constexpr_value->SetInitDataType(
      CommonlyUsedTypeGenerator::GetBasicTypeNotTemplate(
          return_constexpr_value_type, SignTag::kSigned));
  assert(result);
  // 返回空容器，继承的容器自动释放
  return std::make_pair(
      std::move(return_constexpr_value),
      std::make_shared<std::list<std::unique_ptr<FlowInterface>>>());
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableSizeOfAssignable(
    std::string&& str_sizeof, std::string&& str_left_bracket,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data,
    std::string&& str_right_bracket) {
  assert(str_sizeof == "sizeof");
  assert(str_left_bracket == "(");
  assert(str_right_bracket == ")");

  auto& [assignable_node, flow_control_node_container] = variety_data;
  size_t type_size = assignable_node->GetResultTypePointer()->TypeSizeOf();
  auto return_constexpr_value =
      std::make_shared<BasicTypeInitializeOperatorNode>(
          InitializeType::kBasic, std::to_string(type_size));
  BuiltInType return_constexpr_value_type =
      c_parser_frontend::type_system::CalculateBuiltInType(
          return_constexpr_value->GetValue());
  assert(return_constexpr_value_type < BuiltInType::kFloat32);
  bool result = return_constexpr_value->SetInitDataType(
      CommonlyUsedTypeGenerator::GetBasicTypeNotTemplate(
          return_constexpr_value_type, SignTag::kSigned));
  assert(result);
  // 返回空容器，继承的容器自动释放
  return std::make_pair(
      std::move(return_constexpr_value),
      std::make_shared<std::list<std::unique_ptr<FlowInterface>>>());
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableMemberAccess(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data,
    std::string&& str_member_access, std::string&& member_name) {
  assert(str_member_access == ".");

  auto& [assignable, sentences_to_get_assignable] = variety_data;
  auto member_access_node = std::make_shared<MemberAccessOperatorNode>();
  bool result = member_access_node->SetNodeToAccess(assignable);
  if (!result) [[unlikely]] {
    OutputError(std::format("无法对非结构数据或枚举类型访问成员"));
    exit(-1);
  }
  result = member_access_node->SetMemberName(std::move(member_name));
  if (!result) [[unlikely]] {
    OutputError(std::format("给定结构体不存在成员{:}", member_name));
    exit(-1);
  }
  auto flow_control_node = std::make_unique<SimpleSentence>();
  result = flow_control_node->SetSentenceOperateNode(member_access_node);
  assert(result);
  sentences_to_get_assignable->emplace_back(std::move(flow_control_node));
  return std::make_pair(member_access_node->GetResultOperatorNode(),
                        std::move(sentences_to_get_assignable));
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignablePointerMemberAccess(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data,
    std::string&& str_member_access, std::string&& member_name) {
  assert(str_member_access == "->");

  auto& [assignable, sentences_to_get_assignable] = variety_data;
  // 对指针解引用后访问成员
  auto dereference_node = std::make_shared<DereferenceOperatorNode>();
  bool result = dereference_node->SetNodeToDereference(assignable);
  if (!result) [[unlikely]] {
    OutputError(std::format("不能对非指针对象使用\"->\"运算符"));
    exit(-1);
  }
  auto dereferenced_node = dereference_node->GetDereferencedNodePointer();
  auto member_access_node = std::make_shared<MemberAccessOperatorNode>();
  result = member_access_node->SetNodeToAccess(dereferenced_node);
  if (!result) [[unlikely]] {
    OutputError(std::format("无法对非结构数据或枚举类型访问成员"));
    exit(-1);
  }
  result = member_access_node->SetMemberName(std::move(member_name));
  if (!result) [[unlikely]] {
    OutputError(std::format("给定结构数据不存在成员{:}", member_name));
    exit(-1);
  }
  // 添加解引用节点和成员访问节点
  auto flow_control_node = std::make_unique<SimpleSentence>();
  result = flow_control_node->SetSentenceOperateNode(dereference_node);
  assert(result);
  sentences_to_get_assignable->emplace_back(std::move(flow_control_node));
  flow_control_node = std::make_unique<SimpleSentence>();
  result = flow_control_node->SetSentenceOperateNode(member_access_node);
  assert(result);
  sentences_to_get_assignable->emplace_back(std::move(flow_control_node));
  return std::make_pair(member_access_node->GetResultOperatorNode(),
                        std::move(sentences_to_get_assignable));
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableMathematicalOperate(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        destination_variety_data,
    MathematicalOperation mathematical_operation,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        source_variety_data) {
  auto& [left_operator_node, left_flow_control_node_container] =
      destination_variety_data;
  auto& [right_operator_node, right_flow_control_node_container] =
      source_variety_data;
  auto mathematical_operator_node =
      std::make_shared<MathematicalOperatorNode>(mathematical_operation);
  bool left_operator_node_check_result =
      mathematical_operator_node->SetLeftOperatorNode(left_operator_node);
  if (!left_operator_node_check_result) [[unlikely]] {
    OutputError(std::format("左运算数无法参与运算"));
    exit(-1);
  }
  DeclineMathematicalComputeTypeResult right_operator_node_check_result =
      mathematical_operator_node->SetRightOperatorNode(right_operator_node);
  CheckMathematicalComputeTypeResult(right_operator_node_check_result);
  // 添加流程控制节点
  auto flow_control_node = std::make_unique<SimpleSentence>();
  bool result =
      flow_control_node->SetSentenceOperateNode(mathematical_operator_node);
  assert(result);
  // 将生成右运算节点的操作合并到左容器中
  left_flow_control_node_container->splice(
      left_flow_control_node_container->end(),
      std::move(*right_flow_control_node_container));
  // 添加数学运算节点
  left_flow_control_node_container->emplace_back(std::move(flow_control_node));
  // 返回运算得到的节点而不是运算节点
  return std::make_pair(
      mathematical_operator_node->GetComputeResultNodePointer(),
      std::move(left_flow_control_node_container));
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableMathematicalAndAssignOperate(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        destination_variety_data,
    MathematicalAndAssignOperation mathematical_and_assign_operation,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        source_variety_data) {
  auto& [left_operator_node, left_flow_control_node_container] =
      destination_variety_data;
  auto& [right_operator_node, right_flow_control_node_container] =
      source_variety_data;
  // 获取数学运算符
  MathematicalOperation mathematical_operation = c_parser_frontend::
      operator_node::MathematicalAndAssignOperationToMathematicalOperation(
          mathematical_and_assign_operation);
  // 生成运算节点
  auto mathematical_operator_node =
      std::make_shared<MathematicalOperatorNode>(mathematical_operation);
  bool left_operator_node_check_result =
      mathematical_operator_node->SetLeftOperatorNode(left_operator_node);
  if (!left_operator_node_check_result) [[unlikely]] {
    OutputError(std::format("左运算数无法参与运算"));
    exit(-1);
  }
  DeclineMathematicalComputeTypeResult right_operator_node_check_result =
      mathematical_operator_node->SetRightOperatorNode(right_operator_node);
  CheckMathematicalComputeTypeResult(right_operator_node_check_result);
  // 生成赋值节点
  auto assign_operator_node = std::make_shared<AssignOperatorNode>();
  assign_operator_node->SetNodeToBeAssigned(left_operator_node);
  AssignableCheckResult assignable_check_result =
      assign_operator_node->SetNodeForAssign(right_operator_node, false);
  CheckAssignableCheckResult(assignable_check_result);
  // 将生成右运算节点的操作合并到左容器中
  left_flow_control_node_container->splice(
      left_flow_control_node_container->end(),
      std::move(*right_flow_control_node_container));
  // 添加运算流程控制节点和赋值流程控制节点
  auto flow_control_node_mathematical = std::make_unique<SimpleSentence>();
  bool result = flow_control_node_mathematical->SetSentenceOperateNode(
      mathematical_operator_node);
  assert(result);
  left_flow_control_node_container->emplace_back(
      std::move(flow_control_node_mathematical));
  auto flow_control_node_assign = std::make_unique<SimpleSentence>();
  result =
      flow_control_node_assign->SetSentenceOperateNode(assign_operator_node);
  assert(result);
  left_flow_control_node_container->emplace_back(
      std::move(flow_control_node_assign));
  // 返回运算得到的节点而不是运算节点
  return std::make_pair(assign_operator_node->GetResultOperatorNode(),
                        std::move(left_flow_control_node_container));
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableLogicalOperate(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&& lhr,
    LogicalOperation logical_operation,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        rhr) {
  auto& [left_operator_node, left_flow_control_node_container] = lhr;
  auto& [right_operator_node, right_flow_control_node_container] = rhr;
  // 构建逻辑运算节点
  auto logical_operator_node =
      std::make_shared<LogicalOperationOperatorNode>(logical_operation);
  bool check_result =
      logical_operator_node->SetLeftOperatorNode(left_operator_node);
  if (!check_result) [[unlikely]] {
    OutputError(std::format("左操作数无法作为逻辑运算节点"));
    exit(-1);
  }
  check_result =
      logical_operator_node->SetRightOperatorNode(right_operator_node);
  if (!check_result) [[unlikely]] {
    OutputError(std::format("右操作数无法作为逻辑运算节点"));
    exit(-1);
  }
  // 将生成右运算节点的操作合并到左容器中
  left_flow_control_node_container->splice(
      left_flow_control_node_container->end(),
      std::move(*right_flow_control_node_container));
  // 添加运算流程节点
  auto flow_control_node = std::make_unique<SimpleSentence>();
  check_result =
      flow_control_node->SetSentenceOperateNode(logical_operator_node);
  assert(check_result);
  left_flow_control_node_container->emplace_back(std::move(flow_control_node));
  // 返回运算得到的节点而不是运算节点
  return std::make_pair(logical_operator_node->GetResultOperatorNode(),
                        std::move(left_flow_control_node_container));
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableNot(
    std::string&& str_operator,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data) {
  assert(str_operator == "!");

  auto& [sub_assignable_node, flow_control_node_container] = variety_data;
  auto not_operator_node =
      std::make_shared<MathematicalOperatorNode>(MathematicalOperation::kNot);
  bool check_result =
      not_operator_node->SetLeftOperatorNode(sub_assignable_node);
  if (!check_result) [[unlikely]] {
    OutputError(std::format("无法进行逻辑非运算"));
    exit(-1);
  }
  auto flow_control_node = std::make_unique<SimpleSentence>();
  check_result = flow_control_node->SetSentenceOperateNode(not_operator_node);
  assert(check_result);
  flow_control_node_container->emplace_back(std::move(flow_control_node));
  // 返回运算得到的节点而不是运算节点
  return std::make_pair(not_operator_node->GetResultOperatorNode(),
                        std::move(flow_control_node_container));
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableLogicalNegative(
    std::string&& str_operator,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data) {
  assert(str_operator == "~");

  auto& [sub_assignable_node, flow_control_node_container] = variety_data;
  auto logic_negative_node = std::make_shared<MathematicalOperatorNode>(
      MathematicalOperation::kLogicalNegative);
  bool check_result =
      logic_negative_node->SetLeftOperatorNode(sub_assignable_node);
  if (!check_result) [[unlikely]] {
    OutputError(std::format("无法进行按位取反运算"));
    exit(-1);
  }
  auto flow_control_node = std::make_unique<SimpleSentence>();
  check_result = flow_control_node->SetSentenceOperateNode(logic_negative_node);
  assert(check_result);
  flow_control_node_container->emplace_back(std::move(flow_control_node));
  // 返回运算得到的节点而不是运算节点
  return std::make_pair(logic_negative_node->GetResultOperatorNode(),
                        std::move(flow_control_node_container));
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableMathematicalNegative(
    std::string&& str_operator,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data) {
  assert(str_operator == "-");

  auto& [sub_assignable_node, flow_control_node_container] = variety_data;
  auto math_negative_node = std::make_shared<MathematicalOperatorNode>(
      MathematicalOperation::kMathematicalNegative);
  bool check_result =
      math_negative_node->SetLeftOperatorNode(sub_assignable_node);
  if (!check_result) [[unlikely]] {
    OutputError(std::format("无法进行取负运算"));
    exit(-1);
  }
  auto flow_control_node = std::make_unique<SimpleSentence>();
  check_result = flow_control_node->SetSentenceOperateNode(math_negative_node);
  assert(check_result);
  flow_control_node_container->emplace_back(std::move(flow_control_node));
  // 返回运算得到的节点而不是运算节点
  return std::make_pair(math_negative_node->GetResultOperatorNode(),
                        std::move(flow_control_node_container));
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableObtainAddress(
    std::string&& str_operator,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data) {
  assert(str_operator == "&");

  auto& [sub_assignable_node, flow_control_node_container] = variety_data;
  auto node_to_obtain_address =
      std::static_pointer_cast<const VarietyOperatorNode>(sub_assignable_node);
  // 检查被取地址的节点
  if (node_to_obtain_address->GetGeneralOperatorType() !=
      GeneralOperationType::kVariety) [[unlikely]] {
    OutputError(std::format("无法对非变量类型取地址"));
    exit(-1);
  } else if (node_to_obtain_address->GetLeftRightValueTag() !=
             LeftRightValueTag::kLeftValue) [[unlikely]] {
    // 被取地址的节点是右值
    OutputError(std::format("无法对右值取地址"));
    exit(-1);
  }
  auto obtain_address_node = std::make_shared<ObtainAddressOperatorNode>();
  bool result =
      obtain_address_node->SetNodeToObtainAddress(node_to_obtain_address);
  if (!result) [[unlikely]] {
    OutputError(std::format("无法取地址"));
    exit(-1);
  }
  auto flow_control_node = std::make_unique<SimpleSentence>();
  result = flow_control_node->SetSentenceOperateNode(obtain_address_node);
  assert(result);
  flow_control_node_container->emplace_back(std::move(flow_control_node));
  // 返回运算得到的节点而不是运算节点
  return std::make_pair(obtain_address_node->GetResultOperatorNode(),
                        std::move(flow_control_node_container));
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableDereference(
    std::string&& str_operator,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data) {
  assert(str_operator == "*");

  auto& [sub_assignable_node, flow_control_node_container] = variety_data;
  auto dereference_node = std::make_shared<DereferenceOperatorNode>();
  bool result = dereference_node->SetNodeToDereference(sub_assignable_node);
  if (!result) [[unlikely]] {
    OutputError(std::format("无法解引用"));
    exit(-1);
  }
  auto flow_control_node = std::make_unique<SimpleSentence>();
  result = flow_control_node->SetSentenceOperateNode(dereference_node);
  assert(result);
  flow_control_node_container->emplace_back(std::move(flow_control_node));
  // 返回运算得到的节点而不是运算节点
  return std::make_pair(dereference_node->GetResultOperatorNode(),
                        std::move(flow_control_node_container));
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableArrayAccess(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        array_data,
    std::string&& left_square_bracket,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        index_data,
    std::string&& right_square_bracket) {
  assert(left_square_bracket == "[");
  assert(right_square_bracket == "]");

  auto& [node_to_dereference, main_flow_control_node_container] = array_data;
  auto& [index_node, sub_flow_control_node_container] = index_data;
  // 创建将指针地址与偏移量相加的节点
  auto plus_operator_node =
      std::make_shared<MathematicalOperatorNode>(MathematicalOperation::kPlus);
  bool left_node_check_result =
      plus_operator_node->SetLeftOperatorNode(node_to_dereference);
  if (!left_node_check_result) [[unlikely]] {
    OutputError(std::format("无法间接寻址"));
    exit(-1);
  }
  auto right_node_check_result =
      plus_operator_node->SetRightOperatorNode(index_node);
  CheckMathematicalComputeTypeResult(right_node_check_result);
  // 创建对最终地址解引用的节点
  auto dereference_operator_node = std::make_shared<DereferenceOperatorNode>();
  bool dereference_result = dereference_operator_node->SetNodeToDereference(
      plus_operator_node->GetResultOperatorNode());
  if (!dereference_result) [[unlikely]] {
    OutputError(std::format("无法访问数组，可能使用了浮点数下标"));
    exit(-1);
  }
  // 合并获取index的操作到主容器中
  main_flow_control_node_container->splice(
      main_flow_control_node_container->end(),
      std::move(*sub_flow_control_node_container));
  // 创建流程节点
  auto plus_flow_control_node = std::make_unique<SimpleSentence>();
  bool result =
      plus_flow_control_node->SetSentenceOperateNode(plus_operator_node);
  assert(result);
  main_flow_control_node_container->emplace_back(
      std::move(plus_flow_control_node));
  auto dereference_flow_control_node = std::make_unique<SimpleSentence>();
  result = dereference_flow_control_node->SetSentenceOperateNode(
      dereference_operator_node);
  assert(result);
  main_flow_control_node_container->emplace_back(
      std::move(dereference_flow_control_node));
  // 返回运算得到的节点而不是运算节点
  return std::make_pair(dereference_operator_node->GetResultOperatorNode(),
                        std::move(main_flow_control_node_container));
}

std::shared_ptr<const OperatorNodeInterface> PrefixPlusOrMinus(
    MathematicalOperation mathematical_operation,
    const std::shared_ptr<const OperatorNodeInterface>& node_to_operate,
    std::list<std::unique_ptr<FlowInterface>>* flow_control_node_container) {
  assert(mathematical_operation == MathematicalOperation::kPlus ||
         mathematical_operation == MathematicalOperation::kMinus);
  // 创建运算节点
  auto mathematical_operator_node =
      std::make_shared<MathematicalOperatorNode>(mathematical_operation);
  // 创建++/--运算使用的数值1
  auto constexpr_num_for_operate =
      std::make_shared<BasicTypeInitializeOperatorNode>(InitializeType::kBasic,
                                                        "1");
  constexpr_num_for_operate->SetInitDataType(
      CommonlyUsedTypeGenerator::GetBasicType<BuiltInType::kInt1,
                                              SignTag::kSigned>());
  bool left_operator_node_check_result =
      mathematical_operator_node->SetLeftOperatorNode(node_to_operate);
  if (!left_operator_node_check_result) [[unlikely]] {
    OutputError(std::format("无法对该类型使用++运算符"));
    exit(-1);
  }
  auto right_operator_node_check_result =
      mathematical_operator_node->SetRightOperatorNode(
          constexpr_num_for_operate);
  CheckMathematicalComputeTypeResult(right_operator_node_check_result);
  // 生成赋值节点
  auto assign_operator_node = std::make_shared<AssignOperatorNode>();
  assign_operator_node->SetNodeToBeAssigned(node_to_operate);
  auto assignable_check_result = assign_operator_node->SetNodeForAssign(
      mathematical_operator_node->GetResultOperatorNode(), false);
  CheckAssignableCheckResult(assignable_check_result);
  // 生成运算流程控制节点
  auto operate_flow_control_node = std::make_unique<SimpleSentence>();
  bool result = operate_flow_control_node->SetSentenceOperateNode(
      mathematical_operator_node);
  assert(result);
  flow_control_node_container->emplace_back(
      std::move(operate_flow_control_node));
  // 生成赋值流程控制节点
  auto assign_flow_control_node = std::make_unique<SimpleSentence>();
  result =
      assign_flow_control_node->SetSentenceOperateNode(assign_operator_node);
  assert(result);
  flow_control_node_container->emplace_back(
      std::move(assign_flow_control_node));
  // 返回运算得到的节点而不是运算节点
  return mathematical_operator_node->GetResultOperatorNode();
}

std::shared_ptr<const OperatorNodeInterface> SuffixPlusOrMinus(
    MathematicalOperation mathematical_operation,
    const std::shared_ptr<const OperatorNodeInterface>& node_to_operate,
    std::list<std::unique_ptr<FlowInterface>>* flow_control_node_container) {
  // 构建中间变量节点和赋值节点
  // 复制一份变量后将原变量+1/-1（等价于执行前缀++/--）
  auto temp_variety_node = std::make_shared<VarietyOperatorNode>(
      std::string(), ConstTag::kNonConst, LeftRightValueTag::kRightValue);
  bool set_temp_variety_type_result = temp_variety_node->SetVarietyType(
      node_to_operate->GetResultTypePointer());
  assert(set_temp_variety_type_result);
  // 创建复制节点，复制源节点到临时变量
  auto copy_assign_operator_node = std::make_shared<AssignOperatorNode>();
  copy_assign_operator_node->SetNodeToBeAssigned(temp_variety_node);
  auto copy_assign_node_check_result =
      copy_assign_operator_node->SetNodeForAssign(node_to_operate, true);
  CheckAssignableCheckResult(copy_assign_node_check_result);
  auto copy_flow_control_node = std::make_unique<SimpleSentence>();
  bool result =
      copy_flow_control_node->SetSentenceOperateNode(copy_assign_operator_node);
  assert(result);
  flow_control_node_container->emplace_back(std::move(copy_flow_control_node));
  // 对源节点执行前缀++/--操作
  PrefixPlusOrMinus(mathematical_operation, node_to_operate,
                    flow_control_node_container);
  // 返回复制得到的节点
  return copy_assign_operator_node->GetResultOperatorNode();
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignablePrefixPlus(
    std::string&& str_operator,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data) {
  assert(str_operator == "++");

  auto& [node_to_plus, flow_control_node_container] = variety_data;
  return std::make_pair(
      PrefixPlusOrMinus(MathematicalOperation::kPlus, node_to_plus,
                        flow_control_node_container.get()),
      std::move(flow_control_node_container));
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignablePrefixMinus(
    std::string&& str_operator,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data) {
  assert(str_operator == "--");

  auto& [node_to_minus, flow_control_node_container] = variety_data;
  return std::make_pair(
      PrefixPlusOrMinus(MathematicalOperation::kMinus, node_to_minus,
                        flow_control_node_container.get()),
      std::move(flow_control_node_container));
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableSuffixPlus(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data,
    std::string&& str_operator) {
  assert(str_operator == "++");

  auto& [node_to_plus, flow_control_node_container] = variety_data;
  return std::make_pair(
      SuffixPlusOrMinus(MathematicalOperation::kPlus, node_to_plus,
                        flow_control_node_container.get()),
      std::move(flow_control_node_container));
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
AssignableSuffixMinus(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data,
    std::string&& str_operator) {
  assert(str_operator == "--");
  auto& [node_to_plus, flow_control_node_container] = variety_data;
  return std::make_pair(
      SuffixPlusOrMinus(MathematicalOperation::kMinus, node_to_plus,
                        flow_control_node_container.get()),
      std::move(flow_control_node_container));
}

std::nullptr_t ReturnWithValue(
    std::string&& str_return,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        variety_data,
    std::string&& str_semicolon) {
  assert(str_return == "return");
  assert(str_semicolon == ";");

  auto& [return_target, sentences_to_get_assignable] = variety_data;
  auto return_flow_control_node = std::make_unique<Return>();
  auto active_function = c_parser_controller.GetActiveFunctionPointer();
  if (active_function == nullptr) [[unlikely]] {
    OutputError(std::format("当前不处于函数内，无法返回"));
    exit(-1);
  }
  return_flow_control_node->SetReturnValue(return_target);
  bool result =
      c_parser_controller.AddSentences(std::move(*sentences_to_get_assignable));
  if (!result) [[unlikely]] {
    OutputError(std::format("生成返回值过程中存在非法操作"));
    exit(-1);
  }
  result = c_parser_controller.AddSentence(std::move(return_flow_control_node));
  if (!result) [[unlikely]] {
    OutputError(std::format(
        "函数{:}必须有返回值",
        active_function->GetFunctionTypeReference().GetFunctionName()));
    exit(-1);
  }
  return nullptr;
}

std::nullptr_t ReturnWithoutValue(std::string&& str_return,
                                  std::string&& str_semicolon) {
  assert(str_return == "return");
  assert(str_semicolon == ";");

  auto sentences_to_get_assignable =
      std::make_shared<std::list<std::unique_ptr<FlowInterface>>>();
  auto return_flow_control_node = std::make_unique<Return>();
  auto active_function = c_parser_controller.GetActiveFunctionPointer();
  if (active_function == nullptr) [[unlikely]] {
    OutputError(std::format("当前不处于函数内，无法返回"));
    exit(-1);
  }
  return_flow_control_node->SetReturnValue(nullptr);
  bool result =
      c_parser_controller.AddSentences(std::move(*sentences_to_get_assignable));
  if (!result) [[unlikely]] {
    OutputError(std::format("生成返回值过程中存在非法操作"));
    exit(-1);
  }
  result = c_parser_controller.AddSentence(std::move(return_flow_control_node));
  if (!result) [[unlikely]] {
    OutputError(std::format(
        "函数{:}必须有返回值",
        active_function->GetFunctionTypeReference().GetFunctionName()));
    exit(-1);
  }
  return nullptr;
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
TemaryOperator(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        condition,
    std::string&& str_question_mark,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        true_value,
    std::string&& str_colon,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        false_value) {
  assert(str_question_mark == "?");
  assert(str_colon == ":");

  auto& [temary_condition, condition_flow_control_node_container] = condition;
  auto& [temary_true_branch, true_branch_flow_control_node_container] =
      true_value;
  auto& [temary_false_branch, false_branch_flow_control_node_container] =
      false_value;
  // 生成三目运算符节点
  auto temary_operator_node = std::make_shared<TemaryOperatorNode>();
  bool condition_check_result = temary_operator_node->SetBranchCondition(
      temary_condition, condition_flow_control_node_container);
  if (!condition_check_result) [[unlikely]] {
    OutputError(std::format("无法将给定对象作为三目运算符条件"));
    exit(-1);
  }
  bool true_branch_check_result = temary_operator_node->SetTrueBranch(
      temary_true_branch, true_branch_flow_control_node_container);
  if (!true_branch_check_result) [[unlikely]] {
    OutputError(std::format("无法将给定对象作为三目运算符真分支结果"));
    exit(-1);
  }
  bool false_branch_check_result = temary_operator_node->SetFalseBranch(
      temary_false_branch, false_branch_flow_control_node_container);
  if (!false_branch_check_result) [[unlikely]] {
    OutputError(std::format("无法将给定对象作为三目运算符假分支结果"));
    exit(-1);
  }
  // 添加三目运算符流程控制节点
  auto temary_flow_control_node = std::make_unique<SimpleSentence>();
  bool set_sentence_check_result =
      temary_flow_control_node->SetSentenceOperateNode(temary_operator_node);
  assert(set_sentence_check_result);
  auto flow_control_node_container =
      std::make_shared<std::list<std::unique_ptr<FlowInterface>>>();
  flow_control_node_container->emplace_back(
      std::move(temary_flow_control_node));
  // 返回三目运算符处理后最终得到的节点
  return std::make_pair(temary_operator_node->GetResultOperatorNode(),
                        std::move(flow_control_node_container));
}

std::nullptr_t NotEmptyFunctionCallArgumentsBase(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        value_data) {
  auto& [argument_node, sentences_to_get_argument] = value_data;
  AssignableCheckResult check_result =
      function_call_operator_node->AddFunctionCallArgument(
          argument_node, sentences_to_get_argument);
  CheckAssignableCheckResult(check_result);
  return nullptr;
}

std::nullptr_t NotEmptyFunctionCallArgumentsExtend(
    std::nullptr_t, std::string&& str_comma,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        value_data) {
  assert(str_comma == ",");

  auto& [argument_node, sentences_to_get_argument] = value_data;
  AssignableCheckResult check_result =
      function_call_operator_node->AddFunctionCallArgument(
          argument_node, sentences_to_get_argument);
  CheckAssignableCheckResult(check_result);
  return nullptr;
}

std::nullptr_t FunctionCallArguments(std::nullptr_t) { return nullptr; }

std::pair<std::shared_ptr<FunctionCallOperatorNode>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
FunctionCallInitAssignable(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        call_target,
    std::string&& left_bracket) {
  assert(left_bracket == "(");

  auto function_call_node = std::make_shared<FunctionCallOperatorNode>();
  // 设置全局变量，以便添加函数调用实参时使用
  function_call_operator_node = function_call_node;
  auto& [node_to_call, sentences_to_get_node_to_call] = call_target;
  // 检查是否为函数类型或一重函数指针，如果是指针则做一次解引用
  // 同时设置函数调用节点用来调用的对象
  if (node_to_call->GetResultTypePointer()->GetType() !=
      StructOrBasicType::kFunction) {
    auto dereference_operator_node =
        std::make_shared<DereferenceOperatorNode>();
    bool result = dereference_operator_node->SetNodeToDereference(node_to_call);
    if (!result ||
        dereference_operator_node->GetResultTypePointer()->GetType() !=
            StructOrBasicType::kFunction) [[unlikely]] {
      OutputError(
          std::format("用来调用函数的对象既不是函数名也不是一重函数指针"));
      exit(-1);
    }
    // 包装解引用节点
    auto dereference_flow_control_node = std::make_unique<SimpleSentence>();
    result = dereference_flow_control_node->SetSentenceOperateNode(
        dereference_operator_node);
    assert(result);
    // 添加解引用节点
    sentences_to_get_node_to_call->emplace_back(
        std::move(dereference_flow_control_node));
    // 设置调用的节点为解引用后得到的节点
    result = function_call_operator_node->SetFunctionType(
        dereference_operator_node->GetResultOperatorNode());
    assert(result);
  } else {
    // 给定节点可以直接调用
    bool result = function_call_operator_node->SetFunctionType(node_to_call);
    assert(result);
  }

  return std::make_pair(std::move(function_call_node),
                        std::move(sentences_to_get_node_to_call));
}

std::pair<std::shared_ptr<FunctionCallOperatorNode>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
FunctionCallInitId(std::string&& function_name,
                   std::string&& str_left_bracket) {
  assert(str_left_bracket == "(");

  auto function_call_node = std::make_shared<FunctionCallOperatorNode>();
  // 设置全局变量，以便添加函数调用实参时使用
  function_call_operator_node = function_call_node;
  auto [node_to_call, exist] =
      c_parser_controller.GetVarietyOrFunction(function_name);
  if (!exist) {
    OutputError(
        std::format("不存在名为{:}的函数或一级函数指针", function_name));
    exit(-1);
  }
  // 检查是否为函数类型或一重函数指针，如果是指针则做一次解引用
  // 同时设置函数调用节点用来调用的对象
  if (node_to_call->GetResultTypePointer()->GetType() !=
      StructOrBasicType::kFunction) {
    auto dereference_operator_node =
        std::make_shared<DereferenceOperatorNode>();
    bool result = dereference_operator_node->SetNodeToDereference(node_to_call);
    if (!result ||
        dereference_operator_node->GetResultTypePointer()->GetType() !=
            StructOrBasicType::kFunction) [[unlikely]] {
      OutputError(
          std::format("用来调用函数的对象既不是函数名也不是一重函数指针"));
      exit(-1);
    }
    // 包装解引用节点
    auto dereference_flow_control_node = std::make_unique<SimpleSentence>();
    result = dereference_flow_control_node->SetSentenceOperateNode(
        dereference_operator_node);
    assert(result);
    // 添加解引用节点
    result = c_parser_controller.AddSentence(
        std::move(dereference_flow_control_node));
    assert(result);
    // 设置调用的节点类型为解引用后得到的函数类型
    result = function_call_operator_node->SetFunctionType(
        dereference_operator_node->GetResultOperatorNode());
    assert(result);
  } else {
    // 给定节点可以直接调用
    bool result = function_call_operator_node->SetFunctionType(node_to_call);
    assert(result);
  }

  return std::make_pair(
      std::move(function_call_node),
      std::make_shared<std::list<std::unique_ptr<FlowInterface>>>());
}

std::pair<std::shared_ptr<const OperatorNodeInterface>,
          std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>
FunctionCall(
    std::pair<std::shared_ptr<FunctionCallOperatorNode>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        function_call_data,
    std::nullptr_t, std::string&& right_bracket) {
  assert(right_bracket == ")");

  auto& [node_to_call, sentences_to_get_node_to_call] = function_call_data;
  // 创建流程控制节点并添加
  auto function_call_flow_control_node = std::make_unique<SimpleSentence>();
  bool result = function_call_flow_control_node->SetSentenceOperateNode(
      function_call_operator_node);
  assert(result);
  sentences_to_get_node_to_call->emplace_back(
      std::move(function_call_flow_control_node));
  // 返回函数调用得到的节点
  return std::make_pair(function_call_operator_node->GetResultOperatorNode(),
                        std::move(sentences_to_get_node_to_call));
}

std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>&& AssignablesBase(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        value_data) {
  return std::move(value_data.second);
}

std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>&& AssignablesExtend(
    std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>&&
        main_control_node_container,
    std::string&& str_comma,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        value_data) {
  assert(str_comma == ",");

  // 当前Assignable产生的控制节点的容器
  auto& [ignore_assignable, now_control_node_container] = value_data;
  // 合并到主容器中
  main_control_node_container->splice(main_control_node_container->end(),
                                      std::move(*now_control_node_container));
  return std::move(main_control_node_container);
}

std::shared_ptr<std::unique_ptr<Jmp>> Break(std::string&& str_break,
                                            std::string&& str_semicolon) {
  assert(str_break == "break");
  assert(str_semicolon == ";");

  auto& top_flow_control_sentence = static_cast<ConditionBlockInterface&>(
      c_parser_controller.GetTopFlowControlSentence());
  switch (top_flow_control_sentence.GetFlowType()) {
    case FlowType::kDoWhileSentence:
    case FlowType::kWhileSentence:
    case FlowType::kForSentence:
    case FlowType::kSwitchSentence:
      return std::make_shared<std::unique_ptr<Jmp>>(std::make_unique<Jmp>(
          top_flow_control_sentence.GetSentenceEndLabel()));
      break;
    default:
      OutputError(std::format("无法跳出非for/while/do-while/switch语句"));
      exit(-1);
      // 防止警告
      return std::shared_ptr<std::unique_ptr<Jmp>>();
      break;
  }
}

std::shared_ptr<std::unique_ptr<Jmp>> Continue(std::string&& str_continue,
                                               std::string&& str_semicolon) {
  assert(str_continue == "continue");
  assert(str_semicolon == ";");

  auto& top_flow_control_sentence = static_cast<LoopSentenceInterface&>(
      c_parser_controller.GetTopFlowControlSentence());
  switch (top_flow_control_sentence.GetFlowType()) {
    case FlowType::kDoWhileSentence:
    case FlowType::kWhileSentence:
    case FlowType::kForSentence:
      return std::make_shared<std::unique_ptr<Jmp>>(std::make_unique<Jmp>(
          top_flow_control_sentence.GetLoopMainBlockEndLabel()));
      break;
    default:
      OutputError(std::format("无法在非for/while/do-while语句中使用continue"));
      exit(-1);
      // 防止警告
      return std::shared_ptr<std::unique_ptr<Jmp>>();
      break;
  }
}

std::nullptr_t SingleStatementIf(std::nullptr_t) { return nullptr; }

std::nullptr_t SingleStatementDoWhile(std::nullptr_t) { return nullptr; }

std::nullptr_t SingleStatementWhile(std::nullptr_t) { return nullptr; }

std::nullptr_t SingleStatementFor(std::nullptr_t) { return nullptr; }

std::nullptr_t SingleStatementSwitch(std::nullptr_t) { return nullptr; }

std::nullptr_t SingleStatementAssignable(
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        expression,
    std::string&& str_semicolon) {
  assert(str_semicolon == ";");

  auto& [ignore_assignable, flow_control_node_container] = expression;
  bool result =
      c_parser_controller.AddSentences(std::move(*flow_control_node_container));
  if (!result) [[unlikely]] {
    OutputError(std::format("此语句不应出现在该范围内"));
    exit(-1);
  }
  return nullptr;
}

std::nullptr_t SingleStatementAnnounce(
    std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
               std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        announce_statement,
    std::string&& str_semicolon) {
  assert(str_semicolon == ";");

  auto& [ignore_type, ignore_const_tag, flow_control_node_container] =
      announce_statement;
  bool result =
      c_parser_controller.AddSentences(std::move(*flow_control_node_container));
  if (!result) [[unlikely]] {
    OutputError(std::format("此语句不应出现在该范围内"));
    exit(-1);
  }
  return nullptr;
}

std::nullptr_t SingleStatementReturn(std::nullptr_t) { return nullptr; }

std::nullptr_t SingleStatementBreak(
    std::shared_ptr<std::unique_ptr<Jmp>>&& jmp_sentence) {
  bool result = c_parser_controller.AddSentence(std::move(*jmp_sentence));
  // 所有报错应在这步规约前进行
  assert(result);
  return nullptr;
}

std::nullptr_t SingleStatementContinue(
    std::shared_ptr<std::unique_ptr<Jmp>>&& jmp_sentence) {
  bool result = c_parser_controller.AddSentence(std::move(*jmp_sentence));
  // 所有报错应在这步规约前进行
  assert(result);
  return nullptr;
}

std::nullptr_t SingleStatementEmptyStatement(std::string&& str_semicolon) {
  assert(str_semicolon == ";");

  return nullptr;
}

std::nullptr_t IfCondition(
    std::string&& str_if, std::string&& str_left_bracket,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        condition,
    std::string&& str_right_bracket) {
  assert(str_if == "if");
  assert(str_left_bracket == "(");
  assert(str_right_bracket == ")");

  // if条件和获取条件的操作
  auto& [if_condition, sentences_to_get_if_condition] = condition;
  auto if_flow_control_node = std::make_unique<IfSentence>();
  bool result = if_flow_control_node->SetCondition(
      if_condition, std::move(*sentences_to_get_if_condition));
  if (!result) [[unlikely]] {
    OutputError(std::format("该条件无法作为if语句条件"));
    exit(-1);
  }
  bool push_result = c_parser_controller.PushFlowControlSentence(
      std::move(if_flow_control_node));
  if (!push_result) [[unlikely]] {
    std::cerr << std::format(
                     "行数{:} 列数{:} 流程控制语句必须在函数定义内部使用",
                     GetLine(), GetColumn())
              << std::endl;
    exit(-1);
  }
  return nullptr;
}

std::nullptr_t IfWithElse(std::nullptr_t, std::nullptr_t,
                          std::string str_else) {
  assert(str_else == "else");

  // 转换为if-else语句
  c_parser_controller.ConvertIfSentenceToIfElseSentence();
  return nullptr;
}

std::nullptr_t IfElseSence(std::nullptr_t, std::nullptr_t) {
  c_parser_controller.PopActionScope();
  return nullptr;
}

std::nullptr_t IfIfSentence(std::nullptr_t, std::nullptr_t) {
  c_parser_controller.PopActionScope();
  return nullptr;
}

std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>&& ForRenewSentence(
    std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>&& expression) {
  return std::move(expression);
}

std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>&&
ForInitSentenceAssignables(
    std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>&& expression) {
  return std::move(expression);
}

std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>
ForInitSentenceAnnounce(
    std::tuple<std::shared_ptr<const TypeInterface>, ConstTag,
               std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        announce_data) {
  auto& [ignore_extend_type, ignore_extend_const_tag,
         flow_control_node_container] = announce_data;
  return std::move(flow_control_node_container);
}

std::nullptr_t ForInitHead(std::string&& str_for) {
  assert(str_for == "for");

  bool push_result = c_parser_controller.PushFlowControlSentence(
      std::make_unique<ForSentence>());
  if (!push_result) [[unlikely]] {
    std::cerr << std::format(
                     "行数{:} 列数{:} 流程控制语句必须在函数定义内部使用",
                     GetLine(), GetColumn())
              << std::endl;
    exit(-1);
  }
  return nullptr;
}

std::nullptr_t ForHead(
    std::nullptr_t, std::string&& str_left_bracket,
    std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>&&
        for_init_sentences,
    std::string&& str_semicolon1,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        condition,
    std::string&& str_semicolon2,
    std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>&&
        for_renew_sentences,
    std::string&& str_right_bracket) {
  assert(str_left_bracket == "(");
  assert(str_semicolon1 == ";");
  assert(str_semicolon2 == ";");
  assert(str_right_bracket == ")");

  auto& for_sentence = static_cast<ForSentence&>(
      c_parser_controller.GetTopFlowControlSentence());
  assert(for_sentence.GetFlowType() == FlowType::kForSentence);
  auto& [for_condition, sentences_to_get_for_condition] = condition;
  bool result =
      for_sentence.AddForInitSentences(std::move(*for_init_sentences));
  if (!result) [[unlikely]] {
    OutputError(std::format("给定语句无法作为for语句初始化条件"));
    exit(-1);
  }
  result = for_sentence.SetCondition(
      for_condition, std::move(*sentences_to_get_for_condition));
  if (!result) [[unlikely]] {
    OutputError(std::format("给定语句无法作为for语句循环条件"));
    exit(-1);
  }
  result = for_sentence.AddForRenewSentences(std::move(*for_renew_sentences));
  if (!result) [[unlikely]] {
    OutputError(std::format("给定语句无法用在for语句中更新循环条件"));
    exit(-1);
  }
  return nullptr;
}

std::nullptr_t For(std::nullptr_t, std::nullptr_t) {
  c_parser_controller.PopActionScope();
  return nullptr;
}

std::nullptr_t WhileInitHead(
    std::string&& str_while, std::string&& str_left_bracket,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        condition,
    std::string&& str_right_bracket) {
  assert(str_while == "while");
  assert(str_left_bracket == "(");
  assert(str_right_bracket == ")");

  auto& [while_condition, sentences_to_get_condition] = condition;
  auto pointer_to_while_sentence =
      std::make_shared<std::unique_ptr<WhileSentence>>(
          std::make_unique<WhileSentence>());
  auto& while_sentence = *pointer_to_while_sentence;
  bool result = while_sentence->SetCondition(
      while_condition, std::move(*sentences_to_get_condition));
  if (!result) [[unlikely]] {
    OutputError(std::format("给定条件无法作为while循环语句条件"));
    exit(-1);
  }
  bool push_result =
      c_parser_controller.PushFlowControlSentence(std::move(while_sentence));
  if (!push_result) [[unlikely]] {
    std::cerr << std::format(
                     "行数{:} 列数{:} 流程控制语句必须在函数定义内部使用",
                     GetLine(), GetColumn())
              << std::endl;
    exit(-1);
  }
  return nullptr;
}

std::nullptr_t While(std::nullptr_t, std::nullptr_t) {
  c_parser_controller.PopActionScope();
  return nullptr;
}

std::nullptr_t DoWhileInitHead(std::string&& str_do) {
  assert(str_do == "do");

  bool push_result = c_parser_controller.PushFlowControlSentence(
      std::make_unique<DoWhileSentence>());
  if (!push_result) [[unlikely]] {
    std::cerr << std::format(
                     "行数{:} 列数{:} 流程控制语句必须在函数定义内部使用",
                     GetLine(), GetColumn())
              << std::endl;
    exit(-1);
  }
  return nullptr;
}

std::nullptr_t DoWhile(
    std::nullptr_t, std::nullptr_t, std::string&& str_while,
    std::string&& str_left_bracket,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        condition,
    std::string&& str_right_bracket, std::string&& str_semicolon) {
  assert(str_while == "while");
  assert(str_left_bracket == "(");
  assert(str_right_bracket == ")");
  assert(str_semicolon == ";");

  auto& [assignable, sentences_to_get_assignable] = condition;
  DoWhileSentence& do_while_sentence = static_cast<DoWhileSentence&>(
      c_parser_controller.GetTopFlowControlSentence());
  assert(do_while_sentence.GetFlowType() == FlowType::kDoWhileSentence);
  // 设置do-while语句条件
  do_while_sentence.SetCondition(assignable,
                                 std::move(*sentences_to_get_assignable));
  c_parser_controller.PopActionScope();
  return nullptr;
}

std::nullptr_t SwitchCaseSimple(
    std::string&& str_case,
    std::shared_ptr<BasicTypeInitializeOperatorNode>&& case_data,
    std::string&& str_colon) {
  assert(str_case == "case");
  assert(str_colon == ":");

  bool result = c_parser_controller.AddSwitchSimpleCase(case_data);
  if (!result) [[unlikely]] {
    OutputError(std::format(
        "无法添加给定的case选项，可能是不位于switch语句内或case条件已存在"));
    exit(-1);
  }
  return nullptr;
}

std::nullptr_t SwitchCaseDefault(std::string&& str_default,
                                 std::string&& str_colon) {
  assert(str_default == "default");
  assert(str_colon == ":");

  bool result = c_parser_controller.AddSwitchDefaultCase();
  if (!result) [[unlikely]] {
    OutputError(std::format(
        "无法添加default标签，可能不位于switch语句内或已存在default标签"));
    exit(-1);
  }
  return nullptr;
}

std::nullptr_t SingleSwitchStatementCase(std::nullptr_t) { return nullptr; }

std::nullptr_t SingleSwitchStatementStatements(std::nullptr_t) {
  return nullptr;
}

std::nullptr_t SwitchStatements(std::nullptr_t, std::nullptr_t) {
  return nullptr;
}

std::nullptr_t SwitchCondition(
    std::string&& str_switch, std::string&& str_left_bracket,
    std::pair<std::shared_ptr<const OperatorNodeInterface>,
              std::shared_ptr<std::list<std::unique_ptr<FlowInterface>>>>&&
        condition,
    std::string&& str_right_bracket) {
  assert(str_switch == "switch");
  assert(str_left_bracket == "(");
  assert(str_right_bracket == ")");

  auto& [assignable, sentences_to_get_assignable] = condition;
  auto switch_sentence = std::make_unique<SwitchSentence>();
  bool result = switch_sentence->SetCondition(
      assignable, std::move(*sentences_to_get_assignable));
  if (!result) [[unlikely]] {
    OutputError(std::format("给定条件无法作为switch分支条件"));
    exit(-1);
  }
  bool push_result =
      c_parser_controller.PushFlowControlSentence(std::move(switch_sentence));
  if (!push_result) [[unlikely]] {
    std::cerr << std::format(
                     "行数{:} 列数{:} 流程控制语句必须在函数定义内部使用",
                     GetLine(), GetColumn())
              << std::endl;
    exit(-1);
  }
  return nullptr;
}

std::nullptr_t Switch(std::nullptr_t, std::string&& str_left_curly_bracket,
                      std::nullptr_t, std::string&& str_right_curly_bracket) {
  assert(str_left_curly_bracket == "{");
  assert(str_right_curly_bracket == "}");

  return nullptr;
}

std::nullptr_t StatementsSingleStatement(std::nullptr_t, std::nullptr_t) {
  return nullptr;
}

std::nullptr_t StatementsLeftBrace(std::nullptr_t,
                                   std::string&& left_curly_bracket) {
  assert(left_curly_bracket == "{");
  c_parser_controller.AddActionScopeLevel();
  return nullptr;
}

std::nullptr_t StatementsBrace(std::nullptr_t, std::nullptr_t,
                               std::string&& right_curly_bracket) {
  assert(right_curly_bracket == "}");
  c_parser_controller.PopActionScope();
  return nullptr;
}

std::nullptr_t ProcessControlSentenceBodySingleStatement(std::nullptr_t) {
  return nullptr;
}

std::nullptr_t ProcessControlSentenceBodyStatements(
    std::string&& str_left_curly_bracket, std::nullptr_t,
    std::string&& str_right_curly_bracket) {
  assert(str_left_curly_bracket == "{");
  assert(str_right_curly_bracket == "}");

  return nullptr;
}

std::nullptr_t RootFunctionDefine(std::nullptr_t, std::nullptr_t) {
  return nullptr;
}

std::nullptr_t RootAnnounce(std::nullptr_t,
                            std::shared_ptr<FlowInterface>&& flow_control_node,
                            std::string&& str_colon) {
  assert(str_colon == ";");

  switch (flow_control_node->GetFlowType()) {
    case FlowType::kSimpleSentence: {
      // 全局变量定义
      auto variety_operator_node =
          std::static_pointer_cast<VarietyOperatorNode>(
              static_cast<SimpleSentence&>(*flow_control_node)
                  .GetSentenceOperateNodePointer());
      // 添加变量定义
      c_parser_controller.DefineVariety(variety_operator_node);
    } break;
    case FlowType::kFunctionDefine: {
      // 函数声明
      auto [ignore_iter, announce_result] =
          c_parser_controller.AnnounceFunction(
              static_cast<c_parser_frontend::flow_control::FunctionDefine&>(
                  *flow_control_node)
                  .GetFunctionTypePointer());
      CheckAddTypeResult(announce_result);
    } break;
    default:
      assert(false);
      break;
  }
  return nullptr;
}

ObjectConstructData::CheckResult
ObjectConstructData::AttachSingleNodeToTailNodePointer(
    std::shared_ptr<const TypeInterface>&& next_node) {
  switch (type_chain_tail_->GetType()) {
    case StructOrBasicType::kFunction:
      // 函数指针构造语义，将新插入的节点放在返回值的位置
      if (next_node->GetType() == StructOrBasicType::kFunction) {
        // 待插入的第一个返回值节点为函数
        // 函数不能返回函数
        return CheckResult::kReturnFunction;
      }
      static_cast<FunctionType&>(*type_chain_tail_)
          .SetReturnTypePointer(next_node);
      // 此处为了构建类型链违反const原则
      type_chain_tail_ = std::const_pointer_cast<TypeInterface>(next_node);
      break;
    case StructOrBasicType::kPointer:
      // 需要额外检查是否在声明函数数组
      if (std::static_pointer_cast<PointerType>(type_chain_tail_)
                  ->GetArraySize() != 0 &&
          next_node->GetType() == StructOrBasicType::kFunction) [[unlikely]] {
        // 非0代表声明数组
        OutputError(std::format("不支持声明函数数组"));
        exit(-1);
      }
      [[fallthrough]];
    case StructOrBasicType::kEnd:
      // 普通节点构造语义，在原来的结构下连接
      type_chain_tail_->SetNextNode(next_node);
      // 此处为了构建类型链违反const原则
      type_chain_tail_ = std::const_pointer_cast<TypeInterface>(next_node);
      break;
    case StructOrBasicType::kBasic:
    case StructOrBasicType::kStruct:
    case StructOrBasicType::kUnion:
    case StructOrBasicType::kEnum:
      // 这些为终结节点，不能连接下一个节点
      return CheckResult::kAttachToTerminalType;
      break;
    default:
      assert(false);
      break;
  }
  return CheckResult::kSuccess;
}

std::pair<std::unique_ptr<FlowInterface>, ObjectConstructData::CheckResult>
ObjectConstructData::ConstructObject(
    ConstTag const_tag_before_final_type,
    std::shared_ptr<const TypeInterface>&& final_node_to_attach) {
  switch (type_chain_tail_->GetType()) {
    case StructOrBasicType::kPointer: {
      // 类型链末端为指针，设置该指针的const_tag
      auto& pointer_type = static_cast<PointerType&>(*type_chain_tail_);
      if (pointer_type.GetConstTag() == ConstTag::kConst &&
          const_tag_before_final_type == ConstTag::kConst) [[unlikely]] {
        OutputWarning(std::format("在类型两侧重复定义const"));
      }
      pointer_type.SetConstTag(const_tag_before_final_type);
    } break;
    case StructOrBasicType::kFunction:
      // 这种情况下函数返回值类型只有一个有效类型节点（final_node_to_attach）
      // 合法的语义下这个节点应该是基础类型
      static_cast<FunctionType&>(*type_chain_tail_)
          .SetReturnTypeConstTag(const_tag_before_final_type);
      break;
    case StructOrBasicType::kEnd:
      // 之前未创建任何类型节点
      assert(static_cast<SimpleSentence&>(*object_)
                 .GetSentenceOperateNodeReference()
                 .GetGeneralOperatorType() == GeneralOperationType::kVariety);
      switch (final_node_to_attach->GetType()) {
        case StructOrBasicType::kBasic:
          // 声明POD变量
          // 不能声明void类型的变量
          if (static_cast<const BasicType&>(*final_node_to_attach)
                  .GetBuiltInType() == BuiltInType::kVoid) [[unlikely]] {
            OutputError(
                std::format("变量{:}不能声明为\"void\"", GetObjectName()));
            exit(-1);
          }
          // 检查声明的变量的ConstTag与最终构建时的ConstTag是否相同
          if (const_tag_before_final_type == ConstTag::kConst &&
              static_cast<const VarietyOperatorNode&>(
                  static_cast<SimpleSentence&>(*object_)
                      .GetSentenceOperateNodeReference())
                      .GetConstTag() == const_tag_before_final_type)
              [[unlikely]] {
            OutputWarning(std::format("在类型两侧重复定义const"));
          }
          break;
        case StructOrBasicType::kFunction:
          OutputError(std::format("函数{:}未声明返回值", GetObjectName()));
          exit(-1);
          break;
        case StructOrBasicType::kStruct:
        case StructOrBasicType::kUnion:
        case StructOrBasicType::kEnum:
          break;
        [[unlikely]] default:
          assert(false);
          break;
      }
      static_cast<VarietyOperatorNode&>(static_cast<SimpleSentence&>(*object_)
                                            .GetSentenceOperateNodeReference())
          .SetConstTag(const_tag_before_final_type);
      break;
    default:
      // 错误的声明格式，在连接下一个节点时应报错
      break;
  }
  AttachSingleNodeToTailNodePointer(std::move(final_node_to_attach));
  switch (type_chain_tail_->GetType()) {
    [[unlikely]] case StructOrBasicType::kPointer:
      // 未完全构建类型链导致类型链以指针结尾
      return std::make_pair(std::unique_ptr<FlowInterface>(),
                            CheckResult::kPointerEnd);
      break;
    [[unlikely]] case StructOrBasicType::kEnd:
      // 空类型链
      return std::make_pair(std::unique_ptr<FlowInterface>(),
                            CheckResult::kEmptyChain);
      break;
    default:
      break;
  }
  // 跳过哨兵节点
  auto final_type_chain = type_chain_head_->GetNextNodePointer();
  // 判断是否为函数头
  if (final_type_chain->GetType() == StructOrBasicType::kFunction)
      [[unlikely]] {
    // 要构建的类型为函数，将object_从流程类型——基础非流程
    // 转换为流程类型——函数定义

    // 获取最终类型链
    // 为了构建类型链违反const原则
    auto function_type_chain = std::const_pointer_cast<FunctionType>(
        std::static_pointer_cast<const FunctionType>(final_type_chain));
    // 获取变量名指针后设置函数名
    function_type_chain->SetFunctionName(std::move(GetObjectName()));
    // 构建函数定义（FunctionDefine流程）
    ConstructBasicObjectPart<c_parser_frontend::flow_control::FunctionDefine>(
        std::move(function_type_chain));
  } else {
    // 要构建的类型为变量，无需额外转换
    auto& variety_node = static_cast<VarietyOperatorNode&>(
        static_cast<SimpleSentence&>(*object_)
            .GetSentenceOperateNodeReference());
    // 设置变量的类型
    variety_node.SetVarietyType(
        std::move(type_chain_head_->GetNextNodePointer()));
    // 设置变量名
    variety_node.SetVarietyName(std::move(GetObjectName()));
  }
  return std::make_pair(std::move(object_), CheckResult::kSuccess);
}

}  // namespace c_parser_frontend::parse_functions