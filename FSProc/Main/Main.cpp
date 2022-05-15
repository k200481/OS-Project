#include <FSP.h>
#include <iostream>

int main(void)
{
	try
	{
		FSP fsp;
		fsp.Run();
	}
	catch (std::exception& e)
	{
		//MessageBox(NULL, e.what(), "Standard Exception", MB_OK | MB_ICONEXCLAMATION);
        std::cout << "Standare Exception\n" << e.what() << std::endl;
	}

	return 0;
}