#include <iostream>
#include "Tests.cpp"

void callbackFunctionDemo(float s) {}
void addFunction(cc::ccSchedulerFunc& cf) {
	std::cout << &cf;
}
int main()
{
	std::cout << "Compile succeed, Test start" << std::endl;
	/********************* Test 000 : pointers and examples **************************/
	cc::ccSchedulerFunc nptr{ nullptr };
	std::cout << &nptr << std::endl;
	nptr = callbackFunctionDemo;
	addFunction(nptr);
	std::cout << callbackFunctionDemo << std::endl;
	
	std::string* s{ nullptr };
	s = new std::string("Hellow");
	delete s;
	s = nullptr;
	

	/********************* Test 001 :  ListEntry Basic Function and default constructor **********************/
	tt::Test001();
	return 0;
}

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
