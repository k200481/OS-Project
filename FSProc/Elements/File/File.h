#pragma once

#include "FSElement.h"
#include <mutex>
#include <memory>

namespace FS
{
	enum class SeekBase
	{
		Set,
		Cur,
		End
	};

	class File : public FSElement
	{
		friend class Directory;
	public:
		File(File&& rhs) noexcept;

		int Read(BlockManager& bm, char* data, int size) const;
		int Write(BlockManager& bm, const char* data, int size);
		void Seek(SeekBase base, int offset_in);
		unsigned int GetOffset() const;

	private: // only Directory can make a new file
		File(BlockManager& bm, unsigned int inode_block);
		File(BlockManager& bm, int owner, int permissions);
		//File() = default;

        static FilePtr Load(BlockManager& bm, int inode_block)
        {
            return std::make_unique<File>(File(bm, inode_block));
        }

	private:
		mutable unsigned int offset = 0;
		mutable int reader_count = 0;
		mutable std::mutex rw_mtx;
		mutable std::mutex mtx;
	};
}


