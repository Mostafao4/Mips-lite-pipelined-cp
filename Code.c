#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//DEFINE Array of main memory 2d array of size 2048*32
int main_memory[2048][32];
// Define arrays as global variables
int R0[32], R1[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}, R2[32], R3[32], R4[32], R5[32], R6[32], R7[32];
int R8[32], R9[32], R10[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1}, R11[32], R12[32], R13[32], R14[32], R15[32];
int R16[32], R17[32], R18[32], R19[32], R20[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0}, R21[32], R22[32], R23[32];
int R24[32], R25[32], R26[32], R27[32], R28[32], R29[32], R30[32], R31[32];
int PC[32], IR[32]; int countClock=1; int halt=0; int count=0;int isJumping=0; int tempPC[32];int jumpNo=0;


typedef struct {
    int active;  // Stage activity flag
    char instructionB[32];    //FETCHING      //instruction binary string comes from fetch
    int opcode;          //DECODE          // Current operation
    int* rd;             //DECODE          // Destination register pointer (e.g. &R1)
    int rs1_val;         //DECODE          // Source register 1 value
    int rs2_val;         //DECODE          // Source register 2 value
    int imm;             //DECODE          // Immediate value
    int result[32];         // Execution result
    int address;        // Memory address
    int cycles_left;    // For multi-cycle stages
    int instruction_num ;  //FETCHING    // For tracking
} PipelineStage;

PipelineStage IF, ID, EX, MEM, WB; // Global pipeline registers 

void Flush() {
    // Reset IF stage
    IF.active = 0;
    IF.cycles_left = 0;
    IF.instruction_num = 0;
    IF.opcode = 0;
    memset(IF.instructionB, '0', 32);
    IF.rd = NULL;
    IF.rs1_val = 0;
    IF.rs2_val = 0;
    IF.imm = 0;
    memset(IF.result, 0, sizeof(IF.result));

    // Reset ID stage
    ID.active = 0;
    ID.cycles_left = 0;
    ID.instruction_num = 0;
    ID.opcode = 0;
    memset(ID.instructionB, '0', 32);
    ID.rd = NULL;
    ID.rs1_val = 0;
    ID.rs2_val = 0;
    ID.imm = 0;
    memset(ID.result, 0, sizeof(ID.result));

    // Reset EX stage
    EX.active = 0;
    EX.cycles_left = 0;
    EX.instruction_num = 0;
    EX.opcode = 0;
    memset(EX.instructionB, '0', 32);
    EX.rd = NULL;
    EX.rs1_val = 0;
    EX.rs2_val = 0;
    EX.imm = 0;
    memset(EX.result, 0, sizeof(EX.result));
}

typedef enum {
    R_FORMAT,
    I_FORMAT,
    J_FORMAT
} InstructionFormat;

typedef struct {
    char name[10];
    int opcode;
    InstructionFormat format;
} Instruction;

// Add instruction lookup table
const Instruction INSTRUCTIONS[] = {
    {"ADD",  0, R_FORMAT},
    {"SUB",  1, R_FORMAT},
    {"MUL",  2, R_FORMAT},
    {"MOVI", 3, I_FORMAT},
    {"JEQ",  4, I_FORMAT},
    {"AND",  5, R_FORMAT},
    {"XORI", 6, I_FORMAT},
    {"JMP",  7, J_FORMAT},
    {"LSL",  8, R_FORMAT},
    {"LSR",  9, R_FORMAT},
    {"MOVR", 10, I_FORMAT},
    {"MOVM", 11, I_FORMAT}
};

// Convert decimal to binary string
void decimalToBinary(int decimal, char* binary, int bits) {
    for (int i = bits - 1; i >= 0; i--) {
        binary[i] = (decimal & 1) + '0';
        decimal >>= 1;
    }
    binary[bits] = '\0';
}

int binaryToDecimal(int *binary) {
    int decimal = 0;
    for (int i = 0; i < 32; i++) {
        decimal = (decimal << 1) + binary[i];
    }
    return decimal;
}

// Parse register number from string (e.g., "R1" -> 1)
int parseRegister(const char* reg) {
    return atoi(reg + 1); // Skip 'R' and convert to integer
}

// Convert assembly instruction to binary
void assemblyToBinary(char* line, char* binary) {
    char instruction[10];
    char operands[3][10];
    char temp[33];
    
    // Initialize operands to handle cases with fewer arguments
    memset(operands, 0, sizeof(operands));
    
    // Parse instruction and operands
    sscanf(line, "%s %s %s %s", instruction, operands[0], operands[1], operands[2]);
    
    // Find instruction in lookup table
    Instruction instr;
    for (int i = 0; i < sizeof(INSTRUCTIONS)/sizeof(Instruction); i++) {
        if (strcmp(instruction, INSTRUCTIONS[i].name) == 0) {
            instr = INSTRUCTIONS[i];
            break;
        }
    }
    
    // Convert opcode to binary (4 bits)
    decimalToBinary(instr.opcode, temp, 4);
    strcpy(binary, temp);
    
    switch(instr.format) {
        case R_FORMAT:
            // OPCODE(4) R1(5) R2(5) R3(5) SHAMT(13)
            decimalToBinary(parseRegister(operands[0]), temp, 5);
            strcat(binary, temp);
            decimalToBinary(parseRegister(operands[1]), temp, 5);
            strcat(binary, temp);
            
            if (strcmp(instruction, "LSL") == 0 || strcmp(instruction, "LSR") == 0) {
                // For LSL and LSR, R3 is 0 and operands[2] contains SHAMT
                strcat(binary, "00000"); // R3 = 0
                decimalToBinary(atoi(operands[2]), temp, 13); // SHAMT
                strcat(binary, temp);
            } else {
                // Normal R-format instruction
                decimalToBinary(parseRegister(operands[2]), temp, 5);
                strcat(binary, temp);
                strcat(binary, "0000000000000"); // SHAMT
            }
            break;
            
        case I_FORMAT:
            // OPCODE(4) R1(5) R2(5) IMM(18)
            decimalToBinary(parseRegister(operands[0]), temp, 5);
            strcat(binary, temp);
            
            if (strcmp(instruction, "MOVI") == 0) {
                strcat(binary, "00000"); // R2 = 0 for MOVI
            } else {
                decimalToBinary(parseRegister(operands[1]), temp, 5);
                strcat(binary, temp);
            }
            
            decimalToBinary(atoi(operands[strcmp(instruction, "MOVI") == 0 ? 1 : 2]), temp, 18);
            strcat(binary, temp);
            break;
            
        case J_FORMAT:
            // OPCODE(4) ADDRESS(28)
            decimalToBinary(atoi(operands[0]), temp, 28);
            strcat(binary, temp);
            break;
    }
}

// Modified processFile function
void processFile(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening file!\n");
        return;
    }

    char line[256];
    char binary[33];
    int instruction_count = 0;
    while (fgets(line, sizeof(line), file)) {
        // Remove newline character if present
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        printf("Assembly: %s\n", line);
        assemblyToBinary(line, binary);
        for( int i = 0; i < 32; i++) {
            main_memory[instruction_count][i] = binary[i] - '0'; // Convert char '0'/'1' to int 0/1
        }
        instruction_count++;  
        printf("Binary:   %s\n\n", binary);
    }

    fclose(file);
}

// Function prototype

void int_to_binary_array(int v, int reg[32]) {
    for (int i = 31; i >= 0; i--) {
        reg[i] = v & 1;
        v >>= 1;
    }
}

int binary_array_to_int_len(int bits[], int len) {
    int v = 0;
    for (int i = 0; i < len; i++) {
        v = (v << 1) | (bits[i] & 1);
    }
    return v;
}


void WriteBack() {
    int opcode = WB.opcode;
    int* R_source1 = WB.rd;
    int R_source2 = WB.rs1_val;
    int value = WB.imm;
    // Check if writing to R0 (read-only register)
    if (R_source1 == R0) {
        printf("Cannot write to R0 (read-only)\n");
        return;
    }
    switch (opcode) {
        case 0: // ADD
            for (int i = 0; i < 32; i++) {
            R_source1[i] = WB.result[i];
            }
            printf("Register R_source1: ");
            for (int i = 0; i < 32; i++) {
                printf("%d", R_source1[i]);
            }
            printf("\n");
            break;
        case 1: // SUB
            for (int i = 0; i < 32; i++) {
            R_source1[i] = WB.result[i];
            }
            printf("Register R_source1: ");
            for (int i = 0; i < 32; i++) {
                printf("%d", R_source1[i]);
            }
            printf("\n");
            break;
        case 2: // MUL
            for (int i = 0; i < 32; i++) {
            R_source1[i] = WB.result[i];
            }
            printf("Register R_source1: ");
            for (int i = 0; i < 32; i++) {
                printf("%d", R_source1[i]);
            }
            printf("\n");
            break;
        case 3:{ // MOVI
        // Convert decimal value to binary array
            int valueB[32];
            for (int i = 31; i >= 0; i--) {
                valueB[i] = (value >> (31 - i)) & 1;
            }
            for (int i = 0; i < 32; i++) {
            R_source1[i] = valueB[i];
            }
            printf("Register R_source1: ");
            for (int i = 0; i < 32; i++) {
                printf("%d", R_source1[i]);
            }
            printf("\n");
        }
            break;
        case 4: // JEQ
            // for(int i = 0; i < 32; i++) {
            //     PC[i] = WB.result[i];}
            //     printf("PC Register: ");
            //     for (int i = 0; i < 32; i++) {
            //         printf("%d", PC[i]);
            //     }
            //     printf("\n");
            break;
        case 5: // AND
            for (int i = 0; i < 32; i++) {
            R_source1[i] = WB.result[i];
            }
            printf("Register R_source1: ");
            for (int i = 0; i < 32; i++) {
                printf("%d", R_source1[i]);
            }
            printf("\n");
            break;
        case 6: // XORI
            for (int i = 0; i < 32; i++) {
            R_source1[i] = WB.result[i];
            }
            printf("Register R_source1: ");
            for (int i = 0; i < 32; i++) {
                printf("%d", R_source1[i]);
            }
            printf("\n");
            break;
        case 7: // JMP
            // for (int i = 0; i < 32; i++) {
            // PC[i] = WB.result[i];
            // }
            // for (int i = 0; i < 32; i++) {
            //         printf("%d", PC[i]);
            //     }
            //     printf("\n");
            break;
        case 8: // LSL
            for (int i = 0; i < 32; i++) {
            R_source1[i] = WB.result[i];
            }
            printf("Register R_source1: ");
            for (int i = 0; i < 32; i++) {
                printf("%d", R_source1[i]);
            }
            printf("\n");
            break;
        case 9: // LSR
            
            for (int i = 0; i < 32; i++) {
            R_source1[i] = WB.result[i];
            }
            printf("Register R_source1: ");
            for (int i = 0; i < 32; i++) {
                printf("%d", R_source1[i]);
            }
            printf("\n");
            break;
        case 10: // MOVR
            for(int i = 0; i < 32; i++) {
                R_source1[i] = WB.result[i];
            }
            printf("Register R_source1: ");
            for (int i = 0; i < 32; i++) {
                printf("%d", R_source1[i]);
            }
            printf("\n");
            break;
        case 11: // MOVM
            // MOVM is handled in Memory stage
            break;
        default:
            printf("Invalid opcode!\n");
    }
    if(opcode != 4 && opcode != 7) { // JEQ and JMP do not write back to registers
    printf("[WRITEBACK] Writing to register: ");
    for(int i = 0; i < 32; i++) {
        printf("%d", R_source1[i]);
        
    }
    printf(" (R%d: %d)\n", (int)(R_source1 - R0)/32, binary_array_to_int_len(R_source1, 32));}
    printf("[WRITEBACK] Writing back instruction %d\n", WB.instruction_num);

}


void Memory(){
    // Memory operation based on opcode
    int opcode = MEM.opcode;
    int* R_source1 = MEM.rd;
    int R_source2 = MEM.rs1_val;
    int value = MEM.imm;
    int row = 0;
    // Check if R_source1 points to R0s
    if (R_source1 == R0) {
        printf("Cannot access R0 in memory operations\n");
        return;
    }
            for (int i = 0; i < 32; i++) {
                row = (row << 1) | MEM.result[i];
            }
    switch (opcode) {
        case 0: // ADD
            break;
            
        case 1: // SUB
            break;
            
        case 2: // MUL
            break;
            
        case 3: // MOVI
            break;
            
        case 4: // JEQ
            break;
            
        case 5: // AND
            break;
            
        case 6: // XORI
            break;
            
        case 7: // JMP
            break;
            
        case 8: // LSL
            break;
            
        case 9: // LSR
            break;
            
        case 10: // MOVR
            for (int i = 0; i < 32; i++) {
                MEM.result[i] = main_memory[row+1024][i];
            }
           printf("[MEMORY] Loading from address %d: ", row+1023);
        for(int i = 0; i < 32; i++) {
            printf("%d", MEM.result[i]);
        }
        printf("\n");
            break;
            
        case 11: // MOVM
            for (int i = 0; i < 32; i++) {
                main_memory[row+1024][i] = R_source1[i];
            } 
    printf("[MEMORY] Storing to address %d: ", row+1023);
        for(int i = 0; i < 32; i++) {
            printf("%d", R_source1[i]);
        }
        printf("\n");            break;
        
        default:
            printf("Invalid opcode!\n");
    }
    
}

// Unified execute stage: computes ALU or control result and writes 32-bit into IR
void execute() {
    int r2_val= EX.rs1_val;
    int r3_val= EX.rs2_val;
    int value= EX.imm;
    int opcode= EX.opcode;
    int* r1_ptr = EX.rd;  // Keep as pointer
    int r1_val = binary_array_to_int_len(r1_ptr, 32);  // Convert to int when needed
    int result = 0;
    switch (opcode) {
        case 0:  // ADD
            result = r2_val + r3_val;
            printf("ADD: %d + %d = %d\n", r2_val, r3_val, result);
            break;
        case 1:  // SUB
            result = r2_val - r3_val;
            printf("SUB: %d - %d = %d\n", r2_val, r3_val, result);
            break;
        case 2:  // MUL
            result = r2_val * r3_val;
            printf("MUL: %d * %d = %d\n", r2_val, r3_val, result);
            break;
        case 5:  // AND
            result = r2_val & r3_val;
            printf("AND: %d & %d = %d\n", r2_val, r3_val, result);
            break;
        case 8:  // LSL
            result = r2_val << value;
            printf("LSL: %d << %d = %d\n", r2_val, value, result);
            break;
        case 9:  // LSR
            result = (unsigned int)r2_val >> value;
            printf("LSR: %d >> %d = %d\n", r2_val, value, result);
            break;

        // === I-TYPE ===
        case 3:  // MOVI
            result = value;
            printf("MOVI: %d\n", result);
            break;
        case 4:{  // JEQ
            int r1value=r1_val;  // r1_val is already an int
            int pc_valL = binary_array_to_int_len(tempPC, 32);
            if (r1value == r2_val) {
                result = pc_valL + value;   
                jumpNo=EX.instruction_num;
                isJumping = 1;
                printf("JEQ: %d == %d, PC = %d + %d = %d\n", r1value, r2_val, pc_valL, value, result);
            } else {
                int normalPC = binary_array_to_int_len(PC, 32);
                result = normalPC;  // PC remains unchanged
                printf("JEQ: %d != %d, PC remains %d\n", r1value, r2_val, result);  
                    
            }
            int_to_binary_array(result, EX.result);
            for (int i = 0; i < 32; i++) {
                PC[i] = EX.result[i];
                
            }
        }
            break;
        case 6:  // XORI
            result = r2_val ^ value;
            printf("XORI: %d ^ %d = %d\n", r2_val, value, result);
            break;
        case 10: result = r2_val + value; // MOVR
            printf("MOVR: %d + %d = %d\n", r2_val, value, result);
            break;
        case 11: // MOVM
            result = r2_val + value;  // effective address
            printf("MOVM: %d + %d = %d\n", r2_val, value, result);
            break;

        // === J-TYPE ===
      case 7:  // JMP
        {
            // printf("PC Register: ");
            // for (int i = 0; i < 32; i++) {
            // printf("%d", PC[i]);
            // }
            // printf("\nTempPC Register: ");
            // for (int i = 0; i < 32; i++) {
            // printf("%d", tempPC[i]);
            // }
            // printf("\n");
            int top4 = binary_array_to_int_len(tempPC, 4);
            result = (top4 << 28) | value;
            printf("JMP: %d << 28 | %d = %d\n", top4, value, result);
            result--;
            int_to_binary_array(result, EX.result);
            // printf("PC Register: ");
            // for (int i = 0; i < 32; i++) {
            // printf("%d", PC[i]);
            // }
            // printf("\nTempPC Register: ");
            // for (int i = 0; i < 32; i++) {
            // printf("%d", tempPC[i]);
            // }
            printf("\n%d", result); 
            for (int i = 0; i < 32; i++) {
                EX.result[i] = (result >> (31 - i)) & 1;
            }
            for (int i = 0; i < 32; i++) {
                PC[i] = EX.result[i];
                
            }
            isJumping = 1; // Set jumping flag
            jumpNo=EX.instruction_num;
            break;
        }

        default:
            printf("Unknown opcode: %d\n", opcode);
            result = 0;
            int_to_binary_array(result, EX.result);
            break;
    }
    // Store the 32-bit binary result into IR register
    int_to_binary_array(result, EX.result);
    // Print IR register values
    // printf("IR Register: ");
    // for (int i = 0; i < 32; i++) {
    //     printf("%d", IR[i]);
    // }
    // printf("\n");

    
}

void Decode() {
    // Parse the 32-bit instruction string
    int opcode = 0;
    int r1 = 0, r2 = 0, r3 = 0, shamt = 0; 
    int *registers[] = {R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, 
                              R13, R14, R15, R16, R17, R18, R19, R20, R21, R22, R23, 
                              R24, R25, R26, R27, R28, R29, R30, R31};
    int *reg1, *reg2, *reg3;

    // Extract opcode (first 4 bits)
    for(int i = 0; i < 4; i++) {
        opcode = (opcode << 1) + (ID.instructionB[i] - '0');
    }
    
    // Find instruction type
    InstructionFormat format;
    for(int i = 0; i < sizeof(INSTRUCTIONS)/sizeof(Instruction); i++) {
        if(opcode == INSTRUCTIONS[i].opcode) {
            format = INSTRUCTIONS[i].format;
            break;
        }
    }
    
    // Parse based on instruction format
    switch(format) {
        case R_FORMAT: {
            // Parse R1, R2, R3, and SHAMT
            for(int i = 4; i < 9; i++) {
                r1 = (r1 << 1) + (ID.instructionB[i] - '0');
            }
            for(int i = 9; i < 14; i++) {
                r2 = (r2 << 1) + (ID.instructionB[i] - '0');
            }
            for(int i = 14; i < 19; i++) {
                r3 = (r3 << 1) + (ID.instructionB[i] - '0');
            }
            for(int i = 19; i < 32; i++) {
                shamt = (shamt << 1) + (ID.instructionB[i] - '0');
            }
            // Convert binary registers to decimal integers
            // printf("R1: %d, R2: %d, R3: %d, SHAMT: %d\n", r1, r2, r3, shamt);
            // Execute based on register numbers - using pointer arrays for direct manipulation
            
            
            reg1 = registers[r1];
            reg2 = registers[r2];
            reg3 = registers[r3];
            r2 = binaryToDecimal(reg2);
            r3 = binaryToDecimal(reg3);
            // printf("R2 value: %d, R3 value: %d\n", r2, r3);
            ID.opcode = opcode;
            ID.rd = reg1;
            ID.rs1_val = r2;
            ID.rs2_val = r3;
            ID.imm = shamt;
        
            break;
        }
            
        case I_FORMAT:{
            // Parse R1, R2, and immediate value
            for(int i = 4; i < 9; i++) {
                r1 = (r1 << 1) + (ID.instructionB[i] - '0');
            }
            for(int i = 9; i < 14; i++) {
                r2 = (r2 << 1) + (ID.instructionB[i] - '0');
            }
            // Immediate value (18 bits)
            for(int i = 14; i < 32; i++) {
                shamt = (shamt << 1) + (ID.instructionB[i] - '0');
            }
            // Convert binary registers to decimal integers
            // printf("R1: %d, R2: %d, R3: %d, SHAMT: %d\n", r1, r2, r3, shamt);
            // Execute based on register numbers - using pointer arrays for direct manipulation
            

            reg1 = registers[r1];
            reg2 = registers[r2];
            reg3 = registers[r3];
            r2 = binaryToDecimal(reg2);
            r3 = binaryToDecimal(reg3);
            // printf("R2 value: %d, R3 value: %d\n", r2, r3);
            ID.opcode = opcode;
            ID.rd = reg1;
            ID.rs1_val = r2;
            ID.rs2_val = r3;
            ID.imm = shamt;
            break;
        }
        case J_FORMAT:{
            // Parse address
            // For J_FORMAT, extract the 28-bit address as decimal
            shamt = 0;
            for(int i = 4; i < 32; i++) {
                if (ID.instructionB[i] == '1') {
                    shamt = shamt * 2 + 1;
                } else {
                    shamt = shamt * 2;
                }
            }
            // printf("Jump Address: %d\n", shamt);
            ID.opcode = opcode;
            ID.rd = reg1;
            ID.rs1_val = r2;
            ID.rs2_val = r3;
            ID.imm = shamt;
            break;
        }
    }
}


// Fetch instruction from memory based on PC
void fetch() {
    if (halt) return;
    // Step 1: Convert PC bits to binary string
    // Save PC values to tempPC
    for (int i = 0; i < 32; i++) {
        tempPC[i] = PC[i];
    }
    char instruction_str[33];
    char binary_str[33];
    for (int i = 0; i < 32; i++) {
        binary_str[i] = PC[i] + '0';
    }
    binary_str[32] = '\0';

    // Step 2: Convert binary string to decimal row number
    unsigned int row = 0;
    for (int i = 0; i < 32 && binary_str[i] != '\0'; i++) {
        row = (row << 1) | (binary_str[i] - '0');
    }

    // Step 3: Fetch instruction from memory and convert to string
    for (int i = 0; i < 32; i++) {
        instruction_str[i] = main_memory[row][i] + '0';
    }
    instruction_str[32] = '\0';  // Null-terminate
    
    // Step 4: Increment PC
    unsigned int pc_val = row + 1;  // Increment PC value
    // printf("PC Register now: ");
    // for (int i = 0; i < 32; i++) {
    //     printf("%d", PC[i]);
    // }
    // printf("\n");
    for (int i = 31; i >= 0; i--) {
        PC[i] = (pc_val >> (31 - i)) & 1;  // Convert back to binary array
    }
    // Print PC register binary values before Decode
    // printf("PC Register incremented: ");
    // for (int i = 0; i < 32; i++) {
    //     printf("%d", PC[i]);
    // }
    // printf("\n");

    // Copy instruction string to IF stage instruction array
    for (int i = 0; i < 32; i++) {
        IF.instructionB[i] = instruction_str[i];
    }
    IF.instruction_num= pc_val;  // Store instruction number
    
}

void print_final_state() {
    printf("\n=== FINAL STATE ===\n");
    
    // Print all registers
    printf("\nRegister Contents:\n");
    for (int r = 0; r < 32; r++) {
        printf("R%d: ", r);
        int* reg = (int*[]){R0,R1,R2,R3,R4,R5,R6,R7,R8,R9,R10,R11,R12,R13,R14,R15,
                            R16,R17,R18,R19,R20,R21,R22,R23,R24,R25,R26,R27,R28,R29,R30,R31}[r];
        for (int i = 0; i < 32; i++) {
            printf("%d", reg[i]);
        }
        printf(" (%d)\n", binary_array_to_int_len(reg, 32));
    }
    
    // Print memory contents
    printf("\nMemory Contents (first 20 instructions and data section):\n");
    printf("Instructions:\n");
    for (int i = 0; i < 20; i++) {
        printf("%4d: ", i);
        for (int j = 0; j < 32; j++) {
            printf("%d", main_memory[i][j]);
        }
        printf("\n");
    }
    
    printf("\nData Section (1024-1043):\n");
    for (int i = 1024; i < 1044; i++) {
        printf("%4d: ", i);
        for (int j = 0; j < 32; j++) {
            printf("%d", main_memory[i][j]);
        }
        printf("\n");
    }
}

void run_pipeline( ) {
    
    while ( (!halt || count<6) ) // Run until halt or all stages are inactive
    {
        
    
    // Check for halt instruction at the beginning of each cycle
        int pc_val = binary_array_to_int_len(PC, 32);
        int isHalt = 1;
        for (int i = 0; i < 32; i++) {
            if (main_memory[pc_val][i] != 0) {
                isHalt = 0;
                break;
            }
        }
        
       
    printf("=====================================  Clock Cycle: %d  =====================================\n", countClock);

    if(!halt && (countClock % 2 == 1) && isJumping==0)  {

        fetch(IF);
        IF.active = 1;
        IF.cycles_left = 1;

    }
    if(isJumping==0 && count<2 && IF.cycles_left == 0 && ID.cycles_left==0 && IF.instruction_num>=ID.instruction_num && ID.instruction_num!=0) { //makes sure it enters only when if is done and IF becomes equal 0 in the empty cycle (even)
        Decode();
        ID.active = 1;
        ID.cycles_left = 2;
        
         //save number to make sure it is the one to enter in order to EXCECUTE
    }
    
    if(count<4 && (ID.instruction_num>EX.instruction_num || count==2 ) && EX.instruction_num!=0) {
        
        execute();
        EX.active = 1;
        EX.cycles_left = 2;
        printf("Executing instruction %d: ", EX.instruction_num);
        printf("\n");
    }else{
        if (count == 3) {
            printf("Executing instruction %d: ", EX.instruction_num);
        } else if (count < 4) {
            printf("Executing instruction %d: ", EX.instruction_num-1);
        }
        printf("\n");
    }
    if(count<6 && IF.active==0 && WB.active==0 && (countClock % 2 == 0) && (EX.instruction_num>MEM.instruction_num || count==4) && MEM.instruction_num!=0) {
        Memory();
        MEM.active = 1;
        MEM.cycles_left = 1;
        
    }
    if(count<7 && MEM.active==0 && MEM.instruction_num>=WB.instruction_num && WB.instruction_num!=0) {
        WriteBack();  
        WB.active = 1;
        WB.cycles_left = 1;
        
    }

    if (isHalt) {
        halt = 1;
        count++;
        if(count==0)
            printf("HALT instruction detected at PC=%d: ", pc_val);

    printf("\n");
    
    // Print pipeline stages that still need to complete
    printf("Waiting for pipeline to drain:\n");
    if (IF.active) printf(" - IF stage still active\n");
    if (ID.active) printf(" - ID stage still active\n");
    if (EX.active) printf(" - EX stage still active\n");
    if (MEM.active) printf(" - MEM stage still active\n");
    if (WB.active) printf(" - WB stage still active\n");
}

    countClock++;
    // Calculate row from PC before fetching
        unsigned int row = 0;
        for (int i = 0; i < 32; i++) {
            row = (row << 1) | PC[i-1];
        }
    if(WB.cycles_left == 0) {
        WB.active = 0;
      
    }else{
        WB.cycles_left=0;
        WB.active=0;
    }
    if(MEM.cycles_left == 0) {
        MEM.active = 0;
       
    }else{
        printf("Memory instruction %d: ", MEM.instruction_num);
        // for(int i = 0; i < 32; i++) {
        //     printf("%d", main_memory[row][i]);
        // }
        printf("\n");
        MEM.cycles_left=0;
        MEM.active=0;
        WB.instruction_num = MEM.instruction_num;
        for (int i = 0; i < 32; i++) {
            WB.instructionB[i] = MEM.instructionB[i];
        }
        WB.opcode = MEM.opcode;
        WB.rd = MEM.rd;
        WB.rs1_val = MEM.rs1_val;
        WB.rs2_val = MEM.rs2_val;
        WB.imm = MEM.imm;
        for (int i = 0; i < 32; i++) {
            WB.result[i] = MEM.result[i];
        }
    }
     if(EX.cycles_left == 0) {
        EX.active = 0;
    }else{
        
        EX.cycles_left--;
        if(EX.cycles_left==1){
            MEM.instruction_num = EX.instruction_num;
            MEM.instruction_num = EX.instruction_num;
            for (int i = 0; i < 32; i++) {
                MEM.instructionB[i] = EX.instructionB[i];
            }
            MEM.opcode = EX.opcode;
            MEM.rd = EX.rd;
            MEM.rs1_val = EX.rs1_val;
            MEM.rs2_val = EX.rs2_val;
            MEM.imm = EX.imm;
            for (int i = 0; i < 32; i++) {
                MEM.result[i] = EX.result[i];
            }
        }
        if(EX.cycles_left == 0) {
            EX.active = 0;
        }
    }
    if(ID.cycles_left == 0) {
        ID.active = 0;
    }else{
        ID.cycles_left--;
        if(ID.cycles_left==0){
            ID.active=0;
        }
        if (count < 3) {
            printf("[DECODE] Instruction %d: ", ID.instruction_num);
            for(int i = 0; i < 32; i++) {

            printf("%d", ID.instructionB[i] - '0'); // Convert char '0'/'1' to integer 0/1
            }
            printf("\n");
            printf("  Opcode: %d, R destination: R%d, R2_val: %d, R3_val: %d, Imm: %d\n", 
               ID.opcode, (int)(ID.rd - R0)/32, ID.rs1_val, ID.rs2_val, ID.imm);
            
        }
        EX.instruction_num = ID.instruction_num;
        for (int i = 0; i < 32; i++) {
            EX.instructionB[i] = ID.instructionB[i];
        }
        EX.opcode = ID.opcode;
        EX.rd = ID.rd;
        EX.rs1_val = ID.rs1_val;
        EX.rs2_val = ID.rs2_val;
        EX.imm = ID.imm;
        
    }
    if(IF.cycles_left == 0) {
        IF.active = 0;
        
    }else{
        printf("[FETCH] Instruction %d: ", IF.instruction_num);
        for(int i = 0; i < 32; i++) {
            printf("%c", IF.instructionB[i]); // Prints each bit as '0' or '1'
        }
    printf("\n");
    printf("  PC: ");
    for(int i = 0; i < 32; i++) printf("%d", PC[i]);
    printf(" (%d)\n", binary_array_to_int_len(PC, 32));
        IF.active = 0;
        IF.cycles_left--;
        ID.instruction_num = IF.instruction_num; //save number to make sure it is the one to enter in order to decode
        for (int i = 0; i < 32; i++) {
            ID.instructionB[i] = IF.instructionB[i];
        }
    }


    if(isJumping==1 && MEM.instruction_num==jumpNo)  {
        isJumping = 0;
        Flush();
        printf("Jumping to instruction %d\n", jumpNo);
        printf("Flushing pipeline...\n");
        printf("IF stage flushed - active: %d, cycles_left: %d, instruction_num: %d, opcode: %d, rd: %p\n",
            IF.active, IF.cycles_left, IF.instruction_num, IF.opcode, (void*)IF.rd);
        printf("ID stage flushed - active: %d, cycles_left: %d, instruction_num: %d, opcode: %d, rd: %p\n", 
            ID.active, ID.cycles_left, ID.instruction_num, ID.opcode, (void*)ID.rd);
        printf("EX stage flushed - active: %d, cycles_left: %d, instruction_num: %d, opcode: %d, rd: %p\n",
            EX.active, EX.cycles_left, EX.instruction_num, EX.opcode, (void*)EX.rd);
         // Reset jumping flag after handling jump
    }
    }
    count=0;


   
    print_final_state();
}


int main() {
   processFile("test.txt");
   run_pipeline();
}