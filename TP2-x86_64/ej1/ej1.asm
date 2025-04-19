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
;   RDI = string_proc_list*  (lista)
;   RSI = uint8_t            (type)
;   RDX = char*              (prefijo)
; ---------------------------------------------------------------
string_proc_list_concat_asm:
    ; — Prologue completo —
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

    mov     rbx, rdi       ; rbx = lista
    mov     r12b, sil      ; r12b = type
    mov     r13, rdx       ; r13 = prefijo

    ; result = str_concat("", prefijo)
    lea     rdi, [rel empty_string]
    mov     rsi, r13
    call    str_concat
    test    rax, rax
    je      .ret_null
    mov     r14, rax       ; r14 = result acumulado

    ; puntero al primer nodo
    mov     r15, [rbx]     ; r15 = lista->first

.loop_nodes:
    test    r15, r15
    jz      .done

    ; compara r15->type con r12b
    movzx   eax, byte [r15+16]
    cmp     al, r12b
    jne     .next_node

    ; tmp = str_concat(result, r15->hash)
    mov     rdi, r14
    mov     rsi, [r15+24]
    call    str_concat
    test    rax, rax
    je      .ret_null

    ; libera el antiguo result
    mov     rdx, r14
    mov     r14, rax       ; r14 = nuevo result
    mov     rdi, rdx
    call    free

.next_node:
    mov     r15, [r15]     ; avanzar al siguiente nodo
    jmp     .loop_nodes

.done:
    mov     rax, r14       ; devolver result
    jmp     .epilogue

.ret_null:
    xor     rax, rax       ; devuelve NULL

.epilogue:
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    mov     rsp, rbp
    pop     rbp
    ret