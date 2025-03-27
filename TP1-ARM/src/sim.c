#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "shell.h"
#include <stdlib.h> 

#define ADD_IMM 0b10110001
#define ADD_REG 0b10101011000 
int halted = 0;  // 0: en ejecución, 1: detenido

void process_instruction() {

    printf("PC actual: 0x%llx\n", CURRENT_STATE.PC); // Se lee la instruccion
    uint32_t instruction = mem_read_32(CURRENT_STATE.PC);  //Se almacena la instrucción leída.

    NEXT_STATE = CURRENT_STATE;
    

    uint32_t opcode24 = (instruction >> 24) & 0b11111111;  // Extraer los primeros 8 bits como opcode (indican la operacion a realizar)
    uint32_t opcode21 = (instruction >> 21) & 0b11111111111;      // Extraer los siguientes 6 bits como opcode (indican la operacion a realizar)
      // Registro destino (donde ser gaurda)    // Operando inmediato o dirección

    if (halted) {
        printf("Can't simulate, Simulator is halted\n");
        return; // Si el simulador está detenido, no se puede simular
    }
    
    printf("Opcode 24: 0x%X\n", opcode24);
    printf("Opcode 21: 0x%X\n", opcode21);
    printf("0x%X\n",ADD_IMM);
    printf("0x%X\n",ADD_REG);
    switch (opcode24) {
        case ADD_IMM: // suma entre el valor de rn y el oeprando y se guarda en rd
            printf("ADD_IMM\n");
            uint32_t rn = (instruction >> 5) & 0b11111;      // Registro fuente (origen)
            uint32_t rd = (instruction >> 0) & 0b11111;
            printf("rd: %d\n", rd);
            printf("rn: %d\n", rn);
            uint32_t imm12 = (instruction >> 10) & 0b111111111111;
            printf("imm12: %d\n", imm12);
            uint32_t shift = (instruction >> 22) & 0b11;
            uint64_t imm = imm12; // Inicializar imm
            if (shift == 0b00){
                imm = (uint64_t)imm12;
                printf("imm: %d\n", imm);
            } else if (shift == 0b01){
                imm = (uint64_t)imm12 << 12;
                printf("imm: %d\n", imm);
            }
            uint64_t result;
            uint64_t operand1 = NEXT_STATE.REGS[rn];
            printf("nextstate: %d\n", operand1);
            result = operand1 + imm;
            NEXT_STATE.REGS[rd] = result;
            break;

        case ADD_REG: // suma entre el valor de rn y el valor de rm y se guarda en rd
            printf("ADD_REG\n");
            uint32_t rm = (instruction >> 16) & 0b11111;      // Registro fuente (origen)
            rn = (instruction >> 5) & 0b11111;      // Registro fuente (origen)
            rd = (instruction >> 0) & 0b11111;
            printf("rd: %d\n", rd);
            printf("rn: %d\n", rn);
            printf("rm: %d\n", rm);
            operand1 = NEXT_STATE.REGS[rn];
            uint64_t operand2 = NEXT_STATE.REGS[rm];
            result = operand1 + operand2;
            NEXT_STATE.REGS[rd] = result;
            break;
        
    
}
}
