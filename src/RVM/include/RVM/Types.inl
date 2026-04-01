#include <RVM/Types.hpp>

#include <Core/Utils.hpp>

namespace re::rvm
{

template <typename TChar>
std::basic_ostream<TChar>& operator<<(std::basic_ostream<TChar>& os, Value const& val)
{
	// clang-format off
	std::visit(utils::overloaded{
		[&os](Null_t) { os << "null"; },
		[&os](const Int v){ os << v; },
		[&os](const Double v){ os << v; },
		[&os](String const& str){ os << str.ToString(); },
		[&os](InstancePtr const& ptr) {
			if (ptr && ptr->typeInfo) {
				os << "Instance(" << ptr->typeInfo->name.ToString() << ")";
			} else {
				os << "null_instance";
			}
		},
		[&os](TypeInfoPtr const& ptr) {
			if (ptr) {
				os << "Class(" << ptr->name.ToString() << ")";
			} else {
				os << "null_class";
			}
		},
		[&os](ArrayInstancePtr const& ptr) {
			if (!ptr)
			{
				os << "null_array";
				return;
			}

			os << "[";
			for(size_t i = 0; i < ptr->elements.size(); ++i) {
				os << ptr->elements[i] << (i == ptr->elements.size()-1 ? "" : ", ");
			}
			os << "]";
		},
		[&os](const auto&) { os << "unknown_type"; }
	}, val);
	// clang-format on

	return os;
}

} // namespace re::rvm