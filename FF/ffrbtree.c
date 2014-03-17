/**
Copyright (c) 2013 Simon Zolin
*/

#include <FF/rbtree.h>


static fftree_node * tree_rm(fftree_node *nod, fftree_node **root, void *sentl, fftree_node **tmp);

static void left_rotate(fftree_node *nod, fftree_node **root, void *sentl);
static void right_rotate(fftree_node *nod, fftree_node **root, void *sentl);


/** Make node become a left child of 'parent'. */
#define link_leftchild(parnt, left_child) \
do { \
	(parnt)->left = (left_child); \
	(left_child)->parent = (parnt); \
} while (0)

/** Make node become a right child of 'parent'. */
#define link_rightchild(parnt, right_child) \
do { \
	(parnt)->right = (right_child); \
	(right_child)->parent = (parnt); \
} while (0)

/** Make node become a left/right child of parent of 'nold'. */
static FFINL void relink_parent(fftree_node *nold, fftree_node *nnew, fftree_node **proot, void *sentl) {
	fftree_node *p = nold->parent;
	if (p == sentl)
		*proot = nnew;
	else if (nold == p->left)
		p->left = nnew;
	else
		p->right = nnew;
	nnew->parent = p;
}


fftree_node * fftree_findnode(ffrbtkey key, fftree_node **root, void *sentl)
{
	fftree_node *nod = *root;
	while (nod != sentl) {
		*root = nod;
		if (key < nod->key)
			nod = nod->left;
		else if (key > nod->key)
			nod = nod->right;
		else //key == nod->key
			return nod;
	}
	return NULL;
}

fftree_node * fftree_successor(fftree_node *it, void *sentl)
{
	fftree_node *nod;
	FF_ASSERT(it != sentl);

	if (it->right != sentl)
		return fftree_min(it->right, sentl); // find minimum node in the right subtree

	// go up until we find a node that is the left child of its parent
	nod = it->parent;
	while (nod != sentl && it == nod->right) {
		it = nod;
		nod = nod->parent;
	}
	return nod;
}

/** Insert into binary search tree. */
void fftree_insert(fftree_node *nod, fftree_node **root, void *sentl)
{
	if (*root == sentl) {
		*root = nod; // set root node
		nod->parent = sentl;

	} else {
		fftree_node **pchild;
		fftree_node *parent = *root;

		// find parent node and the pointer to its left/right node
		for (;;) {
			if (nod->key < parent->key)
				pchild = &parent->left;
			else
				pchild = &parent->right;

			if (*pchild == sentl)
				break;
			parent = *pchild;
		}

		*pchild = nod; // set parent's child
		nod->parent = parent;
	}

	nod->left = nod->right = sentl;
}

/** Remove from binary search tree. */
static fftree_node * tree_rm(fftree_node *nod, fftree_node **root, void *sentl, fftree_node **pnext)
{
	fftree_node *x
		, *nnew;

	if (nod->left == sentl) {
		x = nod->right;
		nnew = nod->right; // to replace node by its right child
		*pnext = NULL;

	} else if (nod->right == sentl) {
		x = nod->left;
		nnew = nod->left; // to replace node by its left child
		*pnext = NULL;

	} else { // node has both children
		fftree_node *next;

		next = fftree_min(nod->right, sentl);
		// next->left == sentl
		x = next->right;

		if (next == nod->right)
			x->parent = next; //set parent in case x == sentl
		else {
			// 'next' is not a direct child of 'nod'
			link_leftchild(next->parent, next->right);
			link_rightchild(next, nod->right);
		}

		link_leftchild(next, nod->left);
		*pnext = next;
		nnew = next; // to replace node by its successor
	}

	relink_parent(nod, nnew, root, sentl);

	nod->left = nod->right = nod->parent = NULL;

	return x;
}


static void left_rotate(fftree_node *nod, fftree_node **root, void *sentl)
{
	fftree_node *r = nod->right;
	FF_ASSERT(nod != sentl && nod->right != sentl);

	//link_rightchild(nod, r->left);
	nod->right = r->left;
	if (r->left != sentl)
		r->left->parent = nod;

	relink_parent(nod, r, root, sentl);

	link_leftchild(r, nod);
}

static void right_rotate(fftree_node *nod, fftree_node **root, void *sentl)
{
	fftree_node *l = nod->left;
	FF_ASSERT(nod != sentl && nod->left != sentl);

	//link_leftchild(nod, l->right);
	nod->left = l->right;
	if (l->right != sentl)
		l->right->parent = nod;

	relink_parent(nod, l, root, sentl);

	link_rightchild(l, nod);
}

#define rbt_left_rotate(nod, proot, sentl) \
	left_rotate((fftree_node*)(nod), (fftree_node**)(proot), sentl)

#define rbt_right_rotate(nod, proot, sentl) \
	right_rotate((fftree_node*)(nod), (fftree_node**)(proot), sentl)


enum {
	BLACK
	, RED
};

void ffrbt_insert(ffrbtree *tr, ffrbt_node *nod, ffrbt_node *parent)
{
	ffrbt_node **root = &tr->root;
	void *sentl = &tr->sentl;

	if (parent != NULL && parent != sentl)
		root = &parent;

	fftree_insert((fftree_node*)nod, (fftree_node**)root, sentl);
	nod->color = RED;
	tr->len++;

	root = &tr->root;
	// fixup after insert if the parent is also red
	while (nod->parent->color == RED) {
		ffrbt_node *p = nod->parent;
		ffrbt_node *gp = nod->parent->parent; // grandparent exists because parent is red
		ffrbt_node *uncle;

		if (p == gp->left) {
			uncle = gp->right;
			if (uncle->color == BLACK) {
				if (nod == p->right) {
					rbt_left_rotate(p, root, sentl);
					nod = p;
				}

				(nod->parent)->color = BLACK;
				gp->color = RED;
				rbt_right_rotate(gp, root, sentl);
				break;
			}

		} else {
			uncle = gp->left;
			if (uncle->color == BLACK) {
				if (nod == p->left) {
					rbt_right_rotate(p, root, sentl);
					nod = p;
				}

				(nod->parent)->color = BLACK;
				gp->color = RED;
				rbt_left_rotate(gp, root, sentl);
				break;
			}
		}

		// both parent and uncle are red
		p->color = BLACK;
		uncle->color = BLACK;
		gp->color = RED;
		nod = gp; // repeat the same procedure for grandparent
	}

	(*root)->color = BLACK;
}

void ffrbt_rm(ffrbtree *tr, ffrbt_node *nod)
{
	ffrbt_node **root = &tr->root;
	void *sentl = &tr->sentl;
	ffrbt_node *x;
	ffrbt_node *next;

	FF_ASSERT(tr->len != 0);

	x = (ffrbt_node*)tree_rm((fftree_node*)nod, (fftree_node**)root, sentl, (fftree_node**)&next);
	tr->len--;

	if (next == NULL) {
		if (nod->color == RED)
			return; // black-height has not been changed

	} else {
		// exchange colors of the node and its successor
		int clr = next->color;
		next->color = nod->color;
		if (clr == RED)
			return; // black-height has not been changed
	}

	// fixup after delete
	while (x != *root && x->color == BLACK) {
		ffrbt_node *sib;
		ffrbt_node *p = x->parent;

		if (x == p->left) {
			sib = p->right;

			if (sib->color == RED) {
				sib->color = BLACK;
				p->color = RED;
				rbt_left_rotate(p, root, sentl);
				sib = p->right;
			}

			if (sib->left->color == RED || sib->right->color == RED) {
				if (sib->right->color == BLACK) {
					sib->left->color = BLACK;
					sib->color = RED;
					rbt_right_rotate(sib, root, sentl);
					sib = p->right;
				}

				sib->color = p->color;
				p->color = BLACK;
				sib->right->color = BLACK;
				rbt_left_rotate(p, root, sentl);
				x = *root;
				break;
			}

		} else {
			sib = p->left;

			if (sib->color == RED) {
				sib->color = BLACK;
				p->color = RED;
				rbt_right_rotate(p, root, sentl);
				sib = p->left;
			}

			if (sib->left->color == RED || sib->right->color == RED) {
				if (sib->left->color == BLACK) {
					sib->right->color = BLACK;
					sib->color = RED;
					rbt_left_rotate(sib, root, sentl);
					sib = p->left;
				}

				sib->color = p->color;
				p->color = BLACK;
				sib->left->color = BLACK;
				rbt_right_rotate(p, root, sentl);
				x = *root;
				break;
			}
		}

		// both children of 'sib' are black
		sib->color = RED;
		x = p; // repeat for parent
	}

	x->color = BLACK;
}


void ffrbtl_insert(ffrbtree *tr, ffrbtl_node *k)
{
	ffrbt_node *n = tr->root;
	ffrbtl_node *found = (ffrbtl_node*)fftree_findnode(k->key, (fftree_node**)&n, &tr->sentl);

	if (found == NULL)
		ffrbt_insert(tr, (ffrbt_node*)k, n);
	else {
		fflist_link(&k->sib, &found->sib);
		tr->len++;
	}
}

/*
     50
    /  \
   25   75
  /  \
15    35[0] <-> 35[1] <-> 35[2] ->|
     /  \
   30    45
*/
void ffrbtl_rm(ffrbtree *tr, ffrbtl_node *k)
{
	ffrbt_node *next;

	FF_ASSERT(tr->len != 0);

	if (k->sib.prev != NULL) {
		// the node is just a list item
		fflist_unlink(&k->sib);
		tr->len--;
		return;
	}

	if (k->sib.next == NULL) {
		// the node has no siblings
		ffrbt_rm(tr, (ffrbt_node*)k);
		return;
	}

	// the node has a sibling
	next = (ffrbt_node*)ffrbtl_nodebylist(k->sib.next);
	next->key = k->key;
	next->color = k->color;
	((ffrbtl_node*)next)->sib.prev = NULL;

	relink_parent((fftree_node*)k, (fftree_node*)next, (fftree_node**)&tr->root, &tr->sentl);

	next->left = &tr->sentl;
	if (k->left != &tr->sentl)
		link_leftchild(next, k->left);

	next->right = &tr->sentl;
	if (k->right != &tr->sentl)
		link_rightchild(next, k->right);

	k->left = k->right = k->parent = NULL;
	k->sib.next = k->sib.prev = NULL;
	tr->len--;
}

int ffrbtl_enumsafe(ffrbtree *tr, fftree_on_item_t on_item, void *udata, uint off)
{
	int rc;
	fftree_node *nod, *next;
	fflist_item *nextli, *li;

	FFTREE_WALKSAFE(tr, nod, next) {

		FFLIST_WALKNEXTSAFE(((ffrbtl_node*)nod)->sib.next, li, nextli) {
			rc = on_item((byte*)ffrbtl_nodebylist(li) - off, udata);
			if (rc != 0)
				return rc;
		}

		rc = on_item((byte*)nod - off, udata);
		if (rc != 0)
			return rc;
	}

	return 0;
}
