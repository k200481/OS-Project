#pragma once

#include "FSElement.h"
#include <mutex>
#include <memory>

namespace FS
{
	class File : public FSElement
	{
		friend class Directory;
	public:
        // reads the given data to the file
        // returns the number of bytes read
		int Read(BlockManager& bm, char* data, int offset, int size) const;
		// writes the given data to the file
        // returns the number of bytes written
        int Write(BlockManager& bm, const char* data, int offset, int size);

	private: // only Directory can make a new file
		File(BlockManager& bm, unsigned int inode_block);
		File(BlockManager& bm, int owner, int permissions);

        // just for QoL
        static FilePtr Load(BlockManager& bm, int inode_block)
        {
            return std::make_unique<File>(File(bm, inode_block));
        }
	};
}


