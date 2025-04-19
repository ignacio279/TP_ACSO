%define NULL 0

section .data
empty_string: db 0

section .text

global string_proc_list_create_asm
global string_proc_node_create_asm
global string_proc_list_add_node_asm
global string_proc_list_concat_asm

extern malloc
extern free
extern str_concat


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
    push    rbx
    push    r12            

    mov     bl, dil        
    mov     r12, rsi        

    mov     edi, 32         
    call    malloc
    test    rax, rax
    jz      .fail

    mov     byte  [rax + 16], bl         
    mov     qword [rax + 24], r12        
    mov     qword [rax], 0              
    mov     qword [rax + 8], 0           

    pop     r12
    pop     rbx
    ret

.fail:
    pop     r12
    xor     rax, rax
    pop     rbx
    ret

string_proc_list_add_node_asm:
    push    rbx
    mov     rbx, rdi           
    mov     dil, sil
    mov     rsi, rdx
    call    string_proc_node_create_asm
    test    rax, rax
    jz      .fin
    mov     rcx, [rbx]          
    test    rcx, rcx
    jnz     .not_empty

    ; lista vacía
    mov     [rbx], rax
    mov     [rbx + 8], rax
    jmp     .fin

.not_empty:
    mov     rdx, [rbx + 8]      
    mov     [rdx], rax          
    mov     [rax + 8], rdx      
    mov     [rbx + 8], rax      

.fin:
    pop     rbx
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