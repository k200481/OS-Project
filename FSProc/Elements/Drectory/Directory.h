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
        // only move constructor available publicly
        Directory(Directory&& d) = default;
        // happens automatically, but just to be explicit about it
        Directory(const Directory& rhs) = delete;
        Directory& operator=(const Directory& rhs) = delete;
        // create a root directory
        static DirPtr CreateRoot(BlockManager& bm, int root_uid = 0, int rootdir_permissions = 0x6);
        // load a root directory if one exists
        static DirPtr LoadRoot(BlockManager& bm);
        // add a new FSElement into the directory's entry list
        bool Add(BlockManager& bm, const char* name, ElementType t, int owner, int permissions);
        // get rhe number of entries
		unsigned int GetNumEntries() const;
        // ge ta list of the entries in the directory with their metadata
		std::vector<data_pair> List(BlockManager& bm) const;
        // open a FSElement in the drectroy
        FSElementPtr Open(BlockManager& bm, const std::string& filename);

	private:
        /* kept hidden so only root can be loaded directly, all other directories must be loaded from root */

        // load a directory from the inode blokc
        Directory(BlockManager& bm, unsigned int inode_block);
        // create a directory with the inode at the given block
		Directory(BlockManager& bm, int owner, int permissions);
        // load an inode from the inode block
        static DirPtr Load(BlockManager& bm, unsigned int inode_block);

        // get the entry structure at the given index
		Entry GetEntry(const BlockManager& bm, unsigned int idx) const;
        // check if an entry with the name exists
        bool EntryExists(const BlockManager& bm, const std::string& filename);
        // get the index of the entry with the given name
        int GetEntryIdx(const BlockManager& bm, const std::string& filename) const;

	private:
		int num_entries = 0;
	};
}


