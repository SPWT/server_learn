#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum _node_color{
	RED = 1,
	BLACK
} EN_NODE_COLOR;

typedef int KEY_TYPE;

typedef struct _rbtree_node {
	unsigned char color;
	KEY_TYPE key;
	void *value;

	struct _rbtree_node *left;
	struct _rbtree_node *right;
	struct _rbtree_node *parent;
} ST_RBTREE_NODE;

typedef struct _rbtree {
	ST_RBTREE_NODE *root;
	ST_RBTREE_NODE *nil;
} ST_RBTREE;

ST_RBTREE_NODE *rbtree_mini(ST_RBTREE *T, ST_RBTREE_NODE *x)
{
	while (x->left != T->nil)
	{
		x = x->left;
	}

	return x;
}

ST_RBTREE_NODE *rbtree_successor(ST_RBTREE *T, ST_RBTREE_NODE *x)
{
	return rbtree_mini(T, x->right);
}

void rbtree_left_rotate(ST_RBTREE *T, ST_RBTREE_NODE *x)
{
	ST_RBTREE_NODE *y = x->right;

	x->right = y->left;
	if (y->left != T->nil)
	{
		y->left->parent = x;
	}

	if (x->parent == T->nil)
	{
		T->root = y;
	}
	else if (x == x->parent->left)
	{
		x->parent->left = y;
		y->parent = x->parent;
	}
	else
	{
		x->parent->right = y;
		y->parent = x->parent;
	}

	x->parent = y;
	y->left = x;
}

void rbtree_right_rotate(ST_RBTREE *T, ST_RBTREE_NODE *x)
{
	ST_RBTREE_NODE *y = x->left;

	x->left = y->right;
	if (y->right != T->nil)
	{
		y->right->parent = x;
	}

	if (x->parent == T->nil)
	{
		T->root = y;
	}
	else if (x == x->parent->right)
	{
		x->parent->right = y;
		y->parent = x->parent;
	}
	else
	{
		x->parent->left = y;
		y->parent = x->parent;
	}

	x->parent = y;
	y->right = x;
}

void rbtree_insert_fixup(ST_RBTREE *T, ST_RBTREE_NODE *x)
{
	while (x->parent->color == RED)
	{
		if (x->parent == x->parent->parent->left)
		{
			ST_RBTREE_NODE *y = x->parent->parent->right;

			if (y->color == RED)		// 叔结点为红色
			{
				x->parent->color = BLACK;
				y->color = BLACK;
				x->parent->parent->color = RED;

				x = x->parent->parent;
			}
			else
			{
				if (x == x->parent->right)					// 为 < 形式
				{
					x = x->parent;
					rbtree_left_rotate(T, x);
				}

				x->parent->color = BLACK;					// 为 / 形式
				x->parent->parent->color = RED;
				rbtree_right_rotate(T, x->parent->parent);
			}
		}
		else
		{
			ST_RBTREE_NODE *y = x->parent->parent->left;

			if (y->color == RED)		// 叔结点为红色
			{
				x->parent->color = BLACK;
				y->color = BLACK;
				x->parent->parent->color = RED;

				x = x->parent->parent;
			}
			else
			{
				if (x == x->parent->left)					// 为 > 形式
				{
					x = x->parent;
					rbtree_right_rotate(T, x);
				}

				x->parent->color = BLACK;					// 为 \ 形式
				x->parent->parent->color = RED;
				rbtree_left_rotate(T, x->parent->parent);
			}
		}
	}

	T->root->color = BLACK;
}

void rbtree_insert(ST_RBTREE *T, ST_RBTREE_NODE *x)
{
	ST_RBTREE_NODE *y = T->root;
	ST_RBTREE_NODE *z = T->nil;

	while (y != T->nil)
	{
		z = y;
		if (x->key < y->key)
		{
			y = y->left;
		}
		else if (x->key > y->key)
		{
			y = y->right;
		}
		else
		{
			return;
		}
	}

	x->parent = z;
	if (z == T->nil)
	{
		T->root = x;
	}
	if (x->key < z->key)
	{
		z->left = x;
	}
	else
	{
		z->right = x;
	}

	x->left = T->nil;
	x->right = T->nil;
	x->color = RED;

	rbtree_insert_fixup(T, x);
}

void rbtree_delete_fixup(ST_RBTREE *T, ST_RBTREE_NODE *N)
{
	while ((N != T->root) && (N->color == BLACK))
	{
		if (N == N->parent->left)		// 兄在右
		{
			ST_RBTREE_NODE *S = N->parent->right;

			if (S->color == RED)		// 3.2兄红
			{
				S->color = BLACK;
				N->parent->color = RED;
				rbtree_left_rotate(T, N->parent);

				S = N->parent->right;
			}

			if (S->color == BLACK)		// 2.兄黑
			{
				if (S->left->color == BLACK && S->right->color == BLACK)		// 2.1 兄子全黑
				{
					S->color = RED;
					N = N->parent;		// parent如果为红，则相当于与兄交换颜色
				}
				else															// 2.2 兄子不全黑
				{
					if (S->right->color == BLACK)
					{
						S->left->color = BLACK;
						S->color = RED;

						rbtree_right_rotate(T, S);
						S = N->parent->right;
					}


					S->color = N->parent->color;
					S->right->color = BLACK;
					rbtree_left_rotate(T, S->parent);

					N = T->root;
				}
			}
		}
		else
		{
			ST_RBTREE_NODE *S = N->parent->left;

			if (S->color == RED)		// 3.1兄红
			{
				S->color = BLACK;
				N->parent->color = RED;
				rbtree_right_rotate(T, N->parent);

				S = N->parent->left;
			}

			if (S->color == BLACK)		// 2.兄黑
			{
				if (S->right->color == BLACK && S->left->color == BLACK)		// 2.1 兄子全黑
				{
					S->color = RED;
					N = N->parent;		// parent如果为红，则相当于与兄交换颜色
				}
				else															// 2.2 兄子不全黑
				{
					if (S->left->color == BLACK)
					{
						S->right->color = BLACK;
						S->color = RED;

						rbtree_left_rotate(T, S);
						S = N->parent->left;
					}


					S->color = N->parent->color;
					S->left->color = BLACK;
					rbtree_right_rotate(T, S->parent);

					N = T->root;
				}
			}
		}
	}

	N->color = BLACK;
}

ST_RBTREE_NODE *rbtree_delete(ST_RBTREE *T, ST_RBTREE_NODE *x)
{
	ST_RBTREE_NODE *y = T->nil;
	ST_RBTREE_NODE *z = T->nil;

	if (x == T->nil)
	{
		printf("delete node is nil\n");
		return T->nil;
	}

	if ((x->left == T->nil) || (x->right == T->nil))	// 子树不满
	{
		y = x;
	}
	else
	{
		y = rbtree_successor(T, x);
	}

	if (y->left != T->nil)
	{
		z = y->left;
	}
	else if (y->right != T->nil)
	{
		z = y->right;
	}
	z->parent = y->parent;

	if (y->parent == T->nil)
	{
		T->root = z;
	}
	else if (y == y->parent->left)
	{
		y->parent->left = z;
	}
	else
	{
		y->parent->right = z;
	}

	if (y != x)
	{
		x->key = y->key;
		x->value = y->value;
	}

	if (y->color == BLACK)
	{
		rbtree_delete_fixup(T, z);
	}

	return y;
}

ST_RBTREE_NODE *rbtree_search(ST_RBTREE *T, KEY_TYPE key)
{
	ST_RBTREE_NODE *x = T->root;

	while (x != T->nil)
	{
		if (key < x->key)
		{
			x = x->left;
		}
		else if (key > x->key)
		{
			x = x->right;
		}
		else
		{
			return x;
		}
	}

	return T->nil;
}

void rbtree_traversal(ST_RBTREE *T, ST_RBTREE_NODE *node)
{
	if (node != T->nil)
	{
		rbtree_traversal(T, node->left);
		printf("key:%d, color:%d\n", node->key, node->color);
		rbtree_traversal(T, node->right);
	}
}

int main(void)
{
	int array[20] = {8, 46, 70, 82, 32, 57, 38, 92, 48, 36, 74, 45, 39, 34, 84, 42, 59, 95, 20, 75};
	int len = sizeof(array) / sizeof(array[0]);
	ST_RBTREE_NODE *node = NULL;
	int i;

	ST_RBTREE *T = (ST_RBTREE *)malloc(sizeof(ST_RBTREE));
	if (T == NULL)
	{
		printf("malloc T failed\n");
		return -1;
	}
	memset(T, 0, sizeof(ST_RBTREE));

	T->nil = (ST_RBTREE_NODE *)malloc(sizeof(ST_RBTREE_NODE));
	if (T->nil == NULL)
	{
		printf("malloc T->nil failed\n");
		free(T);
		return -2;
	}
	memset(T->nil, 0, sizeof(ST_RBTREE));
	T->nil->color = BLACK;
	T->root = T->nil;

	node = T->nil;
	for (i = 0; i < len; i++)
	{
		node = (ST_RBTREE_NODE *)malloc(sizeof(ST_RBTREE_NODE));
		node->key = array[i];
		node->color = RED;
		node->value = NULL;

		rbtree_insert(T, node);
	}

	rbtree_traversal(T, T->root);

	printf("-----------------------------------\n");
	ST_RBTREE_NODE *del_cur = T->nil;
	rbtree_delete(T, rbtree_search(T, 74));
	if (del_cur != T->nil)
	{
		free(del_cur);
		del_cur = NULL;
	}

	rbtree_traversal(T, T->root);

	return 0;
}



