#include <unistd.h>
#include <iostream>

int main()
{
	char buf[2];
	buf[0] = 'a';
	buf[1] = 0;

	std::cout << write(1, buf, 0) << std::endl;
}
