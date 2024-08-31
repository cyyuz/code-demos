
const int RB_RED = 0;
const int RB_BLACK = 1;


typedef struct rbtree_node
{
    int                 key;
    void*               value;
    struct rbtree_node* left;
    struct rbtree_node* right;
    struct rbtree_node* parent;
    int                 color;
} rbtree_node_t;


typedef struct rbtree
{
    rbtree_node_t* root;
    rbtree_node_t* nil;
} rbtree_t;


void rbtree_left_rotate(rbtree_t* tree, rbtree_node_t* x) {
    rbtree_node_t* y = x->right;

    x->right = y->left;
    if (y->left != tree->nil) {
        y->left->parent = x;
    }

    y->parent = x->parent;
    if (x->parent == tree->nil) {   // x is root
        tree->root = y;
    }
    else if (x == x->parent->left) {   // x is left child
        x->parent->left = y;
    }
    else {   // x is right child
        x->parent->right = y;
    }

    y->left = x;
    x->parent = y;
}

void rbtree_right_rotate(rbtree_t* tree, rbtree_node_t* y) {
    rbtree_node_t* x = y->left;

    y->left = x->right;
    if (x->right != tree->nil) {
        x->right->parent = y;
    }

    x->parent = y->parent;
    if (y->parent == tree->nil) {   // y is root
        tree->root = x;
    }
    else if (y == y->parent->left) {   // y is left child
        y->parent->left = x;
    }
    else {   // y is right child
        y->parent->right = x;
    }
    x->right = y;
    y->parent = x;
}

void rbtree_insert_fixup(rbtree_t* tree, rbtree_node_t* z) {
    // z is the newly inserted node, and it is red
    while (z->parent->color == RB_RED) {
        if (z->parent == z->parent->parent->left) {   // z's parent is left child
            rbtree_node_t* y = z->parent->parent->right;
            if (y->color == RB_RED) {
                z->parent->color = RB_BLACK;
                y->color = RB_BLACK;
                z->parent->parent->color = RB_RED;
                z = z->parent->parent;   // z is red
            }
            else {
                // z's parent is left child, z is right child
                if (z == z->parent->right) {
                    z = z->parent;
                    rbtree_left_rotate(tree, z);
                }
                // z and z's parent are left children
                z->parent->color = RB_BLACK;
                z->parent->parent->color = RB_RED;
                rbtree_left_rotate(tree, z->parent->parent);
            }
        }
        else {
            // z's parent is right child
        }
    }
}

void rbtree_insert(rbtree_t* tree, rbtree_node_t* z) {
    rbtree_node_t* x = tree->root;
    rbtree_node_t* y = tree->nil;
    while (x != tree->nil) {
        y = x;
        if (z->key < x->key) {
            x = x->left;
        }
        else if (z->key > x->key) {
            x = x->right;
        }
        else {
            return;   // key already exists
        }
    }
    if (y == tree->nil) {   // tree is empty
        tree->root = z;
    }
    else if (z->key < y->key) {   // y == x->parent
        y->left = z;
    }
    else {
        y->right = z;
    }

    z->parent = y;
    z->left = tree->nil;
    z->right = tree->nil;
    z->color = RB_RED;   // new node must be red
}