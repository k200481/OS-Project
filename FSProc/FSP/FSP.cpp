#include <FSP.h>
#include <iostream>
#include <sstream>

#include <File.h>
#include <thread>

#include <algorithm>
#include <iterator>
#include <stack>

FSP::FSP()
	:
	inf(filename)
{
    int def = inf.Open("/home/default");
    inf.Add(def, "file1", FS::ElementType::File, 1, 0x6);
    inf.Close(def);
}

FSP::~FSP()
{}

void FSP::Run()
{
    int file = inf.Open("/home/default/file1");
    if(file == -1)
        return;
    
    char buf[4] = {};
    inf.Read(file, buf, 0, 3);
    inf.Close(file);
    std::cout << buf << std::endl;
}
