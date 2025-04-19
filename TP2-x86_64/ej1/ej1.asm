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
    ; Reservar memoria para la lista (tamaño de string_proc_list)
    mov edi, 16                 ; Tamaño de string_proc_list (2 punteros)
    call malloc
    test rax, rax              ; Comprobar si malloc devolvió NULL
    je .return_null             ; Si es NULL, saltar a .return_null

    ; Inicializar la lista
    mov qword [rax], 0          ; list->first = NULL
    mov qword [rax+8], 0        ; list->last = NULL

    ret                         ; Retornar la dirección de la lista

.return_null:
    xor rax, rax               ; Retornar NULL (0)
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
    ; Reservar memoria para el nodo (tamaño de string_proc_node)
    mov edi, 32                 ; Tamaño de string_proc_node (4 punteros + 1 byte)
    call malloc
    test rax, rax              ; Comprobar si malloc devolvió NULL
    je .return_null             ; Si es NULL, saltar a .return_null

    ; Inicializar el nodo
    mov byte [rax+16], dil      ; node->type = type (en dil)
    mov qword [rax+24], rsi     ; node->hash = hash (en rsi)
    mov qword [rax], 0          ; node->next = NULL
    mov qword [rax+8], 0        ; node->previous = NULL

    ret                         ; Retornar la dirección del nodo

.return_null:
    xor rax, rax               ; Retornar NULL (0)
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
    ; Verificar si la lista es NULL
    test rdi, rdi            ; Comprobar si list es NULL
    je  .return              ; Si es NULL, retornar

    ; Llamar a string_proc_node_create_asm para crear un nuevo nodo
    mov rsi, dl              ; type en rsi
    mov rdx, r8              ; hash en rdx
    call string_proc_node_create_asm
    test rax, rax            ; Comprobar si node es NULL
    je  .return              ; Si node es NULL, retornar

    ; Guardamos el puntero al nuevo nodo en r9
    mov r9, rax

    ; Verificar si list->last es NULL (es decir, si la lista está vacía)
    mov rax, qword [rdi+8]   ; Cargar list->last en rax
    test rax, rax            ; Comprobar si list->last es NULL
    je  .empty_list          ; Si es NULL, ir a .empty_list

    ; Si la lista no está vacía, enlazar el nuevo nodo al final
    mov qword [r9+8], rax    ; new_node->previous = list->last
    mov qword [rax], r9      ; list->last->next = new_node
    mov qword [rdi+8], r9    ; list->last = new_node
    jmp .return              ; Finalizar

.empty_list:
    ; Si la lista está vacía, asignar el primer y último nodo a new_node
    mov qword [rdi], r9      ; list->first = new_node
    mov qword [rdi+8], r9    ; list->last = new_node

.return:
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
    ; Verificar si list o hash son NULL
    test rdi, rdi            ; Comprobar si list es NULL
    je  .return_null         ; Si es NULL, retornar NULL
    test rdx, rdx            ; Comprobar si hash es NULL
    je  .return_null         ; Si es NULL, retornar NULL

    ; Llamar a strdup(hash) para crear una copia de hash
    mov rdi, rdx             ; primer parámetro: hash
    call strdup
    test rax, rax            ; Comprobar si strdup falló
    je  .return_null         ; Si strdup falló, retornar NULL
    mov r10, rax             ; r10 = result

    ; Iterar por los nodos de la lista
    mov r13, qword [rdi]     ; r13 = list->first
.loop_start:
    test r13, r13            ; Comprobar si current_node es NULL
    je  .end_loop            ; Si es NULL, terminamos

    mov al, byte [r13+16]    ; r13->type
    cmp al, sil              ; Comparar type de nodo con type (en sil)
    jne .skip_concat         ; Si no son iguales, saltamos

    ; Concatenar result con current_node->hash
    mov rdi, r10             ; primer parámetro: result
    mov rsi, qword [r13+24]  ; segundo parámetro: current_node->hash
    call str_concat
    test rax, rax            ; Comprobar si str_concat falló
    je  .concat_fail         ; Si falla, liberar result y retornar NULL

    call free               ; Liberar el viejo result
    mov r10, rax             ; Actualizar result con el nuevo puntero

.skip_concat:
    mov r13, qword [r13]     ; Avanzar al siguiente nodo
    jmp .loop_start          ; Continuar con el siguiente nodo

.end_loop:
    mov rax, r10             ; Devolver result

    ret

.concat_fail:
    call free               ; Liberar result si str_concat falló
    xor rax, rax             ; Retornar NULL (0)
    ret

.return_null:
    xor rax, rax             ; Retornar NULL (0)
    ret
