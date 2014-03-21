/*******************************************************************************
  Copyright (c) 2008 WonderMedia Technologies, Inc.

  Module Name:

    $Workfile: display_aligment.S $

  Abstract:

    Memory copy with aligment issue

  Registers using:
    r0: source address
    r1: destination address
    r2: len
*******************************************************************************/

    .text
    .code 32

    .global byte1_alig_mem_copy
	.type	byte1_alig_mem_copy, %function
    .global byte1_less_bundle_copy
	.type	byte1_less_bundle_copy, %function
	
    .global byte2_alig_mem_copy
	.type	byte2_alig_mem_copy, %function	
    .global byte2_less_bundle_copy
	.type	byte2_less_bundle_copy, %function	
	
    .global byte3_alig_mem_copy
	.type	byte3_alig_mem_copy, %function
    .global byte3_less_bundle_copy
	.type	byte3_less_bundle_copy, %function
		
	.global mem_copy
	.type	mem_copy, %function
	.global less_mem_copy
	.type	less_mem_copy, %function
	
	.global bit24_to_bit16_ali24
	.type	bit24_to_bit16_ali24, %function		
	.global bit24_to_bit16_ali32
	.type	bit24_to_bit16_ali32, %function	
byte1_alig_mem_copy:
	stmfd sp!, {r0-r12}
byte1cpy_loop:
	ldmia r0!, {r3-r11}		// 	r3-r10 store
	mov r12, r4				//	backup r4
	mov r3, r3, lsr #8
	mov r12, r12, lsl #24
	orr r3, r3, r12			//	r3 shift 

	mov r12, r5				//	backup r5
	mov r4, r4, lsr #8
	mov r12, r12, lsl #24
	orr r4, r4, r12			//	r4 shift
	
	mov r12, r6				//	backup r6
	mov r5, r5, lsr #8
	mov r12, r12, lsl #24
	orr r5, r5, r12			//	r5 shift	
	
	mov r12, r7				//	backup r7
	mov r6, r6, lsr #8
	mov r12, r12, lsl #24
	orr r6, r6, r12			//	r6 shift	
	
	mov r12, r8				//	backup r8
	mov r7, r7, lsr #8
	mov r12, r12, lsl #24
	orr r7, r7, r12			//	r7 shift		
	
	mov r12, r9				//	backup r9
	mov r8, r8, lsr #8
	mov r12, r12, lsl #24
	orr r8, r8, r12			//	r8 shift		
	
	mov r12, r10			//	backup r10
	mov r9, r9, lsr #8
	mov r12, r12, lsl #24
	orr r9, r9, r12			//	r9 shift

	mov r12, r11			//	backup r11
	mov r10, r10, lsr #8
	mov r12, r12, lsl #24
	orr r10, r10, r12		//	r10 shift

	stmia r1!, {r3-r10}
	sub	r0, r0, #4			// back to 32 bundle
	subs r2, r2, #32
	bne	byte1cpy_loop
	ldmfd sp!, {r0-r12}
	mov pc, lr

byte1_less_bundle_copy:
	stmfd sp!, {r0-r4}
byte1cpy_less_loop:
	ldmia r0!, {r3-r4}		// 	r3
	mov r12, r4				//	backup r4
	mov r3, r3, lsr #8
	mov r12, r12, lsl #24
	orr r3, r3, r12			//	r3 shift 
	stmia r1!, {r3}
	sub	r0, r0, #4	
	subs r2, r2, #4
	bne	byte1cpy_less_loop
	ldmfd sp!, {r0-r4}
	mov pc, lr	
	
byte2_alig_mem_copy:
	stmfd sp!, {r0-r12}
byte2cpy_loop:
	ldmia r0!, {r3-r11}		// 	r3-r10 store
	mov r12, r4				//	backup r4
	mov r3, r3, lsr #16
	mov r12, r12, lsl #16
	orr r3, r3, r12			//	r3 shift 

	mov r12, r5				//	backup r5
	mov r4, r4, lsr #16
	mov r12, r12, lsl #16
	orr r4, r4, r12			//	r4 shift
	
	mov r12, r6				//	backup r6
	mov r5, r5, lsr #16
	mov r12, r12, lsl #16
	orr r5, r5, r12			//	r5 shift	
	
	mov r12, r7				//	backup r7
	mov r6, r6, lsr #16
	mov r12, r12, lsl #16
	orr r6, r6, r12			//	r6 shift	
	
	mov r12, r8				//	backup r8
	mov r7, r7, lsr #16
	mov r12, r12, lsl #16
	orr r7, r7, r12			//	r7 shift		
	
	mov r12, r9				//	backup r9
	mov r8, r8, lsr #16
	mov r12, r12, lsl #16
	orr r8, r8, r12			//	r8 shift		
	
	mov r12, r10			//	backup r10
	mov r9, r9, lsr #16
	mov r12, r12, lsl #16
	orr r9, r9, r12			//	r9 shift

	mov r12, r11			//	backup r11
	mov r10, r10, lsr #16
	mov r12, r12, lsl #16
	orr r10, r10, r12		//	r10 shift

	stmia r1!, {r3-r10}
	sub	r0, r0, #4			// back to 32 bundle
	subs r2, r2, #32
	bne	byte2cpy_loop	
	ldmfd sp!, {r0-r12}
	mov pc, lr

byte2_less_bundle_copy:
	stmfd sp!, {r0-r4}
byte2cpy_less_loop:
	ldmia r0!, {r3-r4}		// 	r3
	mov r12, r4				//	backup r4
	mov r3, r3, lsr #16
	mov r12, r12, lsl #16
	orr r3, r3, r12			//	r3 shift 
	stmia r1!, {r3}
	sub	r0, r0, #4	
	subs r2, r2, #4
	bne	byte2cpy_less_loop
	ldmfd sp!, {r0-r4}
	mov pc, lr	
	
byte3_alig_mem_copy:
	stmfd sp!, {r0-r12}
byte3cpy_loop:
	ldmia r0!, {r3-r11}		// 	r3-r10 store
	mov r12, r4				//	backup r4
	mov r3, r3, lsr #24
	mov r12, r12, lsl #8
	orr r3, r3, r12			//	r3 shift 

	mov r12, r5				//	backup r5
	mov r4, r4, lsr #24
	mov r12, r12, lsl #8
	orr r4, r4, r12			//	r4 shift
	
	mov r12, r6				//	backup r6
	mov r5, r5, lsr #24
	mov r12, r12, lsl #8
	orr r5, r5, r12			//	r5 shift	
	
	mov r12, r7				//	backup r7
	mov r6, r6, lsr #24
	mov r12, r12, lsl #8
	orr r6, r6, r12			//	r6 shift	
	
	mov r12, r8				//	backup r8
	mov r7, r7, lsr #24
	mov r12, r12, lsl #8
	orr r7, r7, r12			//	r7 shift		
	
	mov r12, r9				//	backup r9
	mov r8, r8, lsr #24
	mov r12, r12, lsl #8
	orr r8, r8, r12			//	r8 shift		
	
	mov r12, r10			//	backup r10
	mov r9, r9, lsr #24
	mov r12, r12, lsl #8
	orr r9, r9, r12			//	r9 shift

	mov r12, r11			//	backup r11
	mov r10, r10, lsr #24
	mov r12, r12, lsl #8
	orr r10, r10, r12		//	r10 shift

	stmia r1!, {r3-r10}
	sub	r0, r0, #4			// back to 32 bundle
	subs r2, r2, #32
	bne	byte3cpy_loop
	ldmfd sp!, {r0-r12}
	mov pc, lr

byte3_less_bundle_copy:
	stmfd sp!, {r0-r4}
byte3cpy_less_loop:
	ldmia r0!, {r3-r4}		// 	r3
	mov r12, r4				//	backup r4
	mov r3, r3, lsr #24
	mov r12, r12, lsl #8
	orr r3, r3, r12			//	r3 shift 
	stmia r1!, {r3}
	sub	r0, r0, #4	
	subs r2, r2, #4
	bne	byte3cpy_less_loop
	ldmfd sp!, {r0-r4}
	mov pc, lr
	
mem_copy:
	stmfd sp!, {r0-r10}
ali_mem_copy:
	ldmia r0!, {r3-r10}
	stmia r1!, {r3-r10}
	subs r2, r2, #32
	bne	ali_mem_copy
	ldmfd sp!, {r0-r10}
	mov pc, lr

less_mem_copy:
	stmfd sp!, {r0-r3}
ali_less_mem_copy:
	ldmia r0!, {r3}
	stmia r1!, {r3}
	subs r2, r2, #4
	bne	ali_less_mem_copy
	ldmfd sp!, {r0-r3}
	mov pc, lr	
	
bit24_to_bit16_ali24:
	stmfd sp!, {r0-r11}
bit24_to_bit16_ali24_loop:
	ldmia r1!, {r3-r8}
	// r11 : final data
	// r10 : tmp_reg
	mov  r11, #0
	
	// handle first 16bits ================
	ldr  r9,  =0xFF
	and  r10, r3, r9		//B
	lsr  r10, #3
	mov  r11, r10
	
	ldr  r9,  =0xFF00
	and  r10, r3, r9        //G
	lsr  r10, #10
	lsl  r10, #5
	orr  r11, r11, r10
	
	ldr  r9,  =0xFF0000
	and  r10, r3, r9        //R
	lsr  r10, #19
	lsl  r10, #11
	orr  r11, r11, r10
	
	// handle second 16bits ================
	ldr  r9,  =0xFF000000
	and  r10, r3, r9		//B
	lsr  r10, #27
	lsl  r10, #16	
	orr  r11, r11, r10
	
	ldr  r9,  =0xFF
	and  r10, r4, r9		//G
	lsr  r10, #2
	lsl  r10, #21	
	orr  r11, r11, r10	
	
	ldr  r9,  =0xFF00
	and  r10, r4, r9		//R
	lsr  r10, #11
	lsl  r10, #27	
	orr  r11, r11, r10	
	mov  r3, r11

	// handle third 16bits ================
	mov  r11, #0
	ldr  r9,  =0xFF0000
	and  r10, r4, r9		//B
	lsr  r10, #19
	orr  r11, r11, r10	
	
	ldr  r9,  =0xFF000000
	and  r10, r4, r9		//G
	lsr  r10, #26
	lsl  r10, #5
	orr  r11, r11, r10		
	
	ldr  r9,  =0xFF
	and  r10, r5, r9		//R
	lsr  r10, #3
	lsl  r10, #11
	orr  r11, r11, r10		
	
	// handle forth 16bits ================	
	ldr  r9,  =0xFF00
	and  r10, r5, r9		//B
	lsr  r10, #11
	lsl  r10, #16
	orr  r11, r11, r10		
	
	ldr  r9,  =0xFF0000
	and  r10, r5, r9		//G
	lsr  r10, #18
	lsl  r10, #21
	orr  r11, r11, r10			

	ldr  r9,  =0xFF000000
	and  r10, r5, r9		//R
	lsr  r10, #27
	lsl  r10, #27
	orr  r11, r11, r10
	mov  r4, r11

	// handle fivth 16bits ================
	ldr  r9,  =0xFF
	and  r10, r6, r9		//B
	lsr  r10, #3
	mov  r11, r10
	
	ldr  r9,  =0xFF00
	and  r10, r6, r9        //G
	lsr  r10, #10
	lsl  r10, #5
	orr  r11, r11, r10
	
	ldr  r9,  =0xFF0000
	and  r10, r6, r9        //R
	lsr  r10, #19
	lsl  r10, #11
	orr  r11, r11, r10
	
	// handle sixth 16bits ================
	ldr  r9,  =0xFF000000
	and  r10, r6, r9		//B
	lsr  r10, #27
	lsl  r10, #16	
	orr  r11, r11, r10
	
	ldr  r9,  =0xFF
	and  r10, r7, r9		//G
	lsr  r10, #2
	lsl  r10, #21	
	orr  r11, r11, r10	
	
	ldr  r9,  =0xFF00
	and  r10, r7, r9		//R
	lsr  r10, #11
	lsl  r10, #27	
	orr  r11, r11, r10	
	mov  r5, r11

	// handle seventh 16bits ================
	mov  r11, #0
	ldr  r9,  =0xFF0000
	and  r10, r7, r9		//B
	lsr  r10, #19
	orr  r11, r11, r10	
	
	ldr  r9,  =0xFF000000
	and  r10, r7, r9		//G
	lsr  r10, #26
	lsl  r10, #5
	orr  r11, r11, r10		
	
	ldr  r9,  =0xFF
	and  r10, r8, r9		//R
	lsr  r10, #3
	lsl  r10, #11
	orr  r11, r11, r10		
	
	// handle eighth 16bits ================	
	ldr  r9,  =0xFF00
	and  r10, r8, r9		//B
	lsr  r10, #11
	lsl  r10, #16
	orr  r11, r11, r10		
	
	ldr  r9,  =0xFF0000
	and  r10, r8, r9		//G
	lsr  r10, #18
	lsl  r10, #21
	orr  r11, r11, r10			

	ldr  r9,  =0xFF000000
	and  r10, r8, r9		//R
	lsr  r10, #27
	lsl  r10, #27
	orr  r11, r11, r10
	mov  r6, r11
	
	stmia r0!, {r3-r6}
	subs r2, r2, #24
	bne	bit24_to_bit16_ali24_loop
	ldmfd sp!, {r0-r11}
	mov pc, lr	
	
bit24_to_bit16_ali32:
	stmfd sp!, {r0-r11}
bit24_to_bit16_ali32_copy:
	ldmia r1!, {r3-r8}
	// r11 : final data
	// r10 : tmp_reg
	// handle first 32bits ================
	ldr r9, =0xffffff
	and r11, r9, r3
	// handle second 32bits ================
	mov r10, r3, lsr #24
	mov r3, r11
	mov r9, r4  	//backup r4
	lsl r4, #16
	lsr r4, #8
	orr r4, r4, r10
	// handle third 32bits ================
	lsr r9, #16
	mov r10, r5 	//backup r5
	lsl r5, #24
	lsr r5, #8
	orr r5, r5, r9
	// handle fourth 32bits ================
	lsr r10, #8
	stmia r0!, {r3-r5, r10}
	
	// handle fiveth 32bits ================
	ldr r9, =0xffffff
	and r11, r9, r6
	// handle sixth 32bits ================
	mov r10, r6, lsr #24
	mov r6, r11
	mov r9, r7  	//backup r7
	lsl r7, #16
	lsr r7, #8
	orr r7, r7, r10
	// handle seventh 32bits ================
	lsr r9, #16
	mov r10, r8 	//backup r8
	lsl r8, #24
	lsr r8, #8
	orr r8, r8, r9
	// handle eigth 32bits ================
	lsr r10, #8
	stmia r0!, {r6-r8, r10}	
	
	subs r2, r2, #24
	bne	bit24_to_bit16_ali32_copy
	ldmfd sp!, {r0-r11}
	mov pc, lr	
	
	