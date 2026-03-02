#pragma once

namespace re
{

// clang-format off
enum class PrimitiveType
{
	Points,        // Separate points
	Lines,         // Separate lines (0-1, 2-3)
	LineStrip,     // Connected lines (0-1-2-3)
	Triangles,	   // Separate triangles (0-1-2, 3-4-5)
	TriangleStrip, // Connected triangles (0-1-2-3-4-5)
	TriangleFan,   // Connected triangles with a common center
};
// clang-format on

} // namespace re