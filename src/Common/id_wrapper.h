/// @file id_wrapper.h
/// @brief ID包装器，包装ID以避免ID混用导致运行时错误
/// @details
/// 该头文件包含boost-serialization库的头文件，从而允许被序列化
/// 该头文件定义了四个生成不同的ID包装类的模板类
/// 占用空间与不包装相同，编译器优化够好则不会导致额外的开销
/// 无参构造函数会将值赋为default_invalid_value且IsValid()返回false
/// default_invalid_value默认为IdType(-1)
/// 如果需要使用boost序列化功能，请包含头文件：id_wrapper_serializer.h
/// TODO 特化std::swap
#ifndef COMMON_ID_WRAPPER_H_
#define COMMON_ID_WRAPPER_H_

#include <algorithm>

namespace frontend::common {

/// @class BaseIdWrapper id_wrapper.h
/// @attention 基础ID包装器类，不应直接使用
/// @param[in] IdType_ ： 要包装的ID的实际类型
/// @param[in] LabelEnum_ ：分发标签的枚举类型，与label_共同决定唯一的ID
/// @param[in] label_
/// ：分发标签枚举类型中的具体枚举，与LabelEnum_共同决定唯一的ID类型
/// @param[in] invalid_value_ ：第四个参数为无效值
/// @details
/// 使用例：
///  enum class ExampleEnum { kExampleType };
///  using Id = BaseIdWrapper<size_t,ExampleEnum,ExampleEnum::kExampleType,
///                               static_cast<size_t>(-1)>;
template <class IdType_, class LabelEnum_, const LabelEnum_ label_,
          const IdType_ invalid_value_>
class BaseIdWrapper {
 public:
  /// @brief 提供使用的ID原始类型
  using IdType = IdType_;
  /// @brief 提供定义时使用的枚举类型
  using LabelEnum = LabelEnum_;
  /// @brief 提供定义时使用的枚举中具体枚举
  static constexpr LabelEnum_ label = label_;

  BaseIdWrapper() : id_(invalid_value_) {}
  explicit BaseIdWrapper(IdType production_node_id) : id_(production_node_id) {}
  BaseIdWrapper(const BaseIdWrapper& id_wrapper) : id_(id_wrapper.id_) {}
  BaseIdWrapper& operator=(const BaseIdWrapper& id_wrapper) {
    id_ = id_wrapper.id_;
    return *this;
  }
  virtual ~BaseIdWrapper() {}

  operator IdType&() { return const_cast<IdType&>(operator const IdType&()); }
  operator const IdType&() const { return id_; }

  BaseIdWrapper& operator++() {
    ++id_;
    return *this;
  }
  BaseIdWrapper operator++(int) { return BaseIdWrapper(id_++); }
  bool operator==(const BaseIdWrapper& id_wrapper) const {
    return id_ == id_wrapper.id_;
  }
  bool operator!=(const BaseIdWrapper& id_wrapper) const {
    return !operator==(id_wrapper);
  }
  /// @brief 获取未包装的原始值
  /// @return 原始类型的原始值
  IdType GetRawValue() const { return id_; }
  /// @brief 设置使用的ID
  /// @param[in] production_node_id ：要保存的原始ID
  void SetId(IdType production_node_id) { id_ = production_node_id; }
  /// @brief 交换两个ID容器
  /// @param[in] id_wrapper ：指向要交换的容器的指针
  /// @note 允许传递this作为参数
  void Swap(BaseIdWrapper* id_wrapper) { std::swap(id_, id_wrapper->id_); }
  /// @brief 判断当前存储的值是否有效
  bool IsValid() const { return id_ != invalid_value_; }
  /// @brief 获取原始的无效值
  /// @return 原始类型的无效值
  static constexpr IdType InvalidValue() { return invalid_value_; }
  /// @brief 获取存储无效值的容器
  /// @return 存储无效值的容器，无效值同InvalidValue
  static constexpr BaseIdWrapper InvalidId() {
    return BaseIdWrapper<IdType, LabelEnum, label, InvalidValue()>(
        InvalidValue());
  }

 private:
  /// @var id_
  /// @brief ID的值
  IdType id_;
};

/// @class ExplicitIdWrapper id_wrapper.h
/// @brief 强制显式调用构造函数且使用默认无效值版Wrapper
/// @param[in] IdType_ ： 要包装的ID的实际类型
/// @param[in] LabelEnum_ ：分发标签的枚举类型，与label_共同决定唯一的ID
/// @param[in] label_
/// ：分发标签枚举类型中的具体枚举，与LabelEnum_共同决定唯一的ID类型
/// @details
/// 使用例：
///  enum class ExampleEnum { kExampleType };
///  using Id = ExplicitIdWrapper<size_t,ExampleEnum,ExampleEnum::kExampleType>;
template <class IdType_, class LabelEnum_, const LabelEnum_ label_>
class ExplicitIdWrapper : public BaseIdWrapper<IdType_, LabelEnum_, label_,
                                               static_cast<IdType_>(-1)> {
 private:
  /// @brief 使用的基类
  using MyBase =
      BaseIdWrapper<IdType_, LabelEnum_, label_, static_cast<IdType_>(-1)>;

 public:
  /// @brief 提供使用的ID原始类型
  using IdType = MyBase::IdType;
  /// @brief 提供定义时使用的枚举类型
  using LabelEnum = MyBase::LabelEnum;
  /// @brief 提供定义时使用的枚举中具体枚举
  static const LabelEnum label = MyBase::label;

  ExplicitIdWrapper() {}
  explicit ExplicitIdWrapper(IdType production_node_id) {
    MyBase::SetId(production_node_id);
  }
  ExplicitIdWrapper(const MyBase& id_wrapper) : MyBase(id_wrapper) {}
  ExplicitIdWrapper& operator=(const MyBase& id_wrapper) {
    MyBase::operator=(id_wrapper);
    return *this;
  }

  ExplicitIdWrapper& operator++() {
    return static_cast<ExplicitIdWrapper&>(++static_cast<MyBase&>(*this));
  }
  ExplicitIdWrapper operator++(int) {
    return static_cast<ExplicitIdWrapper>(static_cast<MyBase>(*this)++);
  }
  bool operator==(const ExplicitIdWrapper& id_wrapper) {
    return MyBase::operator==(id_wrapper);
  }
  bool operator!=(const ExplicitIdWrapper& id_wrapper) {
    return MyBase::operator!=(id_wrapper);
  }
  /// @brief 获取存储无效值的容器
  /// @return 存储无效值的容器，无效值同InvalidValue
  static constexpr ExplicitIdWrapper InvalidId() {
    return ExplicitIdWrapper<IdType, LabelEnum, label>(MyBase::InvalidValue());
  }
};

/// @class ExplicitIdWrapperCustomizeInvalidValue id_wrapper.h
/// @brief强制显示调用构造函数不使用默认无效值版
/// @param[in] IdType_ ： 要包装的ID的实际类型
/// @param[in] LabelEnum_ ：分发标签的枚举类型，与label_共同决定唯一的ID
/// @param[in] label_
/// ：分发标签枚举类型中的具体枚举，与LabelEnum_共同决定唯一的ID类型
/// @param[in] invalid_value_ ：自定义的无效值
/// 例：
///  enum class ExampleEnum { kExampleType };
///  using Id = ExplicitIdWrapper<size_t,ExampleEnum,ExampleEnum::kExampleType>;
template <class IdType_, class LabelEnum_, const LabelEnum_ label_,
          const IdType_ invalid_value_>
class ExplicitIdWrapperCustomizeInvalidValue
    : public BaseIdWrapper<IdType_, LabelEnum_, label_, invalid_value_> {
 private:
  /// @brief 使用的基类
  using MyBase = BaseIdWrapper<IdType_, LabelEnum_, label_, invalid_value_>;

 public:
  using IdType = MyBase::IdType;
  using LabelEnum = MyBase::LabelEnum;
  static const LabelEnum label = MyBase::label;

  ExplicitIdWrapperCustomizeInvalidValue() {}
  explicit ExplicitIdWrapperCustomizeInvalidValue(IdType production_node_id) {
    MyBase::SetId(production_node_id);
  }
  ExplicitIdWrapperCustomizeInvalidValue(
      const ExplicitIdWrapperCustomizeInvalidValue& id_wrapper) {
    MyBase::SetId(id_wrapper.GetThisNodeId());
  }
  ExplicitIdWrapperCustomizeInvalidValue& operator=(
      const ExplicitIdWrapperCustomizeInvalidValue& id_wrapper) {
    MyBase::SetId(id_wrapper.GetThisNodeId());
    return *this;
  }

  ExplicitIdWrapperCustomizeInvalidValue& operator++() {
    return static_cast<ExplicitIdWrapperCustomizeInvalidValue&>(
        ++static_cast<MyBase&>(*this));
  }
  ExplicitIdWrapperCustomizeInvalidValue operator++(int) {
    return static_cast<ExplicitIdWrapperCustomizeInvalidValue>(
        static_cast<MyBase&>(*this)++);
  }
  bool operator==(const ExplicitIdWrapperCustomizeInvalidValue& id_wrapper) {
    return MyBase::operator==(id_wrapper);
  }
  bool operator!=(const ExplicitIdWrapperCustomizeInvalidValue& id_wrapper) {
    return MyBase::operator!=(id_wrapper);
  }
  /// @brief 获取存储无效值的容器
  /// @return 存储无效值的容器，无效值同InvalidValue
  static constexpr ExplicitIdWrapperCustomizeInvalidValue InvalidId() {
    return ExplicitIdWrapperCustomizeInvalidValue<IdType, LabelEnum, label,
                                                  static_cast<IdType>(-1)>(
        MyBase::InvalidValue());
  }
};

/// @class NonExplicitIdWrapper id_wrapper.h
/// @brief 不强制显示调用构造函数使用默认无效值版
/// @param[in] IdType_ ： 要包装的ID的实际类型
/// @param[in] LabelEnum_ ：分发标签的枚举类型，与label_共同决定唯一的ID
/// @param[in] label_
/// ：分发标签枚举类型中的具体枚举，与LabelEnum_共同决定唯一的ID类型
/// 例：
///  enum class ExampleEnum { kExampleType };
///  using Id = NonExplicitIdWrapper<size_t,ExampleEnum,
///                                  ExampleEnum::kExampleType>;
template <class IdType_, class LabelEnum_, const LabelEnum_ label_>
class NonExplicitIdWrapper : public BaseIdWrapper<IdType_, LabelEnum_, label_,
                                                  static_cast<IdType_>(-1)> {
 private:
  /// @brief 使用的基类
  using MyBase =
      BaseIdWrapper<IdType_, LabelEnum_, label_, static_cast<IdType_>(-1)>;

 public:
  /// @brief 提供使用的ID原始类型
  using IdType = MyBase::IdType;
  /// @brief 提供定义时使用的枚举类型
  using LabelEnum = MyBase::LabelEnum;
  /// @brief 提供定义时使用的枚举中具体枚举
  static const LabelEnum label = MyBase::label;

  NonExplicitIdWrapper() {}
  NonExplicitIdWrapper(IdType production_node_id) {
    MyBase::SetId(production_node_id);
  }
  NonExplicitIdWrapper(const NonExplicitIdWrapper& id_wrapper) {
    MyBase::SetId(id_wrapper.GetThisNodeId());
  }
  NonExplicitIdWrapper& operator=(const NonExplicitIdWrapper& id_wrapper) {
    MyBase::SetId(id_wrapper.GetThisNodeId());
    return *this;
  }

  NonExplicitIdWrapper& operator++() {
    return static_cast<NonExplicitIdWrapper&>(++static_cast<MyBase&>(*this));
  }
  NonExplicitIdWrapper operator++(int) {
    return static_cast<NonExplicitIdWrapper>(static_cast<MyBase&>(*this)++);
  }
  bool operator==(const NonExplicitIdWrapper& id_wrapper) {
    return MyBase::operator==(id_wrapper);
  }
  bool operator!=(const NonExplicitIdWrapper& id_wrapper) {
    return MyBase::operator!=(id_wrapper);
  }
  /// @brief 获取存储无效值的容器
  /// @return 存储无效值的容器，无效值同InvalidValue
  static constexpr NonExplicitIdWrapper InvalidId() {
    return NonExplicitIdWrapper<IdType, LabelEnum, label>(
        MyBase::InvalidValue());
  }
};

/// @class NonExplicitIdWrapperCustomizeInvalidValue id_wrapper.h
/// @brief 不强制显示调用构造函数不使用默认无效值版
/// @param[in] IdType_ ： 要包装的ID的实际类型
/// @param[in] LabelEnum_ ：分发标签的枚举类型，与label_共同决定唯一的ID
/// @param[in] label_
/// ：分发标签枚举类型中的具体枚举，与LabelEnum_共同决定唯一的ID类型
/// @param[in] invalid_value_ ：自定义的无效值
/// 例：
///  enum class ExampleEnum { kExampleType };
///  using Id = NonExplicitIdWrapper<size_t,ExampleEnum,
///                                  ExampleEnum::kExampleType,
///                                  static_cast<size_t>(-1)>;
template <class IdType_, class LabelEnum_, const LabelEnum_ label_,
          const IdType_ invalid_value_>
class NonExplicitIdWrapperCustomizeInvalidValue
    : public BaseIdWrapper<IdType_, LabelEnum_, label_, invalid_value_> {
 private:
  /// @brief 使用的基类
  using MyBase = BaseIdWrapper<IdType_, LabelEnum_, label_, invalid_value_>;

 public:
  /// @brief 提供使用的ID原始类型
  using IdType = MyBase::IdType;
  /// @brief 提供定义时使用的枚举类型
  using LabelEnum = MyBase::LabelEnum;
  /// @brief 提供定义时使用的枚举中具体枚举
  static const LabelEnum label = MyBase::label;

  NonExplicitIdWrapperCustomizeInvalidValue() {}
  NonExplicitIdWrapperCustomizeInvalidValue(IdType production_node_id) {
    MyBase::SetId(production_node_id);
  }
  NonExplicitIdWrapperCustomizeInvalidValue(
      const NonExplicitIdWrapperCustomizeInvalidValue& id_wrapper) {
    MyBase::SetId(id_wrapper.GetThisNodeId());
  }
  NonExplicitIdWrapperCustomizeInvalidValue& operator=(
      const NonExplicitIdWrapperCustomizeInvalidValue& id_wrapper) {
    MyBase::SetId(id_wrapper.GetThisNodeId());
    return *this;
  }

  NonExplicitIdWrapperCustomizeInvalidValue& operator++() {
    return static_cast<NonExplicitIdWrapperCustomizeInvalidValue&>(
        ++static_cast<MyBase&>(*this));
  }
  NonExplicitIdWrapperCustomizeInvalidValue operator++(int) {
    return static_cast<NonExplicitIdWrapperCustomizeInvalidValue>(
        static_cast<MyBase&>(*this)++);
  }
  bool operator==(const NonExplicitIdWrapperCustomizeInvalidValue& id_wrapper) {
    return MyBase::operator==(id_wrapper);
  }
  bool operator!=(const NonExplicitIdWrapperCustomizeInvalidValue& id_wrapper) {
    return MyBase::operator!=(id_wrapper);
  }
  /// @brief 获取存储无效值的容器
  /// @return 存储无效值的容器，无效值同InvalidValue
  static constexpr NonExplicitIdWrapperCustomizeInvalidValue InvalidId() {
    return NonExplicitIdWrapperCustomizeInvalidValue<IdType, LabelEnum, label,
                                                     static_cast<IdType>(-1)>(
        MyBase::InvalidValue());
  }
};

}  // namespace frontend::common

namespace std {
/// @brief
/// 具体化hash模板，否则使用unordered系列容器时编译器无法自动将类类型转换为
/// IdType类型导致报错
template <class IdType_, class LabelEnum_, LabelEnum_ label_>
struct hash<frontend::common::ExplicitIdWrapper<IdType_, LabelEnum_, label_>> {
  size_t operator()(
      const frontend::common::ExplicitIdWrapper<IdType_, LabelEnum_, label_>&
          id_wrapper) const {
    return _Do_hash(id_wrapper);
  }
  static size_t _Do_hash(
      const frontend::common::ExplicitIdWrapper<IdType_, LabelEnum_, label_>&
          id_wrapper) noexcept {
    return hash<typename frontend::common::ExplicitIdWrapper<
        IdType_, LabelEnum_, label_>::IdType>::_Do_hash(id_wrapper);
  }
};

/// @brief
/// 具体化hash模板，否则使用unordered系列容器时编译器无法自动将类类型转换为
/// IdType类型导致报错
template <class IdType_, class LabelEnum_, LabelEnum_ label_,
          IdType_ invalid_value_>
struct hash<frontend::common::ExplicitIdWrapperCustomizeInvalidValue<
    IdType_, LabelEnum_, label_, invalid_value_>> {
  size_t operator()(
      const frontend::common::ExplicitIdWrapperCustomizeInvalidValue<
          IdType_, LabelEnum_, label_, invalid_value_>& id_wrapper) const {
    return _Do_hash(id_wrapper);
  }
  static size_t _Do_hash(
      const frontend::common::ExplicitIdWrapperCustomizeInvalidValue<
          IdType_, LabelEnum_, label_, invalid_value_>& id_wrapper) noexcept {
    return hash<
        typename frontend::common::ExplicitIdWrapperCustomizeInvalidValue<
            IdType_, LabelEnum_, label_>::IdType>::_Do_hash(id_wrapper);
  }
};

/// @brief
/// 具体化hash模板，否则使用unordered系列容器时编译器无法自动将类类型转换为
/// IdType类型导致报错
template <class IdType_, class LabelEnum_, LabelEnum_ label_>
struct hash<
    frontend::common::NonExplicitIdWrapper<IdType_, LabelEnum_, label_>> {
  size_t operator()(
      const frontend::common::NonExplicitIdWrapper<IdType_, LabelEnum_, label_>&
          id_wrapper) const {
    return _Do_hash(id_wrapper);
  }
  static size_t _Do_hash(
      const frontend::common::NonExplicitIdWrapper<IdType_, LabelEnum_, label_>&
          id_wrapper) noexcept {
    return hash<typename frontend::common::NonExplicitIdWrapper<
        IdType_, LabelEnum_, label_>::IdType>::_Do_hash(id_wrapper);
  }
};

/// @brief
/// 具体化hash模板，否则使用unordered系列容器时编译器无法自动将类类型转换为
/// IdType类型导致报错
template <class IdType_, class LabelEnum_, LabelEnum_ label_,
          IdType_ invalid_value_>
struct hash<frontend::common::NonExplicitIdWrapperCustomizeInvalidValue<
    IdType_, LabelEnum_, label_, invalid_value_>> {
  size_t operator()(
      const frontend::common::NonExplicitIdWrapperCustomizeInvalidValue<
          IdType_, LabelEnum_, label_, invalid_value_>& id_wrapper) const {
    return _Do_hash(id_wrapper);
  }
  static size_t _Do_hash(
      const frontend::common::NonExplicitIdWrapperCustomizeInvalidValue<
          IdType_, LabelEnum_, label_, invalid_value_>& id_wrapper) noexcept {
    return hash<
        typename frontend::common::NonExplicitIdWrapperCustomizeInvalidValue<
            IdType_, LabelEnum_, label_>::IdType>::_Do_hash(id_wrapper);
  }
};
}  // namespace std

#endif  /// !COMMON_ID_WRAPPER