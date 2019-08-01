//
// Microsoft C inline assembler
//

//{{{

#ifdef pragmas_h_
#ifndef pragmas_x86_h_
#define pragmas_x86_h_

#define pragmas_have_mulscale

static __inline int32_t mulscale(int32_t a, int32_t d, int32_t c)
{
    _asm {
        mov ecx, c
            mov eax, a
            imul d
            shrd eax, edx, cl
    }
}

#define EDUKE32_SCALER_PRAGMA(x) \
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
static __inline int32_t tmulscale##x (int32_t a, int32_t d, int32_t b, int32_t c, int32_t S, int32_t D) \
{ \
    _asm mov eax, a \
    _asm mov ebx, b \
    _asm imul d \
    _asm xchg eax, ebx \
    _asm mov ecx, c \
    _asm xchg edx, ecx \
    _asm imul edx \
    _asm add ebx, eax \
    _asm adc ecx, edx \
    _asm mov eax, S \
    _asm imul D \
    _asm add eax, ebx \
    _asm adc edx, ecx \
    _asm shrd eax, edx, x \
} \

EDUKE32_GENERATE_PRAGMAS
#undef EDUKE32_SCALER_PRAGMA

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

static __inline int32_t tmulscale32(int32_t a, int32_t d, int32_t b, int32_t c, int32_t S, int32_t D)
{
    _asm {
        mov eax, a
            mov ebx, b
            imul d
            xchg eax, ebx
            mov ecx, c
            xchg edx, ecx
            imul edx
            add ebx, eax
            adc ecx, edx
            mov eax, S
            imul D
            add eax, ebx
            adc edx, ecx
            mov eax, edx
    }
}

#define pragmas_have_clearbuf

static __inline void clearbuf(void *d, int32_t c, int32_t a)
{
    _asm {
        mov edi, d
            mov ecx, c
            mov eax, a
            rep stosd
    }
}

#define pragmas_have_clearbufbyte

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

#define pragmas_have_copybuf

static __inline void copybuf(const void *s, void *d, int32_t c)
{
    _asm {
        mov esi, s
            mov edi, d
            mov ecx, c
            rep movsd
    }
}

#define pragmas_have_copybufbyte

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

#define pragmas_have_copybufreverse

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

#define pragmas_have_qinterpolatedown16

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

#define pragmas_have_klabs

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

#define pragmas_have_ksgn

static __inline int ksgn(int32_t b)
{
    _asm {
        mov ebx, b
            add ebx, ebx
            sbb eax, eax
            cmp eax, ebx
            adc al, 0
    }
}

#define pragmas_have_swaps

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

#define swapdouble swap64bit

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

#define pragmas_have_krecipasm

//0x007ff000 is (11<<13), 0x3f800000 is (127<<23)
static inline int32_t krecipasm(int32_t a)
{
    _asm
    {
        push ebx
            mov eax, a
            mov fpuasm, eax
            fild dword ptr fpuasm
            add eax, eax
            fstp dword ptr fpuasm
            sbb ebx, ebx
            mov eax, fpuasm
            mov ecx, eax
            and eax, 0x007ff000
            shr eax, 10
            sub ecx, 0x3f800000
            shr ecx, 23
            mov eax, dword ptr reciptable[eax]
            sar eax, cl
            xor eax, ebx
            pop ebx
    }
}

//}}}

#endif // pragmas_x86_h_
#endif // pragmas_h_
