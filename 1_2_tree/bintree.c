#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#define KEY_VALUE int
#define NODE_ENTRY(name, type)			\
	struct name {					\
		struct type *left;			\
		struct type *right;			\
	}

typedef enum {
	EN_PREORDER,
	EN_INORDER,
	EN_POSTORDER,
} TRAVER_TYPE;

typedef struct {
	struct btree_node *left;
	struct btree_node *right;
} stNodeEntry;

typedef struct btree_node {
	KEY_VALUE value;
//	NODE_ENTRY(,btree_node) entry;
	stNodeEntry entry;
} stBtreeNode;

typedef struct btree {
	stBtreeNode *root;
} stBtree;

int btree_destroy_node(stBtreeNode *parent_node, stBtreeNode *del_node)
{
	printf("%d, %d\n", parent_node->value, del_node->value);
	if (del_node == NULL)
	{
		return -1;
	}

	if (del_node->entry.left == NULL && del_node->entry.right == NULL)
	{
		printf("%d\n", parent_node->value);
		if (del_node->value < parent_node->value)
		{
			parent_node->entry.left = NULL;
		}
		else
		{
			parent_node->entry.right = NULL;
		}
	}
	else if (del_node->entry.left != NULL && del_node->entry.right == NULL)
	{
		if (del_node->value < parent_node->value)
		{
			parent_node->entry.left = del_node->entry.left;
		}
		else
		{
			parent_node->entry.left = del_node->entry.right;
		}
	}
	else if (del_node->entry.left == NULL && del_node->entry.right != NULL)
	{
		if (del_node->value < parent_node->value)
		{
			parent_node->entry.left = del_node->entry.right;
		}
		else
		{
			parent_node->entry.right = del_node->entry.right;
		}
	}
	else if (del_node->entry.left != NULL && del_node->entry.right != NULL)
	{
		stBtreeNode *parent = del_node->entry.left;
		stBtreeNode *node = parent->entry.right;

		if (node != NULL)
		{
			while (node != NULL)
			{

				if (node->entry.right != NULL)
				{
					parent = node;
					node = node->entry.right;
				}
				else
				{
					break;
				}
			}

			del_node->value = node->value;
			parent->entry.right = node->entry.left;

			free(node);
			node = NULL;
		}
		else
		{
			if (del_node->value < parent_node->value)
			{
				parent_node->entry.left = parent;
			}
			else
			{
				parent_node->entry.right = parent;
			}
			parent->entry.right = del_node->entry.right;
		}
	}

	if (parent_node != del_node)
	{
		free(del_node);
		del_node = NULL;
	}

	return 0;
}

stBtreeNode *btree_create_node(KEY_VALUE value)
{
	stBtreeNode *node;

	node = (stBtreeNode *)malloc(sizeof(stBtreeNode));
	assert(node != NULL);

	node->value = value;
	node->entry.left = NULL;
	node->entry.right = NULL;

	return node;
}

int btree_insert_node(stBtree *tree, KEY_VALUE value)
{
	assert(tree != NULL);

	if (tree->root == NULL)
	{
		tree->root = btree_create_node(value);
		return 0;
	}

	stBtreeNode *node;
	stBtreeNode *temp;
	node = tree->root;

	while (node != NULL)
	{
		temp = node;
		if (value < node->value)
		{
			node = node->entry.left;
		}
		else
		{
			node = node->entry.right;
		}
	}

	if (value < temp->value)
	{
		temp->entry.left = btree_create_node(value);
	}
	else
	{
		temp->entry.right = btree_create_node(value);
	}

	return 0;
}

int btree_del_value(stBtree *tree, KEY_VALUE value)
{
	assert(tree != NULL);

	int ret = -1;
	stBtreeNode *parent, *node;

	parent = tree->root;
	node = tree->root;

	while (node != NULL)
	{
		if (value == node->value)
		{
			ret = btree_destroy_node(parent, node);

			break;
		}
		else if (value < node->value)
		{
			parent = node;
			node = node->entry.left;
		}
		else
		{
			parent = node;
			node = node->entry.right;
		}
	}

	return ret;
}


int btree_traversal(stBtreeNode *node, TRAVER_TYPE type)
{
	if (node == NULL)
	{
		return 0;
	}

	if (EN_PREORDER == type) printf("%4d", node->value);

	btree_traversal(node->entry.left, type);

	if (EN_INORDER == type) printf("%4d", node->value);
	btree_traversal(node->entry.right, type);


	if (EN_POSTORDER == type) printf("%4d", node->value);

	return 0;
}

int main(void)
{
	int i;
//	int array[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
	int array[] = {11,7,13,6,8,9,10,12,14};
	int len = 8;
	stBtree btree = {0};

	for (i = 0; i < len; i++)
	{
		btree_insert_node(&btree, array[i]);
	}

	btree_traversal(btree.root, EN_PREORDER);
	printf("\n");

	printf("del: %s\n", btree_del_value(&btree, 11) == 0 ?  "succeed":"failed");

	btree_traversal(btree.root, EN_PREORDER);
	printf("\n");

	return 0;
}
