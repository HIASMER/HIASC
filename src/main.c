#include "lexer.h"
#include "parser.h"
#include "codegen.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: hiasc <input.hiasm>\n");
        return 1;
    }

    // Read input file
    FILE* input = fopen(argv[1], "r");
    fseek(input, 0, SEEK_END);
    long size = ftell(input);
    rewind(input);
    char* source = malloc(size + 1);
    fread(source, 1, size, input);
    fclose(input);

    // Lex, parse, generate code
    Token* tokens = tokenize(source);
    ASTNode* ast = parse(tokens);
    FILE* output = fopen("output.asm", "w");
    codegen(ast, output);
    fclose(output);

    free(source);
    return 0;
}