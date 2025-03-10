#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

// Helper function to check if a character is a valid identifier start
int is_identifier_start(char c) {
    return isalpha(c) || c == '_';
}

// Helper function to check if a character is a valid identifier part
int is_identifier_part(char c) {
    return isalnum(c) || c == '_';
}

// Tokenize the input source code
Token* tokenize(const char* input) {
    Token* tokens = malloc(1024 * sizeof(Token)); // Allocate space for tokens
    int token_count = 0;
    int line = 1;
    const char* src = input;

    while (*src) {
        if (*src == '\n') line++;
        // Skip whitespace
        if (isspace(*src)) {
            src++;
            continue;
        }

        // Handle identifiers and keywords
        if (is_identifier_start(*src)) {
            const char* start = src;
            while (is_identifier_part(*src)) src++;
            int length = src - start;
            char* value = malloc(length + 1);
            strncpy(value, start, length);
            value[length] = '\0';

            // Check for keywords
            if (strcmp(value, "reg") == 0) {
                tokens[token_count++] = (Token){TOKEN_REG, value};
            } else if (strcmp(value, "func") == 0) {
                tokens[token_count++] = (Token){TOKEN_FUNC, value};
            } else if (strcmp(value, "if") == 0) {
                tokens[token_count++] = (Token){TOKEN_IF, value};
            } else if (strcmp(value, "else") == 0) {
                tokens[token_count++] = (Token){TOKEN_ELSE, value};
            } else if (strcmp(value, "while") == 0) {
                tokens[token_count++] = (Token){TOKEN_WHILE, value};
            } else if (strcmp(value, "for") == 0) {
                tokens[token_count++] = (Token){TOKEN_FOR, value};
            } else if (strcmp(value, "return") == 0) {
                tokens[token_count++] = (Token){TOKEN_RETURN, value};
            } else if (strcmp(value, "asm") == 0) {
                tokens[token_count++] = (Token){TOKEN_ASM, value};
            } else {
                tokens[token_count++] = (Token){TOKEN_IDENT, value};
            }
            continue;
        }

        // Handle numbers
        if (isdigit(*src)) {
            const char* start = src;
            while (isdigit(*src)) src++;
            int length = src - start;
            char* value = malloc(length + 1);
            strncpy(value, start, length);
            value[length] = '\0';
            tokens[token_count++] = (Token){TOKEN_NUMBER, value};
            continue;
        }

        // Handle symbols
        switch (*src) {
            case '{': tokens[token_count++] = (Token){TOKEN_LBRACE, strdup("{"}; break;
            case '}': tokens[token_count++] = (Token){TOKEN_RBRACE, strdup("}"); break;
            case '(': tokens[token_count++] = (Token){TOKEN_LPAREN, strdup("("); break;
            case ')': tokens[token_count++] = (Token){TOKEN_RPAREN, strdup(")"); break;
            case ';': tokens[token_count++] = (Token){TOKEN_SEMICOLON, strdup(";"); break;
            case ',': tokens[token_count++] = (Token){TOKEN_COMMA, strdup(","); break;
            case '=': tokens[token_count++] = (Token){TOKEN_EQ, strdup("="); break;
            case '+': tokens[token_count++] = (Token){TOKEN_PLUS, strdup("+"); break;
            case '-': tokens[token_count++] = (Token){TOKEN_MINUS, strdup("-"); break;
            case '*': tokens[token_count++] = (Token){TOKEN_STAR, strdup("*"); break;
            case '/': tokens[token_count++] = (Token){TOKEN_SLASH, strdup("/"); break;
            case '&': tokens[token_count++] = (Token){TOKEN_AND, strdup("&"); break;
            case '@': tokens[token_count++] = (Token){TOKEN_AT, strdup("@"); break;
            default:
                fprintf(stderr, "Unknown character: %c\n", *src);
                exit(1);
        }
        src++;
    }

    // Add EOF token
    tokens[token_count++] = (Token){TOKEN_EOF, NULL};
    return tokens;
}