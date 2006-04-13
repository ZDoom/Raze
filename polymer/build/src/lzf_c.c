/*
 * Copyright (c) 2000-2005 Marc Alexander Lehmann <schmorp@schmorp.de>
 * 
 * Redistribution and use in source and binary forms, with or without modifica-
 * tion, are permitted provided that the following conditions are met:
 * 
 *   1.  Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 * 
 *   2.  Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 *   3.  The name of the author may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MER-
 * CHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPE-
 * CIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTH-
 * ERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License version 2 (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of the above. If you wish to
 * allow the use of your version of this file only under the terms of the
 * GPL and not to allow others to use your version of this file under the
 * BSD license, indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by the GPL. If
 * you do not delete the provisions above, a recipient may use your version
 * of this file under either the BSD or the GPL.
 */

#include "lzfP.h"

#define HSIZE (1 << (HLOG))

/*
 * don't play with this unless you benchmark!
 * decompression is not dependent on the hash function
 * the hashing function might seem strange, just believe me
 * it works ;)
 */
#ifndef FRST
# define FRST(p) (((p[0]) << 8) | p[1])
# define NEXT(v,p) (((v) << 8) | p[2])
# define IDX(h) ((((h ^ (h << 5)) >> (3*8 - HLOG)) - h*5) & (HSIZE - 1))
#endif
/*
 * IDX works because it is very similar to a multiplicative hash, e.g.
 * ((h * 57321 >> (3*8 - HLOG)) & (HSIZE - 1))
 * the latter is also quite fast on newer CPUs, and sligthly better
 *
 * the next one is also quite good, albeit slow ;)
 * (int)(cos(h & 0xffffff) * 1e6)
 */

#if 0
/* original lzv-like hash function, much worse and thus slower */
# define FRST(p) (p[0] << 5) ^ p[1]
# define NEXT(v,p) ((v) << 5) ^ p[2]
# define IDX(h) ((h) & (HSIZE - 1))
#endif

#define        MAX_LIT        (1 <<  5)
#define        MAX_OFF        (1 << 13)
#define        MAX_REF        ((1 <<  8) + (1 << 3))

/*
 * compressed format
 *
 * 000LLLLL <L+1>    ; literal
 * LLLooooo oooooooo ; backref L
 * 111ooooo LLLLLLLL oooooooo ; backref L+7
 *
 */

unsigned int
lzf_compress (const void *const in_data, unsigned int in_len,
	      void *out_data, unsigned int out_len
#if LZF_STATE_ARG
              , LZF_STATE *htab
#endif
              )
{
#if !LZF_STATE_ARG
  LZF_STATE htab;
#endif
  const u8 **hslot;
  const u8 *ip = (const u8 *)in_data;
        u8 *op = (u8 *)out_data;
  const u8 *in_end  = ip + in_len;
        u8 *out_end = op + out_len;
  const u8 *ref;

  unsigned int hval = FRST (ip);
  unsigned long off;
           int lit = 0;

#if INIT_HTAB
# if USE_MEMCPY
    memset (htab, 0, sizeof (htab));
# else
    for (hslot = htab; hslot < htab + HSIZE; hslot++)
      *hslot++ = ip;
# endif
#endif

  for (;;)
    {
      if (ip < in_end - 2)
        {
          hval = NEXT (hval, ip);
          hslot = htab + IDX (hval);
          ref = *hslot; *hslot = ip;

          if (1
#if INIT_HTAB && !USE_MEMCPY
              && ref < ip /* the next test will actually take care of this, but this is faster */
#endif
              && (off = ip - ref - 1) < MAX_OFF
              && ip + 4 < in_end
              && ref > (u8 *)in_data
#if STRICT_ALIGN
              && ref[0] == ip[0]
              && ref[1] == ip[1]
              && ref[2] == ip[2]
#else
              && *(u16 *)ref == *(u16 *)ip
              && ref[2] == ip[2]
#endif
            )
            {
              /* match found at *ref++ */
              unsigned int len = 2;
              unsigned int maxlen = in_end - ip - len;
              maxlen = maxlen > MAX_REF ? MAX_REF : maxlen;

              if (op + lit + 1 + 3 >= out_end)
                return 0;

              do
                len++;
              while (len < maxlen && ref[len] == ip[len]);

              if (lit)
                {
                  *op++ = lit - 1;
                  lit = -lit;
                  do
                    *op++ = ip[lit];
                  while (++lit);
                }

              len -= 2;
              ip++;

              if (len < 7)
                {
                  *op++ = (off >> 8) + (len << 5);
                }
              else
                {
                  *op++ = (off >> 8) + (  7 << 5);
                  *op++ = len - 7;
                }

              *op++ = off;

#if ULTRA_FAST || VERY_FAST
              ip += len;
#if VERY_FAST && !ULTRA_FAST
              --ip;
#endif
              hval = FRST (ip);

              hval = NEXT (hval, ip);
              htab[IDX (hval)] = ip;
              ip++;

#if VERY_FAST && !ULTRA_FAST
              hval = NEXT (hval, ip);
              htab[IDX (hval)] = ip;
              ip++;
#endif
#else
              do
                {
                  hval = NEXT (hval, ip);
                  htab[IDX (hval)] = ip;
                  ip++;
                }
              while (len--);
#endif
              continue;
            }
        }
      else if (ip == in_end)
        break;

      /* one more literal byte we must copy */
      lit++;
      ip++;

      if (lit == MAX_LIT)
        {
          if (op + 1 + MAX_LIT >= out_end)
            return 0;

          *op++ = MAX_LIT - 1;
#if USE_MEMCPY
          memcpy (op, ip - MAX_LIT, MAX_LIT);
          op += MAX_LIT;
          lit = 0;
#else
          lit = -lit;
          do
            *op++ = ip[lit];
          while (++lit);
#endif
        }
    }

  if (lit)
    {
      if (op + lit + 1 >= out_end)
	return 0;

      *op++ = lit - 1;
      lit = -lit;
      do
	*op++ = ip[lit];
      while (++lit);
    }

  return op - (u8 *) out_data;
}
