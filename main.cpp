
#include "include/stack_function.hpp"
#include <iostream>
#include <cassert>
#include <string>

int func(int a)
{
	return a + 1;
}

class Test
{
public:
	Test() = default;
	~Test() = default;
	Test(Test& other) { mData = other.mData; std::cout << "copy" << std::endl; }
	Test(Test&& other) { mData = std::move( other.mData ); std::cout << "move" << std::endl; }

	std::string operator()(int a)
	{
		return mData + std::to_string( a );
	}

private:
	std::string mData = "data";
};


int main(int argc, char** argv)
{
	std::string str("string");
	StackFunction<std::string(int) > function([&](int a)
		{
			std::cout << str << a << std::endl;
			return str;
		});
	auto function_copy = std::move(function);
	function_copy(5);

	StackFunction<int(int)> function2(func);
	std::cout << function2(5) << std::endl;

	StackFunction<std::string(int)> function3(Test{});
	std::cout << function3(5) << std::endl;
	return 0;
}
