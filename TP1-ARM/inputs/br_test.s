.text
.global _start

mov X0, #1
br BR_target
MOVZ X0, #2

MOVZ X1, #42
HLT
