bits 64
%include "xLogger_x64.inc"
INIT:
    gs mov  rax, [TEB_PPEB_OFFSET]
    mov     rax, [rax + PEB_PLDR_OFFSET]
    mov     rax, [rax + LDR_PIN_ORDER_MOD_LIST_OFFSET]
    mov     r10, rax
FIND_CALLING_MODULE:
    mov     r11, [rax + LDR_MODULE_BASE_OFFSET]
    cmp     qword [rsp + 10h], r11
    jl      NEXT_MODULE
    mov     r11d, [rax + LDR_SIZE_OF_IMAGE_OFFSET]
    add     r11, [rax + LDR_MODULE_BASE_OFFSET]
    cmp     qword [rsp + 10h], r11
    jge     NEXT_MODULE
    jmp     CHECK_CALLING_MODULE
NEXT_MODULE:
    mov     rax, [rax]
    cmp     rax, r10
    jne     FIND_CALLING_MODULE
CHECK_CALLING_MODULE:
    cmp     rax, r10
    je      LOG_CALL
    lea     r10, [rel EXTERNAL_MODS_LIST]
    movzx   r11, word [r10]
    test    r11, r11
    jz      DO_NOT_LOG_CALL
    sub     rsp, 28h
    mov     qword [rsp], rcx
    mov     qword [rsp + 8h], rdx
    mov     qword [rsp + 10h], r9
    mov     qword [rsp + 18h], r8
    mov     qword [rsp + 20h], rax
    mov     rax, [rax + LDR_MODULE_NAME_OFFSET + 8h]
    xor     rcx, rcx
    add     r10, 2
EXTERNAL_MODS_LOOP:
    mov     cl, byte [r10]
    inc     r10
    dec     r11
    xor     rdx, rdx
MODULE_CHECK_LOOP:
    mov     r9b, [rax + rdx * 2h]
    cmp     r9b, 'Z'
    jg      LOWER_CASE
    cmp     r9b, 'A'
    jl      LOWER_CASE
    add     r9b, 20h
LOWER_CASE:
    mov     r8b, [r10 + rdx]
    cmp     r8b, r9b
    jne     MODULE_CHECK_LOOP_END
    inc     rdx
    cmp     rdx, rcx
    jne     MODULE_CHECK_LOOP
    mov     rcx, [rsp]
    mov     rdx, [rsp + 8h]
    mov     r9, [rsp + 10h]
    mov     r8, [rsp + 18h]
    mov     rax, [rsp + 20h]
    add     rsp, 28h
    jmp     LOG_CALL
MODULE_CHECK_LOOP_END:
    sub     r11, rcx
    add     r10, rcx
    test    r11, r11
    jnz     EXTERNAL_MODS_LOOP
    mov     rcx, [rsp]
    mov     rdx, [rsp + 8h]
    mov     r9, [rsp + 10h]
    mov     r8, [rsp + 18h]
    add     rsp, 28h
DO_NOT_LOG_CALL:
    ret     8h
LOG_CALL:
    mov     r11, rax
    lea     rax, [rel EXTERNAL_MODS_LIST]
    movzx   r10, word [rax]
    lea     rax, [rax + r10 + 2h]
    jmp     rax 
EXTERNAL_MODS_LIST:
    db      0h, 0h
    lea     rax, [rel RETURN_ADDRESS]
    cmp     rax, [rsp + 10h]
    jne     START_LOGGING
    ret     8h
START_LOGGING:
    push    rsi
    push    rdi
    push    rbx
    push    rbp
    push    r15
    push    r9
    push    r8
    push    rdx
    push    rcx
    mov     rbx, LOG_FILE_HANDLE
    mov     rbp, API_INFO_ADDRESS
    xchg    qword [rsp + 50h], rbp
    add     rbp, [rsp + 50h]
    xor     rdi, rdi
    xor     rdx, rdx
    push    rdi
    push    rdi
    push    rdi
    push    rdi
    lea     rsi, [rel GLOBAL_BUFFER]
SPIN_LOCK:
    pause
    mov     al, 1
    xchg    al, [rsi]
    test    al, al
    jnz     SPIN_LOCK
    add     rsi, 9h
    mov     r10, rsi
    add     r11, LDR_MODULE_NAME_OFFSET
    mov     rax, [r11 + 8h]
    mov     dx, [r11]
    shr     dx, 1h
    mov     rdi, r10
    mov     byte [rdi], '['
    mov     byte [rdi + rdx + 1h], ']'
    mov     byte [rdi + rdx + 2h], ' '
    xor     r11, r11
WRITE_MODULE_NAME:
    cmp     r11, rdx
    je      WRITE_THREAD_ID
    mov     cl, [rax + r11 * 2h]
    mov     byte [rdi + r11 + 1h], cl
    inc     r11
    jmp     WRITE_MODULE_NAME
WRITE_THREAD_ID:
    add     rdx, 3h
    add     rdi, rdx
    mov     qword [r10 - 8h], rdx
    gs mov  rax, qword [TEB_THREADID_OFFSET]
    mov     eax, eax
    call    RAX_TO_HEX
    mov     byte [rdi], '['
    mov     byte [rdi + rdx + 1h], ']'
    mov     byte [rdi + rdx + 2h], ' '
    inc     rdi
    mov     rsi, rcx
    mov     rcx, rdx
    rep     movsb
    add     rdx, 3h
    add     qword [r10 - 8h], rdx
    mov     dl, [rbp]
    inc     rbp
    mov     rcx, rdx
    mov     rdi, r10
    add     rdi, [r10 - 8h]
    mov     rsi, rbp
    rep     movsb
    mov     rsi, r10
    add     qword [rsi - 8h], rdx
    add     rbp, rdx
    lea     rax, [rel IO_STATUS_BLOCK]
    push    rax
    sub     rsp, 20h
    movzx   rdi, byte [rbp]
    inc     rbp
    test    rdi, rdi
    jz      NO_PARAMETERS_LOOP_END
    xor     r15, r15
PARAMETERS_LOOP:
    movzx   rdx, byte [rbp]
    inc     rbp
    mov     rcx, rdx
    mov     r8, rsi
    mov     r9, rdi
    mov     rdi, rsi
    add     rdi, [rsi - 8h]
    mov     rsi, rbp
    rep     movsb
    mov     rsi, r8
    add     qword [rsi - 8h], rdx
    mov     rdi, r9
    add     rbp, rdx
    cmp     r15, 4h
    jl      PARAM_FROM_REG
    mov     rax, [rsp + r15 * 8h + 0a8h]
    jmp     ENCODE_PARAM
PARAM_FROM_REG:
    mov     rax, [rsp + r15 * 8h + 48h]
ENCODE_PARAM:
    movzx   rdx, byte [rbp]
    inc     rbp
    cmp     rdx, 8h
    je      VALUE_SHIFTED
    mov     eax, eax
    cmp     rdx, 4h
    je      VALUE_SHIFTED
    and     eax, 0ffffh
    cmp     rdx, 2h
    je      VALUE_SHIFTED
    and     eax, 0ffh
VALUE_SHIFTED:
    movzx   rdx, byte [rbp]
    inc     rbp
    test    rdx, rdx
    jz      VALUE_PARAMETER
    cmp     rdx, 1h
    je      HEADER_PARAMETER
    cmp     rdx, 2h
    je      STRING_PARAMETER
    jmp     BOOL_PARAMETER
VALUE_PARAMETER:
    call    RAX_TO_HEX
    mov     r8, rsi
    mov     r9, rdi
    mov     rdi, rsi
    add     rdi, [rsi - 8h]
    mov     rsi, rcx
    mov     rcx, rdx
    rep     movsb
    mov     rsi, r8
    add     qword [rsi - 8h], rdx
    mov     rdi, r9
NEXT_PARAM:
    inc     r15
    cmp     r15, rdi
    je      PARAMETERS_LOOP_END
    mov     rdx, qword [rsi - 8h]
    mov     byte [rsi + rdx], 2ch
    inc     qword [rsi - 8h]
    jmp     PARAMETERS_LOOP
BOOL_PARAMETER:
    mov     r8, rsi
    add     r8, [rsi - 8h]
    test    rax, rax
    jnz     TRUE_PARAM
    mov     dword [r8], "FALS"
    mov     byte [r8 + 4h], 'E'
    add     qword [rsi - 8h], 5
    jmp     NEXT_PARAM
TRUE_PARAM:
    mov     dword [r8], "TRUE"
    add     qword [rsi - 8h], 4
    jmp     NEXT_PARAM
HEADER_PARAMETER:
    mov     r10, rbp
    xor     rcx, rcx
    mov     r9, rdi
    mov     ebp, [rbp]
    add     rbp, [rsp + 98h]
    cmp     byte [rbp], 0
    jne     FLAG_HEADER
    inc     rbp
    movzx   rdx, word [rbp]
    add     rbp, 2h
ENUM_LOOP:
    test    rdx, rdx
    jz      HEADER_NOT_FOUND
    dec     rdx
    add     rbp, rcx
    mov     edi, [rbp]
    add     rbp, 4
    movzx   rcx, byte [rbp]
    inc     rbp
    cmp     rax, rdi
    jne     ENUM_LOOP
    mov     rdi, rsi
    add     rdi, [rsi - 8h]
    mov     rsi, rbp
    rep     movsb
    mov     rsi, r8
    mov     rcx, rdi
    sub     rcx, rsi
    mov     qword [rsi - 8h], rcx
    jmp     HEADER_PARAMETER_END
FLAG_HEADER:
    inc     rbp
    movzx   rdx, word [rbp]
    add     rbp, 2h
    mov     r11, rax
    xor     rax, rax
FLAG_LOOP:
    test    rdx, rdx
    jz      FLAG_LOOP_END
    dec     rdx
    add     rbp, rcx
    mov     edi, [rbp]
    add     rbp, 4h
    movzx   rcx, byte [rbp]
    inc     rbp
    mov     rsi, rdi
    and     rdi, r11
    cmp     rsi, rdi
    jne     FLAG_LOOP
    mov     rdi, r8
    add     rdi, [rdi - 8h]
    mov     rsi, rbp
    rep     movsb
    mov     rsi, r8
    mov     word [rdi], " |"
    mov     byte [rdi + 2], " "
    add     rdi, 3
    mov     rcx, rdi
    sub     rcx, rsi
    mov     qword [rsi - 8h], rcx
    inc     rax
    movzx   rcx, byte [rbp - 1h]
    jmp     FLAG_LOOP
FLAG_LOOP_END:
    mov     rdx, rax
    mov     rax, r11
    mov     rsi, r8
    test    rdx, rdx
    jz      HEADER_NOT_FOUND
    sub     qword [rsi - 8h], 3
    jmp     HEADER_PARAMETER_END
HEADER_NOT_FOUND:
    mov     rdi, r9
    mov     rbp, r10
    add     rbp, 4h
    jmp     VALUE_PARAMETER
HEADER_PARAMETER_END:
    mov     rdi, r9
    mov     rbp, r10
    add     rbp, 4h
    jmp     NEXT_PARAM
STRING_PARAMETER:
    xor     rdx, rdx
    mov     r8, rsi
    add     r8, [rsi - 8h]
    cmp     rax, 0ffffh
    jg      VALID_POINTER
    test    rax, rax
    jnz     NEXT_PARAM
    mov     dword [r8], "NULL"
    add     qword [rsi - 8h], 4
    jmp     NEXT_PARAM
VALID_POINTER:
    cmp     byte [rax + 1h], 0
    je      WIDE_STRING_PARAMETER
    mov     byte [r8], '"'
ASCII_STRING_PARAMETER_LOOP:
    mov     cl, byte [rax + rdx]
    test    cl, cl
    jz      STRING_PARAMETER_LOOP_END
    cmp     cl, '\'
    jne     DOUBLE_QOUTE_ESCAPE_A
    mov     byte [r8 + rdx + 1h], '\'
    inc     r8
    jmp     NO_ESCAPE_A
DOUBLE_QOUTE_ESCAPE_A:
    cmp     cl, '"'
    jne     NEW_LINE_ESCAPE_A
    mov     byte [r8 + rdx + 1h], '\'
    inc     r8
    jmp     NO_ESCAPE_A
NEW_LINE_ESCAPE_A:
    cmp     cl, 0ah
    jne     TAP_ESCAPE_A
    mov     word [r8 + rdx + 1h], "\n"
    inc     rdx
    inc     r8
    jmp     ASCII_STRING_PARAMETER_LOOP
TAP_ESCAPE_A:
    cmp     cl, 9h
    jne     CARRIAGE_RETURN_ESCAPE_A
    mov     word [r8 + rdx + 1h], "\t"
    inc     rdx
    inc     r8
    jmp     ASCII_STRING_PARAMETER_LOOP
CARRIAGE_RETURN_ESCAPE_A:
    cmp     cl, 0dh
    jne     NO_ESCAPE_A
    mov     word [r8 + rdx + 1h], "\r"
    inc     rdx
    inc     r8
    jmp     ASCII_STRING_PARAMETER_LOOP
NO_ESCAPE_A:
    cmp     cl, ' '
    jl      STRING_PARAMETER_LOOP_END
    cmp     cl, '~'
    jg      STRING_PARAMETER_LOOP_END
    mov     byte [r8 + rdx + 1h], cl
    inc     rdx
    jmp     ASCII_STRING_PARAMETER_LOOP
WIDE_STRING_PARAMETER:
    mov     byte [r8], 'L'
    mov     byte [r8 + 1h], '"'
WIDE_STRING_PARAMETER_LOOP:
    mov     cl, byte [rax + rdx * 2h]
    test    cl, cl
    jz      WIDE_STRING_PARAMETER_LOOP_END
    cmp     cl, '\'
    jne     DOUBLE_QOUTE_ESCAPE_W
    mov     byte [r8 + rdx + 2h], '\'
    inc     r8
    jmp     NO_ESCAPE_W
DOUBLE_QOUTE_ESCAPE_W:
    cmp     cl, '"'
    jne     NEW_LINE_ESCAPE_W
    mov     byte [r8 + rdx + 2h], '\'
    inc     r8
    jmp     NO_ESCAPE_W
NEW_LINE_ESCAPE_W:
    cmp     cl, 0ah
    jne     TAP_ESCAPE_W
    mov     word [r8 + rdx + 2h], "\n"
    inc     rdx
    inc     r8
    jmp     WIDE_STRING_PARAMETER_LOOP
TAP_ESCAPE_W:
    cmp     cl, 9h
    jne     CARRIAGE_RETURN_ESCAPE_W
    mov     word [r8 + rdx + 2h], "\t"
    inc     rdx
    inc     r8
    jmp     WIDE_STRING_PARAMETER_LOOP
CARRIAGE_RETURN_ESCAPE_W:
    cmp     cl, 0dh
    jne     NO_ESCAPE_W
    mov     word [r8 + rdx + 2h], "\t"
    inc     rdx
    inc     r8
    jmp     WIDE_STRING_PARAMETER_LOOP
NO_ESCAPE_W:
    cmp     cl, ' '
    jl      WIDE_STRING_PARAMETER_LOOP_END
    cmp     cl, '~'
    jg      WIDE_STRING_PARAMETER_LOOP_END
    mov     byte [r8 + rdx + 2h], cl
    mov     cl, byte [rax + rdx * 2h + 1h]
    test    cl, cl
    jnz     WIDE_STRING_PARAMETER_LOOP_END
    inc     rdx
    jmp     WIDE_STRING_PARAMETER_LOOP
WIDE_STRING_PARAMETER_LOOP_END:
    inc     rdx
STRING_PARAMETER_LOOP_END:
    cmp     rdx, 3h
    jl      VALUE_PARAMETER
    mov     byte [r8 + rdx + 1h], '"'
    add     rdx, 2h
    sub     r8, rsi
    sub     r8, qword [rsi - 8h]
    add     rdx, r8
    add     qword [rsi - 8h], rdx
    jmp     NEXT_PARAM
NO_PARAMETERS_LOOP_END:
    mov     r8, rsi
    mov     r9, rdi
    mov     rdi, rsi
    add     rdi, [rsi - 8h]
    mov     rdx, 2h
    lea     rsi, [rel PARAM_END_BUFFER + 1]
    jmp     PARAM_END
PARAMETERS_LOOP_END:
    mov     r8, rsi
    mov     r9, rdi
    mov     rdi, rsi
    add     rdi, [rsi - 8h]
    mov     rdx, 3h
    lea     rsi, [rel PARAM_END_BUFFER]
PARAM_END:
    mov     rcx, rdx
    rep     movsb
    mov     rsi, r8
    add     qword [rsi - 8h], rdx
    mov     rdi, r9
    cmp     byte [rbp], 0
    jne     USE_OLD_STACK
    sub     rsp, 8h
    mov     rcx, rsp
    sub     rsp, 20h
    mov     rax, RTL_QUERY_PERF_COUNTER_ADDRESS
    call    rax
    add     rsp, 20h
    mov     r10, [rsi - 8h]
    sub     rsp, r10
    push    r10
    mov     r8, rsi
    mov     r9, rdi
    mov     rdi, rsp
    add     rdi, 8h
    mov     rcx, r10
    rep     movsb
    mov     rsi, r8
    mov     rdi, r9
    xor     al, al
    xchg    al, [rsi - 9h]
    mov     rcx, [rsp + r10 + 58h]
    mov     rdx, [rsp + r10 + 60h]
    mov     r8, [rsp + r10 + 68h]
    mov     r9, [rsp + r10 + 70h]
    mov     r15, rsp
    add     r15, r10
    cmp     rdi, 4h
    jle     NO_ALLOC
    sub     rdi, 4h
    and     rsp, 0fffffffffffffff0h
    mov     rbp, rdi
    shr     rbp, 1h
    shl     rbp, 1h
    cmp     rbp, rdi
    je      RE_PUSH_PARAMS_LOOP
    inc     rbp
    sub     rsp, 8h
RE_PUSH_PARAMS_LOOP:
    test    rbp, rbp
    jz      RE_PUSH_PARAMS_LOOP_END
    push    qword [r15 + rbp * 8h + 0d0h]
    dec     rbp
    jmp     RE_PUSH_PARAMS_LOOP
RE_PUSH_PARAMS_LOOP_END:
    jmp     CONTINUE_CALL
NO_ALLOC:
    and     rsp, 0fffffffffffffff0h
CONTINUE_CALL:
    sub     rsp, 20h
    sub     r15, r10
    call    qword [r15 + r10 + 0a0h]
RETURN_ADDRESS:
    mov     rsp, r15
SPIN_LOCK_R:
    pause
    mov     cl, 1
    xchg    cl, [rsi - 9h]
    test    cl, cl
    jnz     SPIN_LOCK_R
    pop     rdx
    mov     r8, rsi
    mov     rdi, rsi
    mov     rsi, rsp
    mov     rcx, rdx
    rep     movsb
    mov     rsi, r8
    mov     qword [rsi - 8h], rdx
    add     rsp, rdx
    call    RAX_TO_HEX
    mov     r8, rsi
    mov     rdi, rsi
    add     rdi, [rsi - 8h]
    mov     byte [rdi], '-'
    mov     byte [rdi + 1h], '>'
    mov     byte [rdi + 2h], ' '
    mov     byte [rdi + rdx + 3h], ' '
    add     rdi, 3h
    mov     rsi, rcx
    mov     rcx, rdx
    rep     movsb
    mov     rsi, r8
    add     rdx, 4h
    add     qword [rsi - 8h], rdx
    sub     rsp, 10h
    mov     rcx, rsp
    lea     rdx, [rsp + 8h]
    mov     rbp, rax
    sub     rsp, 20h
    mov     rax, NT_QUERY_PERF_COUNTER_ADDRESS
    call    rax
    add     rsp, 20h
    mov     rax, [rsp]
    sub     rax, [rsp + 10h]
    imul    rax, rax, 0f4240h
    xor     rdx, rdx
    div     qword [rsp + 8h]
    add     rsp, 18h
    call    RAX_TO_HEX
    mov     rax, rbp
    mov     r8, rsi
    mov     rdi, rsi
    add     rdi, [rsi - 8h]
    mov     byte [rdi], '['
    mov     byte [rdi + rdx + 1h], ' '
    mov     dword [rdi + rdx + 2h], 5d73b5c2h
    add     rdi, 1h
    mov     rsi, rcx
    mov     rcx, rdx
    rep     movsb
    mov     rsi, r8
    add     rdx, 6h
    add     qword [rsi - 8h], rdx
    call    WRITE_TO_FILE
    add     rsp, 10h
    ret
USE_OLD_STACK:
    call    WRITE_TO_FILE
    ret     8h
WRITE_TO_FILE:
    pop     r15
    mov     rdx, [rsi - 8h]
    mov     dword [rsi + rdx], 0a0ah
    add     rdx, 2h
    mov     qword [rsp + 30h], rdx
    mov     qword [rsp + 28h], rsi
    xor     r9, r9
    xor     r8, r8
    xor     rdx, rdx
    mov     rcx, rbx
    mov     rbp, rax
    mov     rax, NT_WRITE_FILE_ADDRESS
    call    rax
    mov     rax, rbp
    xor     r10b, r10b
    xchg    r10b, [rsi - 9h]
    add     rsp, 48h
    mov     r10, r15
    pop     rcx
    pop     rdx
    pop     r8
    pop     r9
    pop     r15
    pop     rbp
    pop     rbx
    pop     rdi
    pop     rsi
    push    r10
    ret
RAX_TO_HEX:
    lea     rcx, [rel HEX_VALUE_BUFFER]
    mov     rdx, 2h
    push    rax
    test    rax, rax
    jnz     START_RAX_LOOP
    mov     byte [rcx+rdx], 30h
    inc     rdx
    jmp     BYTE_TO_HEX_LOOP_END
START_RAX_LOOP:
    mov     rax, rsp
    add     rax, 7h
RAX_LOOP:
    mov     r8b, [rax]
    test    r8b, r8b
    jnz     BYTE_TO_HEX_LOOP
    dec     rax
    jmp     RAX_LOOP
BYTE_TO_HEX_LOOP:
    mov     r8b, [rax]
    mov     r9b, r8b
    shr     r8b, 4h
    cmp     r8b, 0ah
    jl      TO_NUM_FIRST
    add     r8b, 57h
    jmp     WRITE_BYTE_FIRST
TO_NUM_FIRST:
    test    r8b, r8b
    jnz     HIGHER_NIPPLE_IS_NOT_ZERO
    cmp     rdx, 2h
    je      HIGHER_NIPPLE_IS_ZERO
HIGHER_NIPPLE_IS_NOT_ZERO:    
    add     r8b, 30h
WRITE_BYTE_FIRST:
    mov     byte [rcx+rdx], r8b
    inc     rdx
HIGHER_NIPPLE_IS_ZERO:
    and     r9b, 0fh
    cmp     r9b, 0ah
    jl      TO_NUM_SECOND
    add     r9b, 57h
    jmp     WRITE_BYTE_SECOND
TO_NUM_SECOND:
    add     r9b, 30h
WRITE_BYTE_SECOND:
    mov     byte [rcx+rdx], r9b
    inc     rdx
    cmp     rax, rsp
    je      BYTE_TO_HEX_LOOP_END
    dec     rax
    jmp     BYTE_TO_HEX_LOOP
BYTE_TO_HEX_LOOP_END:
    pop     rax
    ret
PARAM_END_BUFFER:
    db      0ah,29h,20h
HEX_VALUE_BUFFER:
    db      "0x0000000000000000",90h,90h
IO_STATUS_BLOCK:
    db      90h,90h,90h,90h,90h,90h,90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
GLOBAL_BUFFER: