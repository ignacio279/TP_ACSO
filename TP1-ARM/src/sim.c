#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "shell.h"
#include <stdlib.h> 

#define ADDS_IMM     0b10110001
#define ADDS_REG     0b10101011000
#define HALT         0b11010100010
#define ANDS_REG     0b11101010
#define ORR_REG      0b10101010
#define B_cond       0b01010100
#define MOVZ         0b11010010100 

#define SUBS_IMM     0b11110001 
#define SUBS_EXT_REG 0b11101011001 
#define EOR_REG      0b11001010 
#define BR           0b1101011000011111000000 
#define LSR_IMM      0b110100110 //
#define STURB        0b00111000000
#define STURH        0b01111000000
#define LDURB        0b00111000010 
#define ADD_IMM      0b10010001 
#define ADD_EXT_REG  0b10001011001 
#define CBZ          0b10110100

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
void sturb(uint32_t instruction);
void sturh(uint32_t instruction);
void ldurb(uint32_t instruction);
void add_ext_reg(uint32_t instruction);
void add_imm(uint32_t instruction);
void cbz(uint32_t instruction);

void process_instruction() {
    printf("PC actual: 0x%llx\n", CURRENT_STATE.PC);
    
    uint32_t instruction = mem_read_32(CURRENT_STATE.PC);
    NEXT_STATE = CURRENT_STATE;

    uint32_t opcode24 = (instruction >> 24) & 0xFF;   // Bits 24-31
    uint32_t opcode21 = (instruction >> 21) & 0x7FF;  // Bits 21-31

    printf("Instrucción: 0x%X\n", instruction);
    printf("Opcode24: %d | Opcode21: %d\n", opcode24, opcode21);
    
    switch (opcode21) {
        case ADDS_REG: {
            adds_reg(instruction);
            break;
        }
        case MOVZ: {
            movz(instruction);
            break;
        }
        case SUBS_EXT_REG: {
            subs_ext_reg(instruction);
            break;
        }
        // case STURB: {
        //     sturb(instruction);
        //     break;
        // }
        // case STURH: {
        //     sturh(instruction);
        //     break;
        // }
        // case LDURB: {
        //     ldurb(instruction);
        //     break;
        // }
        case ADD_EXT_REG: {
            add_ext_reg(instruction);
            break;
        }

        case HALT:
            printf("HALT detected. Stopping simulation.\n");
            RUN_BIT = 0;
            return;
    }

    switch (opcode24) {
        case ADDS_IMM: {
            adds_imm(instruction);
            break;
        }
        case ANDS_REG: {
            ands_reg(instruction);
            break;
        }
        case ORR_REG:{
            orr_reg(instruction);
            break;
        }
        case SUBS_IMM:{
            subs_imm(instruction);
            break;
        }
        case EOR_REG:{
            eor_reg(instruction);
            break;
        }
        case ADD_IMM:{
            add_imm(instruction);
            break;
        }
        case CBZ:{
            cbz(instruction);
            break;
        }
        case B_cond:{
            b_cond(instruction);}
    }

    NEXT_STATE.PC += 4;
}


void adds_imm(uint32_t instruction) {
        printf("ADD_IMM\n");
        uint32_t rn   = (instruction >> 5)  & 0b11111;
        uint32_t rd   = (instruction >> 0)  & 0b11111;
        uint32_t imm12 = (instruction >> 10) & 0xFFF;
        uint32_t shift = (instruction >> 22) & 0b11;

        uint64_t imm = (shift == 0b01) ? ((uint64_t)imm12 << 12) : (uint64_t)imm12;

        printf("rd: %d, rn: %d, imm: %llu\n", rd, rn, imm);

        NEXT_STATE.REGS[rd] = NEXT_STATE.REGS[rn] + imm;

        NEXT_STATE.FLAG_Z = (NEXT_STATE.REGS[rd] == 0);
        NEXT_STATE.FLAG_N = (NEXT_STATE.REGS[rd] >> 63) & 1;}

void adds_reg(uint32_t instruction) {
        uint32_t rm  = (instruction >> 16) & 0b11111;
        uint32_t rn  = (instruction >> 5)  & 0b11111;
        uint32_t rd  = (instruction >> 0)  & 0b11111;

        printf("rd: %d, rn: %d, rm: %d\n", rd, rn, rm);

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
        
            printf("rd: %d, imm16: 0x%X, shift: %d\n", rd, imm16, shift);
        
            uint32_t result = 0;
        
                // Asignar el valor inmediato sin desplazamiento (hw = 0 implica shift = 0)
                // Si hw != 0, se desplaza el valor inmediato 16 bits a la izquierda
                // y se guarda en el registro destino
                // Si hw = 0, se guarda el valor inmediato en el registro destino
            result = imm16;
                
                // Guardar el resultado en el registro destino
            NEXT_STATE.REGS[rd]= result;
            
            printf("Final result: 0x%lX\n", result);}

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
        printf("SUBS\n");
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
        NEXT_STATE.REGS[rd] = result;
        NEXT_STATE.FLAG_Z = (result == 0);
        NEXT_STATE.FLAG_N = (result >> 63) & 1;}


void subs_ext_reg(uint32_t instruction){
        printf("SUBS\n");
        uint32_t rm = (instruction >> 16) & 0b11111;      // Registro fuente (origen)
        uint32_t rn = (instruction >> 5) & 0b11111;      // Registro fuente (origen)
        uint32_t rd = (instruction >> 0) & 0b11111;
        uint32_t imm3 = (instruction >> 10) & 0b111;
        uint32_t option = (instruction >> 13) & 0b111;
        uint64_t result;
        uint64_t operand1 = NEXT_STATE.REGS[rn];
        uint64_t operand2 = NEXT_STATE.REGS[rm];
        result= operand1 - operand2;
        NEXT_STATE.REGS[rd] = result;
        NEXT_STATE.FLAG_Z = (result == 0);
        NEXT_STATE.FLAG_N = (result >> 63) & 1;
}


//EOR (Shifted Register)
//eor X0, X1, X2 (descripción: X0 = X1 ^ X2)
//En el opcode se considerar que shift y N son siempre ceros, por lo que se chequean los bits
//<31:21>
//No se tiene que implementar el shift


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


//B
//b target:
//target:
//(descripción: saltar a la instrucción de target, el target se calcula relativo a donde está
//apuntando el PC, así que puede ser positivo o negativo el salto, prestar especial atención.
// Imm26:'00 quiere decir, el immediate seguido de dos bits siendo cero, osea un numero de 28
// bits.)
// BR
// br X1 (descripción: saltar a la dirección guardada en el registro X1)

void br(uint32_t instruction){
        printf("BR\n");
        uint32_t imm26 = (instruction >> 0) & 0b11111111111111111111111111;
        uint64_t result;
        uint64_t operand1 = NEXT_STATE.PC;
        result= operand1 + imm26;
        NEXT_STATE.PC = result;
}

// BNE (B.Cond)
// cmp X1,X2
// bne target
// .
// .
// target
// (descripción: salto a target si X1 != X2, se valida el caso con los flags. Si requiere flags C o V,
// asumir que son cero. Vale para todos los b.conditional. Esta instrucción es un caso de b.cond)

void bne(uint32_t instruction){
        printf("BNE\n");
        uint32_t imm26 = (instruction >> 0) & 0b11111111111111111111111111;
        uint64_t result;
        uint64_t operand1 = NEXT_STATE.PC;
        result= operand1 + imm26;
        NEXT_STATE.PC = result;
}


// BLE (B.Cond)
// cmp X1,X2
// ble target
// .
// .
// target
// (descripción: salto a target si X1 <= X2, se valida el caso con los flags. Si requiere flags C o V,
// asumir que son cero. Vale para todos los b.conditional. Esta instrucción es un caso de b.cond)

void ble(uint32_t instruction){
        printf("BLE\n");
        uint32_t imm26 = (instruction >> 0) & 0b11111111111111111111111111;
        uint64_t result;
        uint64_t operand1 = NEXT_STATE.PC;
        result= operand1 + imm26;
        NEXT_STATE.PC = result;
}

// LSR (Immediate)
// lsr X4, X3, 4 (descripción: Logical right shift (X4 = X3 >> 4)

void lsr_imm(uint32_t instruction){
        printf("LSR\n");
        uint32_t rm = (instruction >> 16) & 0b11111;      // Registro fuente (origen)
        uint32_t rn = (instruction >> 5) & 0b11111; 
        uint32_t imm6 = (instruction >> 10) & 0b111111;     // Registro fuente (origen)
        uint32_t rd = (instruction >> 0) & 0b11111;
        uint64_t result;
        uint64_t operand1 = NEXT_STATE.REGS[rn];
        uint64_t operand2 = NEXT_STATE.REGS[rm];
        result= operand1 >> operand2;
        NEXT_STATE.REGS[rd] = result;
}

// STURB
// sturb X1, [X2, #0x10] (descripción: M[X2 + 0x10](7:0) = X1(7:0), osea los primeros 8 bits del
// registro son guardados en los primeros 8 bits guardados en la dirección de memoria).
// Importante acordarse que la memoria es little endian en Arm.
// Acuerdense que en el simulador la memoria empieza en 0x10000000, ver especificaciones, no
// cambia la implementación pero si el testeo.



// STURH
// sturh W1, [X2, #0x10] (descripción: M[X2 + 0x10](15:0) = X1(15:0), osea los primeros 16 bits
// del registro son guardados en los primeros 16 bits guardados en la dirección de memoria).
// Importante acordarse que la memoria es little endian en Arm.
// Acuerdense que en el simulador la memoria empieza en 0x10000000, ver especificaciones, no
// cambia la implementación pero si el testeo.


// LDURB
// ldurb W1, [X2, #0x10] (descripción: X1= 56’b0, M[X2 + 0x10](7:0), osea 56 ceros y los
// primeros 8 bits guardados en la dirección de memoria)
// Acuerdense que en el simulador la memoria empieza en 0x10000000, ver especificaciones, no
// cambia la implementación pero si el testeo.



// ADD (Extended Register & Immediate)
// Immediate: add X0, X1, 3 (descripción: X0 = X1 + 3)
// El caso de shift == 01 se debe implementar, osea moviendo el imm12, 12 bits a la izquierda.
// También se debe implementar shift 00, pero no el caso de ReservedValue.
// Extended Register: add X0 = X1, X2 (descripción: X0 = X1 + X2)

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

// CBZ
// cbz X3, label
// .
// .
// label (descripción: saltar a label, si X3 es 0)

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