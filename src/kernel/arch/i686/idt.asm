[bits 32]

; void __attribute__((cdecl)) i686_IDTLoad(IDTDescriptor *idtDescriptor);
global i686_IDTLoad
i686_IDTLoad:
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame
    
    mov eax, [ebp + 8]
    lidt [eax]


    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret
