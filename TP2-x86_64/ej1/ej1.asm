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

; ---------------------------------------------------------------
; string_proc_list_concat_asm:
; Parámetros:
;   RDI = list*, RSI = type, RDX = prefijo (char*)
; Duplica el prefijo con strdup y luego concatena, con str_concat,
; todos los hashes de los nodos cuyo campo type coincida.
; ---------------------------------------------------------------
string_proc_list_concat_asm:
    push rbx
    push r12
    push r13
    push r14

    test rdi, rdi
    je .return_concat_null_preserve
    test rdx, rdx
    je .return_concat_null_preserve

    mov r14, rdi        ; r14 = lista
    mov r12, rsi        ; r12b = type buscado

    ; strdup(prefijo)
    mov rdi, rdx        ; parámetro: prefijo
    call strdup
    test rax, rax
    je .return_concat_null_preserve
    mov r10, rax        ; r10 = result acumulado

    mov r13, qword [r14] ; r13 = lista->first

.concat_loop:
    test r13, r13
    je .end_concat_loop

    mov al, byte [r13+16] ; al = current_node->type
    cmp al, r12b
    jne .skip_concat

    ; str_concat(result, current_node->hash)
    mov rdi, r10               ; primer parámetro: result
    mov rsi, qword [r13+24]    ; segundo parámetro: hash
    call str_concat
    test rax, rax
    je .concat_fail

    ; liberar el antiguo result y actualizar r10
    mov rbx, rax
    mov rdi, r10
    call free
    mov r10, rbx

.skip_concat:
    mov r13, qword [r13] ; avanzar current_node = current_node->next
    jmp .concat_loop

.end_concat_loop:
    mov rax, r10         ; devolver result
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

.concat_fail:
    mov rdi, r10
    call free
.return_concat_null_preserve:
    xor rax, rax         ; retornar NULL
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
