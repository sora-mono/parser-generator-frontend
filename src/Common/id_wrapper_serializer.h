#ifndef COMMON_ID_WRAPPER_SERIALIZER_H_
#define COMMON_ID_WRAPPER_SERIALIZER_H_
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "id_wrapper.h"

namespace boost::serialization {
template <class Archive, class IdType_, class LabelEnum_,
          const LabelEnum_ label_, const IdType_ invalid_value_>
void serialize(Archive& ar,
               frontend::common::BaseIdWrapper<IdType_, LabelEnum_, label_,
                                               invalid_value_>& wrapper,
               const unsigned int version) {
  ar& static_cast<IdType_&>(wrapper);
}

template <class Archive, class IdType_, class LabelEnum_,
          const LabelEnum_ label_>
void serialize(
    Archive& ar,
    frontend::common::ExplicitIdWrapper<IdType_, LabelEnum_, label_>& wrapper,
    const unsigned int version) {
  ar& static_cast<IdType_&>(wrapper);
}

template <class Archive, class IdType_, class LabelEnum_,
          const LabelEnum_ label_, const IdType_ invalid_value_>
void serialize(Archive& ar,
               frontend::common::ExplicitIdWrapperCustomizeInvalidValue<
                   IdType_, LabelEnum_, label_, invalid_value_>& wrapper,
               const unsigned int version) {
  ar& static_cast<IdType_&>(wrapper);
}

template <class Archive, class IdType_, class LabelEnum_,
          const LabelEnum_ label_>
void serialize(Archive& ar,
               frontend::common::NonExplicitIdWrapper<IdType_, LabelEnum_,
                                                      label_>& wrapper,
               const unsigned int version) {
  ar& static_cast<IdType_&>(wrapper);
}

template <class Archive, class IdType_, class LabelEnum_,
          const LabelEnum_ label_, const IdType_ invalid_value_>
void serialize(Archive& ar,
               frontend::common::NonExplicitIdWrapperCustomizeInvalidValue<
                   IdType_, LabelEnum_, label_, invalid_value_>& wrapper,
               const unsigned int version) {
  ar& static_cast<IdType_&>(wrapper);
}
}  // namespace boost::serialization
#endif  // !COMMON_ID_WRAPPER_SERIALIZER_H_