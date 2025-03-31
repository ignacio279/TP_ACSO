.text
.global _start
_start:
    MOVZ   X3, #0          // Pone a X3 en 0
    CBZ    X3, branch_taken // Si X3 es 0, salta a branch_taken
    MOVZ   X0, #1          // Esta instrucción no se ejecutará si CBZ se toma
    B      end             // Salta al final para no ejecutar el código de abajo
branch_taken:
    MOVZ   X1, #42         // En branch_taken, se asigna 42 a X1
end:
    HLT                   // Detiene la simulación
