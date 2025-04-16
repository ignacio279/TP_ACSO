; ---------------------------------------------------------------
; string_proc_list_concat_asm:
;   Parámetros:
;      RDI = pointer a string_proc_list
;      RSI = type (uint8_t)
;      RDX = hash (char*)
;   Función:
;      1. Si list o hash son NULL, retorna NULL.
;      2. Duplica la cadena 'hash' con strdup y la guarda en "result" (r10).
;      3. Itera por la lista (comenzando en list->first, que está en [r11])
;         y para cada nodo cuyo campo type sea igual al parámetro (RSI):
;           a. Llama a str_concat(result, current_node->hash).
;           b. Si falla (retorna NULL), libera result y retorna NULL.
;           c. Libera el string anterior (result) y actualiza result.
;      4. Retorna result en RAX.
; ---------------------------------------------------------------
string_proc_list_concat_asm:
    ; Verificar si list o hash son NULL.
    test rdi, rdi
    je .return_concat_null
    test rdx, rdx
    je .return_concat_null

    ; Guardar el puntero a la lista en r11.
    mov r11, rdi         ; r11 = list pointer

    ; Llamar a strdup para duplicar 'hash' (se encuentra en rdx).
    mov rdi, rdx         ; parámetro para strdup: hash
    call strdup
    test rax, rax
    je .return_concat_null
    ; Guardar el resultado en r10: r10 = result pointer.
    mov r10, rax

    ; Obtener el primer nodo: list->first está en [r11].
    mov r8, qword [r11]  ; r8 = current_node

.concat_loop:
    test r8, r8          ; mientras current_node != NULL
    je .end_concat_loop

    ; Comparar current_node->type (byte en [r8+16]) con el tipo (lower 8 bits de RSI, es SIL).
    mov al, byte [r8+16]
    cmp al, sil
    jne .skip_concat

    ; Si los tipos coinciden, preparar llamada a str_concat:
    ; Primer parámetro: result (r10), Segundo: current_node->hash (offset 24).
    mov rdi, r10
    mov rsi, qword [r8+24]
    call str_concat
    test rax, rax
    je .concat_fail    ; Si falla, liberar result y retornar NULL.
    ; Liberar el string anterior.
    mov rdi, r10
    call free
    ; Actualizar result.
    mov r10, rax

.skip_concat:
    ; Avanzar al siguiente nodo: current_node = current_node->next (offset 0).
    mov r8, qword [r8]
    jmp .concat_loop

.end_concat_loop:
    ; Retornar result en RAX.
    mov rax, r10
    ret

.concat_fail:
    ; En caso de error, liberar result y saltar a retornar NULL.
    mov rdi, r10
    call free
.return_concat_null:
    xor rax, rax    ; Retornar NULL (r0).
    ret
