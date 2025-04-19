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
; Alloc 16 bytes for a list (two pointers) and zero them.
; ---------------------------------------------------------------
string_proc_list_create_asm:
    mov   edi, 16            ; sizeof(string_proc_list)
    call  malloc
    test  rax, rax
    je    .L_create_null
    mov   qword [rax    ], 0 ; list->first  = NULL
    mov   qword [rax + 8], 0 ; list->last   = NULL
    ret
.L_create_null:
    xor   rax, rax           ; return NULL
    ret

; ---------------------------------------------------------------
; string_proc_node_create_asm:
; Alloc 32 bytes for a node and set its fields:
;   next=0 @offset0, prev=0 @off8, type @off16, hash @off24
; ---------------------------------------------------------------
string_proc_node_create_asm:
    ; save type (rdi) and hash (rsi) across malloc
    push  rdi
    push  rsi

    mov   edi, 32            ; sizeof(string_proc_node)
    call  malloc
    test  rax, rax
    je    .L_node_null

    ; restore type/hash
    pop   rsi                ; rsi = hash
    pop   rdi                ; rdi = type

    mov   byte  [rax +16], dil  ; node->type
    mov   qword [rax +24], rsi  ; node->hash
    mov   qword [rax    ], 0    ; node->next
    mov   qword [rax + 8], 0    ; node->previous
    ret

.L_node_null:
    pop   rsi
    pop   rdi
    xor   rax, rax
    ret

; ---------------------------------------------------------------
; string_proc_list_add_node_asm:
;   RDI = pointer a lista
;   RSI = type (uint8_t)
;   RDX = hash (char*)
; ---------------------------------------------------------------
string_proc_list_add_node_asm:
    test    rdi, rdi             ; if (!list) return;
    je      .Lreturn

    ; Preparamos los parámetros para string_proc_node_create_asm:
    mov     edi, esi             ; edi = (uint32_t)type
    mov     rsi, rdx             ; rsi = hash pointer
    call    string_proc_node_create_asm
    test    rax, rax             ; if (new_node == NULL) return;
    je      .Lreturn

    mov     r9, rax              ; r9 = new_node

    ; Si la lista está vacía (last == NULL)...
    mov     rax, [rdi + 8]       ; rax = list->last
    test    rax, rax
    je      .Lempty

    ; ...sino enlazamos al final:
    mov     [r9 + 8], rax        ; new_node->previous = old_last
    mov     [rax    ], r9        ; old_last->next   = new_node
    mov     [rdi + 8], r9        ; list->last       = new_node
    jmp     .Lreturn

.Lempty:
    mov     [rdi    ], r9        ; list->first = new_node
    mov     [rdi + 8], r9        ; list->last  = new_node

.Lreturn:
    ret


; ---------------------------------------------------------------
; string_proc_list_concat_asm:
; RDI=list, RSI=type, RDX=prefix
; Returns concatenated string or NULL on error.
; ---------------------------------------------------------------
string_proc_list_concat_asm:
    ; preserve callee‑saved
    push  rbx
    push  r12
    push  r13
    push  r14
    push  r15

    test  rdi, rdi           ; list != NULL ?
    je    .L_concat_null
    test  rdx, rdx           ; prefix != NULL ?
    je    .L_concat_null

    mov   r14, rdi           ; r14 = list
    mov   r12b, sil          ; r12b = type

    ; result = strdup(prefix)
    mov   rdi, rdx
    call  strdup
    test  rax, rax
    je    .L_concat_null
    mov   r15, rax           ; r15 = current result

    ; iterate nodes
    mov   r13, [r14]         ; r13 = list->first
.L_loop:
    test  r13, r13
    je    .L_done

    mov   al, [r13 +16]      ; node->type
    cmp   al, r12b
    jne   .L_next

    ; tmp = str_concat(result, node->hash)
    mov   rdi, r15
    mov   rsi, [r13 +24]
    call  str_concat
    test  rax, rax
    je    .L_cleanup

    mov   rbx, rax           ; rbx = new_result
    mov   rdi, r15
    call  free               ; free(old result)
    mov   r15, rbx           ; r15 = new_result

.L_next:
    mov   r13, [r13]         ; current = current->next
    jmp   .L_loop

.L_done:
    mov   rax, r15
    jmp   .L_return

.L_cleanup:
    mov   rdi, r15
    call  free
.L_concat_null:
    xor   rax, rax

.L_return:
    ; restore callee‑saved
    pop   r15
    pop   r14
    pop   r13
    pop   r12
    pop   rbx
    ret
