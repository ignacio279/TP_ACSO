;/** defines bool y puntero **/
%define NULL 0
%define TRUE 1
%define FALSE 0

section .data

section .text

global string_proc_list_create_asm
global string_proc_node_create_asm
global string_proc_list_add_node_asm
global string_proc_list_concat_asm

; FUNCIONES auxiliares:
extern malloc
extern free
extern str_concat
extern strdup

; ---------------------------------------------------------------
; string_proc_list_create_asm:
; Reserva 16 bytes para una lista (dos punteros) e inicializa ambos a 0.
; ---------------------------------------------------------------
string_proc_list_create_asm:
    mov edi, 16           ; tamaño de string_proc_list
    call malloc
    test rax, rax
    je .return_null
    mov qword [rax], 0    ; list->first = NULL
    mov qword [rax+8], 0  ; list->last  = NULL
    ret
.return_null:
    xor rax, rax
    ret

; ---------------------------------------------------------------
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

; ---------------------------------------------------------------
; string_proc_list_add_node_asm:
; Parámetros:
;   RDI = list*, RSI = type, RDX = hash
; Crea un nodo y lo enlaza al final de la lista.
; ---------------------------------------------------------------
string_proc_list_add_node_asm:
    test rdi, rdi
    je .return_add_node
    mov r8, rdi              ; r8 = pointer a list

    ; llamar a string_proc_node_create_asm(type, hash)
    mov rdi, rsi             ; rdi = type
    mov rsi, rdx             ; rsi = hash
    call string_proc_node_create_asm
    test rax, rax
    je .return_add_node
    mov r9, rax              ; r9 = new_node

    ; ¿Lista vacía?
    mov rax, qword [r8+8]    ; rax = list->last
    test rax, rax
    je .empty_list

    ; enlazar new_node al final
    mov qword [r9+8], rax    ; new_node->previous = old_last
    mov qword [rax], r9      ; old_last->next = new_node
    mov qword [r8+8], r9     ; list->last = new_node
    jmp .return_add_node_done

.empty_list:
    mov qword [r8],   r9     ; list->first = new_node
    mov qword [r8+8], r9     ; list->last  = new_node

.return_add_node_done:
.return_add_node:
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