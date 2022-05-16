#pragma once

#include <string>

class Disk
{
public:
	static constexpr unsigned int BlockSize = 512u;
public:
    // default constructor only
	Disk() = default;
    // destructor safely unmounts before exiting
	~Disk();
	Disk(const Disk&) = delete;
	Disk& operator=(const Disk&) = delete;
    // open a file as disk
    // returns true if successfuly mounted
	bool Mount(const std::string& filename);
    // close disk file
	void Unmount();
    // read a block from the file
	void Read(int block_num, void* buf) const;
    // write a block to the file
	void Write(int block_num, const void* buf);
    // create/initialize a new file as disk with the given name
	template <int NUM_BLOCKS = BlockSize * 8>
	static void Create(const std::string& filename);
private:
	int fd = -1;
};