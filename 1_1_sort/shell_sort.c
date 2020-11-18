#include <stdio.h>

void shell_sort(int *data, int length)
{
	int gap = 0;
	int i = 0, j = 0;
	
	for (gap = length / 2; gap > 0; gap /= 2)
	{
		for (i = gap; i < length; i++)
		{
			for (j = i - gap; j >= 0 && data[j] > data[i]; j = j - gap)
			{
				int temp = data[j];
				data[j] = data[i];
				data[i] = temp;
			}
		}
	}
}

int main(void)
{
	int data[] = {56, 34, 54, 50, 81, 70, 7, 74, 26, 65, 71, 4, 37, 76, 91, 36};
	int length = sizeof(data) / sizeof(data[0]);
	int i;
	
	shell_sort(data, length);
	
	for (i = 0; i < length; i++)
	{
		printf("%4d", data[i]);
	}
	printf("\n");
	
	return 0;
}