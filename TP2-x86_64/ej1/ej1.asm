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
string_proc_node_create_asm:
    mov edi, 32         ; sizeof(string_proc_node) = 32 bytes
    call malloc           
    test rax, rax           
    je .node_null
    mov qword [rax], 0      ; node->next = 0
    mov qword [rax+8], 0    ; node->previous = 0
    mov byte [rax+16], dil  ; node->type = type (de dil)
    mov qword [rax+24], rsi ; node->hash = hash (en rsi)
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
    test rdi, rdi
    je .return_add_node      

    mov r8, rdi              ; r8 = pointer a list

    ; Preparar parámetros para crear el nodo:
    ; Pasar RSI = type, RDX = hash. Llamamos a string_proc_node_create_asm.
    mov rdi, rsi            ; rdi = type
    mov rsi, rdx            ; rsi = hash
    call string_proc_node_create_asm
    test rax, rax            
    je .return_add_node
    mov r9, rax             ; r9 = new node

    ; Revisar si la lista no está vacía: list->last se encuentra en [r8+8].
    mov rax, qword [r8+8]    
    test rax, rax
    je .empty_list           

    ; Si la lista no está vacía:
    mov qword [r9+8], rax   ; new_node->previous = list->last
    mov qword [rax], r9     ; list->last->next = new_node
    mov qword [r8+8], r9    ; list->last = new_node
    jmp .return_add_node_done

.empty_list:
    mov qword [r8], r9      ; list->first = new_node
    mov qword [r8+8], r9    ; list->last = new_node

.return_add_node_done:
.return_add_node:
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