#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

// Current token index
int current_token = 0;
Token* tokens = NULL;

// Helper function to advance to the next token
void advance() {
    current_token++;
}

// Helper function to check the current token type
int match(TokenType type) {
    return tokens[current_token].type == type;
}

// Helper function to create an AST node
ASTNode* create_node(NodeType type, char* value) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = type;
    node->value = value;
    node->children = NULL;
    node->num_children = 0;
    return node;
}

// Parse a function
ASTNode* parse_function() {
    if (!match(TOKEN_FUNC)) return NULL;

    advance(); // Consume 'func'
    char* name = tokens[current_token].value;
    advance(); // Consume function name

    if (!match(TOKEN_LPAREN)) {
        fprintf(stderr, "Expected '(' after function name\n");
        exit(1);
    }
    advance(); // Consume '('

    // Parse parameters (if any)
    ASTNode* params = create_node(NODE_FUNC, "params");
    while (!match(TOKEN_RPAREN)) {
        // Add parameter parsing logic here
        advance();
    }
    advance(); // Consume ')'

    if (!match(TOKEN_LBRACE)) {
        fprintf(stderr, "Expected '{' after function parameters\n");
        exit(1);
    }
    advance(); // Consume '{'

    // Parse function body
    ASTNode* body = create_node(NODE_FUNC, "body");
    while (!match(TOKEN_RBRACE)) {
        // Add statement parsing logic here
        advance();
    }
    advance(); // Consume '}'

    // Create function node
    ASTNode* func_node = create_node(NODE_FUNC, name);
    func_node->children = malloc(2 * sizeof(ASTNode*));
    func_node->children[0] = params;
    func_node->children[1] = body;
    func_node->num_children = 2;

    return func_node;
}

// Parse the entire program
ASTNode* parse(Token* input_tokens) {
    tokens = input_tokens;
    ASTNode* program = create_node(NODE_FUNC, "program");

    while (!match(TOKEN_EOF)) {
        ASTNode* func = parse_function();
        if (func) {
            program->children = realloc(program->children, (program->num_children + 1) * sizeof(ASTNode*));
            program->children[program->num_children++] = func;
        } else {
            fprintf(stderr, "Unexpected token: %s\n", tokens[current_token].value);
            exit(1);
        }
    }

    return program;
}