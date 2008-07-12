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

%define MV_Mix8BitMono		_MV_Mix8BitMono
%define MV_Mix8BitStereo	_MV_Mix8BitStereo
%define MV_Mix16BitMono		_MV_Mix16BitMono
%define MV_Mix16BitStereo	_MV_Mix16BitStereo

%else

%define _MV_HarshClipTable	MV_HarshClipTable
%define _MV_MixDestination	MV_MixDestination
%define _MV_MixPosition		MV_MixPosition
%define _MV_LeftVolume		MV_LeftVolume
%define _MV_RightVolume		MV_RightVolume
%define _MV_SampleSize		MV_SampleSize
%define _MV_RightChannelOffset	MV_RightChannelOffset

%endif

	EXTERN _MV_HarshClipTable
	EXTERN _MV_MixDestination
	EXTERN _MV_MixPosition
	EXTERN _MV_LeftVolume
	EXTERN _MV_RightVolume
	EXTERN _MV_SampleSize
	EXTERN _MV_RightChannelOffset

	GLOBAL MV_Mix8BitMono
	GLOBAL MV_Mix8BitStereo
	GLOBAL MV_Mix16BitMono
	GLOBAL MV_Mix16BitStereo


;================
;
; MV_Mix8BitMono
;
;================

; eax - position
; edx - rate
; ebx - start
; ecx - number of samples to mix

	ALIGN 4
MV_Mix8BitMono:
; Two at once
        pushad
	mov     eax, dword [esp + 0*4 + 9*4]
	mov     edx, dword [esp + 1*4 + 9*4]
	mov     ebx, dword [esp + 2*4 + 9*4]
	mov     ecx, dword [esp + 3*4 + 9*4]

        mov     ebp, eax

        mov     esi, ebx                        ; Source pointer

        ; Sample size
        mov     ebx, dword [_MV_SampleSize]
        mov     byte [apatch7+2],bl
        mov     byte [apatch8+2],bl
        mov     byte [apatch9+3],bl

        ; Volume table ptr
        mov     ebx, dword [_MV_LeftVolume]     ; Since we're mono, use left volume
        mov     dword [apatch1+4],ebx
        mov     dword [apatch2+4],ebx

        ; Harsh Clip table ptr
        mov     ebx, dword [_MV_HarshClipTable]
        add     ebx, 128
        mov     dword [apatch3+2],ebx
        mov     dword [apatch4+2],ebx

        ; Rate scale ptr
        mov     dword [apatch5+2],edx
        mov     dword [apatch6+2],edx

        mov     edi, dword [_MV_MixDestination] ; Get the position to write to

        ; Number of samples to mix
        shr     ecx, 1                          ; double sample count
        cmp     ecx, 0
        je      short exit8m

;     eax - scratch
;     ebx - scratch
;     edx - scratch
;     ecx - count
;     edi - destination
;     esi - source
;     ebp - frac pointer
; apatch1 - volume table
; apatch2 - volume table
; apatch3 - harsh clip table
; apatch4 - harsh clip table
; apatch5 - sample rate
; apatch6 - sample rate

        mov     eax,ebp                         ; begin calculating first sample
        add     ebp,edx                         ; advance frac pointer
        shr     eax,16                          ; finish calculation for first sample

        mov     ebx,ebp                         ; begin calculating second sample
        add     ebp,edx                         ; advance frac pointer
        shr     ebx,16                          ; finish calculation for second sample

        movzx   eax, byte [esi+eax]             ; get first sample
        movzx   ebx, byte [esi+ebx]             ; get second sample

        ALIGN 4
mix8Mloop:
        movzx   edx, byte [edi]                 ; get current sample from destination
apatch1:
        movsx   eax, byte [2*eax+12345678h]     ; volume translate first sample
apatch2:
        movsx   ebx, byte [2*ebx+12345678h]     ; volume translate second sample
        add     eax, edx                        ; mix first sample
apatch9:
        movzx   edx, byte [edi + 1]             ; get current sample from destination
apatch3:
        mov     eax, dword [eax + 12345678h]    ; harsh clip new sample
        add     ebx, edx                        ; mix second sample
        mov     byte [edi], al                  ; write new sample to destination
        mov     edx, ebp                        ; begin calculating third sample
apatch4:
        mov     ebx, dword [ebx + 12345678h]    ; harsh clip new sample
apatch5:
        add     ebp,12345678h                   ; advance frac pointer
        shr     edx, 16                         ; finish calculation for third sample
        mov     eax, ebp                        ; begin calculating fourth sample
apatch7:
        add     edi, 1                          ; move destination to second sample
        shr     eax, 16                         ; finish calculation for fourth sample
        mov     byte [edi], bl                  ; write new sample to destination
apatch6:
        add     ebp,12345678h                   ; advance frac pointer
        movzx   ebx, byte [esi+eax]             ; get fourth sample
        movzx   eax, byte [esi+edx]             ; get third sample
apatch8:
        add     edi, 2                          ; move destination to third sample
        dec     ecx                             ; decrement count
        jnz     mix8Mloop                       ; loop

        mov     dword [_MV_MixDestination], edi ; Store the current write position
        mov     dword [_MV_MixPosition], ebp    ; return position
exit8m:
        popad
        ret


;================
;
; MV_Mix8BitStereo
;
;================

; eax - position
; edx - rate
; ebx - start
; ecx - number of samples to mix

	ALIGN 4
MV_Mix8BitStereo:
        pushad
	mov     eax, dword [esp + 0*4 + 9*4]
	mov     edx, dword [esp + 1*4 + 9*4]
	mov     ebx, dword [esp + 2*4 + 9*4]
	mov     ecx, dword [esp + 3*4 + 9*4]
	
        mov     ebp, eax

        mov     esi, ebx                        ; Source pointer

        ; Sample size
        mov     ebx, dword [_MV_SampleSize]
        mov     byte [bpatch8+2],bl

        ; Right channel offset
        mov     ebx, dword [_MV_RightChannelOffset]
        mov     dword [bpatch6+3],ebx
        mov     dword [bpatch7+2],ebx

        ; Volume table ptr
        mov     ebx, dword [_MV_LeftVolume]
        mov     dword [bpatch1+4],ebx

        mov     ebx, dword [_MV_RightVolume]
        mov     dword [bpatch2+4],ebx

        ; Rate scale ptr
        mov     dword [bpatch3+2],edx

        ; Harsh Clip table ptr
        mov     ebx, dword [_MV_HarshClipTable]
        add     ebx,128
        mov     dword [bpatch4+2],ebx
        mov     dword [bpatch5+2],ebx

        mov     edi, dword [_MV_MixDestination] ; Get the position to write to

        ; Number of samples to mix
        cmp     ecx, 0
        je      short EXIT8S

;     eax - scratch
;     ebx - scratch
;     edx - scratch
;     ecx - count
;     edi - destination
;     esi - source
;     ebp - frac pointer
; bpatch1 - left volume table
; bpatch2 - right volume table
; bpatch3 - sample rate
; bpatch4 - harsh clip table
; bpatch5 - harsh clip table

        mov     eax,ebp                         ; begin calculating first sample
        shr     eax,16                          ; finish calculation for first sample

        movzx   ebx, byte [esi+eax]             ; get first sample

        ALIGN 4
mix8Sloop:
bpatch1:
        movsx   eax, byte [2*ebx+12345678h]     ; volume translate left sample
        movzx   edx, byte [edi]                 ; get current sample from destination
bpatch2:
        movsx   ebx, byte [2*ebx+12345678h]     ; volume translate right sample
        add     eax, edx                        ; mix left sample
bpatch3:
        add     ebp,12345678h                   ; advance frac pointer
bpatch6:
        movzx   edx, byte [edi+12345678h]       ; get current sample from destination
bpatch4:
        mov     eax, dword [eax + 12345678h]    ; harsh clip left sample
        add     ebx, edx                        ; mix right sample
        mov     byte [edi], al                  ; write left sample to destination
bpatch5:
        mov     ebx, dword [ebx + 12345678h]    ; harsh clip right sample
        mov     edx, ebp                        ; begin calculating second sample
bpatch7:
        mov     byte [edi+12345678h], bl        ; write right sample to destination
        shr     edx, 16                         ; finish calculation for second sample
bpatch8:
        add     edi, 2                          ; move destination to second sample
        movzx   ebx, byte [esi+edx]             ; get second sample
        dec     ecx                             ; decrement count
        jnz     mix8Sloop                       ; loop

        mov     dword [_MV_MixDestination], edi ; Store the current write position
        mov     dword [_MV_MixPosition], ebp    ; return position

EXIT8S:
        popad
        ret


;================
;
; MV_Mix16BitMono
;
;================

; eax - position
; edx - rate
; ebx - start
; ecx - number of samples to mix

	ALIGN 4
MV_Mix16BitMono:
; Two at once
        pushad
	mov     eax, dword [esp + 0*4 + 9*4]
	mov     edx, dword [esp + 1*4 + 9*4]
	mov     ebx, dword [esp + 2*4 + 9*4]
	mov     ecx, dword [esp + 3*4 + 9*4]

        mov     ebp, eax

        mov     esi, ebx                        ; Source pointer

        ; Sample size
        mov     ebx, dword [_MV_SampleSize]
        mov     byte [cpatch5+3],bl
        mov     byte [cpatch6+3],bl
        add     bl,bl
        mov     byte [cpatch7+2],bl

        ; Volume table ptr
        mov     ebx, dword [_MV_LeftVolume]
        mov     dword [cpatch1+4],ebx
        mov     dword [cpatch2+4],ebx

        ; Rate scale ptr
        mov     dword [cpatch3+2],edx
        mov     dword [cpatch4+2],edx

        mov     edi, dword [_MV_MixDestination] ; Get the position to write to

        ; Number of samples to mix
        shr     ecx, 1                          ; double sample count
        cmp     ecx, 0
        je      near EXIT16M

;     eax - scratch
;     ebx - scratch
;     edx - scratch
;     ecx - count
;     edi - destination
;     esi - source
;     ebp - frac pointer
; cpatch1 - volume table
; cpatch2 - volume table
; cpatch3 - sample rate
; cpatch4 - sample rate

        mov     eax,ebp                         ; begin calculating first sample
        add     ebp,edx                         ; advance frac pointer
        shr     eax,16                          ; finish calculation for first sample

        mov     ebx,ebp                         ; begin calculating second sample
        add     ebp,edx                         ; advance frac pointer
        shr     ebx,16                          ; finish calculation for second sample

        movzx   eax, byte [esi+eax]             ; get first sample
        movzx   ebx, byte [esi+ebx]             ; get second sample

        ALIGN 4
mix16Mloop:
        movsx   edx, word [edi]                 ; get current sample from destination
cpatch1:
        movsx   eax, word [2*eax+12345678h]     ; volume translate first sample
cpatch2:
        movsx   ebx, word [2*ebx+12345678h]     ; volume translate second sample
        add     eax, edx                        ; mix first sample
cpatch5:
        movsx   edx, word [edi + 2]             ; get current sample from destination

        cmp     eax, -32768                     ; Harsh clip sample
        jge     short m16skip1
        mov     eax, -32768
        jmp     short m16skip2
m16skip1:
        cmp     eax, 32767
        jle     short m16skip2
        mov     eax, 32767
m16skip2:
        add     ebx, edx                        ; mix second sample
        mov     word [edi], ax                  ; write new sample to destination
        mov     edx, ebp                        ; begin calculating third sample

        cmp     ebx, -32768                     ; Harsh clip sample
        jge     short m16skip3
        mov     ebx, -32768
        jmp     short m16skip4
m16skip3:
        cmp     ebx, 32767
        jle     short m16skip4
        mov     ebx, 32767
m16skip4:
cpatch3:
        add     ebp,12345678h                   ; advance frac pointer
        shr     edx, 16                         ; finish calculation for third sample
        mov     eax, ebp                        ; begin calculating fourth sample
cpatch6:
        mov     word [edi + 2], bx              ; write new sample to destination
        shr     eax, 16                         ; finish calculation for fourth sample

cpatch4:
        add     ebp,12345678h                   ; advance frac pointer
        movzx   ebx, byte [esi+eax]             ; get fourth sample
cpatch7:
        add     edi, 4                          ; move destination to third sample
        movzx   eax, byte [esi+edx]             ; get third sample
        dec     ecx                             ; decrement count
        jnz     mix16Mloop                      ; loop

        mov     dword [_MV_MixDestination], edi ; Store the current write position
        mov     dword [_MV_MixPosition], ebp    ; return position
EXIT16M:
        popad
        ret


;================
;
; MV_Mix16BitStereo
;
;================

; eax - position
; edx - rate
; ebx - start
; ecx - number of samples to mix

	ALIGN 4
MV_Mix16BitStereo:
        pushad
	mov     eax, dword [esp + 0*4 + 9*4]
	mov     edx, dword [esp + 1*4 + 9*4]
	mov     ebx, dword [esp + 2*4 + 9*4]
	mov     ecx, dword [esp + 3*4 + 9*4]
	
        mov     ebp, eax

        mov     esi, ebx                        ; Source pointer

        ; Sample size
        mov     ebx, dword [_MV_SampleSize]
        mov     byte [dpatch6+2],bl

        ; Right channel offset
        mov     ebx, dword [_MV_RightChannelOffset]
        mov     dword [dpatch4+3],ebx
        mov     dword [dpatch5+3],ebx

        ; Volume table ptr
        mov     ebx, dword [_MV_LeftVolume]
        mov     dword [dpatch1+4],ebx

        mov     ebx, dword [_MV_RightVolume]
        mov     dword [dpatch2+4],ebx

        ; Rate scale ptr
        mov     dword [dpatch3+2],edx

        mov     edi, dword [_MV_MixDestination] ; Get the position to write to

        ; Number of samples to mix
        cmp     ecx, 0
        je      near exit16S

;     eax - scratch
;     ebx - scratch
;     edx - scratch
;     ecx - count
;     edi - destination
;     esi - source
;     ebp - frac pointer
; dpatch1 - left volume table
; dpatch2 - right volume table
; dpatch3 - sample rate

        mov     eax,ebp                         ; begin calculating first sample
        shr     eax,16                          ; finish calculation for first sample

        movzx   ebx, byte [esi+eax]             ; get first sample

        ALIGN 4
mix16Sloop:
dpatch1:
        movsx   eax, word [2*ebx+12345678h]     ; volume translate left sample
        movsx   edx, word [edi]                 ; get current sample from destination
dpatch2:
        movsx   ebx, word [2*ebx+12345678h]     ; volume translate right sample
        add     eax, edx                        ; mix left sample
dpatch3:
        add     ebp,12345678h                   ; advance frac pointer
dpatch4:
        movsx   edx, word [edi+12345678h]       ; get current sample from destination

        cmp     eax, -32768                     ; Harsh clip sample
        jge     short s16skip1
        mov     eax, -32768
        jmp     short s16skip2
s16skip1:
        cmp     eax, 32767
        jle     short s16skip2
        mov     eax, 32767
s16skip2:
        add     ebx, edx                        ; mix right sample
        mov     word [edi], ax                  ; write left sample to destination

        cmp     ebx, -32768                     ; Harsh clip sample
        jge     short s16skip3
        mov     ebx, -32768
        jmp     short s16skip4
s16skip3:
        cmp     ebx, 32767
        jle     short s16skip4
        mov     ebx, 32767
s16skip4:

        mov     edx, ebp                        ; begin calculating second sample
dpatch5:
        mov     word [edi+12345678h], bx        ; write right sample to destination
        shr     edx, 16                         ; finish calculation for second sample
dpatch6:
        add     edi, 4                          ; move destination to second sample
        movzx   ebx, byte [esi+edx]             ; get second sample
        dec     ecx                             ; decrement count
        jnz     mix16Sloop                      ; loop

        mov     dword [_MV_MixDestination], edi ; Store the current write position
        mov     dword [_MV_MixPosition], ebp    ; return position
exit16S:
        popad
        ret

