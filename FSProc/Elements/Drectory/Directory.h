#pragma once

#include <FSElement.h>
#include <File.h>
#include <vector>

namespace FS
{
	class Directory : public FSElement
	{
		struct Entry
		{
			int block_num;
			char name[MaxNameLen + 1];
		};
	public:
        // happens automatically, but just to be explicit about it
        Directory(const Directory& rhs) = delete;
        Directory& operator=(const Directory& rhs) = delete;
        // move constructor
        Directory(Directory&& d) = default;

        static DirPtr CreateRoot(BlockManager& bm, int root_uid, int rootdir_permissions);
        static DirPtr LoadRoot(BlockManager& bm);

        bool Add(BlockManager& bm, const char* name, ElementType t, int owner, int permissions);

		unsigned int GetNumEntries() const;

		std::vector<data_pair> List(BlockManager& bm) const;

        FSElementPtr Open(BlockManager& bm, const std::string& filename);

	private:
        Directory(BlockManager& bm, unsigned int inode_block);
		Directory(BlockManager& bm, int owner, int permissions);
        static DirPtr Load(BlockManager& bm, unsigned int inode_block);

		Entry GetEntry(const BlockManager& bm, unsigned int idx) const;
        bool EntryExists(const BlockManager& bm, const std::string& filename);
        int GetEntryIdx(const BlockManager& bm, const std::string& filename) const;

	private:
		int num_entries = 0;
	};
}


