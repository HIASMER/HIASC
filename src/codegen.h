// codegen.h
void codegen(ASTNode* root, FILE* output);

typedef enum {
    DT_INT,     // Integer type (32-bit)
    DT_BYTE,    // Byte type (8-bit)
    DT_PTR      // Pointer type (32-bit address)
} DataType;

typedef struct Symbol {
    char* name;
    enum { SYM_REG, SYM_MEM, SYM_PTR } storage_type;
    DataType data_type;       // Data type (int, byte, ptr)
    int reg;                  // Register ID (eax=0, ebx=1, etc.)
    int address;              // Memory address or offset
    struct Symbol* next;
} Symbol;