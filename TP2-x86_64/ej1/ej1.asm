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
    ;––– Guardamos type y hash en la pila –––
    push  rdi        ; rdi = type
    push  rsi        ; rsi = hash pointer

    ;––– Llamamos a malloc(32) –––
    mov   edi, 32    ; tamaño del nodo
    call  malloc
    test  rax, rax
    je    .node_null

    ;––– Recuperamos type y hash –––
    pop   rsi        ; rsi = hash
    pop   rdi        ; rdi = type

    ;––– Inicializamos campos del nodo –––
    mov   qword [rax  ], 0       ; next = NULL
    mov   qword [rax+ 8], 0      ; previous = NULL
    mov   byte  [rax+16], dil    ; type = (uint8_t)rdi
    mov   qword [rax+24], rsi    ; hash = pointer

    ret

.node_null:
    ;––– malloc falló: sacamos los parámetros y retornamos NULL –––
    pop   rsi
    pop   rdi
    xor   rax, rax
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
; ---------------------------------------------------------------
; string_proc_list_concat_asm:
;   RDI = lista, RSI = type, RDX = prefijo (hash inicial)
; ---------------------------------------------------------------
string_proc_list_concat_asm:
    ;––– Guardar los callee‑saved que vamos a usar –––
    push  rbx
    push  r12
    push  r13
    push  r14
    push  r15

    ; Verificar nulls
    test  rdi, rdi
    je    .return_concat_null_preserve
    test  rdx, rdx
    je    .return_concat_null_preserve

    ; Preparar iteración
    mov   r14, rdi         ; r14 = lista
    mov   r12, rsi         ; r12 = type

    ; Prefijo: strdup(hash)
    mov   rdi, rdx         ; primer parámetro: prefijo
    call  strdup
    test  rax, rax
    je    .return_concat_null_preserve
    mov   r15, rax         ; r15 = result (registro callee‑saved)

    ; Iterar por la lista
    mov   r13, qword [r14] ; r13 = primer nodo
.concat_loop:
    test  r13, r13
    je    .end_concat_loop

    mov   al, byte [r13+16] ; current_node->type
    cmp   al, r12b
    jne   .skip_concat

    ; str_concat(result, current_node->hash)
    mov   rdi, r15
    mov   rsi, qword [r13+24]
    call  str_concat
    test  rax, rax
    je    .concat_fail

    ; free(old_result) y actualizar
    mov   rbx, rax         ; rbx = nuevo result
    mov   rdi, r15
    call  free
    mov   r15, rbx         ; r15 = result actualizado

.skip_concat:
    mov   r13, qword [r13] ; next node
    jmp   .concat_loop

.end_concat_loop:
    mov   rax, r15         ; devolver result

    ;––– Restaurar callee‑saved –––
    pop   r15
    pop   r14
    pop   r13
    pop   r12
    pop   rbx
    ret

.concat_fail:
    mov   rdi, r15
    call  free             ; liberar prefijo o acumulado parcial
.return_concat_null_preserve:
    xor   rax, rax
    pop   r15
    pop   r14
    pop   r13
    pop   r12
    pop   rbx
    ret
