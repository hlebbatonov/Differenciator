#include <stdio.h>
#include "binary_tree.h"


BinTreeErrors DiffTree(bin_tree_t* tree);
bin_tree_node_t* DiffNode(bin_tree_node_t* node);
bin_tree_node_t* CopyNode(bin_tree_node_t* node);

#define NUM_(val)       BinTreeCreateNode(bin_tree_elem_t{.num = (double)(val)}, NUM, NULL, NULL, NULL)
#define VAR_(ch)        BinTreeCreateNode(bin_tree_elem_t{.var = (ch)}, VAR, NULL, NULL, NULL)
#define OP_(op, L, R)   BinTreeCreateNode(bin_tree_elem_t{.operand = (op)}, OPERAND, NULL, L, R)
#define ADD_(L, R)      OP_(OP_ADD, L, R)
#define SUB_(L, R)      OP_(OP_SUB, L, R)
#define MUL_(L, R)      OP_(OP_MUL, L, R)
#define DIV_(L, R)      OP_(OP_DIV, L, R)
#define POW_(L, R)      OP_(OP_POW, L, R)
#define LN_(L)          OP_(OP_LN,  L, NULL)
#define LOG_(L, R)      OP_(OP_LOG, L, R)
#define SIN_(L)         OP_(OP_SIN, L, NULL)
#define COS_(L)         OP_(OP_COS, L, NULL)
#define TAN_(L)         OP_(OP_TAN, L, NULL)
#define COT_(L)         OP_(OP_COT, L, NULL)
#define dL              DiffNode(node->left)
#define dR              DiffNode(node->right)
#define cL              CopyNode(node->left)
#define cR              CopyNode(node->right)


int main()
{
    bin_tree_t tree = {};

    BinTreeErrors err = BinTreeInit(&tree);

    BinTreeGetTreeFromFile(&tree, "test.txt");
    DiffTree(&tree);
    BinTreeLispDumpTree(&tree, "test_out.txt");
    BinTreeGraphDumpTree(&tree, "test_gr.txt");
    BinTreeTexDumpTree(&tree, "test_tex.tex");
    BinTreeRemove(&tree);
    fprintf(stderr, "2222");
    fflush(stderr);
}

BinTreeErrors DiffTree(bin_tree_t* tree)
{
    VERIFY_NULL_POINTER(tree);
    bin_tree_node_t* differed_root = DiffNode(tree->root);
    if (differed_root == NULL) return BIN_TREE_ALLOC_ERR;

    BinTreeRemoveSubtree(tree->root);
    tree->root = differed_root;
    tree->num_of_nodes = BinTreeCountNodes(tree->root);

    return BIN_TREE_ERR_OK;
}

bin_tree_node_t* DiffNode(bin_tree_node_t* node)
{
    if (node == NULL) return NULL;

    switch(node->type)
    {
        case NUM: return NUM_(0);
        case VAR: return NUM_(1);
        case OPERAND:
        {
            switch(node->data.operand)
            {
                case OP_ADD: return ADD_(dL, dR);
                case OP_SUB: return SUB_(dL, dR);
                case OP_MUL: return ADD_(MUL_(dL, cR), MUL_(cL, dR));
                case OP_DIV: return DIV_(SUB_(MUL_(dL, cR), MUL_(cL, dR)), POW_(cR, NUM_(2)));

                case OP_POW:
                    if (node->right != NULL && node->right->type == NUM)
                    {
                        double n = node->right->data.num;
                        return MUL_(MUL_(NUM_(n), POW_(cL, NUM_(n - 1))), dL);
                    }
                    else if (node->left->type == NUM)
                        return MUL_(MUL_(POW_(node->left, node->right), LN_(cL)), dR);
                    else if (node->left->type == VAR)
                        return MUL_(POW_(cL, cR), ADD_(MUL_(dR, LN_(cL)), DIV_(MUL_(cR, dL), cL)));
                    return NULL;

                case OP_SIN: return MUL_(COS_(cL), dL);
                case OP_COS: return MUL_(MUL_(NUM_(-1), SIN_(cL)), dL);
                case OP_TAN: return DIV_(dL, POW_(COS_(cL), NUM_(2)));
                case OP_COT: return DIV_(MUL_(NUM_(-1), dL), POW_(SIN_(cL), NUM_(2)));
                case OP_LN:  return DIV_(dL, cL);
                case OP_LOG: return DIV_(SUB_(MUL_(DIV_(dR, cR), LN_(cL)),
                                        MUL_(LN_(cR), DIV_(dL, cL))), POW_(LN_(cL), NUM_(2)));

                case OP_POISON: default: return NULL;
            }
        }
        case POISON_TYPE: default: return NULL;
    }
}

bin_tree_node_t* CopyNode(bin_tree_node_t* node)
{
    if (node == NULL) return NULL;

    return BinTreeCreateNode(node->data, node->type, NULL, CopyNode(node->left), CopyNode(node->right));
}


#undef NUM_
#undef OP_
#undef ADD_
#undef SUB_
#undef MUL_
#undef DIV_
#undef POW_
#undef dL
#undef dR
#undef cL
#undef cR
