//
// GCC Inline Assembler version (x86)
//

//{{{

#ifdef pragmas_h_
#ifndef pragmas_x86_h_
#define pragmas_x86_h_

#ifndef UNDERSCORES
#define _DMVAL "dmval"
#else
#define _DMVAL "_dmval"
#endif

#define pragmas_have_mulscale

#define mulscale(a,d,c) \
	({ int32_t __a=(a), __d=(d), __c=(c); \
	   __asm__ __volatile__ ("imull %%edx; shrdl %%cl, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d), "c" (__c) : "cc"); \
	 __a; })
#define mulscale1(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $1, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale2(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $2, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale3(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $3, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale4(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $4, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale5(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $5, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale6(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $6, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale7(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $7, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale8(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $8, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale9(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $9, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale10(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $10, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale11(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $11, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale12(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $12, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale13(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $13, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale14(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $14, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale15(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $15, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale16(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $16, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale17(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $17, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale18(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $18, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale19(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $19, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale20(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $20, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale21(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $21, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale22(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $22, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale23(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $23, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale24(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $24, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale25(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $25, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale26(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $26, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale27(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $27, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale28(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $28, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale29(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $29, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale30(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $30, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale31(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx; shrdl $31, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __a; })
#define mulscale32(a,d) \
	({ int32_t __a=(a), __d=(d); \
	   __asm__ __volatile__ ("imull %%edx" \
		: "=a" (__a), "=d" (__d) \
		: "a" (__a), "d" (__d) : "cc"); \
	 __d; })

#define dmulscale(a,d,S,D,c) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D), __c=(c); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl %%cl, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D), "c" (__c) : "ebx", "cc"); \
	 __a; })
#define dmulscale1(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $1, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale2(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $2, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale3(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $3, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale4(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $4, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale5(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $5, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale6(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $6, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale7(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $7, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale8(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $8, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale9(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $9, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale10(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $10, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale11(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $11, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale12(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $12, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale13(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $13, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale14(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $14, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale15(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $15, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale16(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $16, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale17(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $17, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale18(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $18, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale19(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $19, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale20(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $20, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale21(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $21, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale22(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $22, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale23(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $23, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale24(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $24, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale25(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $25, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale26(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $26, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale27(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $27, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale28(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $28, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale29(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $29, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale30(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $30, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale31(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx; shrdl $31, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __a; })
#define dmulscale32(a,d,S,D) \
	({ int32_t __a=(a), __d=(d), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; movl %%eax, %%ebx; movl %%esi, %%eax; movl %%edx, %%esi; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%esi, %%edx" \
		: "=a" (__a), "=d" (__d), "=S" (__S) \
		: "a" (__a), "d" (__d), "S" (__S), "D" (__D) : "ebx", "cc"); \
	 __d; })

#define tmulscale1(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $1, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale2(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $2, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale3(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $3, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale4(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $4, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale5(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $5, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale6(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $6, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale7(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $7, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale8(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $8, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale9(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $9, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale10(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $10, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale11(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $11, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale12(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $12, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale13(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $13, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale14(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $14, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale15(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $15, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale16(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $16, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale17(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $17, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale18(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $18, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale19(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $19, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale20(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $20, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale21(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $21, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale22(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $22, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale23(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $23, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale24(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $24, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale25(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $25, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale26(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $26, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale27(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $27, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale28(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $28, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale29(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $29, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale30(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $30, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale31(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx; shrdl $31, %%edx, %%eax" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __a; })
#define tmulscale32(a,d,b,c,S,D) \
	({ int32_t __a=(a), __d=(d), __b=(b), __c=(c), __S=(S), __D=(D); \
	   __asm__ __volatile__ ("imull %%edx; xchgl %%ebx, %%eax; xchgl %%ecx, %%edx; " \
				"imull %%edx; addl %%eax, %%ebx; adcl %%edx, %%ecx; movl %%esi, %%eax; " \
				"imull %%edi; addl %%ebx, %%eax; adcl %%ecx, %%edx" \
		: "=a" (__a), "=d" (__d), "=b" (__b), "=c" (__c) \
		: "a" (__a), "d" (__d), "b" (__b), "c" (__c), "S" (__S), "D" (__D) : "cc"); \
	 __d; })

#define pragmas_have_clearbuf

#define clearbuf(D,c,a) \
	({ void *__D=(D); int32_t __c=(c), __a=(a); \
	   __asm__ __volatile__ ("rep; stosl" \
		: "=&D" (__D), "=&c" (__c) : "0" (__D), "1" (__c), "a" (__a) : "memory", "cc"); \
	 0; })

#define pragmas_have_copybuf

#define copybuf(S,D,c) \
	({ const void *__S=(S), *__D=(D); int32_t __c=(c); \
	   __asm__ __volatile__ ("rep; movsl" \
		: "=&S" (__S), "=&D" (__D), "=&c" (__c) : "0" (__S), "1" (__D), "2" (__c) : "memory", "cc"); \
	 0; })

#define pragmas_have_klabs

#define klabs(a) \
	({ int32_t __a=(a); \
	   __asm__ __volatile__ ("testl %%eax, %%eax; jns 0f; negl %%eax; 0:" \
		: "=a" (__a) : "a" (__a) : "cc"); \
	 __a; })

#define pragmas_have_ksgn

#define ksgn(b) \
	({ int32_t __b=(b), __r; \
	   __asm__ __volatile__ ("addl %%ebx, %%ebx; sbbl %%eax, %%eax; cmpl %%ebx, %%eax; adcb $0, %%al" \
		: "=a" (__r) : "b" (__b) : "cc"); \
	 __r; })

#define pragmas_have_swaps

#define swapchar(a,b) \
	({ void *__a=(a), *__b=(b); \
	   __asm__ __volatile__ ("movb (%%eax), %%cl; movb (%%ebx), %%ch; movb %%cl, (%%ebx); movb %%ch, (%%eax)" \
		: : "a" (__a), "b" (__b) : "ecx", "memory", "cc"); \
	 0; })
#define swapshort(a,b) \
	({ void *__a=(a), *__b=(b); \
	   __asm__ __volatile__ ("movw (%%eax), %%cx; movw (%%ebx), %%dx; movw %%cx, (%%ebx); movw %%dx, (%%eax)" \
		: : "a" (__a), "b" (__b) : "ecx", "edx", "memory", "cc"); \
	 0; })
#define swaplong(a,b) \
	({ void *__a=(a), *__b=(b); \
	   __asm__ __volatile__ ("movl (%%eax), %%ecx; movl (%%ebx), %%edx; movl %%ecx, (%%ebx); movl %%edx, (%%eax)" \
		: : "a" (__a), "b" (__b) : "ecx", "edx", "memory", "cc"); \
	 0; })
#define swapfloat swaplong
#define swapbuf4(a,b,c) \
	({ void *__a=(a), *__b=(b); int32_t __c=(c); \
	   __asm__ __volatile__ ("0: movl (%%eax), %%esi; movl (%%ebx), %%edi; movl %%esi, (%%ebx); " \
				"movl %%edi, (%%eax); addl $4, %%eax; addl $4, %%ebx; decl %%ecx; jnz 0b" \
		: : "a" (__a), "b" (__b), "c" (__c) : "esi", "edi", "memory", "cc"); \
	 0; })
#define swap64bit(a,b) \
	({ void *__a=(a), *__b=(b); \
	   __asm__ __volatile__ ("movl (%%eax), %%ecx; movl (%%ebx), %%edx; movl %%ecx, (%%ebx); " \
				"movl 4(%%eax), %%ecx; movl %%edx, (%%eax); movl 4(%%ebx), %%edx; " \
				"movl %%ecx, 4(%%ebx); movl %%edx, 4(%%eax)" \
		: : "a" (__a), "b" (__b) : "ecx", "edx", "memory", "cc"); \
	 0; })
#define swapdouble swap64bit
//swapchar2(ptr1,ptr2,xsiz); is the same as:
//swapchar(ptr1,ptr2); swapchar(ptr1+1,ptr2+xsiz);
#define swapchar2(a,b,S) \
	({ void *__a=(a), *__b=(b); int32_t __S=(S); \
	   __asm__ __volatile__ ("addl %%ebx, %%esi; movw (%%eax), %%cx; movb (%%ebx), %%dl; " \
				"movb %%cl, (%%ebx); movb (%%esi), %%dh; movb %%ch, (%%esi); " \
				"movw %%dx, (%%eax)" \
		: "=S" (__S) : "a" (__a), "b" (__b), "S" (__S) : "ecx", "edx", "memory", "cc"); \
	 0; })


#define pragmas_have_qinterpolatedown16

#define qinterpolatedown16(a,c,d,S) \
	({ void *__a=(void*)(a); int32_t __c=(c), __d=(d), __S=(S); \
	   __asm__ __volatile__ ("movl %%ecx, %%ebx; shrl $1, %%ecx; jz 1f; " \
				"0: leal (%%edx,%%esi,), %%edi; sarl $16, %%edx; movl %%edx, (%%eax); " \
				"leal (%%edi,%%esi,), %%edx; sarl $16, %%edi; movl %%edi, 4(%%eax); " \
				"addl $8, %%eax; decl %%ecx; jnz 0b; testl $1, %%ebx; jz 2f; " \
				"1: sarl $16, %%edx; movl %%edx, (%%eax); 2:" \
		: "=a" (__a), "=c" (__c), "=d" (__d) : "a" (__a), "c" (__c), "d" (__d), "S" (__S) \
		: "ebx", "edi", "memory", "cc"); \
	 0; })

#define qinterpolatedown16short(a,c,d,S) \
	({ void *__a=(void*)(a); int32_t __c=(c), __d=(d), __S=(S); \
	   __asm__ __volatile__ ("testl %%ecx, %%ecx; jz 3f; testb $2, %%al; jz 0f; movl %%edx, %%ebx; " \
				"sarl $16, %%ebx; movw %%bx, (%%eax); addl %%esi, %%edx; addl $2, %%eax; " \
				"decl %%ecx; jz 3f; " \
				"0: subl $2, %%ecx; jc 2f; " \
				"1: movl %%edx, %%ebx; addl %%esi, %%edx; sarl $16, %%ebx; movl %%edx, %%edi; " \
				"andl $0xffff0000, %%edi; addl %%esi, %%edx; addl %%edi, %%ebx; " \
				"movl %%ebx, (%%eax); addl $4, %%eax; subl $2, %%ecx; jnc 1b; testb $1, %%cl; " \
				"jz 3f; " \
				"2: movl %%edx, %%ebx; sarl $16, %%ebx; movw %%bx, (%%eax); 3:" \
		: "=a" (__a), "=c" (__c), "=d" (__d) : "a" (__a), "c" (__c), "d" (__d), "S" (__S) \
		: "ebx", "edi", "memory", "cc"); \
	 0; })

#define pragmas_have_krecipasm

#define krecipasm(a) \
    ({ int32_t __a=(a); \
       __asm__ __volatile__ ( \
            "movl %%eax, (" ASMSYM("fpuasm") "); fildl (" ASMSYM("fpuasm") "); " \
            "addl %%eax, %%eax; fstps (" ASMSYM("fpuasm") "); sbbl %%ebx, %%ebx; " \
            "movl (" ASMSYM("fpuasm") "), %%eax; movl %%eax, %%ecx; " \
            "andl $0x007ff000, %%eax; shrl $10, %%eax; subl $0x3f800000, %%ecx; " \
            "shrl $23, %%ecx; movl " ASMSYM("reciptable") "(%%eax), %%eax; " \
            "sarl %%cl, %%eax; xorl %%ebx, %%eax" \
        : "=a" (__a) : "a" (__a) : "ebx", "ecx", "memory", "cc"); \
     __a; })

//}}}

#endif // pragmas_x86_h_
#endif // pragmas_h_
