%define NULL 0

section .data
empty_string: db 0

section .text

global string_proc_list_create_asm
global string_proc_node_create_asm
global string_proc_list_add_node_asm
global string_proc_list_concat_asm

extern malloc
extern free
extern str_concat


string_proc_list_create_asm:
    mov edi, 16           
    call malloc
    test rax, rax
    je .return_null
    mov qword [rax], 0   
    mov qword [rax+8], 0  
    ret
.return_null:
    xor rax, rax
    ret

---------------------------------------------------------------
; string_proc_node_create_asm:
; Reserva 32 bytes para un nodo e inicializa sus campos:
;   next = 0, previous = 0, type = valor, hash = puntero
; ---------------------------------------------------------------
string_proc_node_create_asm:
    mov edi, 32           ; tamaño de string_proc_node
    call malloc
    test rax, rax
    je .node_null
    mov qword [rax], 0       ; node->next     = NULL
    mov qword [rax+8], 0     ; node->previous = NULL
    mov byte  [rax+16], dil  ; node->type     = type (dil)
    mov qword [rax+24], rsi  ; node->hash     = hash (rsi)
    ret
.node_null:
    xor rax, rax
    ret

; ------------------------------------------
; string_proc_list_add_node_asm
; Entrada: rdi = lista, esi = type, rdx = hash
; ------------------------------------------
string_proc_list_add_node_asm:
    push    rbx
    mov     rbx, rdi            ; lista
    mov     dil, sil
    mov     rsi, rdx
    call    string_proc_node_create_asm
    test    rax, rax
    jz      .fin
    mov     rcx, [rbx]          ; head
    test    rcx, rcx
    jnz     .not_empty

    ; lista vacía
    mov     [rbx], rax
    mov     [rbx + 8], rax
    jmp     .fin

.not_empty:
    mov     rdx, [rbx + 8]      ; tail
    mov     [rdx], rax          ; tail->next = nodo
    mov     [rax + 8], rdx      ; nodo->prev = tail
    mov     [rbx + 8], rax      ; tail = nodo

.fin:
    pop     rbx
    ret

; ------------------------------------------
; string_proc_list_concat_asm
; Entrada: rdi = lista, esi = type, rdx = string
; Retorna: rax = string concatenada
; ------------------------------------------
string_proc_list_concat_asm:
    push    rbx
    push    r12
    mov     rbx, rdi            ; lista
    mov     r12b, sil           ; type a buscar
    mov     r13, rdx            ; string a concatenar

    mov     rdi, empty_string
    mov     rsi, r13
    call    str_concat
    mov     r14, rax            ; acumulador

    mov     r15, [rbx]          ; head

.loop:
    test    r15, r15
    jz      .done
    movzx   eax, byte [r15 + 16]
    cmp     al, r12b
    jne     .skip
    ; concat
    mov     rdi, r14
    mov     rsi, [r15 + 24]
    call    str_concat
    mov     rdx, r14
    mov     r14, rax
    mov     rdi, rdx
    call    free
.skip:
    mov     r15, [r15]
    jmp     .loop

.done:
    mov     rax, r14
    pop     r12
    pop     rbx
    ret