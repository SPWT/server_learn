#include <stdio.h>

#define DATA_ARRAY_LENGTH 16

void sort(int *data, int *temp, int start, int middle, int end)
{
	int i, j, k;
	
	i = start;
	j = middle + 1;
	k = start;
	
	while (i <= middle && j <= end)
	{
		if (data[i] > data[j])
		{
			temp[k++] = data[j++];
		}
		else
		{
			temp[k++] = data[i++];
		}
	}
	
	while (i <= middle)
	{
		temp[k++] = data[i++];
	}
	
	while (j <= end)
	{
		temp[k++] = data[j++];
	}
	
	for (i = start; i <= end; i++)
	{
		data[i] = temp[i];
	}
}

void merge_sort(int *data, int *temp, int start, int end)
{
	int middle;
	
	if (start != end)
	{
		middle = start + (end - start) / 2;
		
		merge_sort(data, temp, start, middle);
		merge_sort(data, temp, middle + 1, end);
		
		sort(data, temp, start, middle, end);
	}
}

int main(void)
{
	int data[DATA_ARRAY_LENGTH] = {56, 34, 54, 50, 81, 70, 7, 74, 26, 65, 71, 4, 37, 76, 91, 36};
	int length = DATA_ARRAY_LENGTH;
	int temp[DATA_ARRAY_LENGTH] = {0};
	int i;
	
	merge_sort(data, temp, 0, DATA_ARRAY_LENGTH - 1);
	
	for (i = 0; i < length; i++)
	{
		printf("%4d", data[i]);
	}
	printf("\n");
	
	return 0;
}