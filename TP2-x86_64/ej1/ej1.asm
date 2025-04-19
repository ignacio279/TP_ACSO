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



; ---------------------------------------------------------------
; string_proc_node_create_asm (versión “disfrazada”)
;   RDI = uint8_t type  (dil)
;   RSI = char*   hash  (rsi)
; ---------------------------------------------------------------
string_proc_node_create_asm:
    push    rbx
    push    r12
    push    r13

    ;--- guardar parámetros en regs únicos ---
    mov     r12b, dil        ; r12b = type
    mov     r13, rsi         ; r13  = ptr a hash original

    ;--- duplicar el hash con strdup ---
    mov     rdi, r13         ; RDI = hash
    call    strdup
    test    rax, rax
    je      .fail            ; si falla strdup, saltar a fail
    mov     r13, rax         ; r13 = ptr al hash duplicado

    ;--- reservar memoria para el nodo ---
    mov     edi, 32          ; tamaño de string_proc_node
    call    malloc
    test    rax, rax
    je      .free_hash       ; si malloc falla, liberar el hash y salir

    ;--- inicializar campos del nodo ---
    xor     rbx, rbx
    mov     [rax+0],  rbx    ; node->next     = NULL
    mov     [rax+8],  rbx    ; node->previous = NULL
    mov     byte [rax+16], r12b  ; node->type  = type
    mov     [rax+24], r13    ; node->hash     = hash duplicado

    jmp     .done

.free_hash:
    mov     rdi, r13
    call    free             ; liberar el duplicado si malloc falló

.fail:
    xor     rax, rax         ; retornar NULL

.done:
    pop     r13
    pop     r12
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

string_proc_list_concat_asm:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15

    ; Validar lista
    test    rdi, rdi
    je      .ret_null

    mov     rbx, rdi      
    mov     r12b, sil      
    mov     r13, rdx     

    ; result = str_concat("", prefijo)
    lea     rdi, [rel empty_string]
    mov     rsi, r13
    call    str_concat
    test    rax, rax
    je      .ret_null
    mov     r14, rax       

    ; puntero al primer nodo
    mov     r15, [rbx]     

.loop_nodes:
    test    r15, r15
    jz      .done

    movzx   eax, byte [r15+16]
    cmp     al, r12b
    jne     .next_node

    mov     rdi, r14
    mov     rsi, [r15+24]
    call    str_concat
    test    rax, rax
    je      .ret_null

    ; libera el antiguo result
    mov     rdx, r14
    mov     r14, rax       
    mov     rdi, rdx
    call    free

.next_node:
    mov     r15, [r15]     
    jmp     .loop_nodes

.done:
    mov     rax, r14      
    jmp     .epilogue

.ret_null:
    xor     rax, rax     

.epilogue:
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    mov     rsp, rbp
    pop     rbp
    ret