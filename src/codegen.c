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

static int has_error = 0;

void error(const char* msg, const char* context) {
    fprintf(stderr, "Error: %s (%s)\n", msg, context);
    has_error = 1;
}

// --- Helper Functions ---

// Check if a symbol exists in the current scope
Symbol* check_symbol_exists(char* name) {
    Symbol* sym = find_symbol(name);
    if (!sym) {
        error("Undeclared variable", name);
        return NULL;
    }
    return sym;
}

// Check type compatibility
void check_type(DataType expected, DataType actual, const char* context) {
    if (expected != actual) {
        fprintf(stderr, "Type error: Expected %d, got %d (%s)\n",
                expected, actual, context);
        has_error = 1;
    }
}

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

// Determine the type of an expression node
DataType get_expression_type(ASTNode* node) {
    switch (node->type) {
        case NODE_IDENT: {
            Symbol* sym = find_symbol(node->value);
            return sym ? sym->data_type : DT_INT; // Default to int on error
        }
        case NODE_NUMBER: return DT_INT;
        case NODE_BINOP: {
            DataType left = get_expression_type(node->children[0]);
            DataType right = get_expression_type(node->children[1]);
            return (left == DT_PTR || right == DT_PTR) ? DT_PTR : left;
        }
        default: return DT_INT;
    }
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
        // Inside codegen() for NODE_ASSIGN:
        case NODE_ASSIGN: {
            char* var_name = node->children[0]->value;
            ASTNode* expr = node->children[1];

            Symbol* sym = check_symbol_exists(var_name);
            codegen(expr, output);
            DataType expr_type = get_expression_type(expr);

            if (sym) {
                check_type(sym->data_type, expr_type, "assignment");
                if (sym->storage_type == SYM_REG) {
                    fprintf(output, "  mov %s, eax\n", reg_names[sym->reg]);
                } else {
                    fprintf(output, "  mov [ebp - %d], eax\n", sym->address);
                }
            }
            break;
        }

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

        // Inside codegen() for NODE_CALL:
        case NODE_CALL: {
            char* func_name = node->value;
            Symbol* func_sym = check_symbol_exists(func_name);

            // Check if symbol is a function
            if (func_sym && func_sym->storage_type != SYM_FUNC) {
                error("Not a function", func_name);
            }

            // Check argument count and types (simplified)
            int expected_args = func_sym->num_children;
            if (node->num_children != expected_args) {
                error("Argument count mismatch", func_name);
            }

            for (int i = 0; i < node->num_children; i++) {
                codegen(node->children[i], output);
                check_type(func_sym->children[i]->data_type, 
                        get_expression_type(node->children[i]), 
                        func_name);
                fprintf(output, "  push eax\n");
            }

            fprintf(output, "  call %s\n", func_name);
            fprintf(output, "  add esp, %d\n", node->num_children * 4);
            break;
        }

        // Inside codegen() for NODE_REG and NODE_VAR:
        case NODE_REG: {
            char* type_str = node->children[0]->value; // "int" or "byte"
            char* var_name = node->children[1]->value;
            DataType data_type = DT_INT;

            if (strcmp(type_str, "byte") == 0) data_type = DT_BYTE;
            else if (strcmp(type_str, "ptr") == 0) data_type = DT_PTR;

            if (find_symbol(var_name)) {
                error("Redeclared variable", var_name);
                break;
            }

            int reg = allocate_register();
            add_symbol(var_name, SYM_REG, data_type, reg, -1);

            // Handle initialization (if any)
            if (node->num_children > 2) {
                ASTNode* init_expr = node->children[2];
                codegen(init_expr, output);
                check_type(data_type, get_expression_type(init_expr), var_name);
                fprintf(output, "  mov %s, eax\n", reg_names[reg]);
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

        // Inside codegen() for NODE_PTR:
        /*
        case NODE_PTR: {
            Symbol* ptr_sym = check_symbol_exists(node->value);
            if (ptr_sym && ptr_sym->data_type != DT_PTR) {
                error("Expected pointer type", node->value);
            }

            // Handle dereference (*ptr = ...)
            if (node->num_children > 0) {
                codegen(node->children[0], output); // Value to assign
                fprintf(output, "  mov [%s], eax\n", reg_names[ptr_sym->reg]);
            }
            break;
        }
        */

        // --- Binary Operations ---
        // Inside codegen() for NODE_BINOP:
        case NODE_BINOP: {
            DataType left_type = get_expression_type(node->children[0]);
            DataType right_type = get_expression_type(node->children[1]);

            // Allow int + byte (promote byte to int)
            if (left_type == DT_BYTE && right_type == DT_INT) {
                left_type = DT_INT;
            } else if (right_type == DT_BYTE && left_type == DT_INT) {
                right_type = DT_INT;
            }

            check_type(left_type, right_type, "binary operation");
            codegen(node->children[0], output);
            fprintf(output, "  push eax\n");
            codegen(node->children[1], output);
            fprintf(output, "  pop ebx\n");

            // Handle operation based on type
            if (left_type == DT_PTR || right_type == DT_PTR) {
                error("Invalid operation for pointer type", node->value);
            }

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