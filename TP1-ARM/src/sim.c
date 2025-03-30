#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "shell.h"
#include <stdlib.h> 

#define ADDS_IMM  0b10110001
#define ADDS_REG  0b10101011000
#define HALT     0b11010100010
#define ANDS_REG 0b11101010
#define ORR_REG  0b10101010
#define B_cond 0b01010100
#define MOVZ 0b11010010100 

void adds_imm(uint32_t instruction);
void adds_reg(uint32_t instruction);
void ands_reg(uint32_t instruction);
void orr_reg(uint32_t instruction);
void b_cond(uint32_t instruction);
void movz(uint32_t instruction);


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

        NEXT_STATE.REGS[rd] = NEXT_STATE.REGS[rn] + imm;}

void adds_reg(uint32_t instruction) {
        uint32_t rm  = (instruction >> 16) & 0b11111;
        uint32_t rn  = (instruction >> 5)  & 0b11111;
        uint32_t rd  = (instruction >> 0)  & 0b11111;

        printf("rd: %d, rn: %d, rm: %d\n", rd, rn, rm);

        uint64_t result = NEXT_STATE.REGS[rn] + NEXT_STATE.REGS[rm];
        NEXT_STATE.REGS[rd] = result;}

void orr_reg(uint32_t instruction) {
        printf("ORR_REG\n");
        uint32_t rm  = (instruction >> 16) & 0b11111;
        uint32_t rn  = (instruction >> 5)  & 0b11111;
        uint32_t rd  = (instruction >> 0)  & 0b11111;
        uint32_t inm6 = (instruction >> 10) & 0b111111;

        printf("rd: %d, rn: %d, rm: %d\n", rd, rn, rm);

        uint64_t result = NEXT_STATE.REGS[rn] | NEXT_STATE.REGS[rm];
        NEXT_STATE.REGS[rd] = result;

        // Actualizar banderas
        NEXT_STATE.FLAG_N = 1;
        NEXT_STATE.FLAG_Z = 1;}

void ands_reg(uint32_t instruction) {
        printf("ANDS_REG\n");
        uint32_t rm  = (instruction >> 16) & 0b11111;
        uint32_t rn  = (instruction >> 5)  & 0b11111;
        uint32_t rd  = (instruction >> 0)  & 0b11111;

        printf("rd: %d, rn: %d, rm: %d\n", rd, rn, rm);

        uint64_t result = NEXT_STATE.REGS[rn] & NEXT_STATE.REGS[rm];
        NEXT_STATE.REGS[rd] = result;

        // Actualizar banderas
        NEXT_STATE.FLAG_N = 1;
        NEXT_STATE.FLAG_Z = 1;}

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