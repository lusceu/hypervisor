; @copyright
; Copyright (C) 2020 Assured Information Security, Inc.
;
; @copyright
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; @copyright
; The above copyright notice and this permission notice shall be included in
; all copies or substantial portions of the Software.
;
; @copyright
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
; SOFTWARE.

    ; @brief defines the offset of state_save_t.rax
    #define SS_OFFSET_RAX 000h
    ; @brief defines the offset of state_save_t.rbx
    #define SS_OFFSET_RBX 008h
    ; @brief defines the offset of state_save_t.rcx
    #define SS_OFFSET_RCX 010h
    ; @brief defines the offset of state_save_t.rdx
    #define SS_OFFSET_RDX 018h
    ; @brief defines the offset of state_save_t.rbp
    #define SS_OFFSET_RBP 020h
    ; @brief defines the offset of state_save_t.rsi
    #define SS_OFFSET_RSI 028h
    ; @brief defines the offset of state_save_t.rdi
    #define SS_OFFSET_RDI 030h
    ; @brief defines the offset of state_save_t.r8
    #define SS_OFFSET_R8 038h
    ; @brief defines the offset of state_save_t.r9
    #define SS_OFFSET_R9 040h
    ; @brief defines the offset of state_save_t.r10
    #define SS_OFFSET_R10 048h
    ; @brief defines the offset of state_save_t.r11
    #define SS_OFFSET_R11 050h
    ; @brief defines the offset of state_save_t.r12
    #define SS_OFFSET_R12 058h
    ; @brief defines the offset of state_save_t.r13
    #define SS_OFFSET_R13 060h
    ; @brief defines the offset of state_save_t.r14
    #define SS_OFFSET_R14 068h
    ; @brief defines the offset of state_save_t.r15
    #define SS_OFFSET_R15 070h
    ; @brief defines the offset of state_save_t.rip
    #define SS_OFFSET_RIP 078h
    ; @brief defines the offset of state_save_t.rsp
    #define SS_OFFSET_RSP 080h
    ; @brief defines the offset of state_save_t.rflags
    #define SS_OFFSET_RFLAGS 088h
    ; @brief defines the offset of state_save_t.gdtr
    #define SS_OFFSET_GDTR 0A0h
    ; @brief defines the offset of state_save_t.idtr
    #define SS_OFFSET_IDTR 0B0h
    ; @brief defines the offset of state_save_t.es_selector
    #define SS_OFFSET_ES_SELECTOR 0C0h
    ; @brief defines the offset of state_save_t.cs_selector
    #define SS_OFFSET_CS_SELECTOR 0D0h
    ; @brief defines the offset of state_save_t.ss_selector
    #define SS_OFFSET_SS_SELECTOR 0E0h
    ; @brief defines the offset of state_save_t.ds_selector
    #define SS_OFFSET_DS_SELECTOR 0F0h
    ; @brief defines the offset of state_save_t.fs_selector
    #define SS_OFFSET_FS_SELECTOR 100h
    ; @brief defines the offset of state_save_t.gs_selector
    #define SS_OFFSET_GS_SELECTOR 110h
    ; @brief defines the offset of state_save_t.ldtr_selector
    #define SS_OFFSET_LDTR_SELECTOR 120h
    ; @brief defines the offset of state_save_t.tr_selector
    #define SS_OFFSET_TR_SELECTOR 130h
    ; @brief defines the offset of state_save_t.cr0
    #define SS_OFFSET_CR0 140h
    ; @brief defines the offset of state_save_t.cr2
    #define SS_OFFSET_CR2 150h
    ; @brief defines the offset of state_save_t.cr3
    #define SS_OFFSET_CR3 158h
    ; @brief defines the offset of state_save_t.cr4
    #define SS_OFFSET_CR4 160h
    ; @brief defines the offset of state_save_t.dr6
    #define SS_OFFSET_DR6 1F0h
    ; @brief defines the offset of state_save_t.dr7
    #define SS_OFFSET_DR7 1F8h
    ; @brief defines the offset of state_save_t.ia32_efer
    #define SS_OFFSET_IA32_EFER 240h
    ; @brief defines the offset of state_save_t.ia32_star
    #define SS_OFFSET_IA32_STAR 248h
    ; @brief defines the offset of state_save_t.ia32_lstar
    #define SS_OFFSET_IA32_LSTAR 250h
    ; @brief defines the offset of state_save_t.ia32_cstar
    #define SS_OFFSET_IA32_CSTAR 258h
    ; @brief defines the offset of state_save_t.ia32_fmask
    #define SS_OFFSET_IA32_FMASK 260h
    ; @brief defines the offset of state_save_t.ia32_fs_base
    #define SS_OFFSET_IA32_FS_BASE 268h
    ; @brief defines the offset of state_save_t.ia32_gs_base
    #define SS_OFFSET_IA32_GS_BASE 270h
    ; @brief defines the offset of state_save_t.ia32_kernel_gs_base
    #define SS_OFFSET_IA32_KERNEL_GS_BASE 278h
    ; @brief defines the offset of state_save_t.ia32_sysenter_cs
    #define SS_OFFSET_IA32_SYSENTER_CS 280h
    ; @brief defines the offset of state_save_t.ia32_sysenter_esp
    #define SS_OFFSET_IA32_SYSENTER_ESP 288h
    ; @brief defines the offset of state_save_t.ia32_sysenter_eip
    #define SS_OFFSET_IA32_SYSENTER_EIP 290h
    ; @brief defines the offset of state_save_t.ia32_pat
    #define SS_OFFSET_IA32_PAT 298h
    ; @brief defines the offset of state_save_t.ia32_debugctl
    #define SS_OFFSET_IA32_DEBUGCTL 2A0h

    ; @brief defines MSR_IA32_SYSENTER_CS
    #define MSR_IA32_SYSENTER_CS 00000174h
    ; @brief defines MSR_IA32_SYSENTER_ESP
    #define MSR_IA32_SYSENTER_ESP 00000175h
    ; @brief defines MSR_IA32_SYSENTER_EIP
    #define MSR_IA32_SYSENTER_EIP 00000176h
    ; @brief defines MSR_IA32_DEBUGCTL
    #define MSR_IA32_DEBUGCTL 000001D9h
    ; @brief defines MSR_IA32_PAT
    #define MSR_IA32_PAT 00000277h
    ; @brief defines MSR_IA32_EFER
    #define MSR_IA32_EFER 0C0000080h
    ; @brief defines MSR_IA32_STAR
    #define MSR_IA32_STAR 0C0000081h
    ; @brief defines MSR_IA32_LSTAR
    #define MSR_IA32_LSTAR 0C0000082h
    ; @brief defines MSR_IA32_CSTAR
    #define MSR_IA32_CSTAR 0C0000083h
    ; @brief defines MSR_IA32_FMASK
    #define MSR_IA32_FMASK 0C0000084h
    ; @brief defines MSR_IA32_FS_BASE
    #define MSR_IA32_FS_BASE 0C0000100h
    ; @brief defines MSR_IA32_GS_BASE
    #define MSR_IA32_GS_BASE 0C0000101h
    ; @brief defines MSR_IA32_KERNEL_GS_BASE
    #define MSR_IA32_KERNEL_GS_BASE 0C0000102h

    enable_interrupts PROTO
    disable_interrupts PROTO

    demote_text SEGMENT ALIGN(1000h) 'CODE'
    demote PROC

    ; **************************************************************************
    ; Report Success On Completion
    ; **************************************************************************

    xor rax, rax

    ; **************************************************************************
    ; General Purpose Registers
    ; **************************************************************************

    mov [r8 + SS_OFFSET_RAX], rax
    mov [r8 + SS_OFFSET_RBX], rbx
    mov [r8 + SS_OFFSET_RCX], rcx
    mov [r8 + SS_OFFSET_RDX], rdx
    mov [r8 + SS_OFFSET_RBP], rbp
    mov [r8 + SS_OFFSET_RSI], rsi
    mov [r8 + SS_OFFSET_RDI], rdi
    mov [r8 + SS_OFFSET_R8], r8
    mov [r8 + SS_OFFSET_R9], r9
    mov [r8 + SS_OFFSET_R10], r10
    mov [r8 + SS_OFFSET_R11], r11
    mov [r8 + SS_OFFSET_R12], r12
    mov [r8 + SS_OFFSET_R13], r13
    mov [r8 + SS_OFFSET_R14], r14
    mov [r8 + SS_OFFSET_R15], r15

    lea rax, [demotion_return]
    mov [r8 + SS_OFFSET_RIP], rax
    mov [r8 + SS_OFFSET_RSP], rsp

    ; **************************************************************************
    ; Setup
    ; **************************************************************************

    mov r13, rcx       ; args
    mov r14, rdx       ; mk_state
    mov r15, r8        ; root_vp_state

    ; **************************************************************************
    ; Flags
    ; **************************************************************************

    pushfq
    pop [r15 + SS_OFFSET_RFLAGS]
    push [r14 + SS_OFFSET_RFLAGS]
    popfq

    ; **************************************************************************
    ; IDT
    ; **************************************************************************

    call disable_interrupts

    sidt fword ptr[r15 + SS_OFFSET_IDTR]
    lidt fword ptr[r14 + SS_OFFSET_IDTR]

    ; **************************************************************************
    ; MSRs
    ; **************************************************************************

    mov ecx, MSR_IA32_EFER
    rdmsr
    mov [r15 + SS_OFFSET_IA32_EFER + 0x0], eax
    mov [r15 + SS_OFFSET_IA32_EFER + 0x4], edx
    mov eax, [r14 + SS_OFFSET_IA32_EFER + 0x0]
    mov edx, [r14 + SS_OFFSET_IA32_EFER + 0x4]
    wrmsr

    mov ecx, MSR_IA32_STAR
    rdmsr
    mov [r15 + SS_OFFSET_IA32_STAR + 0x0], eax
    mov [r15 + SS_OFFSET_IA32_STAR + 0x4], edx
    mov eax, [r14 + SS_OFFSET_IA32_STAR + 0x0]
    mov edx, [r14 + SS_OFFSET_IA32_STAR + 0x4]
    wrmsr

    mov ecx, MSR_IA32_LSTAR
    rdmsr
    mov [r15 + SS_OFFSET_IA32_LSTAR + 0x0], eax
    mov [r15 + SS_OFFSET_IA32_LSTAR + 0x4], edx
    mov eax, [r14 + SS_OFFSET_IA32_LSTAR + 0x0]
    mov edx, [r14 + SS_OFFSET_IA32_LSTAR + 0x4]
    wrmsr

    mov ecx, MSR_IA32_CSTAR
    rdmsr
    mov [r15 + SS_OFFSET_IA32_CSTAR + 0x0], eax
    mov [r15 + SS_OFFSET_IA32_CSTAR + 0x4], edx
    mov eax, [r14 + SS_OFFSET_IA32_CSTAR + 0x0]
    mov edx, [r14 + SS_OFFSET_IA32_CSTAR + 0x4]
    wrmsr

    mov ecx, MSR_IA32_FMASK
    rdmsr
    mov [r15 + SS_OFFSET_IA32_FMASK + 0x0], eax
    mov [r15 + SS_OFFSET_IA32_FMASK + 0x4], edx
    mov eax, [r14 + SS_OFFSET_IA32_FMASK + 0x0]
    mov edx, [r14 + SS_OFFSET_IA32_FMASK + 0x4]
    wrmsr

    mov ecx, MSR_IA32_FS_BASE
    rdmsr
    mov [r15 + SS_OFFSET_IA32_FS_BASE + 0x0], eax
    mov [r15 + SS_OFFSET_IA32_FS_BASE + 0x4], edx
    mov eax, [r14 + SS_OFFSET_IA32_FS_BASE + 0x0]
    mov edx, [r14 + SS_OFFSET_IA32_FS_BASE + 0x4]
    wrmsr

    mov ecx, MSR_IA32_GS_BASE
    rdmsr
    mov [r15 + SS_OFFSET_IA32_GS_BASE + 0x0], eax
    mov [r15 + SS_OFFSET_IA32_GS_BASE + 0x4], edx
    mov eax, [r14 + SS_OFFSET_IA32_GS_BASE + 0x0]
    mov edx, [r14 + SS_OFFSET_IA32_GS_BASE + 0x4]
    wrmsr

    mov ecx, MSR_IA32_KERNEL_GS_BASE
    rdmsr
    mov [r15 + SS_OFFSET_IA32_KERNEL_GS_BASE + 0x0], eax
    mov [r15 + SS_OFFSET_IA32_KERNEL_GS_BASE + 0x4], edx
    mov eax, [r14 + SS_OFFSET_IA32_KERNEL_GS_BASE + 0x0]
    mov edx, [r14 + SS_OFFSET_IA32_KERNEL_GS_BASE + 0x4]
    wrmsr

    mov ecx, MSR_IA32_SYSENTER_CS
    rdmsr
    mov [r15 + SS_OFFSET_IA32_SYSENTER_CS + 0x0], eax
    mov [r15 + SS_OFFSET_IA32_SYSENTER_CS + 0x4], edx
    mov eax, [r14 + SS_OFFSET_IA32_SYSENTER_CS + 0x0]
    mov edx, [r14 + SS_OFFSET_IA32_SYSENTER_CS + 0x4]
    wrmsr

    mov ecx, MSR_IA32_SYSENTER_ESP
    rdmsr
    mov [r15 + SS_OFFSET_IA32_SYSENTER_ESP + 0x0], eax
    mov [r15 + SS_OFFSET_IA32_SYSENTER_ESP + 0x4], edx
    mov eax, [r14 + SS_OFFSET_IA32_SYSENTER_ESP + 0x0]
    mov edx, [r14 + SS_OFFSET_IA32_SYSENTER_ESP + 0x4]
    wrmsr

    mov ecx, MSR_IA32_SYSENTER_EIP
    rdmsr
    mov [r15 + SS_OFFSET_IA32_SYSENTER_EIP + 0x0], eax
    mov [r15 + SS_OFFSET_IA32_SYSENTER_EIP + 0x4], edx
    mov eax, [r14 + SS_OFFSET_IA32_SYSENTER_EIP + 0x0]
    mov edx, [r14 + SS_OFFSET_IA32_SYSENTER_EIP + 0x4]
    wrmsr

    mov ecx, MSR_IA32_PAT
    rdmsr
    mov [r15 + SS_OFFSET_IA32_PAT + 0x0], eax
    mov [r15 + SS_OFFSET_IA32_PAT + 0x4], edx
    mov eax, [r14 + SS_OFFSET_IA32_PAT + 0x0]
    mov edx, [r14 + SS_OFFSET_IA32_PAT + 0x4]
    wrmsr

    mov ecx, MSR_IA32_DEBUGCTL
    rdmsr
    mov [r15 + SS_OFFSET_IA32_DEBUGCTL + 0x0], eax
    mov [r15 + SS_OFFSET_IA32_DEBUGCTL + 0x4], edx
    mov eax, [r14 + SS_OFFSET_IA32_DEBUGCTL + 0x0]
    mov edx, [r14 + SS_OFFSET_IA32_DEBUGCTL + 0x4]
    wrmsr

    ; **************************************************************************
    ; GDT
    ; **************************************************************************

    sgdt fword ptr[r15 + SS_OFFSET_GDTR]
    lgdt fword ptr[r14 + SS_OFFSET_GDTR]

    mov dx, es
    mov [r15 + SS_OFFSET_ES_SELECTOR], dx
    mov dx, [r14 + SS_OFFSET_ES_SELECTOR]
    mov es, dx

    mov dx, cs
    mov [r15 + SS_OFFSET_CS_SELECTOR], dx
    mov ax, [r14 + SS_OFFSET_CS_SELECTOR]
    push rax

    mov dx, ss
    mov [r15 + SS_OFFSET_SS_SELECTOR], dx
    mov dx, [r14 + SS_OFFSET_SS_SELECTOR]
    mov ss, dx

    mov dx, ds
    mov [r15 + SS_OFFSET_DS_SELECTOR], dx
    mov dx, [r14 + SS_OFFSET_DS_SELECTOR]
    mov ds, dx

    mov dx, fs
    mov [r15 + SS_OFFSET_FS_SELECTOR], dx
    mov dx, [r14 + SS_OFFSET_FS_SELECTOR]
    mov fs, dx

    mov dx, gs
    mov [r15 + SS_OFFSET_GS_SELECTOR], dx
    mov dx, [r14 + SS_OFFSET_GS_SELECTOR]
    mov gs, dx

    sldt dx
    mov [r15 + SS_OFFSET_LDTR_SELECTOR], dx
    mov dx, [r14 + SS_OFFSET_LDTR_SELECTOR]
    lldt dx

    str dx
    mov [r15 + SS_OFFSET_TR_SELECTOR], dx
    mov dx, [r14 + SS_OFFSET_TR_SELECTOR]
    ltr dx

    lea rax, [gdt_and_cs_loaded]
    push rax

    retfq

gdt_and_cs_loaded:

    ; **************************************************************************
    ; Control Registers
    ; **************************************************************************

    mov rax, cr0
    mov [r15 + SS_OFFSET_CR0], rax
    mov rax, [r14 + SS_OFFSET_CR0]
    mov cr0, rax

    mov rax, cr2
    mov [r15 + SS_OFFSET_CR2], rax
    mov rax, [r14 + SS_OFFSET_CR2]
    mov cr2, rax

    mov rax, cr4
    mov [r15 + SS_OFFSET_CR4], rax
    mov rax, [r14 + SS_OFFSET_CR4]
    mov cr4, rax

    mov rax, cr3
    mov [r15 + SS_OFFSET_CR3], rax
    mov rax, [r14 + SS_OFFSET_CR3]
    mov cr3, rax

    ; **************************************************************************
    ; Stack
    ; **************************************************************************

    mov rsp, [r14 + SS_OFFSET_RSP]

    ; **************************************************************************
    ; Debug Registers
    ; **************************************************************************

    mov rax, dr6
    mov [r15 + SS_OFFSET_DR6], rax
    mov rax, [r14 + SS_OFFSET_DR6]
    mov dr6, rax

    mov rax, dr7
    mov [r15 + SS_OFFSET_DR7], rax
    mov rax, [r14 + SS_OFFSET_DR7]
    mov dr7, rax

    ; **************************************************************************
    ; Call Microkernel
    ; **************************************************************************

    mov rdi, r13
    push [r14 + SS_OFFSET_RIP]
    ret
    int 3

demotion_return:


    ; NOTE:
    ; - If demotion is successful, before we return back to the loader, we
    ;   ensure that at least one exit occurs. This is done to properly handle
    ;   errors with the first VMExit. Specifically, if the first VMExit
    ;   generates a failure, it needs to return to loader. The state in
    ;   the root VP, which is what it will use to return is still the same
    ;   at this point, so a return is safe.

    push rax
    push rbx
    push rcx
    push rdx

    mov rax, 0
    cpuid

    pop rdx
    pop rcx
    pop rbx
    pop rax

    call enable_interrupts
    ret
    int 3

    demote ENDP
    demote_text ENDS
    end
