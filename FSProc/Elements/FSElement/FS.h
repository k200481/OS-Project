#pragma once
#include <memory>

namespace FS
{
    static constexpr int MaxNameLen = 255;

	enum class ElementType
	{
		Directory,
		File,
		SymLink,
	};

    using FSElementPtr = std::unique_ptr<class FSElement>;
    using DirPtr = std::unique_ptr<class Directory>;
    using FilePtr = std::unique_ptr<class File>;
}