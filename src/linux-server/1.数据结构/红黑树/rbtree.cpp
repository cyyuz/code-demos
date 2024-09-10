#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum rbt_color
{
    RBT_BLACK,
    RBT_RED
};

typedef int KEY_TYPE;

struct rbtree_node
{
    enum rbt_color      color;
    struct rbtree_node* right;
    struct rbtree_node* left;
    struct rbtree_node* parent;
    KEY_TYPE            key;
    void*               value;
};

struct rbtree
{
    rbtree_node* root;
    rbtree_node* nil;
};

rbtree_node* rbtree_mini(rbtree* T, rbtree_node* x) {
    while (x->left != T->nil) {
        x = x->left;
    }
    return x;
}

rbtree_node* rbtree_maxi(rbtree* T, rbtree_node* x) {
    while (x->right != T->nil) {
        x = x->right;
    }
    return x;
}

rbtree_node* rbtree_successor(rbtree* T, rbtree_node* x) {
    rbtree_node* y = x->parent;

    if (x->right != T->nil) {
        return rbtree_mini(T, x->right);
    }

    while ((y != T->nil) && (x == y->right)) {
        x = y;
        y = y->parent;
    }
    return y;
}


void rbtree_left_rotate(rbtree* T, rbtree_node* x) {
    rbtree_node* y = x->right;
    // x <--> b
    x->right = y->left;
    if (y->left != T->nil) {
        y->left->parent = x;
    }
    // y <--> parent
    y->parent = x->parent;
    if (x->parent == T->nil) {   // x is root
        T->root = y;
    }
    else if (x == x->parent->left) {   // x is left child
        x->parent->left = y;
    }
    else {   // x is right child
        x->parent->right = y;
    }
    // x <--> y
    y->left = x;
    x->parent = y;
}

void rbtree_right_rotate(rbtree* T, rbtree_node* y) {
    rbtree_node* x = y->left;
    // y <--> b
    y->left = x->right;
    if (x->right != T->nil) {
        x->right->parent = y;
    }
    // x <--> parent
    x->parent = y->parent;
    if (y->parent == T->nil) {   // y is root
        T->root = x;
    }
    else if (y == y->parent->right) {   // y is right child
        y->parent->right = x;
    }
    else {   // y is left child
        y->parent->left = x;
    }
    // x <--> y
    x->right = y;
    y->parent = x;
}

void rbtree_insert_fixup(rbtree* T, rbtree_node* z) {
    while (z->parent->color == rbt_color::RBT_RED) {
        if (z->parent == z->parent->parent->left) {   // z's parent is left child
            rbtree_node* y = z->parent->parent->right;
            if (y->color == rbt_color::RBT_RED) {   // z's uncle is red
                z->parent->color = rbt_color::RBT_BLACK;
                y->color = rbt_color::RBT_BLACK;
                z->parent->parent->color = rbt_color::RBT_RED;
                z = z->parent->parent;   // z' color allways red
            }
            else {                             // z's uncle is black
                if (z == z->parent->right) {   // z is right child, different from z's parent
                    z = z->parent;
                    rbtree_left_rotate(T, z);
                }
                // z is left child, same with z's parent
                // z's parent changes to black, z's grandparent changes to red
                // right rotate z's grandparent
                z->parent->color = rbt_color::RBT_BLACK;
                z->parent->parent->color = rbt_color::RBT_RED;
                rbtree_right_rotate(T, z->parent->parent);
            }
        }
        else {   // z's parent is right child
            rbtree_node* y = z->parent->parent->left;
            if (y->color == rbt_color::RBT_RED) {
                z->parent->color = rbt_color::RBT_BLACK;
                y->color = rbt_color::RBT_BLACK;
                z->parent->parent->color = rbt_color::RBT_RED;
                z = z->parent->parent;
            }
            else {
                if (z == z->parent->left) {
                    z = z->parent;
                    rbtree_right_rotate(T, z);
                }
                z->parent->color = rbt_color::RBT_BLACK;
                z->parent->parent->color = rbt_color::RBT_RED;
                rbtree_left_rotate(T, z->parent->parent);
            }
        }
    }
    T->root->color = rbt_color::RBT_BLACK;
}

void rbtree_insert(rbtree* T, rbtree_node* z) {
    rbtree_node* y = T->nil;
    rbtree_node* x = T->root;
    // binary search
    while (x != T->nil) {
        y = x;
        if (z->key < x->key) {
            x = x->left;
        }
        else if (z->key > x->key) {
            x = x->right;
        }
        else {   // Exist
            return;
        }
    }
    // insert z
    z->parent = y;
    if (y == T->nil) {   // z is root
        T->root = z;
    }
    else if (z->key < y->key) {   // z is left child
        y->left = z;
    }
    else {   // z is right child
        y->right = z;
    }
    z->left = T->nil;
    z->right = T->nil;
    z->color = rbt_color::RBT_RED;
    // fixup
    rbtree_insert_fixup(T, z);
}

void rbtree_delete_fixup(rbtree* T, rbtree_node* x) {

    while ((x != T->root) && (x->color == rbt_color::RBT_BLACK)) {
        if (x == x->parent->left) {

            rbtree_node* w = x->parent->right;
            if (w->color == rbt_color::RBT_RED) {
                w->color = rbt_color::RBT_BLACK;
                x->parent->color = rbt_color::RBT_RED;

                rbtree_left_rotate(T, x->parent);
                w = x->parent->right;
            }

            if ((w->left->color == rbt_color::RBT_BLACK) && (w->right->color == rbt_color::RBT_BLACK)) {
                w->color = rbt_color::RBT_RED;
                x = x->parent;
            }
            else {

                if (w->right->color == rbt_color::RBT_BLACK) {
                    w->left->color = rbt_color::RBT_BLACK;
                    w->color = rbt_color::RBT_RED;
                    rbtree_right_rotate(T, w);
                    w = x->parent->right;
                }

                w->color = x->parent->color;
                x->parent->color = rbt_color::RBT_BLACK;
                w->right->color = rbt_color::RBT_BLACK;
                rbtree_left_rotate(T, x->parent);

                x = T->root;
            }
        }
        else {

            rbtree_node* w = x->parent->left;
            if (w->color == rbt_color::RBT_RED) {
                w->color = rbt_color::RBT_BLACK;
                x->parent->color = rbt_color::RBT_RED;
                rbtree_right_rotate(T, x->parent);
                w = x->parent->left;
            }

            if ((w->left->color == rbt_color::RBT_BLACK) && (w->right->color == rbt_color::RBT_BLACK)) {
                w->color = rbt_color::RBT_RED;
                x = x->parent;
            }
            else {

                if (w->left->color == rbt_color::RBT_BLACK) {
                    w->right->color = rbt_color::RBT_BLACK;
                    w->color = rbt_color::RBT_RED;
                    rbtree_left_rotate(T, w);
                    w = x->parent->left;
                }

                w->color = x->parent->color;
                x->parent->color = rbt_color::RBT_BLACK;
                w->left->color = rbt_color::RBT_BLACK;
                rbtree_right_rotate(T, x->parent);

                x = T->root;
            }
        }
    }

    x->color = rbt_color::RBT_BLACK;
}

rbtree_node* rbtree_delete(rbtree* T, rbtree_node* z) {
    rbtree_node* y = T->nil;
    rbtree_node* x = T->nil;
    if ((z->left == T->nil) || (z->right == T->nil)) {
        y = z;
    }
    else {
        y = rbtree_successor(T, z);
    }

    if (y->left != T->nil) {
        x = y->left;
    }
    else if (y->right != T->nil) {
        x = y->right;
    }

    x->parent = y->parent;
    if (y->parent == T->nil) {
        T->root = x;
    }
    else if (y == y->parent->left) {
        y->parent->left = x;
    }
    else {
        y->parent->right = x;
    }

    if (y != z) {
        z->key = y->key;
        z->value = y->value;
    }

    if (y->color == rbt_color::RBT_BLACK) {
        rbtree_delete_fixup(T, x);
    }

    return y;
}

rbtree_node* rbtree_search(rbtree* T, KEY_TYPE key) {
    rbtree_node* node = T->root;
    while (node != T->nil) {
        if (key < node->key) {
            node = node->left;
        }
        else if (key > node->key) {
            node = node->right;
        }
        else {
            return node;
        }
    }
    return T->nil;
}

void rbtree_traversal(rbtree* T, rbtree_node* node) {
    if (node != T->nil) {
        rbtree_traversal(T, node->left);
        printf("key:%d, color:%d\n", node->key, node->color);
        rbtree_traversal(T, node->right);
    }
}

int main() {

    int keyArray[20] = {24, 25, 13, 35, 23, 26, 67, 47, 38, 98, 20, 19, 17, 49, 12, 21, 9, 18, 14, 15};

    rbtree* T = (rbtree*)malloc(sizeof(rbtree));
    if (T == NULL) {
        printf("malloc failed\n");
        return -1;
    }

    T->nil = (rbtree_node*)malloc(sizeof(rbtree_node));
    T->nil->color = rbt_color::RBT_BLACK;
    T->root = T->nil;

    rbtree_node* node = T->nil;
    int          i = 0;
    for (i = 0; i < 20; i++) {
        node = (rbtree_node*)malloc(sizeof(rbtree_node));
        node->key = keyArray[i];
        node->value = NULL;

        rbtree_insert(T, node);
    }

    rbtree_traversal(T, T->root);
    printf("----------------------------------------\n");

    for (i = 0; i < 20; i++) {

        rbtree_node* node = rbtree_search(T, keyArray[i]);
        rbtree_node* cur = rbtree_delete(T, node);
        free(cur);

        rbtree_traversal(T, T->root);
        printf("----------------------------------------\n");
    }
}
