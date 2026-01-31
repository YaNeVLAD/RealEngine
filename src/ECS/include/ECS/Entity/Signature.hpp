#pragma once

#include <bitset>

namespace re::ecs
{

constexpr std::size_t MAX_COMPONENTS = 64;

using Signature = std::bitset<MAX_COMPONENTS>;

} // namespace re::ecs