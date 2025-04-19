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
; Allocates 16 bytes for a string_proc_list and zeroes its fields.
; ---------------------------------------------------------------
string_proc_list_create_asm:
    mov   edi, 16            ; sizeof(string_proc_list)
    call  malloc
    test  rax, rax
    je    .Lcreate_null
    mov   qword [rax    ], 0 ; list->first = NULL
    mov   qword [rax + 8], 0 ; list->last  = NULL
    ret

.Lcreate_null:
    xor   rax, rax           ; return NULL
    ret

; ---------------------------------------------------------------
; string_proc_node_create_asm:
; Allocates 32 bytes for a string_proc_node, initializes fields.
;   next @0, prev @8, type @16, hash @24
; ---------------------------------------------------------------
string_proc_node_create_asm:
    push  rdi                ; save type
    push  rsi                ; save hash ptr

    mov   edi, 32            ; sizeof(string_proc_node)
    call  malloc
    test  rax, rax
    je    .Lnode_null

    pop   rsi                ; restore hash ptr
    pop   rdi                ; restore type
    mov   byte  [rax +16], dil  ; node->type
    mov   qword [rax +24], rsi  ; node->hash
    mov   qword [rax    ], 0    ; node->next
    mov   qword [rax + 8], 0    ; node->previous
    ret

.Lnode_null:
    pop   rsi
    pop   rdi
    xor   rax, rax           ; return NULL
    ret

; ---------------------------------------------------------------
; string_proc_list_add_node_asm:
;   RDI = list ptr
;   RSI = type (uint8_t)
;   RDX = hash ptr
; Adds a new node to the end of the list.
; ---------------------------------------------------------------
string_proc_list_add_node_asm:
    test  rdi, rdi
    je    .Ladd_return

    ; prepare call to string_proc_node_create_asm(type, hash)
    mov   edi, esi           ; edi = type
    mov   rsi, rdx           ; rsi = hash ptr
    call  string_proc_node_create_asm
    test  rax, rax
    je    .Ladd_return

    mov   r9, rax            ; r9 = new_node
    mov   rax, [rdi + 8]     ; rax = list->last
    test  rax, rax
    jne   .Ladd_link

    ; list was empty
    mov   [rdi    ], r9      ; list->first = new_node
    mov   [rdi + 8], r9      ; list->last  = new_node
    jmp   .Ladd_return

.Ladd_link:
    mov   [r9 + 8], rax      ; new_node->previous = old last
    mov   [rax    ], r9      ; old last->next      = new_node
    mov   [rdi + 8], r9      ; list->last          = new_node

.Ladd_return:
    ret

; ---------------------------------------------------------------
; string_proc_list_concat_asm:
;   RDI = list ptr
;   RSI = type (uint8_t)
;   RDX = prefix string ptr
; Returns a newly malloc’ed concatenation of prefix + node->hash for each matching type.
; ---------------------------------------------------------------
string_proc_list_concat_asm:
    push  rbx
    push  r12
    push  r13
    push  r14
    push  r15

    test  rdi, rdi           ; list != NULL?
    je    .Lconcat_null
    test  rdx, rdx           ; prefix != NULL?
    je    .Lconcat_null

    mov   r14, rdi           ; r14 = list ptr
    mov   r12b, sil          ; r12b = type

    ; result = strdup(prefix)
    mov   rdi, rdx
    call  strdup
    test  rax, rax
    je    .Lconcat_null
    mov   r15, rax           ; r15 = result

    ; iterate through nodes
    mov   r13, [r14]         ; r13 = list->first
.Lconcat_loop:
    test  r13, r13
    je    .Lconcat_done

    mov   al, [r13 + 16]     ; node->type
    cmp   al, r12b
    jne   .Lconcat_next

    ; tmp = str_concat(result, node->hash)
    mov   rdi, r15
    mov   rsi, [r13 + 24]
    call  str_concat
    test  rax, rax
    je    .Lconcat_fail

    mov   rbx, rax           ; rbx = new_result
    mov   rdi, r15
    call  free               ; free(old result)
    mov   r15, rbx           ; r15 = new_result

.Lconcat_next:
    mov   r13, [r13]         ; current = current->next
    jmp   .Lconcat_loop

.Lconcat_done:
    mov   rax, r15
    jmp   .Lconcat_return

.Lconcat_fail:
    mov   rdi, r15
    call  free
    xor   rax, rax

.Lconcat_return:
    pop   r15
    pop   r14
    pop   r13
    pop   r12
    pop   rbx
    ret

.Lconcat_null:
    xor   rax, rax
    pop   r15
    pop   r14
    pop   r13
    pop   r12
    pop   rbx
    ret
