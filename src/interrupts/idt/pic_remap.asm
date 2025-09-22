[bits 32]
global pic_remap

pic_remap:
    mov al, 0x11
    out 0x20, al
    out 0xA0, al

    mov al, 0x20
    out 0x21, al
    mov al, 0x28
    out 0xA1, al

    mov al, 0x00
    out 0x21, al
    out 0xA1, al

    mov al, 0x01
    out 0x21, al
    out 0xA1, al

    mov al, 0xff
    out 0x21, al
    out 0xA1, al

    ret
