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
    test rdi, rdi
    je .return_concat_null
    test rdx, rdx
    je .return_concat_null

    mov r11, rdi         

    mov rdi, rdx        
    call strdup
    test rax, rax
    je .return_concat_null
    mov r10, rax         

    mov r8, qword [r11]  

.concat_loop:
    test r8, r8          
    je .end_concat_loop

    mov al, byte [r8+16]
    cmp al, sil
    jne .skip_concat

    mov rdi, r10
    mov rsi, qword [r8+24]
    call str_concat

    test rax, rax
    je .concat_fail    

    mov rdi, r10
    call free
    mov r10, rax       

.skip_concat:
    mov r8, qword [r8]
    jmp .concat_loop

.end_concat_loop:
    mov rax, r10
    ret

.concat_fail:
    mov rdi, r10
    call free
.return_concat_null:
    xor rax, rax      
    ret
