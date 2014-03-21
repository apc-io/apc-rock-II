	.section .piggydata,#alloc
	.globl	input_data
input_data:
	.incbin	"piggy.gzip"
	.globl	input_data_end
input_data_end:
