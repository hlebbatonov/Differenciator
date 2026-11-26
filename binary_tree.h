#include <stdlib.h>

#define VERIFY_ERROR(err) do {if (err != BIN_TREE_ERR_OK) return err;} while(0)

#define VERIFY_NULL_POINTER(pointer) do                    \
{                                                          \
    if (pointer == NULL) return BIN_TREE_ERR_NULL_POINTER; \
} while(0)

enum OP_CODE
{
    OP_POISON = -1,
    OP_ADD = 0,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_POW,
    OP_SIN,
    OP_COS,
    OP_TAN,
    OP_COT,
    OP_LN,
    OP_LOG
};

enum DUMP_FILE_TYPE
{
    POISON_FILE = -1,
    LISP_FILE = 0,
    DOT_FILE,
    TEX_FILE,
    NUM_OF_FILE_TYPES
};

struct op_info
{
    OP_CODE op_code;
    const char* lisp_name;
    const char* tex_name;
    unsigned int num_of_args;
};

static const op_info op_table[] =
{
    [OP_ADD] = {OP_ADD, "+",   "+",      2},
    [OP_SUB] = {OP_SUB, "-",   "-",      2},
    [OP_MUL] = {OP_MUL, "*",   "\\cdot", 2},
    [OP_DIV] = {OP_DIV, "/",   "/", 2},
    [OP_POW] = {OP_POW, "^",   "^",      2},
    [OP_SIN] = {OP_SIN, "sin", "\\sin",  1},
    [OP_COS] = {OP_COS, "cos", "\\cos",  1},
    [OP_TAN] = {OP_TAN, "tg",  "\\tan",  1},
    [OP_COT] = {OP_COT, "ctg", "\\cot",  1},
    [OP_LN]  = {OP_LN,  "ln",  "\\ln",   1},
    [OP_LOG] = {OP_LOG, "log", "\\log",  2}
};
static const size_t OP_TABLE_SIZE = sizeof(op_table) / sizeof(op_table[0]);


union bin_tree_elem_t
{
    double num;
    char var;
    OP_CODE operand;
};

enum DifferType
{
    POISON_TYPE = -1,
    NUM = 0,
    VAR,
    OPERAND
};

static const char* BIN_TREE_ELEM_SPEC[] =
{
    [NUM] = "%lg",
    [VAR] = "%c",
    [OPERAND] = "%s"
};

enum BinTreeErrors
{
    BIN_TREE_ERR_OK = 0,

    BIN_TREE_ERR_NULL_POINTER,
    BIN_TREE_ERR_NODE_NULL_POINTER,
    BIN_TREE_ERR_LEFT_NULL_POINTER,
    BIN_TREE_ERR_RIGHT_NULL_POINTER,
    BIN_TREE_ERR_PARENT_NULL_POINTER,

    BIN_TREE_ERR_LENGTH_TOO_BIG,

    BIN_TREE_ERR_NODE_CALLOC_ERROR,
    BIN_TREE_ERR_LEFT_CALLOC_ERROR,
    BIN_TREE_ERR_RIGHT_CALLOC_ERROR,

    BIN_TREE_ERR_GOT_POISON,
    BIN_TREE_ERR_FILE_NOT_OPENED,
    BIN_TREE_ERR_DUPLICATE_ELEM,
    BIN_TREE_ERR_WRONG_NUM_OF_NODES,
    ERR_CANT_GET_SIZE_OF_FILE,
    ERR_CANT_GET_INFO_OF_FILE,
    BIN_TREE_SYNTAX_ERR,

    BIN_TREE_ALLOC_ERR
};

struct bin_tree_node_t
{
    DifferType type;
    bin_tree_elem_t data;
    bin_tree_node_t* left;
    bin_tree_node_t* right;
    bin_tree_node_t* parent;
};

struct bin_tree_t
{
    bin_tree_node_t* root;
    unsigned int num_of_nodes;
    int (*bin_tree_comparator)(bin_tree_elem_t, bin_tree_elem_t);
};

const int MAX_CAPACITY = 500;
const int LEN_OF_POINTER_NAME = 32;
const int LEN_OF_DATA_STR = 16;
const double POISON = 0xBEDA;

BinTreeErrors BinTreeInit(bin_tree_t* tree);
BinTreeErrors BinTreeRemove(bin_tree_t* tree);
void          BinTreeRemoveSubtree(bin_tree_node_t* node);
unsigned int  BinTreeCountNodes(bin_tree_node_t* node);
BinTreeErrors BinTreeVerify(bin_tree_t* tree);
BinTreeErrors BinTreeLispDumpTree(bin_tree_t* tree, const char* lisp_filename);
BinTreeErrors BinTreeTexDumpTree(bin_tree_t* tree, const char* tex_filename);
BinTreeErrors BinTreeGraphDumpTree(bin_tree_t* tree, const char* dot_filename);
BinTreeErrors BinTreeGetTreeFromFile(bin_tree_t* tree, const char* lisp_filename);
BinTreeErrors BinTreeAllocateNode(bin_tree_node_t** node, bin_tree_elem_t elem,
                                    DifferType type, bin_tree_node_t* parent);
bin_tree_node_t* BinTreeCreateNode(bin_tree_elem_t elem, DifferType type, bin_tree_node_t* parent,
                                    bin_tree_node_t* left, bin_tree_node_t* right);
