/* MD4.H - header file for MD4C.C
   Modified from original version published in RFC1320 by
   Jonathon Fowler (jf@jonof.id.au)
 */

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
   rights reserved.

   License to copy and use this software is granted provided that it
   is identified as the "RSA Data Security, Inc. MD4 Message-Digest
   Algorithm" in all material mentioning or referencing this software
   or this function.

   License is also granted to make and use derivative works provided
   that such works are identified as "derived from the RSA Data
   Security, Inc. MD4 Message-Digest Algorithm" in all material
   mentioning or referencing the derived work.

   RSA Data Security, Inc. makes no representations concerning either
   the merchantability of this software or the suitability of this
   software for any particular purpose. It is provided "as is"
   without express or implied warranty of any kind.

   These notices must be retained in any copies of any part of this
   documentation and/or software.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* MD4 context. */
typedef struct {
  unsigned int state[4];		/* state (ABCD) */
  unsigned int count[2];		/* number of bits, modulo 2^64 (lsb first) */
  unsigned char buffer[64];		/* input buffer */
} MD4_CTX;

void md4once(const unsigned char *block, unsigned int len, unsigned char digest[16]);
void md4init(MD4_CTX *);
void md4block(MD4_CTX *, const unsigned char *, unsigned int);
void md4finish(unsigned char [16], MD4_CTX *);

#ifdef __cplusplus
}
#endif
