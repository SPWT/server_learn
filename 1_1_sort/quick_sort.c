#include <stdio.h>

void sort(int *data, int left, int right)
{
	int key = data[left];
	int start = left;
	int end = right;
	
	if (right < left) return;
	
	while (left != right)
	{
		while (left < right && key <= data[right])
		{
			right--;
		}
		data[left] = data[right];
		
		while (left < right && key >= data[left])
		{
			left++;
		}
		data[right] = data[left];
	}
	
	data[left] = key;
	
	sort(data, start, left - 1);
	sort(data, left + 1, end);
}

void quick_sort(int *data, int length)
{
	sort(data, 0, length - 1);
}

int main(void)
{
	int data[] = {56, 34, 54, 50, 81, 70, 7, 74, 26, 65, 71, 4, 37, 76, 91, 36};
	int length = sizeof(data) / sizeof(data[0]);
	int i;
	
	quick_sort(data, length);
	
	for (i = 0; i < length; i++)
	{
		printf("%4d", data[i]);
	}
	printf("\n");
	
	return 0;
}