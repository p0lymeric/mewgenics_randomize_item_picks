/* LibTomCrypt, modular cryptographic library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* This is the build config file.
 *
 * With this you can setup what to include/exclude automatically during any build.  Just comment
 * out the line that #define's the word for the thing you want to remove.  phew!
 */

#ifndef TOMCRYPT_CFG_H
#define TOMCRYPT_CFG_H

/* Controls endianess and size of registers.
 */

#define ENDIAN_LITTLE
#define ENDIAN_64BITWORD

/* ulong64: 64-bit data type */
#ifdef _MSC_VER
   #define CONST64(n) n ## ui64
   typedef unsigned __int64 ulong64;
   typedef __int64 long64;
#else
   #define CONST64(n) n ## uLL
   typedef unsigned long long ulong64;
   typedef long long long64;
#endif

typedef unsigned ulong32;

#endif /* TOMCRYPT_CFG_H */
