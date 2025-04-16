; Archivo: ej1.asm
; TP2 x86_64 – Implementación en Assembly de las funciones de Ej1

; Definiciones y directivas
%define NULL 0
%define TRUE 1
%define FALSE 0

section .data
; (Si necesitás datos estáticos, agrégalos acá)

section .text

global string_proc_list_create_asm
global string_proc_node_create_asm
global string_proc_list_add_node_asm
global string_proc_list_concat_asm

extern malloc
extern free
extern strdup
extern str_concat

; ---------------------------------------------------------------
; string_proc_list_create_asm:
;   Crea una nueva lista (reserve 16 bytes)
;   Retorna el puntero en RAX.
; ---------------------------------------------------------------
string_proc_list_create_asm:
    ; Argumentos: ninguno
    mov edi, 16             ; tamaño para string_proc_list (2 pointers = 16 bytes)
    call malloc
    test rax, rax
    je .return_null
    ; Inicializar list->first y list->last a NULL
    mov qword [rax], 0      ; list->first = NULL
    mov qword [rax+8], 0    ; list->last  = NULL
    ret
.return_null:
    ret

; ---------------------------------------------------------------
; string_proc_node_create_asm:
;   Parámetros:
;     RDI = type (uint8_t)
;     RSI = hash (char*)
;   Crea un nodo (32 bytes) y asigna:
;     node->next     = 0  (offset 0)
;     node->previous = 0  (offset 8)
;     node->type     = type (offset 16)
;     node->hash     = hash (offset 24)
;   Retorna el puntero al nodo en RAX.
; ---------------------------------------------------------------
string_proc_node_create_asm:
    ; Si se quisiera verificar que hash ≠ NULL, se podría hacerlo; en este ejemplo se asume que el caller lo provee.
    mov edi, 32             ; tamaño para string_proc_node (32 bytes)
    call malloc
    test rax, rax
    je .node_null
    ; Inicializar campos next y previous a NULL.
    mov qword [rax], 0      ; node->next = 0
    mov qword [rax+8], 0    ; node->previous = 0
    ; Guardar el parámetro type (en dil) en node->type (offset 16)
    mov byte [rax+16], dil
    ; Guardar el puntero hash (RSI) en node->hash (offset 24)
    mov qword [rax+24], rsi
    ret
.node_null:
    xor rax, rax
    ret

; ---------------------------------------------------------------
; string_proc_list_add_node_asm:
;   Parámetros:
;     RDI = pointer a string_proc_list
;     RSI = type (uint8_t)
;     RDX = hash (char*)
;   Llama a string_proc_node_create_asm para crear el nuevo nodo y
;   lo agrega al final de la lista:
;     - Si la lista está vacía, setea first y last al nuevo nodo.
;     - Si no, enlaza el nuevo nodo al final.
; ---------------------------------------------------------------
string_proc_list_add_node_asm:
    ; Guardamos el puntero a la lista en R8.
    mov r8, rdi           ; r8 = list pointer

    ; Preparar parámetros para llamar a string_proc_node_create_asm:
    ; Debemos pasar: RDI = type (de nuestro RSI), RSI = hash (de RDX)
    mov rdi, rsi          ; rdi := type
    mov rsi, rdx          ; rsi := hash
    call string_proc_node_create_asm
    test rax, rax
    je .return_add_node   ; Si falla la creación, retornamos.
    ; Guardar el nuevo nodo en R9.
    mov r9, rax           ; r9 = new node pointer

    ; Restaurar el puntero a la lista (está en r8)
    ; Revisar si la lista está vacía: comprobar list->last en [r8+8]
    mov rax, qword [r8+8] ; rax = list->last
    test rax, rax
    je .empty_list

    ; Si la lista NO está vacía:
    ; 1) new_node->previous = list->last
    mov qword [r9+8], rax
    ; 2) list->last->next = new_node  (lista->last se encuentra en rax)
    mov qword [rax], r9
    ; 3) Actualizar list->last = new_node
    mov qword [r8+8], r9
    jmp .return_add_node

.empty_list:
    ; Si la lista está vacía, se setean first y last a new_node.
    mov qword [r8], r9      ; list->first = new_node
    mov qword [r8+8], r9    ; list->last  = new_node

.return_add_node:
    ret

; ---------------------------------------------------------------
; string_proc_list_concat_asm:
;   Parámetros:
;      RDI = pointer a string_proc_list
;      RSI = type (uint8_t)
;      RDX = hash (char*)
;   Función:
;      1. Si list o hash son NULL, retorna NULL.
;      2. Duplicar la cadena "hash" con strdup (guardando el resultado en "result").
;      3. Itera por la lista (list->first) y para cada nodo cuyo campo type sea igual a RSI:
;           a. Llama a str_concat(result, current_node->hash) para concatenar.
;           b. Si falla (retorna NULL), libera result y retorna NULL.
;           c. Libera el string anterior (result) y actualiza result.
;      4. Retorna el puntero result.
; ---------------------------------------------------------------
string_proc_list_concat_asm:
    ; Parámetros: rdi = list, rsi = type, rdx = hash
    ; Verificar si list == NULL o hash == NULL.
    test rdi, rdi
    je .return_concat_null
    test rdx, rdx
    je .return_concat_null

    ; Guardar el puntero a la lista en r11
    mov r11, rdi         ; r11 = list pointer

    ; Llamar a strdup para duplicar 'hash' (usamos rdx)
    mov rdi, rdx         ; parámetro para strdup: hash
    call strdup
    test rax, rax
    je .return_concat_null
    ; Guardar el resultado en r10
    mov r10, rax         ; r10 = result pointer

    ; Obtener el primer nodo: list->first está en [r11]
    mov r8, qword [r11]  ; r8 = current_node_
