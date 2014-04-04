#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif
#include <assert.h>
#include "array.h"
#include <assert.h>

using namespace vczh;

int main()
{
	{
		array<int, 3> numbers((array_sizes(),2,3,4));
		assert(numbers.size()==2);
		assert(numbers.total_size()==24);
		for(int i=0;i<numbers.size();i++)
		{
			array_ref<int, 2> numbers_2=numbers[i];
			assert(numbers_2.size()==3);
			assert(numbers_2.total_size()==12);
			for(int j=0;j<numbers_2.size();j++)
			{
				array_ref<int, 1> numbers_3=numbers_2[j];
				assert(numbers_3.size()==4);
				assert(numbers_3.total_size()==4);
				for(int k=0;k<numbers_3.size();k++)
				{
					numbers[i][j][k]=(i+1)*(j+1)*(k+1);
					assert(numbers[i][j][k] == numbers_3[k]);
				}
			}
		}

		assert(numbers[1][2][1]==12);
		assert(numbers[0][1][3]==8);
	}
#ifdef _MSC_VER
	_CrtDumpMemoryLeaks();
#endif
	return 0;
}

