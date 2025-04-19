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
; Versión “alternativa” de string_proc_list_concat_asm
; hace lo mismo que tu original, pero con cambios de registro,
; etiquetas y prologue/epilogue.
; Parámetros:
;   RDI = lista (string_proc_list*)
;   RSI = type (uint8_t)
;   RDX = prefijo (char*)
; ---------------------------------------------------------------
string_proc_list_concat_asm:
    ; — Prologue clásico con frame pointer —
    push    rbp
    mov     rbp, rsp
    push    r12
    push    r13
    push    r14

    ; Guarda parámetros en registros “nuevos”
    mov     r13, rdi        ; r13 = lista
    mov     r12b, sil       ; r12b = type
    mov     r14, rdx        ; r14 = prefijo

    ; Duplica empty_string+prefijo para arrancar result
    lea     rdi, [rel empty_string]  ; rdi = &empty_string
    mov     rsi, r14                  ; rsi = prefijo
    call    str_concat
    test    rax, rax
    je      .cleanup_rtn
    mov     rbx, rax                  ; rbx = result acumulado

    ; Ahora recorremos la lista nodo a nodo
    mov     rcx, [r13]      ; rcx = lista->first (primer nodo)
.alt_loop:
    cmp     rcx, 0
    je      .alt_return

    movzx   eax, byte [rcx+16]   ; eax = current_node->type
    cmp     al, r12b
    jne     .alt_next

    ; Concatenar hash: result = str_concat(result, node->hash)
    mov     rdi, rbx
    mov     rsi, [rcx+24]        ; rsi = current_node->hash
    call    str_concat
    test    rax, rax
    je      .cleanup_rtn

    ; Si tuvo éxito, liberar el antiguo result
    mov     rdx, rbx
    mov     rbx, rax              ; rbx = nuevo result
    mov     rdi, rdx
    call    free

.alt_next:
    mov     rcx, [rcx]   ; next node
    jmp     .alt_loop

.alt_return:
    mov     rax, rbx      ; devolver result

    ; — Epilogue limpio —
    pop     r14
    pop     r13
    pop     r12
    mov     rsp, rbp
    pop     rbp
    ret

.cleanup_rtn:
    ; en caso de error liberamos y devolvemos NULL
    mov     rdi, rbx
    call    free
    xor     rax, rax
    pop     r14
    pop     r13
    pop     r12
    mov     rsp, rbp
    pop     rbp
    ret