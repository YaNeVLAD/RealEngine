#pragma once

namespace re::detail
{

template <typename T>
struct DirtyTag
{
	bool dummy = true;
};

struct TransparentTag
{
	bool dummy = true;
};

struct OpaqueTag
{
	bool dummy = true;
};

} // namespace re::detail