.CODE
; void __stdcall AsmVmcall(_In_ void* hook_target);
AsmVmcall PROC
    vmcall                  ; vmcall(hook_target)
    xor rax, rax            ; return 0
    ret
AsmVmcall ENDP

END
