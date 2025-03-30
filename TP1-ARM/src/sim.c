#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "shell.h"
#include <stdlib.h> 

#define ADDS_IMM     0b10110001 //
#define ADDS_REG     0b10101011000 //
#define HALT         0b11010100010 //
#define ANDS_REG     0b11101010 //
#define ORR_REG      0b10101010
#define B_cond       0b01010100
#define MOVZ         0b11010010100 //
#define LSL          0b1101001101 //
#define LSR          0b1101001100 //
#define CMP_IMM      0b11110000 //
#define CMP_REG      0b11101011001 //
#define STUR         0b11111000000 //
#define SUBS_IMM     0b11110001 //
#define SUBS_EXT_REG 0b11101011000 //
#define EOR_REG      0b11001010 //
#define BR           0b1101011000011111000000 
#define STURB        0b00111000000 //
#define STURH        0b01111000000
#define LDURB        0b00111000010
#define LDUR32       0b10111000010
#define LDUR64       0b11111000010
#define ADD_IMM      0b10010001 
#define ADD_EXT_REG  0b10001011001 
#define CBZ          0b10110100

int64_t signextend64(int32_t value, int bit_count) {
    int64_t mask = (int64_t)1 << (bit_count - 1); // Máscara para el bit de signo
    return (value ^ mask) - mask; // Extensión de signo a 64 bits
}

void ldur_32(uint32_t instruction) {
    printf("LDUR\n");
    uint32_t rt   = (instruction >> 0) & 0x1F;   // bits [4:0]
    uint32_t rn   = (instruction >> 5) & 0x1F;   // bits [9:5]
    uint32_t imm9 = (instruction >> 12) & 0x1FF;  // bits [20:12]

    int64_t offset = signextend64(imm9,9);
    uint64_t address = CURRENT_STATE.REGS[rn] + offset;

    // Leer 64 bits desde 'address'
    uint64_t data = mem_read_32(address);
    NEXT_STATE.REGS[rt] = data;
}

void ldur_64(uint32_t instruction) {
    printf("LDUR\n");
    uint32_t rt   = (instruction >> 0) & 0x1F;   // bits [4:0]
    uint32_t rn   = (instruction >> 5) & 0x1F;   // bits [9:5]
    uint32_t imm9 = (instruction >> 12) & 0x1FF;  // bits [20:12]

    int64_t offset = sign_extend_9(imm9);
    uint64_t address = CURRENT_STATE.REGS[rn] + offset;

    // Leer 64 bits combinando dos lecturas de 32 bits (asumiendo little-endian)
    uint32_t low  = mem_read_32(address);
    uint32_t high = mem_read_32(address + 4);
    uint64_t data = ((uint64_t) high << 32) | low;
    NEXT_STATE.REGS[rt] = data;
}

void adds_imm(uint32_t instruction);
void adds_reg(uint32_t instruction);
void ands_reg(uint32_t instruction);
void orr_reg(uint32_t instruction);
void b_cond(uint32_t instruction);
void movz(uint32_t instruction);
void subs_imm(uint32_t instruction);
void subs_ext_reg(uint32_t instruction);
void eor_reg(uint32_t instruction);
void br(uint32_t instruction);
void bne(uint32_t instruction);
void lsr_imm(uint32_t instruction);
void lsl_imm(uint32_t instruction);
void sturb(uint32_t instruction);
void sturh(uint32_t instruction);
void ldurb(uint32_t instruction);
void add_ext_reg(uint32_t instruction);
void add_imm(uint32_t instruction);
void cbz(uint32_t instruction);
void cmp_reg(uint32_t instruction);
void stur(uint32_t instruction);
void sturb(uint32_t instruction);
void sturh(uint32_t instruction);
void cmp_imm(uint32_t instruction);

void process_instruction() {
    
    uint32_t instruction = mem_read_32(CURRENT_STATE.PC);
    NEXT_STATE = CURRENT_STATE;

    uint32_t opcode24 = (instruction >> 24) & 0xFF;
    uint32_t opcode21 = (instruction >> 21) & 0x7FF;  
    uint32_t opcode22 = (instruction >> 22) & 0x3FF;

    
    switch (opcode21) {
        case ADDS_REG: {
            adds_reg(instruction);
            NEXT_STATE.PC += 4;
            break;
        }
        case MOVZ: {
            movz(instruction);
            NEXT_STATE.PC += 4;
            break;
        }
        case SUBS_EXT_REG: {
            subs_ext_reg(instruction);
            NEXT_STATE.PC += 4;
            break;
        }
        case CMP_REG: {
            cmp_reg(instruction);
            NEXT_STATE.PC += 4;
            break;
        }
        case STUR: {
            printf("STUR\n");
            uint32_t rt = (instruction >> 0) & 0b11111;      // Registro fuente (origen)
            uint32_t rn = (instruction >> 5) & 0b11111;      // Registro fuente (origen)
            uint32_t imm9 = (instruction >> 12) & 0b111111111;
            uint64_t address = CURRENT_STATE.REGS[rn] + imm9;
            uint64_t data = CURRENT_STATE.REGS[rt];
            // Verificación de la memoria en la simulación
            if (address < 0x10000000) {
                printf("Error: Acceso a memoria fuera de rango en 0x%lx\n", address);
                break;}
            mem_write_32(address, data);
            NEXT_STATE.PC += 4;
            break;
        }
        case STURB: {
            printf("STURB\n");
        
            uint32_t rt   = (instruction >> 0) & 0x1F;      // Registro destino
            uint32_t rn   = (instruction >> 5) & 0x1F;      // Registro base
            uint32_t imm9 = (instruction >> 12) & 0x1FF;    // Desplazamiento de 9 bits (con signo)
        
            // Sign-extend de imm9 a 64 bits (9 bits → int64_t)
            int64_t offset = 0;
            // Verifica si el bit 8 (de imm9) está en 1 (indicando número negativo):
            if (imm9 & 0x100) { 
                offset = imm9 | 0xFFFFFFFFFFFFFE00ULL; // Extiende con 1s
            } else {
                offset = imm9; // Positivo, no hace falta rellenar
            }
        
            uint64_t address = CURRENT_STATE.REGS[rn] + offset;
        
            // Solo el byte menos significativo de Rt
            uint8_t data_byte = (uint8_t)(CURRENT_STATE.REGS[rt] & 0xFF);
        
        
            // Guardar 1 byte
            mem_write_32(address, data_byte);
        
            NEXT_STATE.PC += 4;
            break;
        }
        case STURH: {
            printf("STURH\n");
            uint32_t rt = (instruction >> 0) & 0b11111;      // Registro fuente (origen)
            uint32_t rn = (instruction >> 5) & 0b11111;      // Registro fuente (origen)
            uint32_t imm9 = (instruction >> 12) & 0b111111111;
            uint64_t address = CURRENT_STATE.REGS[rn] + imm9;
            uint64_t data = CURRENT_STATE.REGS[rt];
            // Verificación de la memoria en la simulación
            if (address < 0x10000000) {
                printf("Error: Acceso a memoria fuera de rango en 0x%lx\n", address);
                break;
            }

            mem_write_32(address, data);
            NEXT_STATE.PC += 4;
            break;
        }
        case LDUR32: {
            ldur_32(instruction);
            NEXT_STATE.PC += 4;
            break;
        }
        case LDUR64: {
            ldur_64(instruction);
            NEXT_STATE.PC += 4;
            break;
        }
        case ADD_EXT_REG: {
            add_ext_reg(instruction);
            NEXT_STATE.PC += 4;
            break;
        }

        case HALT:
            printf("HALT detected. Stopping simulation.\n");
            RUN_BIT = 0;
            NEXT_STATE.PC += 4;
            return;
    }
    switch (opcode22)
        {
        case LSL : { //lsl X4, X3, 4 (descripción: Logical left shift (X4 = X3 << 4 ))
            lsl_imm(instruction);
            NEXT_STATE.PC += 4;
            break; 
        }
        case LSR : {
            lsr_imm(instruction);
            NEXT_STATE.PC += 4;
            break;
        }}
    switch (opcode24) {
        case ADDS_IMM: {
            adds_imm(instruction);
            NEXT_STATE.PC += 4;
            break;
        }
        case ANDS_REG: {
            ands_reg(instruction);
            NEXT_STATE.PC += 4;
            break;
        }
        case ORR_REG:{
            orr_reg(instruction);
            NEXT_STATE.PC += 4;
            break;
        }
        case SUBS_IMM:{
            subs_imm(instruction);
            NEXT_STATE.PC += 4;
            break;
        }
        case EOR_REG:{
            eor_reg(instruction);
            NEXT_STATE.PC += 4;
            break;
        }
        case ADD_IMM:{
            add_imm(instruction);
            NEXT_STATE.PC += 4;    
            break;
        }
        case CBZ:{
            cbz(instruction);
            NEXT_STATE.PC += 4;
            break;
        }
        case CMP_IMM:{
            cmp_imm(instruction);
            NEXT_STATE.PC += 4;
            break;
        }
        case B_cond:{
            uint32_t cond = (instruction >> 0) & 0b1111;
            uint32_t imm19 = (instruction >> 5) & 0b1111111111111111111;
        
            printf("cond: %d, imm: %d\n", cond, imm19);
        
            switch (cond) {
                case 0b0000:  // BEQ (Z == 1)
                    printf("BEQ\n");
                    NEXT_STATE.FLAG_Z = 1;
                    break;
                case 0b0001:  // BNE (Z == 0)
                    printf("BNE\n");
                    NEXT_STATE.FLAG_Z = 0;
                    break;
                case 0b1100:  // BGT (Z == 0 && N == V)
                    printf("BGT\n");
                    NEXT_STATE.FLAG_Z = 0;
                    NEXT_STATE.FLAG_N = 0;
                    break;
                case 0b1011:  // BLT (N != V)
                    printf("BLT\n");
                    NEXT_STATE.FLAG_N = !0;
                    break;
                case 0b1010:  // BGE (N == V)
                    printf("BGE\n");
                    NEXT_STATE.FLAG_N = 0;
                    break;
                case 0b1101:  // BLE (!(Z == 0 && N == V))
                    printf("BLE\n");
                    NEXT_STATE.FLAG_Z = 1;
                    NEXT_STATE.FLAG_N = !0;
                    break;
                default:
                    printf("Condición no reconocida: %d\n", cond);
                    break;
                NEXT_STATE.PC += 4;
                break;}
}
    }
}

void adds_imm(uint32_t instruction) {
        printf("ADD_IMM\n");
        uint32_t rn   = (instruction >> 5)  & 0b11111;
        uint32_t rd   = (instruction >> 0)  & 0b11111;
        uint32_t imm12 = (instruction >> 10) & 0xFFF;
        uint32_t shift = (instruction >> 22) & 0b11;

        uint64_t imm = (shift == 0b01) ? ((uint64_t)imm12 << 12) : (uint64_t)imm12;

        NEXT_STATE.REGS[rd] = NEXT_STATE.REGS[rn] + imm;

        NEXT_STATE.FLAG_Z = (NEXT_STATE.REGS[rd] == 0);
        NEXT_STATE.FLAG_N = (NEXT_STATE.REGS[rd] >> 63) & 1;}

void adds_reg(uint32_t instruction) {
        printf("ADD_REG\n");
        uint32_t rm  = (instruction >> 16) & 0b11111;
        uint32_t rn  = (instruction >> 5)  & 0b11111;
        uint32_t rd  = (instruction >> 0)  & 0b11111;

        uint64_t result = NEXT_STATE.REGS[rn] + NEXT_STATE.REGS[rm];
        NEXT_STATE.REGS[rd] = result;
        NEXT_STATE.FLAG_Z = (result == 0);
        NEXT_STATE.FLAG_N = (result >> 63) & 1;
        }

void orr_reg(uint32_t instruction) {
        printf("ORR_REG\n");
        uint32_t rm  = (instruction >> 16) & 0b11111;
        uint32_t rn  = (instruction >> 5)  & 0b11111;
        uint32_t rd  = (instruction >> 0)  & 0b11111;
        uint32_t inm6 = (instruction >> 10) & 0b111111;

        printf("rd: %d, rn: %d, rm: %d\n", rd, rn, rm);

        uint64_t result = NEXT_STATE.REGS[rn] | NEXT_STATE.REGS[rm];
        NEXT_STATE.REGS[rd] = result;
        NEXT_STATE.FLAG_Z = (result == 0);
        NEXT_STATE.FLAG_N = (result >> 63) & 1;}

void ands_reg(uint32_t instruction) {
        printf("ANDS_REG\n");
        uint32_t rm  = (instruction >> 16) & 0b11111;
        uint32_t rn  = (instruction >> 5)  & 0b11111;
        uint32_t rd  = (instruction >> 0)  & 0b11111;

        printf("rd: %d, rn: %d, rm: %d\n", rd, rn, rm);

        uint64_t result = NEXT_STATE.REGS[rn] & NEXT_STATE.REGS[rm];
        NEXT_STATE.REGS[rd] = result;
        NEXT_STATE.FLAG_Z = (result == 0);
        NEXT_STATE.FLAG_N = (result >> 63) & 1;}

void movz(uint32_t instruction) {
            printf("MOVZ\n");
            
            uint32_t imm16 = (instruction >> 5) & 0xFFFF;  // Extraer imm16 (16 bits)
            uint32_t rd    = (instruction >> 0) & 0x1F;    // Extraer rd (5 bits)
            uint32_t shift = (instruction >> 21) & 0x3;    // Extraer shift (2 bits)        
            uint32_t result = 0;
        
                // Asignar el valor inmediato sin desplazamiento (hw = 0 implica shift = 0)
                // Si hw != 0, se desplaza el valor inmediato 16 bits a la izquierda
                // y se guarda en el registro destino
                // Si hw = 0, se guarda el valor inmediato en el registro destino
            result = imm16;
                
                // Guardar el resultado en el registro destino
            NEXT_STATE.REGS[rd]= result;}

void b_cond(uint32_t instruction) {
    printf("B.condBEQ\n");
            uint32_t cond = (instruction >> 0) & 0b1111;
            uint32_t imm19 = (instruction >> 5) & 0b1111111111111111111;

            printf("cond: %d, imm: %d\n", cond, imm19);

            // Implementación del caso BEQ (Z == 1)
            if (cond == 0b0000) {
                NEXT_STATE.FLAG_Z==1;

            }// Implementación del caso BNE (Z == 1) 
            else if (cond==0b0001){
                NEXT_STATE.FLAG_Z==0;
            }// Implementación del caso BGT (Z == 0 && N == V)
            else if (cond==0b1100){
                NEXT_STATE.FLAG_Z==0 && NEXT_STATE.FLAG_N== 0;
            }// Implementación del caso BLT N! = V
            else if (cond==0b1011){
                !(NEXT_STATE.FLAG_N==0);
            } // Implementación del caso BGE (Z == 1 || N != V)
            else if (cond==0b1010)
            {
               NEXT_STATE.FLAG_N ==0;
            }//ble !(Z == 0 && N == V)
            else if (cond==0b1101)
            {
                !(NEXT_STATE.FLAG_Z==0 && NEXT_STATE.FLAG_N== 0); 
            } 
}

void subs_imm(uint32_t instruction){
        printf("SUBS1\n");
        uint32_t rn = (instruction >> 5) & 0b11111;      // Registro fuente (origen)
        uint32_t rd = (instruction >> 0) & 0b11111;
        uint32_t imm12 = (instruction >> 10) & 0b111111111111;
        uint32_t shift = (instruction >> 22) & 0b11;
        uint64_t imm = imm12; // Inicializar imm
        if (shift == 0b00){
            imm = (uint64_t)imm12;
        } else if (shift == 0b01){
            imm = (uint64_t)imm12 << 12;
        }
        uint64_t result;
        uint64_t operand1 = NEXT_STATE.REGS[rn];
        uint64_t operand2= imm;
        result = operand1 - operand2;
        NEXT_STATE.REGS[rd] = result;
        NEXT_STATE.FLAG_Z = (result == 0);
        NEXT_STATE.FLAG_N = (result >> 63) & 1;}

void lsr_imm(uint32_t instruction){
        printf("LSR (Immediate)\n");
        // Extraer campos
        uint32_t rn = (instruction >> 5) & 0b11111;   
        uint32_t inms = (instruction >> 10) & 0b111111;                  
        uint32_t rd = (instruction >> 0) & 0b11111;
        uint32_t shift = (instruction >> 16) & 0x3F; 
        uint64_t operand2;

        if (inms ==0b111111) {
            operand2 = CURRENT_STATE.REGS[rn] >> shift;
        }
        else if (inms != 0b111111) {
            operand2 = CURRENT_STATE.REGS[rn] <<64- shift;
        }
        NEXT_STATE.REGS[rd] = operand2;
}

void lsl_imm(uint32_t instruction){
        printf("LSL (Immediate)\n");

        // Extraer campos
        uint32_t rn = (instruction >> 5) & 0b11111;   
        uint32_t inms = (instruction >> 10) & 0b111111;                  
        uint32_t rd = (instruction >> 0) & 0b11111;
        uint32_t shift = (instruction >> 16) & 0x3F; 
        uint64_t operand2;

        if (inms ==0b111111) {
            operand2 = CURRENT_STATE.REGS[rn] >> shift;
        }
        else if (inms != 0b111111) {
            operand2 = CURRENT_STATE.REGS[rn] <<64- shift;
        }
        NEXT_STATE.REGS[rd] = operand2;
}

void subs_ext_reg(uint32_t instruction){
    printf("SUBS2\n");
    uint32_t rm = (instruction >> 16) & 0b11111;      // Registro fuente (origen)
    uint32_t rn = (instruction >> 5)  & 0b11111;      // Registro fuente (origen)
    uint32_t rd = (instruction >> 0)  & 0b11111;
    uint32_t shift = (instruction >> 22) & 0b11;
    
    uint64_t operand2 = NEXT_STATE.REGS[rm];

    if (shift ==0b01){
        operand2 <<= 12;
    }
    operand2 = ~operand2;
    uint64_t result = NEXT_STATE.REGS[rn] + operand2 + 1;
    NEXT_STATE.REGS[rd] = result;
    
    NEXT_STATE.FLAG_Z = (result == 0);
    NEXT_STATE.FLAG_N = (result >> 63) & 1;
}

void eor_reg(uint32_t instruction){
        printf("EOR\n");
        uint32_t rm = (instruction >> 16) & 0b11111;      // Registro fuente (origen)
        uint32_t rn = (instruction >> 5) & 0b11111; 
        uint32_t imm6 = (instruction >> 10) & 0b111111;     // Registro fuente (origen)
        uint32_t rd = (instruction >> 0) & 0b11111;
        uint64_t result;
        uint64_t operand1 = NEXT_STATE.REGS[rn];
        uint64_t operand2 = NEXT_STATE.REGS[rm];
        result= operand1 ^ operand2;
        NEXT_STATE.REGS[rd] = result;
}

void br(uint32_t instruction){
        printf("BR\n");
        uint32_t imm26 = (instruction >> 0) & 0b11111111111111111111111111;
        uint64_t result;
        uint64_t operand1 = NEXT_STATE.PC;
        result= operand1 + imm26;
        NEXT_STATE.PC = result;
}

void cmp_reg(uint32_t instruction){
        printf("CMP_REG\n");
        uint32_t rm = (instruction >> 16) & 0b11111;      // Registro fuente (origen)
        uint32_t rn = (instruction >> 5) & 0b11111;      // Registro fuente (origen)
        uint32_t rd = (instruction >> 0) & 0b11111;
        uint32_t imm3 = (instruction >> 10) & 0b111;
        uint32_t option = (instruction >> 13) & 0b111;
        uint64_t result;
        uint64_t operand1 = NEXT_STATE.REGS[rn];
        uint64_t operand2 = NEXT_STATE.REGS[rm];
        result= operand1 - operand2;
        NEXT_STATE.FLAG_Z = (result == 0);
        NEXT_STATE.FLAG_N = (result >> 63) & 1;
}

void cmp_imm(uint32_t instruction){
        printf("CMP_IMM\n");
        uint32_t rn = (instruction >> 5) & 0b11111;      // Registro fuente (origen)
        uint32_t rd = (instruction >> 0) & 0b11111;
        uint32_t imm12 = (instruction >> 10) & 0b111111111111;
        uint32_t shift = (instruction >> 22) & 0b11;
        uint64_t imm = imm12; // Inicializar imm
        if (shift == 0b00){
            imm = (uint64_t)imm12;
        } else if (shift == 0b01){
            imm = (uint64_t)imm12 << 12;
        }
        uint64_t result;
        uint64_t operand1 = NEXT_STATE.REGS[rn];
        uint64_t operand2= ~imm;
        result = operand1 + operand2;
        NEXT_STATE.FLAG_Z = (result == 0);
        NEXT_STATE.FLAG_N = (result >> 63) & 1;
}

void add_ext_reg(uint32_t instruction){
        printf("ADD\n");
        uint32_t rm = (instruction >> 16) & 0b11111;      // Registro fuente (origen)
        uint32_t rn = (instruction >> 5) & 0b11111; 
        uint32_t rd = (instruction >> 0) & 0b11111;
        uint32_t imm3 = (instruction >> 10) & 0b111;
        uint32_t option = (instruction >> 13) & 0b111;
        uint64_t result;
        uint64_t operand1 = NEXT_STATE.REGS[rn];
        uint64_t operand2 = NEXT_STATE.REGS[rm];
        result= operand1 + operand2;
        NEXT_STATE.REGS[rd] = result;
}

void add_imm(uint32_t instruction){
        printf("ADD\n");
        uint32_t rn = (instruction >> 5) & 0b11111;      // Registro fuente (origen)
        uint32_t rd = (instruction >> 0) & 0b11111;
        uint32_t imm12 = (instruction >> 10) & 0b111111111111;
        uint32_t shift = (instruction >> 22) & 0b11;
        uint64_t imm = imm12; // Inicializar imm
        if (shift == 0b00){
            imm = (uint64_t)imm12;
        } else if (shift == 0b01){
            imm = (uint64_t)imm12 << 12;
        }
        uint64_t result;
        uint64_t operand1 = NEXT_STATE.REGS[rn];
        result = operand1 + imm;
        NEXT_STATE.REGS[rd] = result;
}

void cbz(uint32_t instruction){
        printf("CBZ\n");
        uint32_t rt = (instruction >> 0) & 0b11111;      // Registro fuente (origen)
        uint32_t imm19 = (instruction >> 5) & 0b1111111111111111111;     // Registro fuente (origen)
        uint64_t result;
        uint64_t operand1 = NEXT_STATE.REGS[rt];
        if (operand1 == 0){
            result= operand1 + imm19;
            NEXT_STATE.PC = result;
        }
}