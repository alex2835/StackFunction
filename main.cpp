
#include "include/stack_function.hpp"
#include <iostream>

int func(int a)
{
	return a + 1;
}

int main(int argc, char** argv)
{
	std::string str("string");
	StackFunction<int(int)> function([&](int a) 
		{
			std::cout << str << std::endl;
			return a + 1;
		});

	auto function_copy = std::move(function);
	function_copy(5);

	StackFunction<int(int)> function2( func );
	std::cout << function2(5) << std::endl;

	return 0;
}
