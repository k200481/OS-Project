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
        Directory(Directory&& d);

        static DirPtr CreateRoot(BlockManager& bm, int root_uid, int rootdir_permissions);
        static DirPtr LoadRoot(BlockManager& bm);

		bool AddDirectory(BlockManager& bm, const char* name, int owner, int permissions);
		bool AddFile(BlockManager& bm, const char* name, int owner, int permissions);

		unsigned int GetNumEntries() const;

		std::vector<data_pair> List(BlockManager& bm) const;

		DirPtr OpenSubdir(BlockManager& bm, const std::string& filename);
		FilePtr OpenFile(BlockManager& bm, const std::string& filename);

        FSElementPtr Open(BlockManager& bm, const std::string& filename)
        {
            Entry* entry_list = new Entry[num_entries];
            inode.Read(bm, sizeof(int), entry_list, sizeof(Entry) * num_entries);
            for(int i = 0; i < num_entries; i++)
            {
                if(entry_list[i].name == filename)
                {
                    Inode inode = Inode::Load(bm, entry_list[i].block_num);
                    if(inode.GetType() == ElementType::File)
                        return File::Load(bm, inode_block);
                    else if(inode.GetType() == ElementType::Directory)
                        return Directory::Load(bm, entry_list[i].block_num);
                }
            }
            return FSElementPtr();
        }

	private:
        Directory(BlockManager& bm, unsigned int inode_block);
		Directory(BlockManager& bm, int owner, int permissions);
        static DirPtr Load(BlockManager& bm, unsigned int inode_block);

		Entry GetEntry(const BlockManager& bm, unsigned int idx) const;
        bool EntryExists(const BlockManager& bm, const std::string& filename);
        int GetEntryIdx(const BlockManager& bm, const std::string& filename) const;

	private:
		int num_entries = 0;
        mutable int reader_count = 0;
        mutable std::mutex mtx;
        mutable std::mutex rw_mtx;
	};
}


