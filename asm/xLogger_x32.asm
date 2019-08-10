bits 32
%include "xLogger_x32.inc"
INIT:
    push    eax
    fs mov  eax, [TEB_PPEB_OFFSET]
    mov     eax, [eax + PEB_PLDR_OFFSET]
    mov     eax, [eax + LDR_PIN_ORDER_MOD_LIST_OFFSET]
    mov     ecx, eax
FIND_CALLING_MODULE:
    mov     edx, [eax + LDR_MODULE_BASE_OFFSET]
    cmp     dword [esp + 0ch], edx
    jl      NEXT_MODULE
    mov     edx, [eax + LDR_SIZE_OF_IMAGE_OFFSET]
    add     edx, [eax + LDR_MODULE_BASE_OFFSET]
    cmp     dword [esp + 0ch], edx
    jge     NEXT_MODULE
    jmp     CHECK_CALLING_MODULE
NEXT_MODULE:
    mov     eax, [eax]
    cmp     eax, ecx
    jne     FIND_CALLING_MODULE
CHECK_CALLING_MODULE:
    cmp     eax, ecx
    je      LOG_CALL
    mov     ecx, EXTERNAL_MODS_LIST
    add     ecx, [esp]
    movzx   edx, word [ecx]
    test    edx, edx
    jz      DO_NOT_LOG_CALL
    sub     esp, 14h
    mov     dword [esp], esi
    mov     dword [esp + 4h], edi
    mov     dword [esp + 8h], ebx
    mov     dword [esp + 0ch], ebp
    mov     dword [esp + 10h], eax
    mov     eax, [eax + LDR_MODULE_NAME_OFFSET + 4h]
    xor     esi, esi
    add     ecx, 2
EXTERNAL_MODS_LOOP:
    movzx   esi, byte [ecx]
    inc     ecx
    dec     edx
    xor     edi, edi
MODULE_CHECK_LOOP:
    movzx   ebx, byte [eax + edi * 2h]
    cmp     bl, 'Z'
    jg      LOWER_CASE
    cmp     bl, 'A'
    jl      LOWER_CASE
    add     bl, 20h
LOWER_CASE:
    movzx   ebp, byte [ecx + edi]
    cmp     ebp, ebx
    jne     MODULE_CHECK_LOOP_END
    inc     edi
    cmp     edi, esi
    jne     MODULE_CHECK_LOOP
    mov     esi, [esp]
    mov     edi, [esp + 4h]
    mov     ebx, [esp + 8h]
    mov     ebp, [esp + 0ch]
    mov     eax, [esp + 10h]
    add     esp, 14h
    jmp     LOG_CALL
MODULE_CHECK_LOOP_END:
    sub     edx, esi
    add     ecx, esi
    test    edx, edx
    jnz     EXTERNAL_MODS_LOOP
    mov     esi, [esp]
    mov     edi, [esp + 4h]
    mov     ebx, [esp + 8h]
    mov     ebp, [esp + 0ch]
    add     esp, 14h
DO_NOT_LOG_CALL:
    add     esp, 4
    ret     4h
LOG_CALL:
    mov     edx, eax
    mov     eax, EXTERNAL_MODS_LIST
    add     eax, [esp]
    movzx   ecx, word [eax]
    lea     eax, [eax + ecx + 2h]
    jmp     eax 
EXTERNAL_MODS_LIST:
    db      0h, 0h
    mov     eax, RETURN_ADDRESS
    add     eax, [esp]
    add     eax, ecx
    cmp     eax, [esp + 0ch]
    jne     START_LOGGING
    add     esp, 4h
    ret     4h
START_LOGGING:
    push    esi
    push    edi
    push    ebx
    push    ebp
    mov     ebp, API_INFO_ADDRESS
    xchg    dword [esp + 18h], ebp
    add     ebp, [esp + 18h]
    mov     esi, GLOBAL_BUFFER
    add     esi, [esp + 10h]
    add     esi, ecx
SPIN_LOCK:
    pause
    mov     al, 1
    xchg    al, [esi]
    test    al, al
    jnz     SPIN_LOCK
    add     esi, 5h
    push    esi
    add     edx, LDR_MODULE_NAME_OFFSET
    mov     eax, [edx + 4h]
    movzx   edx, word [edx]
    shr     dx, 1h
    mov     edi, [esp]
    mov     byte [edi], '['
    mov     byte [edi + edx + 1h], ']'
    mov     byte [edi + edx + 2h], ' '
    xor     ecx, ecx
WRITE_MODULE_NAME:
    cmp     edx, ecx
    je      WRITE_THREAD_ID
    mov     bl, [eax + ecx * 2h]
    mov     byte [edi + ecx + 1h], bl
    inc     ecx
    jmp     WRITE_MODULE_NAME
WRITE_THREAD_ID:
    add     edx, 3h
    add     edi, edx
    mov     dword [esi - 4h], edx
    fs mov  eax, dword [TEB_THREADID_OFFSET]
    call    EAX_TO_HEX
    mov     byte [edi], '['
    mov     byte [edi + edx + 1h], ']'
    mov     byte [edi + edx + 2h], ' '
    inc     edi
    mov     esi, ecx
    mov     ecx, edx
    rep     movsb
    add     edx, 3h
    mov     esi, [esp]
    add     dword [esi - 4h], edx
    mov     dl, [ebp]
    inc     ebp
    mov     ecx, edx
    mov     edi, esi
    add     edi, [esi - 4h]
    mov     esi, ebp
    rep     movsb
    pop     esi
    add     dword [esi - 4h], edx
    add     ebp, edx
    movzx   edi, byte [ebp]
    inc     ebp
    sub     esp, 8h
    test    edi, edi
    jz      NO_PARAMETERS_LOOP_END
    xor     ebx, ebx
PARAMETERS_LOOP:
    movzx   edx, byte [ebp]
    inc     ebp
    mov     ecx, edx
    mov     dword [esp], esi
    mov     dword [esp+4], edi
    mov     edi, esi
    add     edi, [esi - 4h]
    mov     esi, ebp
    rep     movsb
    mov     esi, [esp]
    add     dword [esi - 4h], edx
    mov     edi, [esp + 4h]
    add     ebp, edx
    mov     eax, [esp + ebx * 4h + 28h]
    movzx   edx, byte [ebp]
    inc     ebp
    cmp     edx, 8h
    je      QWORD_PARAM
    cmp     edx, 4h
    je      VALUE_SHIFTED
    and     eax, 0ffffh
    cmp     edx, 2h
    je      VALUE_SHIFTED
    and     eax, 0ffh
VALUE_SHIFTED:
    movzx   edx, byte [ebp]
    inc     ebp
    test    edx, edx
    jz      VALUE_PARAMETER
    cmp     edx, 1h
    je      HEADER_PARAMETER
    cmp     edx, 2h
    je      STRING_PARAMETER
    jmp     BOOL_PARAMETER
QWORD_PARAM:
    call    EAX_TO_HEX
    mov     dword [esp], esi
    mov     dword [esp + 4h], edi
    mov     edi, esi
    add     edi, [esi - 4h]
    mov     byte [edi + edx], ':'
    mov     esi, ecx
    mov     ecx, edx
    rep     movsb
    inc     edx
    mov     esi, [esp]
    add     dword [esi - 4h], edx
    mov     edi, [esp + 4h]
    inc     ebx
    inc     edi
    mov     eax, [esp + ebx * 4h + 28h]
    jmp     VALUE_SHIFTED
VALUE_PARAMETER:
    call    EAX_TO_HEX
    mov     dword [esp], esi
    mov     dword [esp + 4h], edi
    mov     edi, esi
    add     edi, [esi - 4h]
    mov     esi, ecx
    mov     ecx, edx
    rep     movsb
    mov     esi, [esp]
    add     dword [esi - 4h], edx
NEXT_PARAM:
    mov     edi, [esp + 4h]
    inc     ebx
    cmp     ebx, edi
    je      PARAMETERS_LOOP_END
    mov     edx, dword [esi - 4h]
    mov     byte [esi + edx], 2ch
    inc     dword [esi - 4h]
    jmp     PARAMETERS_LOOP
BOOL_PARAMETER:
    mov     ecx, esi
    add     ecx, [esi - 4h]
    test    eax, eax
    jnz     TRUE_PARAM
    mov     dword [ecx], "FALS"
    mov     byte [ecx + 4h], 'E'
    add     dword [esi - 4h], 5
    jmp     NEXT_PARAM
TRUE_PARAM:
    mov     dword [ecx], "TRUE"
    add     dword [esi - 4h], 4
    jmp     NEXT_PARAM
HEADER_PARAMETER:
    push    ebp
    xor     ecx, ecx
    mov     dword [esp + 8h], edi
    mov     ebp, [ebp]
    add     ebp, [esp + 24h]
    cmp     byte [ebp], 0
    jne     FLAG_HEADER
    inc     ebp
    movzx   edx, word [ebp]
    add     ebp, 2h
ENUM_LOOP:
    test    edx, edx
    jz      HEADER_NOT_FOUND
    dec     edx
    add     ebp, ecx
    mov     edi, [ebp]
    add     ebp, 4
    movzx   ecx, byte [ebp]
    inc     ebp
    cmp     eax, edi
    jne     ENUM_LOOP
    mov     edi, esi
    add     edi, [esi - 4h]
    mov     esi, ebp
    rep     movsb
    mov     esi, [esp + 4h]
    mov     ecx, edi
    sub     ecx, esi
    mov     dword [esi - 4h], ecx
    jmp     HEADER_PARAMETER_END
FLAG_HEADER:
    inc     ebp
    movzx   edx, word [ebp]
    add     ebp, 2h
    push    eax
    xor     eax, eax
FLAG_LOOP:
    test    edx, edx
    jz      FLAG_LOOP_END
    dec     edx
    add     ebp, ecx
    mov     edi, [ebp]
    add     ebp, 4
    movzx   ecx, byte [ebp]
    inc     ebp
    mov     esi, edi
    and     edi, [esp]
    cmp     esi, edi
    jne     FLAG_LOOP
    mov     edi, [esp + 8h]
    add     edi, [edi - 4h]
    mov     esi, ebp
    rep     movsb
    mov     esi, [esp + 8h]
    mov     word [edi], " |"
    mov     byte [edi + 2], " "
    add     edi, 3
    mov     ecx, edi
    sub     ecx, esi
    mov     dword [esi - 4h], ecx
    inc     eax
    movzx   ecx, byte [ebp - 1h]
    jmp     FLAG_LOOP
FLAG_LOOP_END:
    mov     edx, eax
    pop     eax
    mov     esi, [esp + 4h]
    test    edx, edx
    jz      HEADER_NOT_FOUND
    sub     dword [esi - 4h], 3
    jmp     HEADER_PARAMETER_END
HEADER_NOT_FOUND:
    mov     edi, [esp + 8h]
    pop     ebp
    add     ebp, 4h
    jmp     VALUE_PARAMETER
HEADER_PARAMETER_END:
    mov     edi, [esp + 8h]
    pop     ebp
    add     ebp, 4h
    jmp     NEXT_PARAM
STRING_PARAMETER:
    xor     edx, edx
    mov     dword [esp + 4h], edi
    mov     edi, esi
    add     edi, [esi - 4h]
    cmp     eax, 0ffffh
    jg      VALID_POINTER
    test    eax, eax
    call    EAX_TO_HEX
    mov     dword [esp], esi
    mov     esi, ecx
    mov     ecx, edx
    rep     movsb
    mov     esi, [esp]
    add     dword [esi - 4h], edx
    jnz     NEXT_PARAM
    mov     dword [edi], "NULL"
    add     dword [esi - 4h], 4
    jmp     NEXT_PARAM
VALID_POINTER:
    cmp     byte [eax + 1h], 0
    je      WIDE_STRING_PARAMETER
    mov     byte [edi], '"'
ASCII_STRING_PARAMETER_LOOP:
    mov     cl, byte [eax + edx]
    test    cl, cl
    jz      STRING_PARAMETER_LOOP_END
    cmp     cl, '\'
    jne     DOUBLE_QUOTE_ESCAPE_A
    mov     byte [edi + edx + 1h], '\'
    inc     edi
    jmp     NO_ESCAPE_A
DOUBLE_QUOTE_ESCAPE_A:
    cmp     cl, '"'
    jne     NEW_LINE_ESCAPE_A
    mov     byte [edi + edx + 1h], '\'
    inc     edi
    jmp     NO_ESCAPE_A
NEW_LINE_ESCAPE_A:
    cmp     cl, 0ah
    jne     TAP_ESCAPE_A
    mov     word [edi + edx + 1h], "\n"
    inc     edx
    inc     edi
    jmp     ASCII_STRING_PARAMETER_LOOP
TAP_ESCAPE_A:
    cmp     cl, 9h
    jne     CARRIAGE_RETURN_ESCAPE_A
    mov     word [edi + edx + 1h], "\t"
    inc     edx
    inc     edi
    jmp     ASCII_STRING_PARAMETER_LOOP
CARRIAGE_RETURN_ESCAPE_A:
    cmp     cl, 0dh
    jne     NO_ESCAPE_A
    mov     word [edi + edx + 1h], "\r"
    inc     edx
    inc     edi
    jmp     ASCII_STRING_PARAMETER_LOOP
NO_ESCAPE_A:
    cmp     cl, ' '
    jl      STRING_PARAMETER_LOOP_END
    cmp     cl, '~'
    jg      STRING_PARAMETER_LOOP_END
    mov     byte [edi + edx + 1h], cl
    inc     edx
    jmp     ASCII_STRING_PARAMETER_LOOP
WIDE_STRING_PARAMETER:
    mov     byte [edi], 'L'
    mov     byte [edi + 1h], '"'
WIDE_STRING_PARAMETER_LOOP:
    mov     cl, byte [eax + edx * 2h]
    test    cl, cl
    jz      WIDE_STRING_PARAMETER_LOOP_END
    cmp     cl, '\'
    jne     DOUBLE_QOUTE_ESCAPE_W
    mov     byte [edi + edx + 2h], '\'
    inc     edi
    jmp     NO_ESCAPE_W
DOUBLE_QOUTE_ESCAPE_W:
    cmp     cl, '"'
    jne     NEW_LINE_ESCAPE_W
    mov     byte [edi + edx + 2h], '\'
    inc     edi
    jmp     NO_ESCAPE_W
NEW_LINE_ESCAPE_W:
    cmp     cl, 0ah
    jne     TAP_ESCAPE_W
    mov     word [edi + edx + 2h], "\n"
    inc     edx
    inc     edi
    jmp     WIDE_STRING_PARAMETER_LOOP
TAP_ESCAPE_W:
    cmp     cl, 9h
    jne     CARRIAGE_RETURN_ESCAPE_W
    mov     word [edi + edx + 2h], "\t"
    inc     edx
    inc     edi
    jmp     WIDE_STRING_PARAMETER_LOOP
CARRIAGE_RETURN_ESCAPE_W:
    cmp     cl, 0dh
    jne     NO_ESCAPE_W
    mov     word [edi + edx + 2h], "\t"
    inc     edx
    inc     edi
    jmp     WIDE_STRING_PARAMETER_LOOP
NO_ESCAPE_W:
    cmp     cl, ' '
    jl      WIDE_STRING_PARAMETER_LOOP_END
    cmp     cl, '~'
    jg      WIDE_STRING_PARAMETER_LOOP_END
    mov     byte [edi + edx + 2h], cl
    mov     cl, byte [eax + edx * 2h + 1h]
    test    cl, cl
    jnz     WIDE_STRING_PARAMETER_LOOP_END
    inc     edx
    jmp     WIDE_STRING_PARAMETER_LOOP
WIDE_STRING_PARAMETER_LOOP_END:
    inc     edx
STRING_PARAMETER_LOOP_END:
    mov     byte [edi + edx + 1h], '"'
    add     edx, 2h
    sub     edi, esi
    sub     edi, dword [esi - 4h]
    add     edx, edi
    add     dword [esi - 4h], edx
    jmp     NEXT_PARAM
SHORT_OR_NULL_STRING:
    mov     esi, [esp]
    mov     edi, [esp + 4h]
    jmp     VALUE_PARAMETER
NO_PARAMETERS_LOOP_END:
    mov     dword [esp], esi
    mov     dword [esp + 4h], edi
    mov     edi, esi
    add     edi, [esi - 4h]
    mov     edx, 2h
    call    GET_NO_PARAM_END_BUFFER
    db      29h,20h
GET_NO_PARAM_END_BUFFER:
    pop     esi
    jmp     PARAM_END
PARAMETERS_LOOP_END:
    mov     dword [esp], esi
    mov     dword [esp + 4h], edi
    mov     edi, esi
    add     edi, [esi - 4h]
    mov     edx, 3h
    call    GET_PARAM_END_BUFFER
    db      0ah,29h,20h
GET_PARAM_END_BUFFER:
    pop     esi
PARAM_END:
    mov     ecx, edx
    rep     movsb
    mov     esi, [esp]
    mov     edi, [esp + 4h]
    add     dword [esi - 4h], edx
    add     esp, 8h
    cmp     byte [ebp], 0
    jne     USE_OLD_STACK
    inc     ebp
    movzx   ebp, byte [ebp]
    push    ebp
    sub     esp, 8h
    push    esp
    mov     eax, RTL_QUERY_PERF_COUNTER_ADDRESS
    call    eax
    sub     esp, [esi - 4h]
    push    dword [esi - 4h]
    sub     esp, 8h
    mov     dword [esp], esi
    mov     dword [esp + 4h], edi
    lea     edi, [esp + 0ch]
    mov     ecx, [esi - 4h]
    rep     movsb
    mov     esi, [esp]
    mov     edi, [esp + 4h]
    add     esp, 8h
    mov     edx, [esi - 4h]
    xor     al, al
    xchg    al, [esi - 5h]
    mov     ebx, esp
    add     ebx, edx
    and     esp, 0fffffff0h
    mov     ebp, edi
RE_PUSH_PARAMS_LOOP:
    test    ebp, ebp
    jz      CONTINUE_CALL
    push    dword [ebx + ebp * 4h + 2ch]
    dec     ebp
    jmp     RE_PUSH_PARAMS_LOOP
CONTINUE_CALL:
    sub     ebx, edx
    call    dword [ebx + edx + 24h]
RETURN_ADDRESS:
    mov     esp, ebx
SPIN_LOCK_R:
    pause
    mov     cl, 1
    xchg    cl, [esi - 5h]
    test    cl, cl
    jnz     SPIN_LOCK_R
    pop     edx
    sub     esp, 8h
    mov     dword [esp], esi
    mov     dword [esp + 4h], edi
    mov     edi, esi
    lea     esi, [esp + 8h]
    mov     ecx, edx
    rep     movsb
    mov     esi, [esp]
    mov     dword [esi - 4h], edx
    mov     esi, [esp]
    mov     edi, [esp + 4h]
    add     esp, 8h
    add     esp, edx
    call    EAX_TO_HEX
    sub     esp, 8h
    mov     dword [esp], esi
    mov     dword [esp + 4h], edi
    mov     edi, esi
    add     edi, [esi - 4h]
    mov     byte [edi], '-'
    mov     byte [edi + 1h], '>'
    mov     byte [edi + 2h], ' '
    mov     byte [edi + edx + 3h], ' '
    add     edi, 3h
    mov     esi, ecx
    mov     ecx, edx
    rep     movsb
    mov     esi, [esp]
    mov     edi, [esp + 4h]
    add     esp, 8h
    add     edx, 4h
    add     dword [esi - 4h], edx
    sub     esp, 10h
    mov     ecx, esp
    lea     edx, [esp + 8h]
    push    edx
    push    ecx
    mov     ebp, eax
    mov     eax, NT_QUERY_PERF_COUNTER_ADDRESS
    call    eax
    mov     eax, [esp]
    sub     eax, [esp + 10h]
    imul    eax, eax, 0f4240h
    xor     edx, edx
    div     dword [esp + 8h]
    add     esp, 18h
    call    EAX_TO_HEX
    mov     eax, ebp
    sub     esp, 8h
    mov     dword [esp], esi
    mov     dword [esp + 4h], edi
    mov     edi, esi
    add     edi, [esi - 4h]
    mov     byte [edi], '['
    mov     byte [edi + edx + 1h], ' '
    mov     dword [edi + edx + 2h], 5d73b5c2h
    add     edi, 1h
    mov     esi, ecx
    mov     ecx, edx
    rep     movsb
    mov     esi, [esp]
    mov     edi, [esp + 4h]
    add     esp, 8h
    add     edx, 6h
    add     dword [esi - 4h], edx
    pop     ebp
    test    ebp, ebp
    jz      CDECL_CALL
    call    WRITE_TO_FILE
    add     esp, 0ch
    mov     ecx, [esp]
    lea     esp, [esp + edx * 4h + 4h]
    push    ecx
    ret
CDECL_CALL:
    call    WRITE_TO_FILE
    add     esp, 0ch
    ret
USE_OLD_STACK:
    call    WRITE_TO_FILE
    add     esp, 4h
    retn    4h
WRITE_TO_FILE:
    pop     ebx
    mov     edx, [esi - 4h]
    mov     dword [esi + edx], 0a0ah
    add     edx, 2h
    xor     ecx, ecx
    push    ecx
    push    ecx
    push    edx
    push    esi
    call    PUSH_IO_STATUS_BLOCK
    db      90h,90h,90h,90h,90h,90h,90h,90h
PUSH_IO_STATUS_BLOCK:
    push    ecx
    push    ecx
    push    ecx
    push    LOG_FILE_HANDLE
    mov     ebp, eax
    mov     eax, NT_WRITE_FILE_ADDRESS
    call    eax
    mov     eax, ebp
    xor     cl, cl
    xchg    cl, [esi - 5h]
    mov     ecx, ebx
    mov     edx, edi
    pop     ebp
    pop     ebx
    pop     edi
    pop     esi
    push    ecx
    ret
EAX_TO_HEX:
    sub     esp, 8h
    mov     dword [esp], esi
    mov     dword [esp + 4h], edi
    call    GET_BUFFER
HEX_VALUE_BUFFER:
    db      "0x00000000",90h,90h
GET_BUFFER:
    pop     ecx
    mov     edx, 2h
    push    eax
    test    eax, eax
    jnz     START_eax_LOOP
    mov     byte [ecx+edx], 30h
    inc     edx
    jmp     BYTE_TO_HEX_LOOP_END
START_eax_LOOP:
    mov     eax, esp
    add     eax, 3h
eax_LOOP:
    movzx   esi, byte [eax]
    test    esi, esi
    jnz     BYTE_TO_HEX_LOOP
    dec     eax
    jmp     eax_LOOP
BYTE_TO_HEX_LOOP:
    movzx   esi, byte [eax]
    mov     edi, esi
    shr     esi, 4h
    cmp     esi, 0ah
    jl      TO_NUM_FIRST
    add     esi, 57h
    jmp     WRITE_BYTE_FIRST
TO_NUM_FIRST:
    test    esi, esi
    jnz     HIGHER_NIPPLE_IS_NOT_ZERO
    cmp     edx, 2h
    je      HIGHER_NIPPLE_IS_ZERO
HIGHER_NIPPLE_IS_NOT_ZERO:    
    add     esi, 30h
WRITE_BYTE_FIRST:
    xchg    esi, eax
    mov     byte [ecx+edx], al
    xchg    esi, eax
    inc     edx
HIGHER_NIPPLE_IS_ZERO:
    and     edi, 0fh
    cmp     edi, 0ah
    jl      TO_NUM_SECOND
    add     edi, 57h
    jmp     WRITE_BYTE_SECOND
TO_NUM_SECOND:
    add     edi, 30h
WRITE_BYTE_SECOND:
    xchg    edi, eax
    mov     byte [ecx+edx], al
    xchg    edi, eax
    inc     edx
    cmp     eax, esp
    je      BYTE_TO_HEX_LOOP_END
    dec     eax
    jmp     BYTE_TO_HEX_LOOP
BYTE_TO_HEX_LOOP_END:
    pop     eax
    mov     esi, [esp]
    mov     edi, [esp + 4h]
    add     esp, 8h
    ret
GLOBAL_BUFFER:
    
