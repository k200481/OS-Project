#pragma once

#include <Interface.h>

class FSP
{
public:
	FSP();
	~FSP();
	FSP(const FSP&) = delete;
	FSP& operator=(const FSP&) = delete;

	void Run();

private:
	const std::string filename = "TestFile";
	FS::Interface inf;
};
