    .intel_syntax noprefix
    .global gamma_correct_asm
    .global gamma_correct_asm_simd
    .global gamma_correct_asm_hash
    .global gamma_correct_asm_hash_simd

    .section .rodata

    /*
    both implementations use the same algorithm as void gamma_correct_c(uint8_t* inputContent, 
    int width, int height, float a, float b, float c, float gamma, uint8_t* outputContent) 
    from gamma_correct.c
    */

    // little endian is the curse of this world
    
    // the consts in .rodata are declared in little endian
    // note that comments that have examples are not in little endian just to make it a little more easy to understand
    // algorithmically the examples and the real code are the same

    // packed consts 255f, 1f, (0f, 1f, 2f, 3f), 4f
    .float_255:     .byte 0x00,0x00,0x7f,0x43, 0x00,0x00,0x7f,0x43, 0x00,0x00,0x7f,0x43, 0x00,0x00,0x7f,0x43
    .float_1:       .byte 0x00,0x00,0x80,0x3f, 0x00,0x00,0x80,0x3f, 0x00,0x00,0x80,0x3f, 0x00,0x00,0x80,0x3f
    .float_0123:    .byte 0x00,0x00,0x40,0x40, 0x00,0x00,0x00,0x40, 0x00,0x00,0x80,0x3f, 0x00,0x00,0x00,0x00
    .float_4444:    .byte 0x00,0x00,0x80,0x40, 0x00,0x00,0x80,0x40, 0x00,0x00,0x80,0x40, 0x00,0x00,0x80,0x40

    // byte masks used to shuffle r g b values into the required spaces of an xmm register
    // (one value per 32 bits, for a total of 4 values in one xmm register)
    .mask_r:        .byte 0x09,0xff,0xff,0xff, 0x06,0xff,0xff,0xff, 0x03,0xff,0xff,0xff, 0x00,0xff,0xff,0xff
    .mask_g:        .byte 0x0A,0xff,0xff,0xff, 0x07,0xff,0xff,0xff, 0x04,0xff,0xff,0xff, 0x01,0xff,0xff,0xff
    .mask_b:        .byte 0x0B,0xff,0xff,0xff, 0x08,0xff,0xff,0xff, 0x05,0xff,0xff,0xff, 0x02,0xff,0xff,0xff

    // byte mask to shuffles 4 uint_8 into one 32 bit chunk of an xmm register in order to write all 4 values to output at once
    // (one value per 8 bits for a total of 4 values in a r32 register)
    .mask_res:      .byte 0x0C,0x08,0x04,0x00, 0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff

    .text

/*
void gamma_correct_asm_simd(uint8_t* inputContent, 
    int width, int height, float a, float b, float c, float gamma, 
    uint8_t* outputContent);
*/
    .align 16
gamma_correct_asm_simd:
    /*
    rdi = input*
    rsi = width
    rdx = height
    rcx = output

    xmm0 = a
    xmm1 = b
    xmm2 = c
    xmm3 = gamma
    */

    // pack a b c
    MOVLHPS xmm0, xmm0
    MOVSLDUP xmm0, xmm0

    MOVLHPS xmm1, xmm1
    MOVSLDUP xmm1, xmm1

    MOVLHPS xmm2, xmm2
    MOVSLDUP xmm2, xmm2

    MOVLHPS xmm3, xmm3
    MOVSLDUP xmm3, xmm3

    // rax is counter (width * height)
    mov rax, rdx
    xor rdx, rdx
    mul rsi

    // null everything that is volatile before use
    xor r8d, r8d
    xor r9d, r9d
    xor r10d, r10d

    // load consts
    movdqu xmm10, [rip + .mask_res]
    movdqu xmm11, [rip + .mask_r]
    movdqu xmm12, [rip + .mask_g]
    movdqu xmm13, [rip + .mask_b]
    movdqu xmm14, [rip + .float_1]
    movdqu xmm15, [rip + .float_255]

    .Lsimdloop:
        // if a full load of 16 bytes (5 pixels and a stray r) isnt possible 
        // then jump to standard implementation
        cmp rax, 0x6
        jl .Lsimdcontinue

        // load next 16 bytes, only use 12 (4 pixels) because of bit limitations in xmm (128)
        // xmm5 = (r1,g1,b1, r2,g2,b2, r3,g3,b3, r4,g4,b4 r5,g5,b5, r6)
        movdqu xmm5, [rdi]
        add rdi, 12

        movdqu xmm6, xmm5
        movdqu xmm7, xmm5

        // shuffle bytes to get r, g and b into seperate registers
        // example: xmm5 = (r1,0,0,0,r2,0,0,0,r3,0,0,0,r4,0,0,0) with mask_r and after float conversion xmm5 = (r1f, r2f, r3f, r4f)
        // xmm5 = r, xmm6 = g, xmm7 = b
        pshufb xmm5, xmm11
        CVTDQ2PS xmm5, xmm5
        pshufb xmm6, xmm12
        CVTDQ2PS xmm6, xmm6
        pshufb xmm7, xmm13
        CVTDQ2PS xmm7, xmm7

        // calculate D packed in xmm5
        mulps xmm5, xmm0
        mulps xmm6, xmm1
        mulps xmm7, xmm2

        addps xmm5, xmm6
        addps xmm5, xmm7

        // no need to divide by a + b + c since coeffs are normalized
        // Done calculating D (Q x,y)

        // start gamma correction
        
        // Q = Q / 255
        divps xmm5, xmm15
        // prep for approximating ln(x) for 0 <= x <= 1 using Taylor series
        movdqu xmm9, xmm14
        // x = 1.0 - x
        subps xmm9, xmm5

        // load all 1.0s
        movdqu xmm5, xmm14
        movdqu xmm6, xmm14
        // make xmm8 = 0.0
        pxor xmm8, xmm8

        // 15 iterations seems to be cutoff for ln x approximation
        // lower this further and inaccuracy becomes noticable
        mov r8, 15
        mov r9, 1
        
        // approximate ln(x) for 0 <= x <= 1 using Taylor series
        // xmm9 = x
        // xmm5 = 1.0
        // xmm8 = sum
        // r9 and xmm6 (packed floats) = n starting at 1.0
        .Lcalclnsimd:
            // sum -= (x^n) / n
            mulps xmm5, xmm9
            movdqu xmm7, xmm5
            divps xmm7, xmm6
            subps xmm8, xmm7

            addps xmm6, xmm14
            inc r9
            cmp r9, r8
            jne .Lcalclnsimd

        // sum = sum * gamma
        mulps xmm8, xmm3

        // load all 1.0
        movdqu xmm5, xmm14
        movdqu xmm6, xmm14
        movdqu xmm9, xmm14

        // 18 iterations seems to be cutoff for e^x approximation
        // lower this further and inaccuracy becomes noticable
        mov r8, 18
        mov r9, 1

        // approximate e^x using Taylor series
        // calculate e^x
        // xmm8 = x
        // xmm5 = 1.0
        // xmm9 = sum
        // r9 and xmm6 (packed floats) = n starting at 1.0
        .Lcalcexpsimd:
            // sum += (x^n) / n!
            mulps xmm5, xmm8
            divps xmm5, xmm6
            addps xmm9, xmm5

            addps xmm6, xmm14
            inc r9
            cmp r9, r8
            jne .Lcalcexpsimd

        // if the result of e^x approximation is negativ just assume its 0
        pxor xmm4, xmm4
        CMPPS xmm4, xmm9, 2
        PAND xmm9, xmm4

        // Q' = sum * 255
        mulps xmm9, xmm15

        // done calculating gamma corrected pixels Q'

        // convert back to integers und shuffle them so that all 4 results are in bytes 0,1,2,3 of r8d
        CVTPS2DQ xmm9, xmm9
        pshufb xmm9, xmm10
        movq r8, xmm9
        // write 4 greyscaled and gamma corrected pixels to output
        mov [rcx], r8d
        add rcx, 4

        // dec counter and loop
        sub rax, 4
        jmp .Lsimdloop
    ret

/*
void gamma_correct_asm_hash_simd(uint8_t* inputContent, 
    int width, int height, float a, float b, float c, float gamma, 
    uint8_t* outputContent);
*/
gamma_correct_asm_hash_simd:
/*
    rdi = input*
    rsi = width
    rdx = height
    rcx = output

    xmm0 = a
    xmm1 = b
    xmm2 = c
    xmm3 = gamma
    */

    // pack gamma
    MOVLHPS xmm3, xmm3
    MOVSLDUP xmm3, xmm3

    // rax is counter (width * height)
    mov rax, rdx
    xor rdx, rdx
    mul rsi

    // null everything that is volatile before use
    xor r8d, r8d
    xor r9d, r9d
    xor r10d, r10d
    xor r11d, r11d
    xor rsi, rsi

    // hash table of 256 uint_8 in rsp
    // initialize values as gamma corrrected version of all possible inputs for gamma correction (0 to 255)
    // rsi is counter for this
    mov rsi, 0
    sub rsp, 256

    movdqu xmm10, [rip + .mask_res]
    movdqu xmm11, [rip + .float_0123]

    .Linitializesimd:
        // load packed counter
        movdqu xmm5, xmm11

        // Q = Q / 255
        divps xmm5, [rip + .float_255]

        // prep for approximating ln(x) for 0 <= x <= 1 using Taylor series
        movdqu xmm9, [rip + .float_1]
        // x = 1.0 - x
        subps xmm9, xmm5

        // load all 1.0s
        movdqu xmm5, [rip + .float_1]
        movdqu xmm6, [rip + .float_1]
        // make xmm8 = 0.0
        pxor xmm8, xmm8

        // 15 iterations seems to be cutoff for ln x approximation
        // lower this further and inaccuracy becomes noticable
        mov r8, 15
        mov r9, 1
        
        // approximate ln(x) for 0 <= x <= 1 using Taylor series
        // xmm9 = x
        // xmm5 = 1.0
        // xmm8 = sum
        // r9 and xmm6 (packed floats) = n starting at 1.0
        .Lcalclnhashsimd:
            // sum -= (x^n) / n
            mulps xmm5, xmm9
            movdqu xmm7, xmm5
            divps xmm7, xmm6
            subps xmm8, xmm7

            addps xmm6, [rip + .float_1]
            inc r9
            cmp r9, r8
            jne .Lcalclnhashsimd

        // sum = sum * gamma
        mulps xmm8, xmm3

        // load all 1.0s
        movdqu xmm5, [rip + .float_1]
        movdqu xmm6, [rip + .float_1]
        movdqu xmm9, [rip + .float_1]

        // 18 iterations seems to be cutoff for e^x approximation
        // lower this further and inaccuracy becomes noticable
        mov r8, 18
        mov r9, 1

        // approximate e^x using Taylor series
        // calculate e^x
        // xmm8 = x
        // xmm5 = 1.0
        // xmm9 = sum
        // r9 and xmm6 (packed floats) = n starting at 1.0
        .Lcalcexphashsimd:
            // sum += (x^n) / n!
            mulps xmm5, xmm8
            divps xmm5, xmm6
            addps xmm9, xmm5

            addps xmm6, [rip + .float_1]
            inc r9
            cmp r9, r8
            jne .Lcalcexphashsimd

        // if the result of exponentiation is negativ just assume its 0
        pxor xmm4, xmm4
        CMPPS xmm4, xmm9, 2
        PAND xmm9, xmm4

        // Q' = sum * 255
        mulps xmm9, [rip + .float_255]

        // convert back to integers und shuffle them so that all 4 results are in bytes 0,1,2,3 of r8d
        CVTPS2DQ xmm9, xmm9
        pshufb xmm9, xmm10
        movq r8, xmm9
        // write 4 greyscaled and gamma corrected pixels to hash table
        mov [rsp + rsi], r8d

        // increment counters and loop
        addps xmm11, [rip + .float_4444]
        add rsi, 4
        cmp rsi, 256
        jl .Linitializesimd

        jmp .Lhashsimdcontinue

        ret

/*
void gamma_correct_asm_hash(uint8_t* inputContent, 
    int width, int height, float a, float b, float c, float gamma, 
    uint8_t* outputContent)
*/
gamma_correct_asm_hash:
/*
    rdi = input*
    rsi = width
    rdx = height
    rcx = output

    xmm0 = a
    xmm1 = b
    xmm2 = c
    xmm3 = gamma
    */

    // rax is counter (width * height)
    mov rax, rdx
    xor rdx, rdx
    mul rsi

    // null everything that is volatile before use
    xor r8d, r8d
    xor r9d, r9d
    xor r10d, r10d
    xor r11d, r11d
    xor rsi, rsi

    // hash table of 256 uint_8 in rsp
    // initialize values as gamma corrrected version of all possible inputs for gamma correction (0 to 255)
    // rsi is counter for this
    mov rsi, 0
    sub rsp, 256
    .Linitialize:
        // convert counter to float
        CVTSI2SS xmm5, rsi

        // Q = Q / 255
        divss xmm5, [rip + .float_255]

        // prep for approximating ln(x) for 0 <= x <= 1 using Taylor series
        movdqu xmm9, [rip + .float_1]
        // x = 1.0 - x
        subss xmm9, xmm5

        // load all 1.0s
        movdqu xmm5, [rip + .float_1]
        movdqu xmm6, [rip + .float_1]
        // make xmm8 = 0.0
        pxor xmm8, xmm8

        // 15 iterations seems to be cutoff for ln x approximation
        // lower this further and inaccuracy becomes noticable
        mov r8, 15
        mov r9, 1
        
        // approximate ln(x) for 0 <= x <= 1 using Taylor series
        // xmm9 = x
        // xmm5 = 1.0
        // xmm8 = sum
        // r9 and xmm6 (packed floats) = n starting at 1.0
        .Lcalclnhash:
            // sum -= (x^n) / n
            mulss xmm5, xmm9
            movdqu xmm7, xmm5
            divss xmm7, xmm6
            subss xmm8, xmm7

            addss xmm6, [rip + .float_1]
            inc r9
            cmp r9, r8
            jne .Lcalclnhash

        // sum = sum * gamma
        mulss xmm8, xmm3

        // load all 1.0s
        movdqu xmm5, [rip + .float_1]
        movdqu xmm6, [rip + .float_1]
        movdqu xmm9, [rip + .float_1]

        // 18 iterations seems to be cutoff for e^x approximation
        // lower this further and inaccuracy becomes noticable
        mov r8, 18
        mov r9, 1

        // approximate e^x using Taylor series
        // calculate e^x
        // xmm8 = x
        // xmm5 = 1.0
        // xmm9 = sum
        // r9 and xmm6 (packed floats) = n starting at 1.0
        .Lcalcexphash:
            // sum += (x^n) / n!
            mulss xmm5, xmm8
            divss xmm5, xmm6
            addss xmm9, xmm5

            addss xmm6, [rip + .float_1]
            inc r9
            cmp r9, r8
            jne .Lcalcexphash

        // if the result of exponentiation is negativ just assume its 0
        pxor xmm4, xmm4
        CMPSS xmm4, xmm9, 2
        PAND xmm9, xmm4

        // Q' = sum * 255
        mulss xmm9, [rip + .float_255]

        // convert back to integer
        CVTSS2SI r8, xmm9
        // update hashtable with result
        mov [rsp + rsi*1], r8b

        inc rsi
        cmp rsi, 256
        jl .Linitialize

    .Lhashsimdcontinue:

    // null everything that is volatile before use
    xor r8d, r8d
    xor r9d, r9d
    xor r10d, r10d
    xor r11d, r11d

    // loop through all pixels and calculate their grgayscale value
    // use hashtable to quickly get gamma corrected pixel
    .Lhashloop:
        // check counter
        cmp rax, 0x0
        je .Lrethash

        xor r11d, r11d

        // load r, g, b values
        mov r8b, [rdi]
        inc rdi
        mov r9b, [rdi]
        inc rdi
        mov r10b, [rdi]
        inc rdi

        // convert uint_8 to float
        // xmm5 = r, xmm6 = g, xmm7 = b
        CVTSI2SS xmm5, r8
        CVTSI2SS xmm6, r9
        CVTSI2SS xmm7, r10

        // calculate D
        mulss xmm5, xmm0
        mulss xmm6, xmm1
        mulss xmm7, xmm2

        // convert back to ints to save us the expensive additon of floats
        CVTTSS2SI r11, xmm5
        CVTTSS2SI r9, xmm6
        CVTTSS2SI r8, xmm7

        add r11, r9
        add r11, r8
        
        // write the gamma corrected pixel to output by using the grayscaled pixel as key for the hashtable
        mov r8b, [rsp + r11*1]
        mov [rcx], r8b
        inc rcx
        dec rax
        jmp .Lhashloop

    .Lrethash:
        // free hashtable from stack
        add rsp, 256

        ret

/*
void gamma_correct_asm(uint8_t* inputContent, 
    int width, int height, float a, float b, float c, float gamma, 
    uint8_t* outputContent);
*/
gamma_correct_asm:

    /*
    rdi = input*
    rsi = width
    rdx = height
    rcx = output

    xmm0 = a
    xmm1 = b
    xmm2 = c
    xmm3 = gamma
    */

    // rax is counter (width * height)
    mov rax, rdx
    xor rdx, rdx
    mul rsi

    .Lsimdcontinue:

    // null everything that is volatile before use
    xor r8d, r8d
    xor r9d, r9d
    xor r10d, r10d
    
    // the same loop as the simd implementation but without the bytemasks
    .Lmainloop:
        // check counter
        cmp rax, 0x0
        je .Lret

        // load r, g, b values
        mov r8b, [rdi]
        inc rdi
        mov r9b, [rdi]
        inc rdi
        mov r10b, [rdi]
        inc rdi

        // convert uint_8 to float
        // xmm5 = r, xmm6 = g, xmm7 = b
        CVTSI2SS xmm5, r8
        CVTSI2SS xmm6, r9
        CVTSI2SS xmm7, r10

        // calculate D
        mulss xmm5, xmm0
        mulss xmm6, xmm1
        mulss xmm7, xmm2

        addss xmm5, xmm6
        addss xmm5, xmm7
        
        // Q = Q / 255
        divss xmm5, [rip + .float_255]

        // prep for approximating ln(x) for 0 <= x <= 1 using Taylor series
        movdqu xmm9, [rip + .float_1]
        // x = 1.0 - x
        subss xmm9, xmm5

        // load all 1.0s
        movdqu xmm5, [rip + .float_1]
        movdqu xmm6, [rip + .float_1]
        // make xmm8 = 0.0
        pxor xmm8, xmm8

        // 15 iterations seems to be cutoff for ln x approximation
        // lower this further and inaccuracy becomes noticable
        mov r8, 15
        mov r9, 1
        
        // approximate ln(x) for 0 <= x <= 1 using Taylor series
        // xmm9 = x
        // xmm5 = 1.0
        // xmm8 = sum
        // r9 and xmm6 (packed floats) = n starting at 1.0
        .Lcalcln:
            // sum -= (x^n) / n
            mulss xmm5, xmm9
            movdqu xmm7, xmm5
            divss xmm7, xmm6
            subss xmm8, xmm7

            addss xmm6, [rip + .float_1]
            inc r9
            cmp r9, r8
            jne .Lcalcln

        // sum = sum * gamma
        mulss xmm8, xmm3

        // load all 1.0s
        movdqu xmm5, [rip + .float_1]
        movdqu xmm6, [rip + .float_1]
        movdqu xmm9, [rip + .float_1]

        // 18 iterations seems to be cutoff for e^x approximation
        // lower this further and inaccuracy becomes noticable
        mov r8, 18
        mov r9, 1

        // approximate e^x using Taylor series
        // calculate e^x
        // xmm8 = x
        // xmm5 = 1.0
        // xmm9 = sum
        // r9 and xmm6 (packed floats) = n starting at 1.0
        .Lcalcexp:
            // sum += (x^n) / n!
            mulss xmm5, xmm8
            divss xmm5, xmm6
            addss xmm9, xmm5

            addss xmm6, [rip + .float_1]
            inc r9
            cmp r9, r8
            jne .Lcalcexp

        // if the result of exponentiation is negativ just assume its 0
        pxor xmm4, xmm4
        CMPSS xmm4, xmm9, 2
        PAND xmm9, xmm4

        // Q' = sum * 255
        mulss xmm9, [rip + .float_255]

        // done calculating gamma corrected pixel Q'

        // convert back to integer
        CVTSS2SI r8, xmm9
        // write grayscaled and gamma corrected pixel to output
        mov [rcx], r8b
        inc rcx
        
        // dec counter and loop
        dec rax
        jmp .Lmainloop
    .Lret:
    
    ret