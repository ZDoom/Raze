//
// Microsoft C inline assembler
// 

//{{{

#ifdef __pragmas_h__
#ifndef __pragmas_x86_h__
#define __pragmas_x86_h__

static __inline int32_t sqr(int32_t a)
{
    _asm {
        mov eax, a
            imul eax, eax
    }
}

static __inline int32_t scale(int32_t a, int32_t d, int32_t c)
{
    _asm {
        mov eax, a
            imul d
            idiv c
    }
}

static __inline int32_t mulscale(int32_t a, int32_t d, int32_t c)
{
    _asm {
        mov ecx, c
            mov eax, a
            imul d
            shrd eax, edx, cl
    }
}

#define _scaler(x) \
static __inline int32_t mulscale##x (int32_t a, int32_t d) \
{ \
	_asm mov eax, a \
	_asm imul d \
	_asm shrd eax, edx, x \
} \
static __inline int32_t dmulscale##x (int32_t a, int32_t d, int32_t S, int32_t D) \
{ \
	_asm mov eax, a \
	_asm imul d \
	_asm mov ebx, eax \
	_asm mov eax, S \
	_asm mov esi, edx \
	_asm imul D \
	_asm add eax, ebx \
	_asm adc edx, esi \
	_asm shrd eax, edx, x \
} \


PRAGMA_FUNCS 
#undef _scaler

static __inline int32_t mulscale32(int32_t a, int32_t d)
{
    _asm {
        mov eax, a
            imul d
            mov eax, edx
    }
}

static __inline int32_t dmulscale(int32_t a, int32_t d, int32_t S, int32_t D, int32_t c)
{
    _asm {
        mov ecx, c
            mov eax, a
            imul d
            mov ebx, eax
            mov eax, S
            mov esi, edx
            imul D
            add eax, ebx
            adc edx, esi
            shrd eax, edx, cl
    }
}

static __inline int32_t dmulscale32(int32_t a, int32_t d, int32_t S, int32_t D)
{
    _asm {
        mov eax, a
            imul d
            mov ebx, eax
            mov eax, S
            mov esi, edx
            imul D
            add eax, ebx
            adc edx, esi
            mov eax, edx
    }
}

#ifdef USE_ASM_DIVSCALE
static __inline int32_t divscale(int32_t a, int32_t b, int32_t c)
{
    _asm {
        mov eax, a
            mov ecx, c
            mov edx, eax
            shl eax, cl
            neg cl
            sar edx, cl
            idiv b
    }
}

static __inline int32_t divscale1(int32_t a, int32_t b)
{
    _asm {
        mov eax, a
            add eax, eax
            sbb edx, edx
            idiv b
    }
}

static __inline int32_t divscale2(int32_t a, int32_t b)
{
    _asm {
        mov eax, a
            mov edx, eax
            sar edx, 30
            lea eax, [eax*4]
            idiv b
    }
}

static __inline int32_t divscale3(int32_t a, int32_t b)
{
    _asm {
        mov eax, a
            mov edx, eax
            sar edx, 29
            lea eax, [eax*8]
            idiv b
    }
}

#define DIVSCALE(x,y) \
static __inline int32_t divscale##y(int32_t a, int32_t b) \
{ \
	_asm mov eax, a \
	_asm mov edx, eax \
	_asm sar edx, x \
	_asm shl eax, y \
	_asm idiv b \
}

DIVSCALE(28, 4)	DIVSCALE(27, 5)	DIVSCALE(26, 6)	DIVSCALE(25, 7)
DIVSCALE(24, 8)	DIVSCALE(23, 9)	DIVSCALE(22, 10)	DIVSCALE(21, 11)
DIVSCALE(20, 12)	DIVSCALE(19, 13)	DIVSCALE(18, 14)	DIVSCALE(17, 15)
DIVSCALE(16, 16)	DIVSCALE(15, 17)	DIVSCALE(14, 18)	DIVSCALE(13, 19)
DIVSCALE(12, 20)	DIVSCALE(11, 21)	DIVSCALE(10, 22)	DIVSCALE(9, 23)
DIVSCALE(8, 24)	DIVSCALE(7, 25)	DIVSCALE(6, 26)	DIVSCALE(5, 27)
DIVSCALE(4, 28)	DIVSCALE(3, 29)	DIVSCALE(2, 30)	DIVSCALE(1, 31)

static __inline int32_t divscale32(int32_t d, int32_t b)
{
    _asm {
        mov edx, d
            xor eax, eax
            idiv b
    }
}
#endif  // defined USE_ASM_DIVSCALE

static __inline char readpixel(void *d)
{
    _asm {
        mov edx, d
            mov al, byte ptr[edx]
    }
}

static __inline void drawpixel(void *d, char a)
{
    _asm {
        mov edx, d
            mov al, a
            mov byte ptr[edx], al
    }
}

static __inline void drawpixels(void *d, int16_t a)
{
    _asm {
        mov edx, d
            mov ax, a
            mov word ptr[edx], ax
    }
}

static __inline void drawpixelses(void *d, int32_t a)
{
    _asm {
        mov edx, d
            mov eax, a
            mov dword ptr[edx], eax
    }
}

static __inline void clearbuf(void *d, int32_t c, int32_t a)
{
    _asm {
        mov edi, d
            mov ecx, c
            mov eax, a
            rep stosd
    }
}

static __inline void clearbufbyte(void *d, int32_t c, int32_t a)
{
    _asm {
        mov edi, d
            mov ecx, c
            mov eax, a
            cmp ecx, 4
            jae longcopy
            test cl, 1
            jz preskip
            stosb
        preskip :
        shr ecx, 1
            rep stosw
            jmp endit
        longcopy :
        test edi, 1
            jz skip1
            stosb
            dec ecx
        skip1 :
        test edi, 2
            jz skip2
            stosw
            sub ecx, 2
        skip2 :
              mov ebx, ecx
              shr ecx, 2
              rep stosd
              test bl, 2
              jz skip3
              stosw
          skip3 :
        test bl, 1
            jz endit
            stosb
        endit :
    }
}

static __inline void copybuf(const void *s, void *d, int32_t c)
{
    _asm {
        mov esi, s
            mov edi, d
            mov ecx, c
            rep movsd
    }
}

static __inline void copybufbyte(const void *s, void *d, int32_t c)
{
    _asm {
        mov esi, s
            mov edi, d
            mov ecx, c
            cmp ecx, 4
            jae longcopy
            test cl, 1
            jz preskip
            movsb
        preskip :
        shr ecx, 1
            rep movsw
            jmp endit
        longcopy :
        test edi, 1
            jz skip1
            movsb
            dec ecx
        skip1 :
        test edi, 2
            jz skip2
            movsw
            sub ecx, 2
        skip2 :
              mov ebx, ecx
              shr ecx, 2
              rep movsd
              test bl, 2
              jz skip3
              movsw
          skip3 :
        test bl, 1
            jz endit
            movsb
        endit :
    }
}

static __inline void copybufreverse(const void *s, void *d, int32_t c)
{
    _asm {
        mov esi, s
            mov edi, d
            mov ecx, c
            shr ecx, 1
            jnc skipit1
            mov al, byte ptr[esi]
            dec esi
            mov byte ptr[edi], al
            inc edi
        skipit1 :
        shr ecx, 1
            jnc skipit2
            mov ax, word ptr[esi-1]
            sub esi, 2
            ror ax, 8
            mov word ptr[edi], ax
            add edi, 2
        skipit2:
        test ecx, ecx
            jz endloop
        begloop :
        mov eax, dword ptr[esi-3]
            sub esi, 4
            bswap eax
            mov dword ptr[edi], eax
            add edi, 4
            dec ecx
            jnz begloop
        endloop :
    }
}

static __inline void qinterpolatedown16(int32_t a, int32_t c, int32_t d, int32_t s)
{
    _asm {
        mov eax, a
            mov ecx, c
            mov edx, d
            mov esi, s
            mov ebx, ecx
            shr ecx, 1
            jz skipbegcalc
        begqcalc :
        lea edi, [edx+esi]
            sar edx, 16
            mov dword ptr[eax], edx
            lea edx, [edi+esi]
            sar edi, 16
            mov dword ptr[eax+4], edi
            add eax, 8
            dec ecx
            jnz begqcalc
            test ebx, 1
            jz skipbegqcalc2
        skipbegcalc :
        sar edx, 16
            mov dword ptr[eax], edx
        skipbegqcalc2 :
    }
}

static __inline void qinterpolatedown16short(int32_t a, int32_t c, int32_t d, int32_t s)
{
    _asm {
        mov eax, a
            mov ecx, c
            mov edx, d
            mov esi, s
            test ecx, ecx
            jz endit
            test al, 2
            jz skipalignit
            mov ebx, edx
            sar ebx, 16
            mov word ptr[eax], bx
            add edx, esi
            add eax, 2
            dec ecx
            jz endit
        skipalignit :
        sub ecx, 2
            jc finishit
        begqcalc :
        mov ebx, edx
            add edx, esi
            sar ebx, 16
            mov edi, edx
            and edi, 0ffff0000h
            add edx, esi
            add ebx, edi
            mov dword ptr[eax], ebx
            add eax, 4
            sub ecx, 2
            jnc begqcalc
            test cl, 1
            jz endit
        finishit :
        mov ebx, edx
            sar ebx, 16
            mov word ptr[eax], bx
        endit :
    }
}

//returns eax/ebx, dmval = eax%edx;
static __inline int32_t divmod(int32_t a, int32_t b)
{
    _asm {
        mov eax, a
            xor edx, edx
            div b
            mov dmval, edx
    }
}

//returns eax%ebx, dmval = eax/edx;
static __inline int32_t moddiv(int32_t a, int32_t b)
{
    _asm {
        mov eax, a
            xor edx, edx
            div b
            mov dmval, eax
            mov eax, edx
    }
}

static __inline int32_t klabs(int32_t a)
{
    _asm {
        mov eax, a
            test eax, eax
            jns skipnegate
            neg eax
        skipnegate :
    }
}

static __inline int32_t ksgn(int32_t b)
{
    _asm {
        mov ebx, b
            add ebx, ebx
            sbb eax, eax
            cmp eax, ebx
            adc al, 0
    }
}

//eax = (unsigned min)umin(eax,ebx)
static __inline int32_t umin(int32_t a, int32_t b)
{
    _asm {
        mov eax, a
            sub eax, b
            sbb ecx, ecx
            and eax, ecx
            add eax, b
    }
}

//eax = (unsigned max)umax(eax,ebx)
static __inline int32_t umax(int32_t a, int32_t b)
{
    _asm {
        mov eax, a
            sub eax, b
            sbb ecx, ecx
            xor ecx, 0xffffffff
            and eax, ecx
            add eax, b
    }
}

static __inline int32_t kmin(int32_t a, int32_t b)
{
    _asm {
        mov eax, a
            mov ebx, b
            cmp eax, ebx
            jl skipit
            mov eax, ebx
        skipit :
    }
}

static __inline int32_t kmax(int32_t a, int32_t b)
{
    _asm {
        mov eax, a
            mov ebx, b
            cmp eax, ebx
            jg skipit
            mov eax, ebx
        skipit :
    }
}

static __inline void swapchar(void *a, void *b)
{
    _asm {
        mov eax, a
            mov ebx, b
            mov cl, [eax]
            mov ch, [ebx]
            mov[ebx], cl
            mov[eax], ch
    }
}

static __inline void swapshort(void *a, void *b)
{
    _asm {
        mov eax, a
            mov ebx, b
            mov cx, [eax]
            mov dx, [ebx]
            mov[ebx], cx
            mov[eax], dx
    }
}

static __inline void swaplong(void *a, void *b)
{
    _asm {
        mov eax, a
            mov ebx, b
            mov ecx, [eax]
            mov edx, [ebx]
            mov[ebx], ecx
            mov[eax], edx
    }
}

#define swapfloat swaplong

static __inline void swapbuf4(void *a, void *b, int32_t c)
{
    _asm {
        mov eax, a
            mov ebx, b
            mov ecx, c
        begswap :
        mov esi, [eax]
            mov edi, [ebx]
            mov[ebx], esi
            mov[eax], edi
            add eax, 4
            add ebx, 4
            dec ecx
            jnz short begswap
    }
}

static __inline void swap64bit(void *a, void *b)
{
    _asm {
        mov eax, a
            mov ebx, b
            mov ecx, [eax]
            mov edx, [ebx]
            mov[ebx], ecx
            mov ecx, [eax+4]
            mov[eax], edx
            mov edx, [ebx+4]
            mov[ebx+4], ecx
            mov[eax+4], edx
    }
}

//swapchar2(ptr1,ptr2,xsiz); is the same as:
//swapchar(ptr1,ptr2); swapchar(ptr1+1,ptr2+xsiz);
static __inline void swapchar2(void *a, void *b, int32_t s)
{
    _asm {
        mov eax, a
            mov ebx, b
            mov esi, s
            add esi, ebx
            mov cx, [eax]
            mov dl, [ebx]
            mov[ebx], cl
            mov dh, [esi]
            mov[esi], ch
            mov[eax], dx
    }
}
//}}}

#endif // __pragmas_x86_h__
#endif // __pragmas_h__
