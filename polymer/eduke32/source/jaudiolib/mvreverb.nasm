;Copyright (C) 1994-1995 Apogee Software, Ltd.
;
;This program is free software; you can redistribute it and/or
;modify it under the terms of the GNU General Public License
;as published by the Free Software Foundation; either version 2
;of the License, or (at your option) any later version.
;
;This program is distributed in the hope that it will be useful,
;but WITHOUT ANY WARRANTY; without even the implied warranty of
;MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
;
;See the GNU General Public License for more details.
;
;You should have received a copy of the GNU General Public License
;along with this program; if not, write to the Free Software
;Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
;
;Modifications for JonoF's port by Jonathon Fowler (jonof@edgenetwk.com)

CPU 386

SECTION .data

%ifdef UNDERSCORES

%define MV_16BitReverb		_MV_16BitReverb
%define MV_8BitReverb		_MV_8BitReverb
%define MV_16BitReverbFast	_MV_16BitReverbFast
%define MV_8BitReverbFast	_MV_8BitReverbFast

%endif

	GLOBAL MV_16BitReverb
	GLOBAL MV_8BitReverb
	GLOBAL MV_16BitReverbFast
	GLOBAL MV_8BitReverbFast

%define OFFSET

;================
;
; MV_16BitReverb
;
;================

; eax - source position
; edx - destination position
; ebx - Volume table
; ecx - number of samples

	ALIGN 4
MV_16BitReverb:
	pushad
	mov     eax, dword [esp + 0*4 + 9*4]
	mov     edx, dword [esp + 1*4 + 9*4]
	mov     ebx, dword [esp + 2*4 + 9*4]
	mov     ecx, dword [esp + 3*4 + 9*4]

        mov     esi, eax
        lea     edi, [edx - 2]

        ALIGN 4
rev16loop:
        movzx   eax, word [esi]                 ; get sample
        add     edi, 2

        movzx   edx, ah
        sub     ah, ah

        movsx   eax, byte [2*eax+ebx+1]         ; volume translate low byte of sample
        xor     edx, 80h

        movsx   edx, word [2*edx+ebx]           ; volume translate high byte of sample
        add     esi, 2

        lea     eax, [ eax + edx + 80h ]        ; mix high byte of sample
        dec     ecx                             ; decrement count

        mov     word [edi], ax                  ; write new sample to destination
        jnz     rev16loop                       ; loop

	popad
        ret


;================
;
; MV_8BitReverb
;
;================

; eax - source position
; edx - destination position
; ebx - Volume table
; ecx - number of samples

	ALIGN 4
MV_8BitReverb:
	pushad
	mov     eax, dword [esp + 0*4 + 9*4]
	mov     edx, dword [esp + 1*4 + 9*4]
	mov     ebx, dword [esp + 2*4 + 9*4]
	mov     ecx, dword [esp + 3*4 + 9*4]

        mov     esi, eax
        lea     edi, [edx - 1]

        xor     eax, eax

        ALIGN 4
rev8loop:
;        movzx   eax, byte ptr [esi]             ; get sample
        mov     al, byte [esi]                  ; get sample
        inc     edi

;        movsx   eax, byte ptr [2*eax+ebx]       ; volume translate sample
        mov     al, byte [2*eax+ebx]            ; volume translate sample
        inc     esi

;        add     eax, 80h
        add     al, 80h
        dec     ecx                             ; decrement count

        mov     byte [edi], al                  ; write new sample to destination
        jnz     rev8loop                        ; loop

	popad
        ret


;================
;
; MV_16BitReverbFast
;
;================

; eax - source position
; edx - destination position
; ebx - number of samples
; ecx - shift

	ALIGN 4
MV_16BitReverbFast:
	pushad
	mov     eax, dword [esp + 0*4 + 9*4]
	mov     edx, dword [esp + 1*4 + 9*4]
	mov     ebx, dword [esp + 2*4 + 9*4]
	mov     ecx, dword [esp + 3*4 + 9*4]

        mov     esi, eax
        mov     eax,OFFSET rpatch16+3

        mov     byte [eax],cl
        lea     edi, [edx - 2]

        ALIGN 4
frev16loop:
        mov     ax, word [esi]                  ; get sample
        add     edi, 2

rpatch16:
        sar     ax, 5    ;;;;Add 1 before shift
        add     esi, 2

        mov     word [edi], ax                  ; write new sample to destination
        dec     ebx                             ; decrement count

        jnz     frev16loop                      ; loop

	popad
        ret


;================
;
; MV_8BitReverbFast
;
;================

; eax - source position
; edx - destination position
; ebx - number of samples
; ecx - shift

	ALIGN 4
MV_8BitReverbFast:
	pushad
	mov     eax, dword [esp + 0*4 + 9*4]
	mov     edx, dword [esp + 1*4 + 9*4]
	mov     ebx, dword [esp + 2*4 + 9*4]
	mov     ecx, dword [esp + 3*4 + 9*4]

	mov     esi, eax
        mov     eax,OFFSET rpatch8+2

        mov     edi, edx
        mov     edx, 80h

        mov     byte [eax],cl
        mov     eax, 80h

        shr     eax, cl

        dec     edi
        sub     edx, eax

        ALIGN 4
frev8loop:
        mov     al, byte [esi]                  ; get sample
        inc     esi

        mov     ecx, eax
        inc     edi

rpatch8:
        shr     eax, 3
        xor     ecx, 80h                        ; flip the sign bit

        shr     ecx, 7                          ; shift the sign down to 1
        add     eax, edx

        add     eax, ecx                        ; add sign bit to round to 0
        dec     ebx                             ; decrement count

        mov     byte [edi], al                  ; write new sample to destination
        jnz     frev8loop                       ; loop

	popad
        ret

