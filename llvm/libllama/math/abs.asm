; int abs (int n);
; ----------------
; This function returns the absolute value of an integer.

        section .code

        global _abs

_abs    push    rbp
        mov     rbp, rsp
        mov     rax, rdi              ; 1st parameter
        or      rax, rax              ; If it is negative
        jge     ok
        neg     rax                  ; i = -i
ok:
        and     eax, 0xffffffff
        pop     rbp
        ret
