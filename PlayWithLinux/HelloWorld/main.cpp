#include <iostream>
#include <string>
#include "calc.h"

int main()
{
	string input;
	int result;
	while (true)
	{
		getline(cin, input);
		if (input == "") break;
		if (calc(input, result))
		{
			cout<<"\t"<<result<<endl;
		}
		else
		{
			cout<<"\t<INVALID_INPUT>"<<endl;
		}
	}
}
