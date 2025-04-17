; /** defines bool y puntero **/
%define NULL 0
%define TRUE 1
%define FALSE 0

section .data

section .text

global string_proc_list_create_asm
global string_proc_node_create_asm
global string_proc_list_add_node_asm
global string_proc_list_concat_asm

; FUNCIONES auxiliares que pueden llegar a necesitar:
extern malloc
extern free
extern str_concat
extern strdup

; ---------------------------------------------------------------
; string_proc_list_create_asm:
; Reserva 16 bytes para una lista (dos punteros) e inicializa ambos a 0.
; ---------------------------------------------------------------
string_proc_list_create_asm:
    mov edi, 16         ; sizeof(string_proc_list) = 16 bytes
    call malloc      
    test rax, rax       
    je .return_null     
    mov qword [rax], 0  ; list->first = NULL
    mov qword [rax+8], 0 ; list->last = NULL
    ret
.return_null:
    xor rax, rax       
    ret

; ---------------------------------------------------------------
; string_proc_node_create_asm:
; Reserva 32 bytes para un nodo y asigna:
;   next = 0 (offset 0)
;   previous = 0 (offset 8)
;   type = valor (offset 16, 1 byte)
;   hash = puntero (offset 24)
; ---------------------------------------------------------------
; string_proc_node_create_asm(RDI=type, RSI=hash)
string_proc_node_create_asm:
    ; rdi = type, rsi = hash
    movzx r8, dil        ; <-- guardar type original (en r8)
    mov edi, 32          ; ahora sí malloc (sobre rdi)
    call malloc
    test rax, rax
    je .node_null

    mov qword [rax], 0       ; next
    mov qword [rax+8], 0     ; previous
    mov byte [rax+16], r8b   ; ahora sí guardo type original
    mov qword [rax+24], rsi  ; hash original

    ret

.node_null:
    xor rax, rax
    ret



; ---------------------------------------------------------------
; string_proc_list_add_node_asm:
; Parámetros:
;   RDI = pointer a string_proc_list
;   RSI = type (uint8_t)
;   RDX = hash (char*)
; Crea un nodo (llamando a string_proc_node_create_asm) y lo enlaza a la lista.
; Si la lista está vacía, asigna ambos first y last al nuevo nodo.
; Si ya tiene nodos, lo enlaza al final.
; ---------------------------------------------------------------
string_proc_list_add_node_asm:
    ; rdi = list, rsi = type, rdx = hash
    push r12
    push r13
    push r14

    mov r12, rdi          ; r12 = list
    movzx r13, sil        ; r13 = type (zero-extend a 64 bits)
    mov r14, rdx          ; r14 = hash

    ; llamar a string_proc_node_create_asm(r13, r14)
    movzx rdi, r13b       ; type → rdi
    mov rsi, r14          ; hash → rsi
    call string_proc_node_create_asm
    test rax, rax
    je .return_add_node   ; si malloc falló

    mov rdx, rax          ; rdx = new_node
    mov rax, qword [r12+8] ; rax = list->last

    test rax, rax
    je .empty_list

    ; caso: lista NO vacía
    mov qword [rdx+8], rax    ; new_node->previous = list->last
    mov qword [rax+0], rdx    ; list->last->next = new_node
    mov qword [r12+8], rdx    ; list->last = new_node
    jmp .return_add_node_done

.empty_list:
    ; caso: lista vacía
    mov qword [r12], rdx      ; list->first = new_node
    mov qword [r12+8], rdx    ; list->last = new_node

.return_add_node_done:
.return_add_node:
    pop r14
    pop r13
    pop r12
    ret


; ---------------------------------------------------------------
; string_proc_list_concat_asm:
; Parámetros:
;   RDI = pointer a string_proc_list (lista)
;   RSI = type (uint8_t)
;   RDX = hash (char*)
; Función:
;   - Verifica que list y hash no sean NULL.
;   - Guarda el parámetro "type" en R12 (no volátil).
;   - Duplica la cadena "hash" con strdup; resultado en R10.
;   - Copia list->first (offset 0) en R13 (usado para la iteración, callee-saved).
;   - Itera por la lista: para cada nodo, si el byte en [r13+16] coincide con r12b,
;     llama a str_concat(result, node->hash), libera el viejo result y actualiza r10.
;   - Al finalizar, retorna el puntero en RAX.
; Se preservan los registros no volátiles: RBX, R12, R13 y se copia list pointer en R14.
; ---------------------------------------------------------------
string_proc_list_concat_asm:
    push rbx
    push r12
    push r13
    push r14

    ; Verifica si lista o hash son NULL
    test rdi, rdi
    je .error_null
    test rdx, rdx
    je .error_null

    mov r14, rdi        ; r14 = list pointer
    mov r12, rsi        ; r12 = type
    mov rdi, rdx        ; rdi = hash
    call strdup         ; r10 = strdup(hash)
    test rax, rax
    je .error_null
    mov r10, rax        ; r10 = resultado actual

    mov r13, [r14]      ; r13 = current_node = list->first

.loop:
    test r13, r13
    je .done

    movzx eax, byte [r13+16] ; eax = current_node->type
    cmp al, r12b
    jne .next

    ; Concatenar si coincide el type
    mov rdi, r10
    mov rsi, [r13+24]   ; rsi = current_node->hash
    call str_concat
    test rax, rax
    je .fail_concat

    ; Si la nueva string es diferente, liberar la vieja
    cmp rax, r10
    je .skip_free
    mov rbx, rax
    mov rdi, r10
    call free
    mov r10, rbx

.skip_free:
.next:
    mov r13, [r13]      ; avanzar al siguiente nodo
    jmp .loop

.done:
    mov rax, r10
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

.fail_concat:
    ; Falló str_concat → liberar lo anterior y retornar NULL
    mov rdi, r10
    call free

.error_null:
    xor rax, rax
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
    