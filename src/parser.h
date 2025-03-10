// parser.h
typedef enum {
    NODE_FUNC, NODE_VAR, NODE_REG, NODE_ASM, NODE_IF, NODE_WHILE,
    NODE_FOR, NODE_ASSIGN, NODE_BINOP, NODE_CALL, NODE_RETURN
} NodeType;

typedef struct ASTNode {
    NodeType type;
    struct ASTNode** children;
    int num_children;
    char* value;
} ASTNode;

ASTNode* parse(Token* tokens);