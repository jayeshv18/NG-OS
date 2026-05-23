bits 16 ;we are still in 16 bit mode
org 0x9000 ;where the code exactly will be present based on previous file/sector
mov ah, 0x0e
mov si, stage2_msg
print_loop:
lodsb
cmp al,0
je halt_cpu
int 0x10
jmp print_loop

halt_cpu:
hlt
jmp halt_cpu

stage2_msg: db 'Stage 2 loaded lets go... excited btw :)',0
;We do NOT need the 512-byte padding or the 0xaa55 signature in this file. The BIOS only checks Sector 0 for that. Stage 2 can be any size


