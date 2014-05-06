#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <strings.h>
#include <assert.h>

#include "rb.h"
#define	rbtn_black_height(a_type, a_field, a_rbt, r_height) do {	\
    a_type *rbp_bh_t;							\
    for (rbp_bh_t = (a_rbt)->rbt_root, (r_height) = 0;			\
      rbp_bh_t != &(a_rbt)->rbt_nil;					\
      rbp_bh_t = rbtn_left_get(a_type, a_field, rbp_bh_t)) {		\
	if (rbtn_red_get(a_type, a_field, rbp_bh_t) == false) {		\
	    (r_height)++;						\
	}								\
    }									\
} while (0)

#define NNODES 200
#define NSETS 200

static bool verbose = false;
static bool tree_print = false;
static bool forward_print = false;
static bool reverse_print = false;

typedef struct node_s node_t;

struct node_s {
#define NODE_MAGIC 0x9823af7e
    uint32_t magic;
    rb_node(node_t) link;
    long key;
};

static int
nodeCmp(node_t *aA, node_t *aB) {
    int rVal;

    assert(aA->magic == NODE_MAGIC);
    assert(aB->magic == NODE_MAGIC);

    rVal = (aA->key > aB->key) - (aA->key < aB->key);
    if (rVal == 0) {
	// Duplicates are not allowed in the tree, so force an arbitrary
	// ordering for non-identical items with equal keys.
	rVal = (((uintptr_t) aA) > ((uintptr_t) aB))
	  - (((uintptr_t) aA) < ((uintptr_t) aB));
    }
    return rVal;
}

typedef rbt(node_t) tree_t;
rb_gen(static, tree_, tree_t, node_t, link, nodeCmp);

static unsigned
treeRecurse(node_t *aNode, unsigned aBlackHeight, unsigned aBlackDepth,
  node_t *aNil) {
    unsigned rVal = 0;
    node_t *leftNode = rbtn_left_get(node_t, link, aNode);
    node_t *rightNode = rbtn_right_get(node_t, link, aNode);

    if (rbtn_red_get(node_t, link, aNode) == false) {
	aBlackDepth++;
    }

    // Red nodes must be interleaved with black nodes.
    if (rbtn_red_get(node_t, link, aNode)) {
	node_t *tNode = rbtn_left_get(node_t, link, leftNode);
	assert(rbtn_red_get(node_t, link, leftNode) == false);
	tNode = rbtn_right_get(node_t, link, leftNode);
	assert(rbtn_red_get(node_t, link, leftNode) == false);
    }

    if (aNode == aNil) {
	if (tree_print) {
	    fprintf(stderr, ".");
	}
	return rVal;
    }
    /* Self. */
    assert(aNode->magic == NODE_MAGIC);
    if (tree_print) {
	fprintf(stderr, "%ld%c", aNode->key,
	  rbtn_red_get(node_t, link, aNode) ? 'r' : 'b');
    }

    /* Left subtree. */
    if (leftNode != aNil) {
	if (tree_print) {
	    fprintf(stderr, "[");
	}
	rVal += treeRecurse(leftNode, aBlackHeight, aBlackDepth, aNil);
	if (tree_print) {
	    fprintf(stderr, "]");
	}
    } else {
	rVal += (aBlackDepth != aBlackHeight);
	if (tree_print) {
	    fprintf(stderr, ".");
	}
    }

    /* Right subtree. */
    if (rightNode != aNil) {
	if (tree_print) {
	    fprintf(stderr, "<");
	}
	rVal += treeRecurse(rightNode, aBlackHeight, aBlackDepth, aNil);
	if (tree_print) {
	    fprintf(stderr, ">");
	}
    } else {
	rVal += (aBlackDepth != aBlackHeight);
	if (tree_print) {
	    fprintf(stderr, ".");
	}
    }

    return rVal;
}

static node_t *
treeIterateCb(tree_t *aTree, node_t *aNode, void *data) {
    unsigned *i = (unsigned *)data;
    node_t *sNode;

    assert(aNode->magic == NODE_MAGIC);
    if (forward_print) {
	if (*i != 0) {
	    fprintf(stderr, "-->");
	}
	fprintf(stderr, "%ld", aNode->key);
    }

    /* Test rb_search(). */
    sNode = tree_search(aTree, aNode);
    assert(sNode == aNode);

    /* Test rb_nsearch(). */
    sNode = tree_nsearch(aTree, aNode);
    assert(sNode == aNode);

    /* Test rb_psearch(). */
    sNode = tree_psearch(aTree, aNode);
    assert(sNode == aNode);

    (*i)++;

    return NULL;
}

static unsigned
treeIterate(tree_t *aTree) {
    unsigned i;

    i = 0;
    tree_iter(aTree, NULL, treeIterateCb, (void *)&i);

    return i;
}

static node_t *
treeReverseIterateCb(tree_t *aTree, node_t *aNode, void *data) {
    unsigned *i = (unsigned *)data;
    node_t *sNode;

    assert(aNode->magic == NODE_MAGIC);
    if (reverse_print) {
	if (*i != 0) {
	    fprintf(stderr, "<--");
	}
	fprintf(stderr, "%ld", aNode->key);
    }

    /* Test rb_search(). */
    sNode = tree_search(aTree, aNode);
    assert(sNode == aNode);

    /* Test rb_nsearch(). */
    sNode = tree_nsearch(aTree, aNode);
    assert(sNode == aNode);

    /* Test rb_psearch(). */
    sNode = tree_psearch(aTree, aNode);
    assert(sNode == aNode);

    (*i)++;

    return NULL;
}

static unsigned
treeIterateReverse(tree_t *aTree) {
    unsigned i;

    i = 0;
    tree_reverse_iter(aTree, NULL, treeReverseIterateCb, (void *)&i);

    return i;
}

static void
nodeRemove(tree_t *aTree, node_t *aNode, unsigned aNNodes) {
    node_t *sNode;
    unsigned blackHeight, imbalances;

    if (verbose) {
	fprintf(stderr, "rb_remove(%3ld)", aNode->key);
    }
    tree_remove(aTree, aNode);

    /* Test rb_nsearch(). */
    sNode = tree_nsearch(aTree, aNode);
    assert(sNode == NULL || sNode->key >= aNode->key);

    /* Test rb_psearch(). */
    sNode = tree_psearch(aTree, aNode);
    assert(sNode == NULL || sNode->key <= aNode->key);

    aNode->magic = 0;

    if (tree_print) {
	fprintf(stderr, "\n\t   tree: ");
    }
    rbtn_black_height(node_t, link, aTree, blackHeight);
    imbalances = treeRecurse(aTree->rbt_root, blackHeight, 0,
      &(aTree->rbt_nil));
    if (imbalances != 0) {
	fprintf(stderr, "\nTree imbalance\n");
	abort();
    }
    if (forward_print) {
	fprintf(stderr, "\n\tforward: ");
    }
    assert(aNNodes - 1 == treeIterate(aTree));
    if (reverse_print) {
	fprintf(stderr, "\n\treverse: ");
    }
    assert(aNNodes - 1 == treeIterateReverse(aTree));
    if (verbose) {
	fprintf(stderr, "\n");
    }
}

static node_t *
removeIterateCb(tree_t *aTree, node_t *aNode, void *data) {
    unsigned *nNodes = (unsigned *)data;
    node_t *ret = tree_next(aTree, aNode);

    nodeRemove(aTree, aNode, *nNodes);

    return ret;
}

static node_t *
removeReverseIterateCb(tree_t *aTree, node_t *aNode, void *data) {
    unsigned *nNodes = (unsigned *)data;
    node_t *ret = tree_prev(aTree, aNode);

    nodeRemove(aTree, aNode, *nNodes);

    return ret;
}

int
main(void) {
    tree_t tree;
    long set[NNODES];
    node_t nodes[NNODES], key, *sNode, *nodeA;
    unsigned i, j, k, blackHeight, imbalances;

    fprintf(stderr, "Test begin\n");

    /* Initialize tree. */
    tree_new(&tree);

    /*
     * Empty tree.
     */
    fprintf(stderr, "Empty tree:\n");

    /* rb_first(). */
    nodeA = tree_first(&tree);
    if (nodeA == NULL) {
	fprintf(stderr, "rb_first() --> nil\n");
    } else {
	fprintf(stderr, "rb_first() --> %ld\n", nodeA->key);
    }

    /* rb_last(). */
    nodeA = tree_last(&tree);
    if (nodeA == NULL) {
	fprintf(stderr, "rb_last() --> nil\n");
    } else {
	fprintf(stderr, "rb_last() --> %ld\n", nodeA->key);
    }

    /* rb_search(). */
    key.key = 0;
    key.magic = NODE_MAGIC;
    nodeA = tree_search(&tree, &key);
    if (nodeA == NULL) {
	fprintf(stderr, "rb_search(0) --> nil\n");
    } else {
	fprintf(stderr, "rb_search(0) --> %ld\n", nodeA->key);
    }

    /* rb_nsearch(). */
    key.key = 0;
    key.magic = NODE_MAGIC;
    nodeA = tree_nsearch(&tree, &key);
    if (nodeA == NULL) {
	fprintf(stderr, "rb_nsearch(0) --> nil\n");
    } else {
	fprintf(stderr, "rb_nsearch(0) --> %ld\n", nodeA->key);
    }

    /* rb_psearch(). */
    key.key = 0;
    key.magic = NODE_MAGIC;
    nodeA = tree_psearch(&tree, &key);
    if (nodeA == NULL) {
	fprintf(stderr, "rb_psearch(0) --> nil\n");
    } else {
	fprintf(stderr, "rb_psearch(0) --> %ld\n", nodeA->key);
    }

    /* rb_insert(). */
    srandom(42);
    for (i = 0; i < NSETS; i++) {
	if (i == 0) {
	    // Insert in order.
	    for (j = 0; j < NNODES; j++) {
		set[j] = j;
	    }
	} else if (i == 1) {
	    // Insert in reverse order.
	    for (j = 0; j < NNODES; j++) {
		set[j] = NNODES - j - 1;
	    }
	} else {
	    for (j = 0; j < NNODES; j++) {
		set[j] = (long) (((double) NNODES)
		  * ((double) random() / ((double)RAND_MAX)));
	    }
	}

	fprintf(stderr, "Tree %u\n", i);
	for (j = 1; j <= NNODES; j++) {
	    if (verbose) {
		fprintf(stderr, "Tree %u, %u node%s\n",
		  i, j, j != 1 ? "s" : "");
	    }

	    /* Initialize tree and nodes. */
	    tree_new(&tree);
	    tree.rbt_nil.magic = 0;
	    for (k = 0; k < j; k++) {
		nodes[k].magic = NODE_MAGIC;
		nodes[k].key = set[k];
	    }

	    /* Insert nodes. */
	    for (k = 0; k < j; k++) {
		if (verbose) {
		    fprintf(stderr, "rb_insert(%3ld)", nodes[k].key);
		}
		tree_insert(&tree, &nodes[k]);

		if (tree_print) {
		    fprintf(stderr, "\n\t   tree: ");
		}
		rbtn_black_height(node_t, link, &tree, blackHeight);
		imbalances = treeRecurse(tree.rbt_root, blackHeight, 0,
		  &(tree.rbt_nil));
		if (imbalances != 0) {
		    fprintf(stderr, "\nTree imbalance\n");
		    abort();
		}
		if (forward_print) {
		    fprintf(stderr, "\n\tforward: ");
		}
		assert(k + 1 == treeIterate(&tree));
		if (reverse_print) {
		    fprintf(stderr, "\n\treverse: ");
		}
		assert(k + 1 == treeIterateReverse(&tree));
		if (verbose) {
		    fprintf(stderr, "\n");
		}

		sNode = tree_first(&tree);
		assert(sNode != NULL);

		sNode = tree_last(&tree);
		assert(sNode != NULL);

		sNode = tree_next(&tree, &nodes[k]);
		sNode = tree_prev(&tree, &nodes[k]);
	    }

	    /* Remove nodes. */
	    switch (i % 4) {
		case 0: {
		    for (k = 0; k < j; k++) {
			nodeRemove(&tree, &nodes[k], j - k);
		    }
		    break;
		} case 1: {
		    for (k = j; k > 0; k--) {
			nodeRemove(&tree, &nodes[k-1], k);
		    }
		    break;
		} case 2: {
		    node_t *start;
		    unsigned nNodes = j;

		    start = NULL;
		    do {
			start = tree_iter(&tree, start, removeIterateCb,
			  (void *)&nNodes);
			nNodes--;
		    } while (start != NULL);
		    assert(nNodes == 0);
		    break;
		} case 3: {
		    node_t *start;
		    unsigned nNodes = j;

		    start = NULL;
		    do {
			start = tree_reverse_iter(&tree, start,
			  removeReverseIterateCb, (void *)&nNodes);
			nNodes--;
		    } while (start != NULL);
		    assert(nNodes == 0);
		    break;
		} default: {
		    assert(false);
		}
	    }
	}
    }

    fprintf(stderr, "Test end\n");
    return 0;
}
