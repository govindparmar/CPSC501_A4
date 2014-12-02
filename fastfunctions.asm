.386
.XMM
.MODEL FLAT, C
.FARDATA
.CONST
dbl_val DQ 040dfe98000000000r ; 37268.0
; Fast swap algorithm for doubles
; Written in assembly to avoid frame pointer overhead
; Arguments:
;  [ESP+4] = First element to swap
;  [ESP+8] = Second element to swap
; Result: 
;  The values in [ESP+4] and [ESP+8] are exchanged
; Note:
;  XCHG instruction does not work on non-integral values!
.CODE
swap PROC
	; Load the function arguments into the register EAX and ECX
	mov 	eax, dword ptr [esp+8]
	mov 	ecx, dword ptr [esp+4]
	; Use XMM registers as temporary place-holders (XMM0<-EAX; XMM1<-ECX)
	movsd 	xmm0, qword ptr [eax]
	movsd 	xmm1, qword ptr [ecx]
	; And then put the values pointed to back into EAX and ECX (but in reverse - EAX<-XMM1; ECX<-XMM0)
	movsd 	qword ptr [ecx], xmm0
	movsd 	qword ptr [eax], xmm1
	; Return (unsafe in CPL=0 but in user-mode we should be good)
	ret
swap ENDP

; Max of 2 32-bit integers
; Written in assembly to avoid frame pointer overhead
; Arguments:
;  [ESP+4] = First of two values to check
;  [ESP+8] = Second of two values to check
; Result:
;  The largest one of [ESP+4] and [ESP+8] in EAX
max2 PROC
	; Load arg #1 into EAX
	mov eax, [esp+4]
	; Compare it to the other arg 
	cmp eax, [esp+8]
	; If EAX is less than the arg compared to, replace EAX with the second argument
	; Otherwise just return because EAX already has the right value
	jl r_case2
	ret
	; Case #2; second arg bigger = Put arg #2 in EAX and return
	r_case2:
	mov eax, [esp+8]
	ret
max2 ENDP

; Normalizes the doubles in the signal array
; Ported over to assembly to:
; #1 - Use additional register (EBX) instead of memory for increased speed
; #2 - Use the SIMD instructions for the double-divide operation
normalize PROC
	; No frame pointer omission here - too unsafe to do that with variable length arrays
	push	ebp
	mov	ebp, esp
	; Register EBX must be preserved when being used
	; EBX will store our loop counter
	push    ebx
	mov ebx, dword ptr [ebp-4]
	; Which is initially 0
	xor ebx, ebx
	; Loop body
l_body:
	; Check if our counting register is greater than the # of times we want to iterate
	; If so, we're done
	cmp ebx, dword ptr [ebp+16]
	jge	l_done
	; Otherwise...
	; Load the value from the short array [ebp+12] into eax
	mov	edx, ebx
	mov	eax, dword ptr [ebp+12]
	; Load that value into ecx
	movsx	ecx, word ptr [eax+edx*2]
	; Convert signed integer to signed double (store in xmm0)
	cvtsi2sd xmm0, ecx
	; Perform division by 37268.0
	divsd	xmm0, qword ptr dbl_val
	; Now load up the array of doubles (output array)
	mov	edx, ebx
	mov	eax, dword ptr [ebp+8]
	; And put our newly calculated result back in that array
	movsd	qword ptr [eax+edx*8], xmm0
	; Increment our loop counter
	add ebx, 1
	; Back up the top
	jmp	short l_body
l_done:
	; Restore stack & leave
	mov	esp, ebp
	pop	ebp
	ret
normalize ENDP
END
