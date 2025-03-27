#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "shell.h"
#include <stdlib.h> 

#define ADDS_IMM     0b10110001  
#define ADDS_REG     0b10110001000  
#define SUBS_IMM     0b11110001 
#define SUBS_EXT_REG 0b11101011001 
#define EOR_REG      0b11001010 
#define BR           0b1101011000011111000000 
#define BCOND        0b01010100 
#define LSR_IMM      0b110100110 //
#define STURB        0b00111000000
#define STURH        0b01111000000
#define LDURB        0b00111000010 
#define ADD_IMM      0b10010001 
#define ADD_EXT_REG  0b10001011001 
#define CBZ          0b10110100 

void adds_imm(uint32_t instruction);
void adds_reg(uint32_t instruction);
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



int halted = 0;  // 0: en ejecución, 1: detenido

void process_instruction() {

    printf("PC actual: 0x%lx\n", CURRENT_STATE.PC);
    uint32_t instruction = mem_read_32(CURRENT_STATE.PC);  //Se almacena la instrucción leída.

    NEXT_STATE = CURRENT_STATE;

    //for para que a switch se le pase los opcodes
    
    
    uint32_t opcode24 = (instruction >> 24) & 0b11111111;  // Extraer los primeros 8 bits como opcode (indican la operacion a realizar)
    uint32_t opcode21 = (instruction >> 21) & 0b11111111111;      // Extraer los bits 21 al 31 como opcode (indican la operacion a realizar)
      // Registro destino (donde ser gaurda)    // Operando inmediato o dirección

    if (halted) {
        printf("Can't simulate, Simulator is halted\n");
        return; // Si el simulador está detenido, no se puede simular
    }
    
    switch (opcode24) {
        case ADDS_IMM: // suma entre el valor de rn y el oeprando y se guarda en rd
            adds_imm(instruction);
            break;
        case ADDS_REG: // suma entre el valor de rn y el valor de rm y se guarda en rd
            adds_reg(instruction);
            break;
        case SUBS_IMM: // resta entre el valor de rn y el oeprando y se guarda en rd
            subs_imm(instruction);
            break;
        case SUBS_EXT_REG: // resta entre el valor de rn y el valor de rm y se guarda en rd
            subs_ext_reg(instruction);
            break;
        case EOR_REG: // XOR entre el valor de rn y el valor de rm y se guarda en rd
            eor_reg(instruction);
            break;
        case BR: // salto a la dirección guardada en el registro X1
            br(instruction);
            break;
        case BCOND: // salto a target si X1 != X2
            bne(instruction);
            break;
        // case BLE: // salto a target si X1 <= X2
        //     ble(instruction);
        //     break;
        case LSR_IMM: // Logical right shift (X4 = X3 >> 4)
            lsr_imm(instruction);
            break;
        // case STURB: // M[X2 + 0x10](7:0) = X1(7:0)
        //     sturb(instruction);
        //     break;
        // case STURH: // M[X2 + 0x10](15:0) = X1(15:0)
        //     sturh(instruction);
        //     break;
        // case LDURB: // X1= 56’b0, M[X2 + 0x10](7:0)
        //     ldurb(instruction);
        //     break;
        case ADD_IMM: // suma entre el valor de rn y el oeprando y se guarda en rd
            add_imm(instruction);
            break;
        case ADD_EXT_REG: // suma entre el valor de rn y el valor de rm y se guarda en rd
            add_ext_reg(instruction);
            break;  
        case CBZ: // salto a label, si X3 es 0
            cbz(instruction);
            break;
        default:
            printf("Instrucción no implementada\n");
            break;
    
}
}

void adds_imm(uint32_t instruction){
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
        NEXT_STATE.REGS[rd] = result;}

void adds_reg(uint32_t instruction){
        uint32_t rm = (instruction >> 16) & 0b11111;      // Registro fuente (origen)
        uint32_t rn = (instruction >> 5) & 0b11111;      // Registro fuente (origen)
        uint32_t rd = (instruction >> 0) & 0b11111;
        uint32_t imm3 = (instruction >> 10) & 0b111;
        uint32_t option = (instruction >> 13) & 0b111;
        uint64_t result;
        uint64_t operand1 = NEXT_STATE.REGS[rn];
        uint64_t operand2 = NEXT_STATE.REGS[rm];
        result= operand1 + operand2;
        NEXT_STATE.REGS[rd] = result;
}


void subs_imm(uint32_t instruction){
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
        NEXT_STATE.REGS[rd] = result;}


void subs_ext_reg(uint32_t instruction){
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
}


//EOR (Shifted Register)
//eor X0, X1, X2 (descripción: X0 = X1 ^ X2)
//En el opcode se considerar que shift y N son siempre ceros, por lo que se chequean los bits
//<31:21>
//No se tiene que implementar el shift


void eor_reg(uint32_t instruction){
        print("EOR\n");
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
        uint32_t imm26 = (instruction >> 0) & 0b11111111111111111111111111;
        uint64_t result;
        uint64_t operand1 = NEXT_STATE.PC;
        result= operand1 + imm26;
        NEXT_STATE.PC = result;
}

// LSR (Immediate)
// lsr X4, X3, 4 (descripción: Logical right shift (X4 = X3 >> 4)

void lsr_imm(uint32_t instruction){
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
        uint32_t rt = (instruction >> 0) & 0b11111;      // Registro fuente (origen)
        uint32_t imm19 = (instruction >> 5) & 0b1111111111111111111;     // Registro fuente (origen)
        uint64_t result;
        uint64_t operand1 = NEXT_STATE.REGS[rt];
        if (operand1 == 0){
            result= operand1 + imm19;
            NEXT_STATE.PC = result;
        }
}