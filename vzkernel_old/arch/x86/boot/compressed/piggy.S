.section ".rodata..compressed","a",@progbits
.globl z_input_len
z_input_len = 4783539
.globl z_output_len
z_output_len = 16157024
.globl z_extract_offset
z_extract_offset = 0xaea000
.globl z_extract_offset_negative
z_extract_offset_negative = -0xaea000
.globl input_data, input_data_end
input_data:
.incbin "arch/x86/boot/compressed/vmlinux.bin.gz"
input_data_end:
