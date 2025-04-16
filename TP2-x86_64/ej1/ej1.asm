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

string_proc_node_create_asm:
    mov edi, 32            
    call malloc           
    test rax, rax           
    je .node_null
    mov qword [rax], 0
    mov qword [rax+8], 0
    mov byte [rax+16], dil
    mov qword [rax+24], rsi
    ret
.node_null:
    xor rax, rax    
    ret

string_proc_list_add_node_asm:
    test rdi, rdi
    je .return_add_node      

    mov r8, rdi              

    mov rdi, rsi            
    mov rsi, rdx            
    call string_proc_node_create_asm
    test rax, rax            
    je .return_add_node
    mov r9, rax              

    mov rax, qword [r8+8]    
    test rax, rax
    je .empty_list           

    mov qword [r9+8], rax
    mov qword [rax], r9
    mov qword [r8+8], r9
    jmp .return_add_node_done

.empty_list:
    mov qword [r8], r9       
    mov qword [r8+8], r9     

.return_add_node_done:
.return_add_node:
    ret

string_proc_list_concat_asm:
    push rbx              ; Preservamos RBX (callee-saved)
    push r12              ; Preservamos R12 (callee-saved)
    ; Verificar que list y hash no sean NULL.
    test rdi, rdi
    je .return_concat_null
    test rdx, rdx
    je .return_concat_null

    ; r11 = list pointer
    mov r11, rdi

    ; Guardar el parámetro "type" (que viene en RSI) en R12.
    mov r12, rsi

    ; Llamar a strdup(hash)
    mov rdi, rdx         ; parámetro: hash (en RDX)
    call strdup
    test rax, rax
    je .return_concat_null
    mov r10, rax         ; r10 = result (cadena acumulada)

    ; Cargar el primer nodo: list->first (offset 0)
    mov r8, qword [r11]  ; r8 = current_node

.concat_loop:
    test r8, r8          ; Si current_node es NULL, terminamos
    je .end_concat_loop

    ; Comparar current_node->type (byte en [r8+16]) con el valor original de "type" (en r12b)
    mov al, byte [r8+16]
    cmp al, r12b
    jne .skip_concat

    ; Llamar a str_concat(result, current_node->hash)
    mov rdi, r10              ; primer parámetro: result
    mov rsi, qword [r8+24]     ; segundo parámetro: current_node->hash (offset 24)
    call str_concat
    test rax, rax
    je .concat_fail         ; Si falla, liberar result y retornar NULL

    ; Guardar el nuevo puntero en RBX antes de llamar a free.
    mov rbx, rax            ; rbx = nuevo result
    mov rdi, r10
    call free               ; Liberar el antiguo result
    mov r10, rbx            ; Actualizar result

.skip_concat:
    ; Avanzar al siguiente nodo (current_node->next, offset 0)
    mov r8, qword [r8]
    jmp .concat_loop

.end_concat_loop:
    mov rax, r10            ; Retornar result en RAX
    pop r12
    pop rbx
    ret

.concat_fail:
    mov rdi, r10
    call free
.return_concat_null:
    xor rax, rax           ; Retornar NULL (0)
    pop r12
    pop rbx
    ret
