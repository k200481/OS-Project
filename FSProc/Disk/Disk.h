#pragma once

#include <string>

class Disk
{
public:
	static constexpr unsigned int BlockSize = 512u;
public:
	Disk() = default;
	~Disk();
	Disk(const Disk&) = delete;
	Disk& operator=(const Disk&) = delete;

	bool Mount(const std::string& filename);

	void Unmount();

	void Read(int block_num, void* buf) const;

	void Write(int block_num, const void* buf);

	template <int NUM_BLOCKS = BlockSize * 8>
	static void Create(const std::string& filename);
private:
	int fd = -1;
};