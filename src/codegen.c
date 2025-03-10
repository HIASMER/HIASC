#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "parser.h"
#include "lexer.h"

// Symbol table for variables and registers
typedef struct Symbol {
    char* name;
    enum { SYM_REG, SYM_MEM, SYM_PTR } type;
    int reg;        // Register ID (eax=0, ebx=1, etc.)
    int address;    // Memory address or offset
    struct Symbol* next;
} Symbol;

static Symbol* symbol_table = NULL;
static int stack_offset = 0;    // For local variables
static int label_counter = 0;    // For control flow labels
static char* current_func = "";  // Track current function

// Available registers: eax, ebx, ecx, edx
static const char* reg_names[] = {"eax", "ebx", "ecx", "edx"};
static int reg_used[] = {0, 0, 0, 0};

// --- Helper Functions ---

// Allocate a register for 'reg' variables
int allocate_register() {
    for (int i = 0; i < 4; i++) {
        if (!reg_used[i]) {
            reg_used[i] = 1;
            return i;
        }
    }
    fprintf(stderr, "Error: No registers available\n");
    exit(1);
}

// Find a symbol in the symbol table
Symbol* find_symbol(char* name) {
    Symbol* sym = symbol_table;
    while (sym) {
        if (strcmp(sym->name, name) == 0) return sym;
        sym = sym->next;
    }
    return NULL;
}

// Add a symbol to the table
void add_symbol(char* name, int type, int reg, int address) {
    Symbol* sym = malloc(sizeof(Symbol));
    sym->name = strdup(name);
    sym->type = type;
    sym->reg = reg;
    sym->address = address;
    sym->next = symbol_table;
    symbol_table = sym;
}

// --- Code Generation ---

void codegen(ASTNode* node, FILE* output) {
    if (!node) return;

    switch (node->type) {
        // --- Functions ---
        case NODE_FUNC: {
            current_func = node->value;
            fprintf(output, "%s:\n", node->value);
            fprintf(output, "  push ebp\n");
            fprintf(output, "  mov ebp, esp\n");
            
            // Allocate space for local variables
            if (stack_offset > 0) {
                fprintf(output, "  sub esp, %d\n", stack_offset);
            }

            // Generate code for function body
            for (int i = 0; i < node->num_children; i++) {
                codegen(node->children[i], output);
            }

            // Function epilogue
            fprintf(output, "  mov esp, ebp\n");
            fprintf(output, "  pop ebp\n");
            fprintf(output, "  ret\n\n");
            stack_offset = 0;  // Reset for next function
            break;
        }

        // --- Variables and Registers ---
        case NODE_REG: {
            int reg = allocate_register();
            add_symbol(node->value, SYM_REG, reg, -1);
            if (node->num_children > 0) {  // Initial value
                fprintf(output, "  mov %s, %s\n", reg_names[reg], node->children[0]->value);
            }
            break;
        }

        case NODE_VAR: {
            stack_offset += 4;  // Assume 4 bytes for int/ptr
            add_symbol(node->value, SYM_MEM, -1, stack_offset);
            if (node->num_children > 0) {  // Initial value
                fprintf(output, "  mov [ebp - %d], %s\n", stack_offset, node->children[0]->value);
            }
            break;
        }

        // --- Inline Assembly ---
        case NODE_ASM: {
            fprintf(output, "%s\n", node->value);
            break;
        }

        // --- Control Flow ---
        case NODE_IF: {
            int label_else = label_counter++;
            int label_end = label_counter++;

            // Generate condition
            codegen(node->children[0], output);
            fprintf(output, "  cmp eax, 0\n");
            fprintf(output, "  je .L%d\n", label_else);

            // Then block
            codegen(node->children[1], output);
            fprintf(output, "  jmp .L%d\n", label_end);
            fprintf(output, ".L%d:\n", label_else);

            // Else block (if present)
            if (node->num_children > 2) {
                codegen(node->children[2], output);
            }

            fprintf(output, ".L%d:\n", label_end);
            break;
        }

        case NODE_WHILE: {
            int label_start = label_counter++;
            int label_end = label_counter++;

            fprintf(output, ".L%d:\n", label_start);
            codegen(node->children[0], output);  // Condition
            fprintf(output, "  cmp eax, 0\n");
            fprintf(output, "  je .L%d\n", label_end);
            codegen(node->children[1], output);  // Body
            fprintf(output, "  jmp .L%d\n", label_start);
            fprintf(output, ".L%d:\n", label_end);
            break;
        }

        // --- Pointers and Memory ---
        case NODE_PTR: {
            if (strcmp(node->value, "alloc") == 0) {
                // Simple alloc: increment heap pointer (using brk)
                fprintf(output, "  mov eax, 45\n");  // SYS_BRK
                fprintf(output, "  xor ebx, ebx\n");
                fprintf(output, "  int 0x80\n");
                fprintf(output, "  add eax, %s\n", node->children[0]->value);
                fprintf(output, "  mov ebx, eax\n");
                fprintf(output, "  mov eax, 45\n");
                fprintf(output, "  int 0x80\n");
            } else if (strcmp(node->value, "free") == 0) {
                // No-op for this simple implementation
            } else {
                // Pointer assignment
                Symbol* sym = find_symbol(node->value);
                if (sym && sym->type == SYM_PTR) {
                    fprintf(output, "  mov eax, [ebp - %d]\n", sym->address);
                    fprintf(output, "  mov [eax], %s\n", node->children[0]->value);
                }
            }
            break;
        }

        // --- Binary Operations ---
        case NODE_BINOP: {
            codegen(node->children[0], output);  // Left operand
            fprintf(output, "  push eax\n");
            codegen(node->children[1], output);  // Right operand
            fprintf(output, "  pop ebx\n");

            switch (node->value[0]) {
                case '+': fprintf(output, "  add eax, ebx\n"); break;
                case '-': fprintf(output, "  sub eax, ebx\n"); break;
                case '*': fprintf(output, "  imul eax, ebx\n"); break;
                case '/': fprintf(output, "  idiv ebx\n"); break;
            }
            break;
        }

        // --- Default: Error ---
        default:
            fprintf(stderr, "Error: Unknown node type %d\n", node->type);
            exit(1);
    }
}