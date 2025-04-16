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
; Reserva 16 bytes para una lista (dos punteros) e inicializa ambos a NULL.
; ---------------------------------------------------------------
string_proc_list_create_asm:
    mov edi, 16         ; sizeof(string_proc_list) = 16 bytes (dos pointers de 8 bytes)
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
;   next      = NULL (offset 0)
;   previous  = NULL (offset 8)
;   type      = valor original (offset 16, 1 byte)
;   hash      = puntero recibido (offset 24)
; Para preservar el valor original de 'type' (pasado en RDI) se utiliza RBX.
; ---------------------------------------------------------------
string_proc_node_create_asm:
    push rbx               ; guardar RBX, que usaremos para conservar "type"
    mov rbx, rdi           ; guardar parámetro original "type" en RBX
    mov edi, 32            ; solicitar 32 bytes para el nodo
    call malloc           
    test rax, rax           
    je .node_null_push
    mov qword [rax], 0      ; node->next = NULL
    mov qword [rax+8], 0    ; node->previous = NULL
    mov byte [rax+16], bl  ; node->type = valor original (RBX's low 8 bits)
    mov qword [rax+24], rsi ; node->hash = valor de hash (en RSI)
    pop rbx                ; restaurar RBX
    ret
.node_null_push:
    pop rbx
    xor rax, rax    
    ret

; ---------------------------------------------------------------
; string_proc_list_add_node_asm:
; Parámetros:
;   RDI = pointer a string_proc_list
;   RSI = type (uint8_t)
;   RDX = hash (char*)
; Crea un nodo (llamando a string_proc_node_create_asm) y lo enlaza a la lista.
; Si la lista está vacía, asigna tanto first como last al nuevo nodo.
; Si ya tiene nodos, lo enlaza al final.
; ---------------------------------------------------------------
string_proc_list_add_node_asm:
    test rdi, rdi
    je .return_add_node      
    mov r8, rdi              ; r8 = pointer a list
    ; Preparar parámetros para crear el nodo.
    ; El parámetro "type" (en RSI) y "hash" (en RDX) se pasan a string_proc_node_create_asm.
    mov rdi, rsi            ; rdi = type
    mov rsi, rdx            ; rsi = hash
    call string_proc_node_create_asm
    test rax, rax            
    je .return_add_node
    mov r9, rax             ; r9 = new node
    ; Revisar si la lista no está vacía: list->last está en [r8+8].
    mov rax, qword [r8+8]    
    test rax, rax
    je .empty_list           
    ; Si no está vacía:
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
;   - Guarda el parámetro "type" en R12 (registro no volátil).
;   - Duplica la cadena "hash" usando strdup; el resultado se guarda en R10.
;   - Copia list->first (offset 0) en R13 (para iterar, registro callee-saved).
;   - Itera por la lista: para cada nodo, si el byte en [r13+16] coincide con r12b,
;     llama a str_concat(result, node->hash), libera el viejo result y actualiza R10.
;   - Al finalizar, retorna el puntero acumulado en RAX.
; Se preservan los registros no volátiles: RBX, R12, R13 y R14.
; ---------------------------------------------------------------
string_proc_list_concat_asm:
    push rbx              ; Preservar RBX
    push r12              ; Preservar R12
    push r13              ; Preservar R13
    push r14              ; Preservar R14

    ; Verificar que list y hash no sean NULL.
    test rdi, rdi
    je .return_concat_null_preserve
    test rdx, rdx
    je .return_concat_null_preserve

    mov r14, rdi          ; r14 = pointer a list
    mov r12, rsi          ; r12 = type (valor original)
    ; Llamar a strdup(hash) con hash en RDX.
    mov rdi, rdx          ; parámetro: hash
    call strdup
    test rax, rax
    je .return_concat_null_preserve
    mov r10, rax          ; r10 = result (cadena acumulada)

    ; Cargar el primer nodo: list->first se encuentra en [r14].
    mov r13, qword [r14]  ; r13 = current_node

.concat_loop:
    test r13, r13        ; Si current_node es NULL, terminamos
    je .end_concat_loop
    ; Comparar current_node->type (byte en [r13+16]) con el valor original en r12b.
    mov al, byte [r13+16]
    cmp al, r12b
    jne .skip_concat
    ; Llamar a str_concat(result, current_node->hash)
    mov rdi, r10              ; primer parámetro: result
    mov rsi, qword [r13+24]     ; segundo parámetro: current_node->hash (offset 24)
    call str_concat
    test rax, rax
    je .concat_fail         ; Si falla, liberar result y retornar NULL
    mov rbx, rax            ; Guardar nuevo result en RBX
    mov rdi, r10
    call free               ; Liberar el viejo result
    mov r10, rbx            ; Actualizar result
.skip_concat:
    mov r13, qword [r13]    ; current_node = current_node->next (offset 0)
    jmp .concat_loop
.end_concat_loop:
    mov rax, r10            ; Colocar result en RAX para retorno
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
.concat_fail:
    mov rdi, r10
    call free
.return_concat_null_preserve:
    xor rax, rax           ; Retornar NULL (0)
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
