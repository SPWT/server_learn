#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef int KEY_VALUE;

typedef struct _btree_node {
	KEY_VALUE *keys;
	struct _btree_node **children;
	int num;
	int leaf;
} ST_BTREE_NODE;

typedef struct _btree {
	ST_BTREE_NODE *root;
	int degree;
} ST_BTREE;

void btree_destroy_node(ST_BTREE_NODE *node)
{
	assert(node);

	free(node->children);
	free(node->keys);
	free(node);
}

ST_BTREE_NODE *btree_create_node(int t, int leaf)
{
	ST_BTREE_NODE *node = (ST_BTREE_NODE *)calloc(1, sizeof(ST_BTREE_NODE));
	if (node == NULL) assert(0);

	node->leaf = leaf;
	node->keys = (KEY_VALUE *)calloc(1, (2*t-1) * sizeof(KEY_VALUE));
	node->children = (ST_BTREE_NODE **)calloc(1, (2*t) * sizeof(ST_BTREE_NODE));
	node->num = 0;
}

void btree_split_child(ST_BTREE *T, ST_BTREE_NODE *x, int i)
{
	int degree = T->degree;
	ST_BTREE_NODE *y = x->children[i];
	ST_BTREE_NODE *z = btree_create_node(degree, y->leaf);

	int j = 0;
	for (j = 0; j < degree - 1; j++)
	{
		z->keys[j] = y->keys[j+degree];
	}

	if (y->leaf == 0)
	{
		for (j = 0; j < degree; j++)
		{
			z->children[j] = y->children[j+degree];
		}
	}
	z->num = degree - 1;
	y->num = degree - 1;

	for (j = x->num; j >= i + 1; j--)
	{
		x->children[j+1] = x->children[j];
	}
	x->children[i+1] = z;

	for (j = x->num - 1; j >= i; j--)
	{
		x->keys[j+1] = x->keys[j];
	}
	x->keys[i] = y->keys[degree - 1];
	x->num += 1;
}

void btree_insert_nonfull(ST_BTREE *T, ST_BTREE_NODE *x, KEY_VALUE key)
{
	int i = x->num - 1;
	if (x->leaf == 1)
	{
		while (i >= 0 && key < x->keys[i])
		{
			x->keys[i+1] = x->keys[i];
			i--;
		}
		x->keys[i+1] = key;
		x->num += 1;
	}
	else
	{
		while (i >= 0 && key < x->keys[i]) i--;

		if (x->children[i+1]->num == (2*T->degree - 1))
		{
			btree_split_child(T, x, i+1);
			if (key > x->keys[i+1]) i++;		// 与裂变添加到的父节点比较
		}

		btree_insert_nonfull(T, x->children[i+1], key);
	}
}

void btree_insert(ST_BTREE *T, KEY_VALUE key)
{
	ST_BTREE_NODE *root = T->root;

	if (root->num != 2 * T->degree -1)
	{
		btree_insert_nonfull(T, root, key);
	}
	else
	{
		ST_BTREE_NODE *node = btree_create_node(T->degree, 0);
		T->root = node;

		node->children[0] = root;

		btree_split_child(T, node, 0);

		int i = 0;
		if (key > node->keys[0]) i++;
		btree_insert_nonfull(T, node->children[i], key);
	}
}

void btree_create(ST_BTREE *T, int degree)
{
	T->degree = degree;

	ST_BTREE_NODE *x = btree_create_node(degree, 1);
	T->root = x;
}

void btree_merge(ST_BTREE *T, ST_BTREE_NODE *node, int idx)
{
	ST_BTREE_NODE *left = node->children[idx];
	ST_BTREE_NODE *right = node->children[idx + 1];

	int i = 0;

	left->keys[T->degree - 1] = node->keys[idx];		// 下沉

	for (i = 0; i < T->degree - 1; i++)
	{
		left->keys[T->degree + i] = right->keys[i];
	}

	if (!left->leaf)
	{
		for (i = 0; i < T->degree - 1; i++)
		{
			left->children[T->degree + i] = right->children[i];
		}
	}
	left->num += T->degree;

	btree_destroy_node(right);

	for (i = idx + 1; i < node->num; i++)
	{
		node->keys[i - 1] = node->keys[i];
		node->children[i] = node->children[i + 1];
	}
	node->children[i + 1] = NULL;
	node->num --;

	if (node->num == 0)
	{
		T->root = left;
		btree_destroy_node(node);
	}
}

void btree_delete_key(ST_BTREE *T, ST_BTREE_NODE *node, KEY_VALUE key)
{
	if (node == NULL) return ;

	int idx = 0, i;

	while (idx < node->num && key > node->keys[idx]) idx++;

	if (idx < node->num && key == node->keys[idx])
	{
		if (node->leaf)
		{
			for (i = idx; i < node->num - 1; i++)
			{
				node->keys[i] = node->keys[i+1];
			}
			node->keys[node->num - 1] = 0;
			node->num --;

			if (node->num == 0)		// root
			{
				free(node);
				T->root = NULL;
			}
			return ;
		}
		else if (node->children[idx]->num > T->degree - 1)
		{
			ST_BTREE_NODE *left = node->children[idx];

			node->keys[idx] = left->keys[left->num -1];
			btree_delete_key(T, left, left->keys[left->num - 1]);
		}
		else if (node->children[idx + 1]->num > T->degree - 1)
		{
			ST_BTREE_NODE *right = node->children[idx + 1];

			node->keys[idx] = right->keys[0];
			btree_delete_key(T, right, right->keys[0]);
		}
		else //megre
		{
			btree_merge(T, node, idx);
			btree_delete_key(T, node->children[idx], key);
		}
	}
	else
	{
		ST_BTREE_NODE *child = node->children[idx];
		if (child == NULL)
		{
			printf("Cannot del key = %d\n", key);
			return ;
		}

		if (child->num == T->degree - 1)
		{
			ST_BTREE_NODE *left = NULL;
			ST_BTREE_NODE *right = NULL;

			if (idx - 1 >= 0)
			{
				left = node->children[idx - 1];
			}
			if (idx + 1 <= node->num)
			{
				right = node->children[idx + 1];
			}

			if ((left && left->num > T->degree - 1)
				|| (right && right->num > T->degree - 1))
			{
				int richR = 0;

				if (right) richR = 1;
				if (left && right) richR = (right->num > left->num) ? 1 : 0;

				if (right && richR)			// 向右子树借
				{
					child->keys[child->num] = node->keys[idx];		// 下沉
					child->children[child->num + 1] = right->children[0];
					child->num ++;

					node->keys[idx] = right->keys[0];		// 上浮
					for (i = 0; i < right->num - 1; i++)
					{
						right->keys[i] = right->keys[i + 1];
						right->children[i] = right->children[i + 1];
					}
					right->keys[right->num - 1] = 0;

					right->children[right->num - 1] = right->children[right->num];
					right->children[right->num] = NULL;
					right->num --;
				}
				else
				{
					for (i = child->num; i > 0; i--)		// 左子树的第一个空了
					{
						child->keys[i] = child->keys[i - 1];
						child->children[i + 1] = child->children[i];
					}
					child->children[1] = child->children[0];

					child->children[0] = left->children[left->num];
					child->keys[0] = node->keys[idx - 1];

					child->num ++;

					node->keys[idx - 1] = left->keys[left->num - 1];
					left->keys[left->num - 1] = 0;
					left->children[left->num] = NULL;
					left->num--;
				}
			}
			else if ((!left || (left->num == T->degree - 1)) && (!right || (right->num == T->degree - 1)))
			{

				if (left && left->num == T->degree - 1)
				{
					btree_merge(T, node, idx - 1);
					child = left;
				}
				else if (right && right->num == T->degree - 1)
				{
					btree_merge(T, node, idx);
				}
			}
		}

		btree_delete_key(T, child, key);
	}

}

int btree_delete(ST_BTREE *T, KEY_VALUE key)
{
	if (T->root == NULL) return -1;

	btree_delete_key(T, T->root, key);

	return 0;
}

void btree_print(ST_BTREE *T, ST_BTREE_NODE *node, int layer)
{
	ST_BTREE_NODE *p = node;
	int i = 0;

	if (p)
	{
		for (i = 0; i < p->num; i++)
			printf("%c ", p->keys[i]);
		printf("\n");
		layer++;
		for (i = 0; i <= p->num; i++)
		{
			if (p->children[i])
				btree_print(T, p->children[i], layer);
		}
	}
	else printf("the tree is empty\n");
}

int main() {
	ST_BTREE T = {0};

	btree_create(&T, 3);
	srand(48);

	int i = 0;
	char key[26] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
//	char key[26] = "ABCDEF";
	for (i = 0; i < 26; i++)
	{
		//key[i] = rand() % 1000;
		printf("%c ", key[i]);
		btree_insert(&T, key[i]);
	}

	printf("\n######################\n");

	btree_print(&T, T.root, 0);

	//演示删除不存在key会改变树的结构
//	btree_delete(&T, '0');
//	btree_print(&T, T.root, 0);


	//删除结点
	for (i = 0; i < 26; i++)
	{
		printf("\n---------------------------------\n");
		btree_delete(&T, key[25 - i]);
		btree_print(&T, T.root, 0);
	}

}
