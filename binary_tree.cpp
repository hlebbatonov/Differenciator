#include <stdio.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "binary_tree.h"

#define $(x) #x

static BinTreeErrors BinTreeVerifySubtree(bin_tree_node_t* node, unsigned int* curr_num_of_elems);
static BinTreeErrors BinTreeLispDumpNode(bin_tree_node_t* node, FILE* lisp_file);
static BinTreeErrors BinTreeGraphDumpSubtree(bin_tree_node_t* node, FILE* html_file);
static BinTreeErrors BinTreeTexDumpSubtree(bin_tree_node_t* node, FILE* tex_file);
static void          BinTreePointerToString(bin_tree_node_t* pointer, char* buffer);
static BinTreeErrors BinTreeLispGetNodeFromBuffer(bin_tree_node_t** node, const char** curr_pos,
                                                    size_t* curr_num_of_nodes);
BinTreeErrors        BinTreeAllocateNode(bin_tree_node_t** node, bin_tree_elem_t elem, bin_tree_node_t* parent);
static void          BinTreeSkipSpaces(const char** curr_pos);
static void BinTreeDataToString(bin_tree_node_t* node, char* buffer, DUMP_FILE_TYPE file_type);
static const         op_info* BinTreeGetOp(const char* data_start, size_t data_len);
static BinTreeErrors BinTreeParseNode(const char* data_start, const char* data_end,
                                       bin_tree_elem_t* elem, DifferType* elem_type);

BinTreeErrors BinTreeInit(bin_tree_t* tree)
{
    VERIFY_NULL_POINTER(tree);
    //VERIFY_NULL_POINTER(comparator_func);
    tree->root = NULL;
    tree->num_of_nodes = 0;
    //tree->bin_tree_comparator = comparator_func;
    return BIN_TREE_ERR_OK;
}

BinTreeErrors BinTreeRemove(bin_tree_t* tree)
{
    VERIFY_NULL_POINTER(tree);

    BinTreeRemoveSubtree(tree->root);
    tree->num_of_nodes = 0;

    return BIN_TREE_ERR_OK;
}

void BinTreeRemoveSubtree(bin_tree_node_t* node)
{
    if (node == NULL) return ;

    BinTreeRemoveSubtree(node->left);
    BinTreeRemoveSubtree(node->right);
    node->type = POISON_TYPE;
    node->data.num = POISON;
    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;
    free(node);
}

BinTreeErrors BinTreeVerify(bin_tree_t* tree)
{
    VERIFY_NULL_POINTER(tree);
    VERIFY_NULL_POINTER(tree->bin_tree_comparator);
    unsigned int curr_num_of_elems = 0;
    VERIFY_ERROR(BinTreeVerifySubtree(tree->root, &curr_num_of_elems));
    if (curr_num_of_elems != tree->num_of_nodes) return BIN_TREE_ERR_WRONG_NUM_OF_NODES;
    return BIN_TREE_ERR_OK;
}


static BinTreeErrors BinTreeVerifySubtree(bin_tree_node_t* node, unsigned int* curr_num_of_elems)
{
    if (node == NULL) return BIN_TREE_ERR_OK;
    (*curr_num_of_elems)++;
    if (node->type == POISON_TYPE) return BIN_TREE_ERR_GOT_POISON;
    if (node->data.num == POISON) return BIN_TREE_ERR_GOT_POISON;
    if (*curr_num_of_elems > MAX_CAPACITY) return BIN_TREE_ERR_LENGTH_TOO_BIG;
    VERIFY_ERROR(BinTreeVerifySubtree(node->left, curr_num_of_elems));
    VERIFY_ERROR(BinTreeVerifySubtree(node->right, curr_num_of_elems));
    return BIN_TREE_ERR_OK;
}

bin_tree_node_t* BinTreeCreateNode(bin_tree_elem_t elem, DifferType type, bin_tree_node_t* parent,
                                    bin_tree_node_t* left, bin_tree_node_t* right)
{
    bin_tree_node_t* new_node = NULL;
    if (BinTreeAllocateNode(&new_node, elem, type, parent) != BIN_TREE_ERR_OK) return NULL;
    new_node->left  = left;
    new_node->right = right;
    if (left != NULL) left->parent  = new_node;
    if (right != NULL) right->parent = new_node;

    return new_node;
}

BinTreeErrors BinTreeAllocateNode(
                bin_tree_node_t** node,
                bin_tree_elem_t elem,
                DifferType type,
                bin_tree_node_t* parent)
{
    if (node == NULL) return BIN_TREE_ERR_NULL_POINTER;

    bin_tree_node_t* new_node = (bin_tree_node_t*)calloc(1, sizeof(bin_tree_node_t));
    if (new_node == NULL) return BIN_TREE_ERR_NODE_CALLOC_ERROR;

    new_node->type   = type;
    new_node->data   = elem;
    new_node->parent = parent;
    new_node->left   = NULL;
    new_node->right  = NULL;

    *node = new_node;
    return BIN_TREE_ERR_OK;
}

unsigned int BinTreeCountNodes(bin_tree_node_t* node)
{
    if (node == NULL) return 0;

    return 1 + BinTreeCountNodes(node->left) + BinTreeCountNodes(node->right);
}

//SECTION - Dump functions

BinTreeErrors BinTreeLispDumpTree(bin_tree_t* tree, const char* lisp_filename)
{
    if (tree == NULL)
    {
        fprintf(stderr, "Error: got NULL tree pointer\n");
        return BIN_TREE_ERR_NULL_POINTER;
    }

    if (lisp_filename == NULL)
    {
        fprintf(stderr, "Error: program filename crashed\n");
        return BIN_TREE_ERR_FILE_NOT_OPENED;
    }

    FILE* lisp_file = fopen(lisp_filename, "w");
    if (lisp_file == NULL)
    {
        fprintf(stderr, "Error: cannot open file <%s>: %s\n", lisp_filename, strerror(errno));
        return BIN_TREE_ERR_FILE_NOT_OPENED;
    }

    BinTreeLispDumpNode(tree->root, lisp_file);

    fclose(lisp_file);
    return BIN_TREE_ERR_OK;
}

static BinTreeErrors BinTreeLispDumpNode(bin_tree_node_t* node, FILE* lisp_file)
{
    if (lisp_file == NULL) return BIN_TREE_ERR_FILE_NOT_OPENED;

    fprintf(lisp_file, "(");
    if (node == NULL) {fprintf(lisp_file, ")"); return BIN_TREE_ERR_OK;}

    char data_str[LEN_OF_DATA_STR] = {};
    BinTreeDataToString(node, data_str, LISP_FILE);

    fprintf(lisp_file, "%s", data_str);
    BinTreeLispDumpNode(node->left, lisp_file);
    BinTreeLispDumpNode(node->right, lisp_file);

    fprintf(lisp_file, ")");
    return BIN_TREE_ERR_OK;
}

BinTreeErrors BinTreeGraphDumpTree(bin_tree_t* tree, const char* dot_filename)
{
    if (tree == NULL) {fprintf(stderr, "Error: got NULL tree pointer\n"); return BIN_TREE_ERR_NULL_POINTER;}

    if (dot_filename == NULL)
    {
        fprintf(stderr, "Error: program filename crashed\n");
        return BIN_TREE_ERR_FILE_NOT_OPENED;
    }

    FILE* dot_file = fopen(dot_filename, "w");
    if (dot_file == NULL)
    {
        fprintf(stderr, "Error: cannot open file <%s>: %s\n", dot_filename, strerror(errno));
        return BIN_TREE_ERR_FILE_NOT_OPENED;
    }

    //Head
    fprintf(dot_file, "digraph Tree_Dump\n{\n");
    fprintf(dot_file, "rankdir=TB;\n");
    fprintf(dot_file, "splines=true;\n");
    fprintf(dot_file, "nodesep=1.0;\n");
    fprintf(dot_file, "ranksep=0.6;\n");
    fprintf(dot_file, "node [shape=record, style=\"filled,rounded\", fontname=\"Arial\", group=1];\n\n");

    BinTreeGraphDumpSubtree(tree->root, dot_file);

    fprintf(dot_file, "\n");
    fprintf(dot_file, "\n}");
    fclose(dot_file);

#ifdef DOT_MODE_IS_ON
    char dot_command[LEN_OF_COMMAND] = {};
    snprintf(dot_command, sizeof(dot_command), "dot -Tsvg %s -o _%s_.svg ", dot_filename, dot_filename);
    system(dot_command);
#endif
    return BIN_TREE_ERR_OK;
}

static BinTreeErrors BinTreeGraphDumpSubtree(bin_tree_node_t* node, FILE* dot_file)
{
    if (dot_file == NULL) {fprintf(stderr, "Error: no dot file\n"); return BIN_TREE_ERR_FILE_NOT_OPENED;}
    if (node == NULL) return BIN_TREE_ERR_OK;

    char left_str[LEN_OF_POINTER_NAME]   = {};
    char right_str[LEN_OF_POINTER_NAME]  = {};
    char parent_str[LEN_OF_POINTER_NAME] = {};
    char data_str[LEN_OF_DATA_STR] = {};

    BinTreePointerToString(node->left, left_str);
    BinTreePointerToString(node->right, right_str);
    BinTreePointerToString(node->parent, parent_str);
    BinTreeDataToString(node, data_str, DOT_FILE);

    if (node->left != NULL || node->right != NULL)
    {
        fprintf(dot_file, "node_%p [label=\"{data: %s | pointer: %p| \
        {left: %s | right: %s} | parent: %s}\", \
        fillcolor=\"#E3F2FD\"];\n", (void*)node, data_str, (void*)node, left_str, right_str, parent_str);

        if (node->left != NULL)
        {
            fprintf(dot_file, "node_%p -> node_%p [color=\"#4CAF50\", label=\"L\"];\n",
                (void*)node, (void*)node->left);
            BinTreeGraphDumpSubtree(node->left, dot_file);
        }
        else
        {
            fprintf(dot_file, "null_left_%p [style=invis];\n", (void*)node);
            fprintf(dot_file, "node_%p -> null_left_%p [style=invis];\n", (void*)node, (void*)node);
        }

        if (node->right != NULL)
        {
            fprintf(dot_file, "node_%p -> node_%p [color=\"#F44336\", label=\"R\"];\n",
                (void*)node, (void*)node->right);
            BinTreeGraphDumpSubtree(node->right, dot_file);
        }
        else
        {
            fprintf(dot_file, "null_right_%p [style=invis];\n", (void*)node);
            fprintf(dot_file, "node_%p -> null_right_%p [style=invis];\n", (void*)node, (void*)node);
        }
    }
    else
        fprintf(dot_file, "node_%p [label=\"{data: %s | pointer: %p| \
        {left: %s | right: %s} | parent: %s}\", \
        fillcolor=\"#e0d6f1\"];\n", (void*)node, data_str, (void*)node, left_str, right_str, parent_str);

    return BIN_TREE_ERR_OK;
}

BinTreeErrors BinTreeTexDumpTree(bin_tree_t* tree, const char* tex_filename)
{
    static bool tex_file_first_run = true;

    if (tree == NULL) {fprintf(stderr, "Error: got NULL tree pointer\n"); return BIN_TREE_ERR_NULL_POINTER;}

    if (tex_filename == NULL)
    {
        fprintf(stderr, "Error: program filename crashed\n");
        return BIN_TREE_ERR_FILE_NOT_OPENED;
    }

    FILE* tex_file = fopen(tex_filename, (tex_file_first_run == true)? "w" : "r+");
    if (tex_file == NULL)
    {
        tex_file = fopen(tex_filename, "wb");
        if (tex_file == NULL)
        {
            fprintf(stderr, "Error: cannot open file <%s>: %s\n", tex_filename, strerror(errno));
            return BIN_TREE_ERR_FILE_NOT_OPENED;
        }
        tex_file_first_run = true;
    }
    const char* tex_file_end = "\n\\end{document}\n";
    //Head
    if (tex_file_first_run == true)
    {
        fprintf(tex_file, "\\documentclass[a4paper,12pt]{article}\n"
                        "\\usepackage[utf8]{inputenc}\n"
                        "\\usepackage[T2A]{fontenc}\n"
                        "\\usepackage{breqn}\n"
                        "\\usepackage[russian]{babel}\n"
                        "\\usepackage{amsmath, amssymb}\n"
                        "\\usepackage[left=1.5cm, right=1.5cm, top=2cm, bottom=2cm]{geometry}\n\n"
                        "\\begin{document}\n\n");
        tex_file_first_run = false;
    }
    else
    {
        fseek(tex_file, -(long)strlen(tex_file_end), SEEK_END);
    }

    fprintf(tex_file, "\\begin{dmath}\n");
    BinTreeTexDumpSubtree(tree->root, tex_file);
    fprintf(tex_file, "\n\\end{dmath}\n");

    fprintf(tex_file, "%s", tex_file_end);

    fclose(tex_file);
    return BIN_TREE_ERR_OK;
}

static BinTreeErrors BinTreeTexDumpSubtree(bin_tree_node_t* node, FILE* tex_file)
{
    VERIFY_NULL_POINTER(tex_file);

    fprintf(tex_file, "{");
    if (node == NULL) {fprintf(tex_file, "}"); return BIN_TREE_ERR_OK;}

    char buffer[LEN_OF_POINTER_NAME] = {};
    BinTreeDataToString(node, buffer, TEX_FILE);
    if (node->type == OPERAND && op_table[node->data.operand].num_of_args == 2)
    {
        BinTreeTexDumpSubtree(node->left, tex_file);
        fprintf(tex_file, "%s", buffer);
        BinTreeTexDumpSubtree(node->right, tex_file);
    }
    else if (op_table[node->data.operand].num_of_args == 1)
    {
        fprintf(tex_file, "%s", buffer);
        BinTreeTexDumpSubtree(node->left, tex_file);
    }
    else
        fprintf(tex_file, "%s", buffer);

    fprintf(tex_file, "}");
    return BIN_TREE_ERR_OK;
}

void CompilePDFTex(const char* tex_filename)
{
    struct stat tex_file_info = {};
    if (stat(tex_filename, &tex_file_info) != 0) return;
#ifdef LATEX_MODE_IS_ON
    char latex_command[LEN_OF_COMMAND] = {};
    snprintf(latex_command, sizeof(latex_command), "xelatex .\\%s", tex_filename);
    system(latex_command);
#endif

}

static void BinTreePointerToString(bin_tree_node_t* pointer, char* buffer)
{
    assert(buffer != NULL);

    if (pointer == NULL)
        snprintf(buffer, LEN_OF_POINTER_NAME, "NULL");
    else
        snprintf(buffer, LEN_OF_POINTER_NAME, "%p", (void*)pointer);
}

static void BinTreeDataToString(bin_tree_node_t* node, char* buffer, DUMP_FILE_TYPE file_type)
{
    assert(node != NULL);
    assert(buffer != NULL);
    if (file_type <= POISON_FILE || file_type >= NUM_OF_FILE_TYPES)
        {fprintf(stderr, "Error: incorrect file type\n"); return;}
    switch (node->type)
    {
        case NUM:
            snprintf(buffer, LEN_OF_DATA_STR, BIN_TREE_ELEM_SPEC[NUM], node->data.num);
            break;
        case VAR:
            snprintf(buffer, LEN_OF_DATA_STR, BIN_TREE_ELEM_SPEC[VAR], node->data.var);
            break;
        case OPERAND:
            if (file_type == LISP_FILE || file_type == DOT_FILE)
                snprintf(buffer, LEN_OF_DATA_STR, BIN_TREE_ELEM_SPEC[OPERAND],
                            op_table[node->data.operand].lisp_name);
            else if (file_type == TEX_FILE)
                snprintf(buffer, LEN_OF_DATA_STR, BIN_TREE_ELEM_SPEC[OPERAND],
                            op_table[node->data.operand].tex_name);
            break;
        case POISON_TYPE: default:
            snprintf(buffer, LEN_OF_DATA_STR, "NULL");
            break;
    }
}

//!SECTION

//SECTION - Read functions
BinTreeErrors BinTreeGetTreeFromFile(bin_tree_t* tree, const char* lisp_filename)
{
    if (tree == NULL)
    {
        fprintf(stderr, "Error: got NULL tree pointer\n");
        return BIN_TREE_ERR_NULL_POINTER;
    }

    if (lisp_filename == NULL)
    {
        fprintf(stderr, "Error: program filename crashed\n");
        return BIN_TREE_ERR_FILE_NOT_OPENED;
    }

    FILE* lisp_file = fopen(lisp_filename, "r");
    if (lisp_file == NULL)
    {
        fprintf(stderr, "Error: no such file <%s>\n", lisp_filename);
        return BIN_TREE_ERR_FILE_NOT_OPENED;
    }

    struct stat lisp_file_stat = {};
    if (stat(lisp_filename, &lisp_file_stat) != 0)
    {
        fprintf(stderr, "Error: can't get size of file <%s>: %s\n", lisp_filename, strerror(errno));
        fclose(lisp_file);
        return BIN_TREE_ERR_FILE_NOT_OPENED;
    }

    size_t lisp_file_size = (size_t)lisp_file_stat.st_size;
    char* buffer = (char*)calloc(lisp_file_size + 1, sizeof(char));
    if (buffer == NULL)
    {
        fprintf(stderr, "Error: calloc failed for file buffer\n");
        fclose(lisp_file);
        return BIN_TREE_ERR_NODE_CALLOC_ERROR;
    }

    size_t bytes_read = fread(buffer, sizeof(char), lisp_file_size, lisp_file);
    buffer[bytes_read] = '\0';
    fclose(lisp_file);

    const char* curr_pos = buffer;
    size_t curr_num_of_nodes = 0;
    BinTreeErrors err = BinTreeLispGetNodeFromBuffer(&(tree->root), &curr_pos, &curr_num_of_nodes);
    if (err != BIN_TREE_ERR_OK) {BinTreeRemove(tree); free(buffer); return err;}

    free(buffer);
    return BIN_TREE_ERR_OK;

}

static BinTreeErrors BinTreeLispGetNodeFromBuffer(
    bin_tree_node_t** node,
    const char** curr_pos,
    size_t* curr_num_of_nodes)
{
    VERIFY_NULL_POINTER(node);
    VERIFY_NULL_POINTER(curr_num_of_nodes);
    if (curr_pos == NULL) return BIN_TREE_ERR_FILE_NOT_OPENED;

    BinTreeSkipSpaces(curr_pos);
    if (**curr_pos != '(') return BIN_TREE_SYNTAX_ERR;

    (*curr_pos)++;

    BinTreeSkipSpaces(curr_pos);
    if (**curr_pos == ')') {(*curr_pos)++; *node = NULL; return BIN_TREE_ERR_OK;}

    const char* data_start_pos = *curr_pos;
    while (**curr_pos != '\0' && **curr_pos != '(' && **curr_pos != ')') {(*curr_pos)++;}

    if (**curr_pos == '\0' || *curr_pos == data_start_pos) return BIN_TREE_SYNTAX_ERR;

    const char* data_end_pos = *curr_pos;
    while (isspace((unsigned char)*(data_end_pos - 1))) {data_end_pos--;}

    if (data_end_pos == data_start_pos) return BIN_TREE_SYNTAX_ERR;


    bin_tree_elem_t elem = {};
    DifferType elem_type = POISON_TYPE;

    VERIFY_ERROR(BinTreeParseNode(data_start_pos, data_end_pos, &elem, &elem_type));
    VERIFY_ERROR(BinTreeAllocateNode(node, elem, elem_type, NULL));
    (*curr_num_of_nodes)++;

    VERIFY_ERROR(BinTreeLispGetNodeFromBuffer(&((*node)->left),  curr_pos, curr_num_of_nodes));
    VERIFY_ERROR(BinTreeLispGetNodeFromBuffer(&((*node)->right), curr_pos, curr_num_of_nodes));
    if ((*node)->left  != NULL) (*node)->left->parent  = *node;
    if ((*node)->right != NULL) (*node)->right->parent = *node;

    BinTreeSkipSpaces(curr_pos);
    if (**curr_pos != ')') return BIN_TREE_SYNTAX_ERR;
    (*curr_pos)++;
    return BIN_TREE_ERR_OK;
}

static void BinTreeSkipSpaces(const char** curr_pos)
{
    while (**curr_pos != '\0' && isspace((unsigned char)**curr_pos)) {(*curr_pos)++;}
}

static const op_info* BinTreeGetOp(const char* data_start, size_t data_len)
{
    if (data_start == NULL) return NULL;

    for (size_t i = 0; i < OP_TABLE_SIZE; i++)
    {
        if (strlen(op_table[i].lisp_name) == data_len &&
            strncmp(data_start, op_table[i].lisp_name, data_len) == 0)
            return &op_table[i];
    }

    return NULL;
}

static BinTreeErrors BinTreeParseNode(const char* data_start, const char* data_end,
                                       bin_tree_elem_t* elem, DifferType* elem_type)
{
    if (data_start == NULL) return BIN_TREE_ERR_NULL_POINTER;
    if (data_end == NULL) return BIN_TREE_ERR_NULL_POINTER;
    if (elem == NULL) return BIN_TREE_ERR_NULL_POINTER;
    if (elem_type == NULL) return BIN_TREE_ERR_NULL_POINTER;

    size_t data_len = (size_t)(data_end - data_start);
    if (data_len == 0) return BIN_TREE_SYNTAX_ERR;

    char* end_of_read_num = NULL;
    double num = strtod(data_start, &end_of_read_num);

    if (end_of_read_num == data_end)
    {
        *elem_type = NUM;
        elem->num = num;
        return BIN_TREE_ERR_OK;
    }

    const op_info* op = BinTreeGetOp(data_start, data_len);
    if (op != NULL)
    {
        *elem_type = OPERAND;
        elem->operand = op->op_code;
        return BIN_TREE_ERR_OK;
    }

    if (data_len == 1 && isalpha((unsigned char)*data_start))
    {
        *elem_type = VAR;
        elem->var = *data_start;
        return BIN_TREE_ERR_OK;
    }

    return BIN_TREE_SYNTAX_ERR;

}
//!SECTION

