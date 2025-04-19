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

string_proc_list_concat_asm:
    push    rbx
    push    r12
    mov     rbx, rdi            
    mov     r12b, sil           
    mov     r13, rdx           

    mov     rdi, empty_string
    mov     rsi, r13
    call    str_concat
    mov     r14, rax            

    mov     r15, [rbx]          

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