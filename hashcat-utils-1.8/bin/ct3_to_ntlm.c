#include <stdio.h>
#include <stdint.h>

/**
 * Name........: deskey-to-ntlm.pl
 * Autor.......: Jens Steube <jens.steube@gmail.com>
 * License.....: MIT
 *
 * Most of the code taken from hashcat
 */

typedef uint8_t  u8;
typedef uint32_t u32;

#define BOX(v,i,S) (S)[(i)][(v)]

#define PERM_OP(a,b,tt,n,m) \
{                           \
  tt = a >> n;              \
  tt = tt ^ b;              \
  tt = tt & m;              \
  b = b ^ tt;               \
  tt = tt << n;             \
  a = a ^ tt;               \
}

#define HPERM_OP(a,tt,n,m)  \
{                           \
  tt = a << (16 + n);       \
  tt = tt ^ a;              \
  tt = tt & m;              \
  a  = a ^ tt;              \
  tt = tt >> (16 + n);      \
  a  = a ^ tt;              \
}

#define IP(l,r,tt)                     \
{                                      \
  PERM_OP (r, l, tt,  4, 0x0f0f0f0f);  \
  PERM_OP (l, r, tt, 16, 0x0000ffff);  \
  PERM_OP (r, l, tt,  2, 0x33333333);  \
  PERM_OP (l, r, tt,  8, 0x00ff00ff);  \
  PERM_OP (r, l, tt,  1, 0x55555555);  \
}

#define FP(l,r,tt)                     \
{                                      \
  PERM_OP (l, r, tt,  1, 0x55555555);  \
  PERM_OP (r, l, tt,  8, 0x00ff00ff);  \
  PERM_OP (l, r, tt,  2, 0x33333333);  \
  PERM_OP (r, l, tt, 16, 0x0000ffff);  \
  PERM_OP (l, r, tt,  4, 0x0f0f0f0f);  \
}

#define MD5_F(x,y,z)    ((z) ^ ((x) & ((y) ^ (z))))
#define MD5_G(x,y,z)    ((y) ^ ((z) & ((x) ^ (y))))
#define MD5_H(x,y,z)    ((x) ^ (y) ^ (z))
#define MD5_I(x,y,z)    ((y) ^ ((x) | ~(z)))
#define MD5_Fo(x,y,z)   (MD5_F((x), (y), (z)))
#define MD5_Go(x,y,z)   (MD5_G((x), (y), (z)))

#define MD5_STEP(f,a,b,c,d,x,K,s)   \
{                                   \
  a += K;                           \
  a += x;                           \
  a += f (b, c, d);                 \
  a  = rotl32 (a, s);               \
  a += b;                           \
}

typedef enum md5_constants
{
  MD5M_A=0x67452301,
  MD5M_B=0xefcdab89,
  MD5M_C=0x98badcfe,
  MD5M_D=0x10325476,

  MD5S00=7,
  MD5S01=12,
  MD5S02=17,
  MD5S03=22,
  MD5S10=5,
  MD5S11=9,
  MD5S12=14,
  MD5S13=20,
  MD5S20=4,
  MD5S21=11,
  MD5S22=16,
  MD5S23=23,
  MD5S30=6,
  MD5S31=10,
  MD5S32=15,
  MD5S33=21,

  MD5C00=0xd76aa478,
  MD5C01=0xe8c7b756,
  MD5C02=0x242070db,
  MD5C03=0xc1bdceee,
  MD5C04=0xf57c0faf,
  MD5C05=0x4787c62a,
  MD5C06=0xa8304613,
  MD5C07=0xfd469501,
  MD5C08=0x698098d8,
  MD5C09=0x8b44f7af,
  MD5C0a=0xffff5bb1,
  MD5C0b=0x895cd7be,
  MD5C0c=0x6b901122,
  MD5C0d=0xfd987193,
  MD5C0e=0xa679438e,
  MD5C0f=0x49b40821,
  MD5C10=0xf61e2562,
  MD5C11=0xc040b340,
  MD5C12=0x265e5a51,
  MD5C13=0xe9b6c7aa,
  MD5C14=0xd62f105d,
  MD5C15=0x02441453,
  MD5C16=0xd8a1e681,
  MD5C17=0xe7d3fbc8,
  MD5C18=0x21e1cde6,
  MD5C19=0xc33707d6,
  MD5C1a=0xf4d50d87,
  MD5C1b=0x455a14ed,
  MD5C1c=0xa9e3e905,
  MD5C1d=0xfcefa3f8,
  MD5C1e=0x676f02d9,
  MD5C1f=0x8d2a4c8a,
  MD5C20=0xfffa3942,
  MD5C21=0x8771f681,
  MD5C22=0x6d9d6122,
  MD5C23=0xfde5380c,
  MD5C24=0xa4beea44,
  MD5C25=0x4bdecfa9,
  MD5C26=0xf6bb4b60,
  MD5C27=0xbebfbc70,
  MD5C28=0x289b7ec6,
  MD5C29=0xeaa127fa,
  MD5C2a=0xd4ef3085,
  MD5C2b=0x04881d05,
  MD5C2c=0xd9d4d039,
  MD5C2d=0xe6db99e5,
  MD5C2e=0x1fa27cf8,
  MD5C2f=0xc4ac5665,
  MD5C30=0xf4292244,
  MD5C31=0x432aff97,
  MD5C32=0xab9423a7,
  MD5C33=0xfc93a039,
  MD5C34=0x655b59c3,
  MD5C35=0x8f0ccc92,
  MD5C36=0xffeff47d,
  MD5C37=0x85845dd1,
  MD5C38=0x6fa87e4f,
  MD5C39=0xfe2ce6e0,
  MD5C3a=0xa3014314,
  MD5C3b=0x4e0811a1,
  MD5C3c=0xf7537e82,
  MD5C3d=0xbd3af235,
  MD5C3e=0x2ad7d2bb,
  MD5C3f=0xeb86d391u

} md5_constants_t;

static const u32 c_SPtrans[8][64] =
{
  {
    /* nibble 0 */
    0x02080800, 0x00080000, 0x02000002, 0x02080802,
    0x02000000, 0x00080802, 0x00080002, 0x02000002,
    0x00080802, 0x02080800, 0x02080000, 0x00000802,
    0x02000802, 0x02000000, 0x00000000, 0x00080002,
    0x00080000, 0x00000002, 0x02000800, 0x00080800,
    0x02080802, 0x02080000, 0x00000802, 0x02000800,
    0x00000002, 0x00000800, 0x00080800, 0x02080002,
    0x00000800, 0x02000802, 0x02080002, 0x00000000,
    0x00000000, 0x02080802, 0x02000800, 0x00080002,
    0x02080800, 0x00080000, 0x00000802, 0x02000800,
    0x02080002, 0x00000800, 0x00080800, 0x02000002,
    0x00080802, 0x00000002, 0x02000002, 0x02080000,
    0x02080802, 0x00080800, 0x02080000, 0x02000802,
    0x02000000, 0x00000802, 0x00080002, 0x00000000,
    0x00080000, 0x02000000, 0x02000802, 0x02080800,
    0x00000002, 0x02080002, 0x00000800, 0x00080802,
  },
  {
    /* nibble 1 */
    0x40108010, 0x00000000, 0x00108000, 0x40100000,
    0x40000010, 0x00008010, 0x40008000, 0x00108000,
    0x00008000, 0x40100010, 0x00000010, 0x40008000,
    0x00100010, 0x40108000, 0x40100000, 0x00000010,
    0x00100000, 0x40008010, 0x40100010, 0x00008000,
    0x00108010, 0x40000000, 0x00000000, 0x00100010,
    0x40008010, 0x00108010, 0x40108000, 0x40000010,
    0x40000000, 0x00100000, 0x00008010, 0x40108010,
    0x00100010, 0x40108000, 0x40008000, 0x00108010,
    0x40108010, 0x00100010, 0x40000010, 0x00000000,
    0x40000000, 0x00008010, 0x00100000, 0x40100010,
    0x00008000, 0x40000000, 0x00108010, 0x40008010,
    0x40108000, 0x00008000, 0x00000000, 0x40000010,
    0x00000010, 0x40108010, 0x00108000, 0x40100000,
    0x40100010, 0x00100000, 0x00008010, 0x40008000,
    0x40008010, 0x00000010, 0x40100000, 0x00108000,
  },
  {
    /* nibble 2 */
    0x04000001, 0x04040100, 0x00000100, 0x04000101,
    0x00040001, 0x04000000, 0x04000101, 0x00040100,
    0x04000100, 0x00040000, 0x04040000, 0x00000001,
    0x04040101, 0x00000101, 0x00000001, 0x04040001,
    0x00000000, 0x00040001, 0x04040100, 0x00000100,
    0x00000101, 0x04040101, 0x00040000, 0x04000001,
    0x04040001, 0x04000100, 0x00040101, 0x04040000,
    0x00040100, 0x00000000, 0x04000000, 0x00040101,
    0x04040100, 0x00000100, 0x00000001, 0x00040000,
    0x00000101, 0x00040001, 0x04040000, 0x04000101,
    0x00000000, 0x04040100, 0x00040100, 0x04040001,
    0x00040001, 0x04000000, 0x04040101, 0x00000001,
    0x00040101, 0x04000001, 0x04000000, 0x04040101,
    0x00040000, 0x04000100, 0x04000101, 0x00040100,
    0x04000100, 0x00000000, 0x04040001, 0x00000101,
    0x04000001, 0x00040101, 0x00000100, 0x04040000,
  },
  {
    /* nibble 3 */
    0x00401008, 0x10001000, 0x00000008, 0x10401008,
    0x00000000, 0x10400000, 0x10001008, 0x00400008,
    0x10401000, 0x10000008, 0x10000000, 0x00001008,
    0x10000008, 0x00401008, 0x00400000, 0x10000000,
    0x10400008, 0x00401000, 0x00001000, 0x00000008,
    0x00401000, 0x10001008, 0x10400000, 0x00001000,
    0x00001008, 0x00000000, 0x00400008, 0x10401000,
    0x10001000, 0x10400008, 0x10401008, 0x00400000,
    0x10400008, 0x00001008, 0x00400000, 0x10000008,
    0x00401000, 0x10001000, 0x00000008, 0x10400000,
    0x10001008, 0x00000000, 0x00001000, 0x00400008,
    0x00000000, 0x10400008, 0x10401000, 0x00001000,
    0x10000000, 0x10401008, 0x00401008, 0x00400000,
    0x10401008, 0x00000008, 0x10001000, 0x00401008,
    0x00400008, 0x00401000, 0x10400000, 0x10001008,
    0x00001008, 0x10000000, 0x10000008, 0x10401000,
  },
  {
    /* nibble 4 */
    0x08000000, 0x00010000, 0x00000400, 0x08010420,
    0x08010020, 0x08000400, 0x00010420, 0x08010000,
    0x00010000, 0x00000020, 0x08000020, 0x00010400,
    0x08000420, 0x08010020, 0x08010400, 0x00000000,
    0x00010400, 0x08000000, 0x00010020, 0x00000420,
    0x08000400, 0x00010420, 0x00000000, 0x08000020,
    0x00000020, 0x08000420, 0x08010420, 0x00010020,
    0x08010000, 0x00000400, 0x00000420, 0x08010400,
    0x08010400, 0x08000420, 0x00010020, 0x08010000,
    0x00010000, 0x00000020, 0x08000020, 0x08000400,
    0x08000000, 0x00010400, 0x08010420, 0x00000000,
    0x00010420, 0x08000000, 0x00000400, 0x00010020,
    0x08000420, 0x00000400, 0x00000000, 0x08010420,
    0x08010020, 0x08010400, 0x00000420, 0x00010000,
    0x00010400, 0x08010020, 0x08000400, 0x00000420,
    0x00000020, 0x00010420, 0x08010000, 0x08000020,
  },
  {
    /* nibble 5 */
    0x80000040, 0x00200040, 0x00000000, 0x80202000,
    0x00200040, 0x00002000, 0x80002040, 0x00200000,
    0x00002040, 0x80202040, 0x00202000, 0x80000000,
    0x80002000, 0x80000040, 0x80200000, 0x00202040,
    0x00200000, 0x80002040, 0x80200040, 0x00000000,
    0x00002000, 0x00000040, 0x80202000, 0x80200040,
    0x80202040, 0x80200000, 0x80000000, 0x00002040,
    0x00000040, 0x00202000, 0x00202040, 0x80002000,
    0x00002040, 0x80000000, 0x80002000, 0x00202040,
    0x80202000, 0x00200040, 0x00000000, 0x80002000,
    0x80000000, 0x00002000, 0x80200040, 0x00200000,
    0x00200040, 0x80202040, 0x00202000, 0x00000040,
    0x80202040, 0x00202000, 0x00200000, 0x80002040,
    0x80000040, 0x80200000, 0x00202040, 0x00000000,
    0x00002000, 0x80000040, 0x80002040, 0x80202000,
    0x80200000, 0x00002040, 0x00000040, 0x80200040,
  },
  {
    /* nibble 6 */
    0x00004000, 0x00000200, 0x01000200, 0x01000004,
    0x01004204, 0x00004004, 0x00004200, 0x00000000,
    0x01000000, 0x01000204, 0x00000204, 0x01004000,
    0x00000004, 0x01004200, 0x01004000, 0x00000204,
    0x01000204, 0x00004000, 0x00004004, 0x01004204,
    0x00000000, 0x01000200, 0x01000004, 0x00004200,
    0x01004004, 0x00004204, 0x01004200, 0x00000004,
    0x00004204, 0x01004004, 0x00000200, 0x01000000,
    0x00004204, 0x01004000, 0x01004004, 0x00000204,
    0x00004000, 0x00000200, 0x01000000, 0x01004004,
    0x01000204, 0x00004204, 0x00004200, 0x00000000,
    0x00000200, 0x01000004, 0x00000004, 0x01000200,
    0x00000000, 0x01000204, 0x01000200, 0x00004200,
    0x00000204, 0x00004000, 0x01004204, 0x01000000,
    0x01004200, 0x00000004, 0x00004004, 0x01004204,
    0x01000004, 0x01004200, 0x01004000, 0x00004004,
  },
  {
    /* nibble 7 */
    0x20800080, 0x20820000, 0x00020080, 0x00000000,
    0x20020000, 0x00800080, 0x20800000, 0x20820080,
    0x00000080, 0x20000000, 0x00820000, 0x00020080,
    0x00820080, 0x20020080, 0x20000080, 0x20800000,
    0x00020000, 0x00820080, 0x00800080, 0x20020000,
    0x20820080, 0x20000080, 0x00000000, 0x00820000,
    0x20000000, 0x00800000, 0x20020080, 0x20800080,
    0x00800000, 0x00020000, 0x20820000, 0x00000080,
    0x00800000, 0x00020000, 0x20000080, 0x20820080,
    0x00020080, 0x20000000, 0x00000000, 0x00820000,
    0x20800080, 0x20020080, 0x20020000, 0x00800080,
    0x20820000, 0x00000080, 0x00800080, 0x20020000,
    0x20820080, 0x00800000, 0x20800000, 0x20000080,
    0x00820000, 0x00020080, 0x20020080, 0x20800000,
    0x00000080, 0x20820000, 0x00820080, 0x00000000,
    0x20000000, 0x20800080, 0x00020000, 0x00820080,
  },
};

static const u32 c_skb[8][64] =
{
  {
    /* for C bits (numbered as per FIPS 46) 1 2 3 4 5 6 */
    0x00000000, 0x00000010, 0x20000000, 0x20000010,
    0x00010000, 0x00010010, 0x20010000, 0x20010010,
    0x00000800, 0x00000810, 0x20000800, 0x20000810,
    0x00010800, 0x00010810, 0x20010800, 0x20010810,
    0x00000020, 0x00000030, 0x20000020, 0x20000030,
    0x00010020, 0x00010030, 0x20010020, 0x20010030,
    0x00000820, 0x00000830, 0x20000820, 0x20000830,
    0x00010820, 0x00010830, 0x20010820, 0x20010830,
    0x00080000, 0x00080010, 0x20080000, 0x20080010,
    0x00090000, 0x00090010, 0x20090000, 0x20090010,
    0x00080800, 0x00080810, 0x20080800, 0x20080810,
    0x00090800, 0x00090810, 0x20090800, 0x20090810,
    0x00080020, 0x00080030, 0x20080020, 0x20080030,
    0x00090020, 0x00090030, 0x20090020, 0x20090030,
    0x00080820, 0x00080830, 0x20080820, 0x20080830,
    0x00090820, 0x00090830, 0x20090820, 0x20090830,
  },
  {
    /* for C bits (numbered as per FIPS 46) 7 8 10 11 12 13 */
    0x00000000, 0x02000000, 0x00002000, 0x02002000,
    0x00200000, 0x02200000, 0x00202000, 0x02202000,
    0x00000004, 0x02000004, 0x00002004, 0x02002004,
    0x00200004, 0x02200004, 0x00202004, 0x02202004,
    0x00000400, 0x02000400, 0x00002400, 0x02002400,
    0x00200400, 0x02200400, 0x00202400, 0x02202400,
    0x00000404, 0x02000404, 0x00002404, 0x02002404,
    0x00200404, 0x02200404, 0x00202404, 0x02202404,
    0x10000000, 0x12000000, 0x10002000, 0x12002000,
    0x10200000, 0x12200000, 0x10202000, 0x12202000,
    0x10000004, 0x12000004, 0x10002004, 0x12002004,
    0x10200004, 0x12200004, 0x10202004, 0x12202004,
    0x10000400, 0x12000400, 0x10002400, 0x12002400,
    0x10200400, 0x12200400, 0x10202400, 0x12202400,
    0x10000404, 0x12000404, 0x10002404, 0x12002404,
    0x10200404, 0x12200404, 0x10202404, 0x12202404,
  },
  {
    /* for C bits (numbered as per FIPS 46) 14 15 16 17 19 20 */
    0x00000000, 0x00000001, 0x00040000, 0x00040001,
    0x01000000, 0x01000001, 0x01040000, 0x01040001,
    0x00000002, 0x00000003, 0x00040002, 0x00040003,
    0x01000002, 0x01000003, 0x01040002, 0x01040003,
    0x00000200, 0x00000201, 0x00040200, 0x00040201,
    0x01000200, 0x01000201, 0x01040200, 0x01040201,
    0x00000202, 0x00000203, 0x00040202, 0x00040203,
    0x01000202, 0x01000203, 0x01040202, 0x01040203,
    0x08000000, 0x08000001, 0x08040000, 0x08040001,
    0x09000000, 0x09000001, 0x09040000, 0x09040001,
    0x08000002, 0x08000003, 0x08040002, 0x08040003,
    0x09000002, 0x09000003, 0x09040002, 0x09040003,
    0x08000200, 0x08000201, 0x08040200, 0x08040201,
    0x09000200, 0x09000201, 0x09040200, 0x09040201,
    0x08000202, 0x08000203, 0x08040202, 0x08040203,
    0x09000202, 0x09000203, 0x09040202, 0x09040203,
  },
  {
    /* for C bits (numbered as per FIPS 46) 21 23 24 26 27 28 */
    0x00000000, 0x00100000, 0x00000100, 0x00100100,
    0x00000008, 0x00100008, 0x00000108, 0x00100108,
    0x00001000, 0x00101000, 0x00001100, 0x00101100,
    0x00001008, 0x00101008, 0x00001108, 0x00101108,
    0x04000000, 0x04100000, 0x04000100, 0x04100100,
    0x04000008, 0x04100008, 0x04000108, 0x04100108,
    0x04001000, 0x04101000, 0x04001100, 0x04101100,
    0x04001008, 0x04101008, 0x04001108, 0x04101108,
    0x00020000, 0x00120000, 0x00020100, 0x00120100,
    0x00020008, 0x00120008, 0x00020108, 0x00120108,
    0x00021000, 0x00121000, 0x00021100, 0x00121100,
    0x00021008, 0x00121008, 0x00021108, 0x00121108,
    0x04020000, 0x04120000, 0x04020100, 0x04120100,
    0x04020008, 0x04120008, 0x04020108, 0x04120108,
    0x04021000, 0x04121000, 0x04021100, 0x04121100,
    0x04021008, 0x04121008, 0x04021108, 0x04121108,
  },
  {
    /* for D bits (numbered as per FIPS 46) 1 2 3 4 5 6 */
    0x00000000, 0x10000000, 0x00010000, 0x10010000,
    0x00000004, 0x10000004, 0x00010004, 0x10010004,
    0x20000000, 0x30000000, 0x20010000, 0x30010000,
    0x20000004, 0x30000004, 0x20010004, 0x30010004,
    0x00100000, 0x10100000, 0x00110000, 0x10110000,
    0x00100004, 0x10100004, 0x00110004, 0x10110004,
    0x20100000, 0x30100000, 0x20110000, 0x30110000,
    0x20100004, 0x30100004, 0x20110004, 0x30110004,
    0x00001000, 0x10001000, 0x00011000, 0x10011000,
    0x00001004, 0x10001004, 0x00011004, 0x10011004,
    0x20001000, 0x30001000, 0x20011000, 0x30011000,
    0x20001004, 0x30001004, 0x20011004, 0x30011004,
    0x00101000, 0x10101000, 0x00111000, 0x10111000,
    0x00101004, 0x10101004, 0x00111004, 0x10111004,
    0x20101000, 0x30101000, 0x20111000, 0x30111000,
    0x20101004, 0x30101004, 0x20111004, 0x30111004,
  },
  {
    /* for D bits (numbered as per FIPS 46) 8 9 11 12 13 14 */
    0x00000000, 0x08000000, 0x00000008, 0x08000008,
    0x00000400, 0x08000400, 0x00000408, 0x08000408,
    0x00020000, 0x08020000, 0x00020008, 0x08020008,
    0x00020400, 0x08020400, 0x00020408, 0x08020408,
    0x00000001, 0x08000001, 0x00000009, 0x08000009,
    0x00000401, 0x08000401, 0x00000409, 0x08000409,
    0x00020001, 0x08020001, 0x00020009, 0x08020009,
    0x00020401, 0x08020401, 0x00020409, 0x08020409,
    0x02000000, 0x0A000000, 0x02000008, 0x0A000008,
    0x02000400, 0x0A000400, 0x02000408, 0x0A000408,
    0x02020000, 0x0A020000, 0x02020008, 0x0A020008,
    0x02020400, 0x0A020400, 0x02020408, 0x0A020408,
    0x02000001, 0x0A000001, 0x02000009, 0x0A000009,
    0x02000401, 0x0A000401, 0x02000409, 0x0A000409,
    0x02020001, 0x0A020001, 0x02020009, 0x0A020009,
    0x02020401, 0x0A020401, 0x02020409, 0x0A020409,
  },
  {
    /* for D bits (numbered as per FIPS 46) 16 17 18 19 20 21 */
    0x00000000, 0x00000100, 0x00080000, 0x00080100,
    0x01000000, 0x01000100, 0x01080000, 0x01080100,
    0x00000010, 0x00000110, 0x00080010, 0x00080110,
    0x01000010, 0x01000110, 0x01080010, 0x01080110,
    0x00200000, 0x00200100, 0x00280000, 0x00280100,
    0x01200000, 0x01200100, 0x01280000, 0x01280100,
    0x00200010, 0x00200110, 0x00280010, 0x00280110,
    0x01200010, 0x01200110, 0x01280010, 0x01280110,
    0x00000200, 0x00000300, 0x00080200, 0x00080300,
    0x01000200, 0x01000300, 0x01080200, 0x01080300,
    0x00000210, 0x00000310, 0x00080210, 0x00080310,
    0x01000210, 0x01000310, 0x01080210, 0x01080310,
    0x00200200, 0x00200300, 0x00280200, 0x00280300,
    0x01200200, 0x01200300, 0x01280200, 0x01280300,
    0x00200210, 0x00200310, 0x00280210, 0x00280310,
    0x01200210, 0x01200310, 0x01280210, 0x01280310,
  },
  {
    /* for D bits (numbered as per FIPS 46) 22 23 24 25 27 28 */
    0x00000000, 0x04000000, 0x00040000, 0x04040000,
    0x00000002, 0x04000002, 0x00040002, 0x04040002,
    0x00002000, 0x04002000, 0x00042000, 0x04042000,
    0x00002002, 0x04002002, 0x00042002, 0x04042002,
    0x00000020, 0x04000020, 0x00040020, 0x04040020,
    0x00000022, 0x04000022, 0x00040022, 0x04040022,
    0x00002020, 0x04002020, 0x00042020, 0x04042020,
    0x00002022, 0x04002022, 0x00042022, 0x04042022,
    0x00000800, 0x04000800, 0x00040800, 0x04040800,
    0x00000802, 0x04000802, 0x00040802, 0x04040802,
    0x00002800, 0x04002800, 0x00042800, 0x04042800,
    0x00002802, 0x04002802, 0x00042802, 0x04042802,
    0x00000820, 0x04000820, 0x00040820, 0x04040820,
    0x00000822, 0x04000822, 0x00040822, 0x04040822,
    0x00002820, 0x04002820, 0x00042820, 0x04042820,
    0x00002822, 0x04002822, 0x00042822, 0x04042822
  }
};

static u32 byte_swap_32 (const u32 n)
{
  return (n & 0xff000000) >> 24
       | (n & 0x00ff0000) >>  8
       | (n & 0x0000ff00) <<  8
       | (n & 0x000000ff) << 24;
}

static u32 rotl32 (const u32 a, const u32 n)
{
  return ((a << n) | (a >> (32 - n)));
}

static void md5_64 (u32 block[16], u32 digest[4])
{
  u32 w0[4];
  u32 w1[4];
  u32 w2[4];
  u32 w3[4];

  w0[0] = block[ 0];
  w0[1] = block[ 1];
  w0[2] = block[ 2];
  w0[3] = block[ 3];
  w1[0] = block[ 4];
  w1[1] = block[ 5];
  w1[2] = block[ 6];
  w1[3] = block[ 7];
  w2[0] = block[ 8];
  w2[1] = block[ 9];
  w2[2] = block[10];
  w2[3] = block[11];
  w3[0] = block[12];
  w3[1] = block[13];
  w3[2] = block[14];
  w3[3] = block[15];

  u32 a = digest[0];
  u32 b = digest[1];
  u32 c = digest[2];
  u32 d = digest[3];

  MD5_STEP (MD5_Fo, a, b, c, d, w0[0], MD5C00, MD5S00);
  MD5_STEP (MD5_Fo, d, a, b, c, w0[1], MD5C01, MD5S01);
  MD5_STEP (MD5_Fo, c, d, a, b, w0[2], MD5C02, MD5S02);
  MD5_STEP (MD5_Fo, b, c, d, a, w0[3], MD5C03, MD5S03);
  MD5_STEP (MD5_Fo, a, b, c, d, w1[0], MD5C04, MD5S00);
  MD5_STEP (MD5_Fo, d, a, b, c, w1[1], MD5C05, MD5S01);
  MD5_STEP (MD5_Fo, c, d, a, b, w1[2], MD5C06, MD5S02);
  MD5_STEP (MD5_Fo, b, c, d, a, w1[3], MD5C07, MD5S03);
  MD5_STEP (MD5_Fo, a, b, c, d, w2[0], MD5C08, MD5S00);
  MD5_STEP (MD5_Fo, d, a, b, c, w2[1], MD5C09, MD5S01);
  MD5_STEP (MD5_Fo, c, d, a, b, w2[2], MD5C0a, MD5S02);
  MD5_STEP (MD5_Fo, b, c, d, a, w2[3], MD5C0b, MD5S03);
  MD5_STEP (MD5_Fo, a, b, c, d, w3[0], MD5C0c, MD5S00);
  MD5_STEP (MD5_Fo, d, a, b, c, w3[1], MD5C0d, MD5S01);
  MD5_STEP (MD5_Fo, c, d, a, b, w3[2], MD5C0e, MD5S02);
  MD5_STEP (MD5_Fo, b, c, d, a, w3[3], MD5C0f, MD5S03);

  MD5_STEP (MD5_Go, a, b, c, d, w0[1], MD5C10, MD5S10);
  MD5_STEP (MD5_Go, d, a, b, c, w1[2], MD5C11, MD5S11);
  MD5_STEP (MD5_Go, c, d, a, b, w2[3], MD5C12, MD5S12);
  MD5_STEP (MD5_Go, b, c, d, a, w0[0], MD5C13, MD5S13);
  MD5_STEP (MD5_Go, a, b, c, d, w1[1], MD5C14, MD5S10);
  MD5_STEP (MD5_Go, d, a, b, c, w2[2], MD5C15, MD5S11);
  MD5_STEP (MD5_Go, c, d, a, b, w3[3], MD5C16, MD5S12);
  MD5_STEP (MD5_Go, b, c, d, a, w1[0], MD5C17, MD5S13);
  MD5_STEP (MD5_Go, a, b, c, d, w2[1], MD5C18, MD5S10);
  MD5_STEP (MD5_Go, d, a, b, c, w3[2], MD5C19, MD5S11);
  MD5_STEP (MD5_Go, c, d, a, b, w0[3], MD5C1a, MD5S12);
  MD5_STEP (MD5_Go, b, c, d, a, w2[0], MD5C1b, MD5S13);
  MD5_STEP (MD5_Go, a, b, c, d, w3[1], MD5C1c, MD5S10);
  MD5_STEP (MD5_Go, d, a, b, c, w0[2], MD5C1d, MD5S11);
  MD5_STEP (MD5_Go, c, d, a, b, w1[3], MD5C1e, MD5S12);
  MD5_STEP (MD5_Go, b, c, d, a, w3[0], MD5C1f, MD5S13);

  MD5_STEP (MD5_H , a, b, c, d, w1[1], MD5C20, MD5S20);
  MD5_STEP (MD5_H , d, a, b, c, w2[0], MD5C21, MD5S21);
  MD5_STEP (MD5_H , c, d, a, b, w2[3], MD5C22, MD5S22);
  MD5_STEP (MD5_H , b, c, d, a, w3[2], MD5C23, MD5S23);
  MD5_STEP (MD5_H , a, b, c, d, w0[1], MD5C24, MD5S20);
  MD5_STEP (MD5_H , d, a, b, c, w1[0], MD5C25, MD5S21);
  MD5_STEP (MD5_H , c, d, a, b, w1[3], MD5C26, MD5S22);
  MD5_STEP (MD5_H , b, c, d, a, w2[2], MD5C27, MD5S23);
  MD5_STEP (MD5_H , a, b, c, d, w3[1], MD5C28, MD5S20);
  MD5_STEP (MD5_H , d, a, b, c, w0[0], MD5C29, MD5S21);
  MD5_STEP (MD5_H , c, d, a, b, w0[3], MD5C2a, MD5S22);
  MD5_STEP (MD5_H , b, c, d, a, w1[2], MD5C2b, MD5S23);
  MD5_STEP (MD5_H , a, b, c, d, w2[1], MD5C2c, MD5S20);
  MD5_STEP (MD5_H , d, a, b, c, w3[0], MD5C2d, MD5S21);
  MD5_STEP (MD5_H , c, d, a, b, w3[3], MD5C2e, MD5S22);
  MD5_STEP (MD5_H , b, c, d, a, w0[2], MD5C2f, MD5S23);

  MD5_STEP (MD5_I , a, b, c, d, w0[0], MD5C30, MD5S30);
  MD5_STEP (MD5_I , d, a, b, c, w1[3], MD5C31, MD5S31);
  MD5_STEP (MD5_I , c, d, a, b, w3[2], MD5C32, MD5S32);
  MD5_STEP (MD5_I , b, c, d, a, w1[1], MD5C33, MD5S33);
  MD5_STEP (MD5_I , a, b, c, d, w3[0], MD5C34, MD5S30);
  MD5_STEP (MD5_I , d, a, b, c, w0[3], MD5C35, MD5S31);
  MD5_STEP (MD5_I , c, d, a, b, w2[2], MD5C36, MD5S32);
  MD5_STEP (MD5_I , b, c, d, a, w0[1], MD5C37, MD5S33);
  MD5_STEP (MD5_I , a, b, c, d, w2[0], MD5C38, MD5S30);
  MD5_STEP (MD5_I , d, a, b, c, w3[3], MD5C39, MD5S31);
  MD5_STEP (MD5_I , c, d, a, b, w1[2], MD5C3a, MD5S32);
  MD5_STEP (MD5_I , b, c, d, a, w3[1], MD5C3b, MD5S33);
  MD5_STEP (MD5_I , a, b, c, d, w1[0], MD5C3c, MD5S30);
  MD5_STEP (MD5_I , d, a, b, c, w2[3], MD5C3d, MD5S31);
  MD5_STEP (MD5_I , c, d, a, b, w0[2], MD5C3e, MD5S32);
  MD5_STEP (MD5_I , b, c, d, a, w2[1], MD5C3f, MD5S33);

  digest[0] += a;
  digest[1] += b;
  digest[2] += c;
  digest[3] += d;
}

static void _des_keysetup (u32 data[2], u32 Kc[16], u32 Kd[16])
{
  u32 c = data[0];
  u32 d = data[1];

  u32 tt;

  PERM_OP  (d, c, tt, 4, 0x0f0f0f0f);
  HPERM_OP (c,    tt, 2, 0xcccc0000);
  HPERM_OP (d,    tt, 2, 0xcccc0000);
  PERM_OP  (d, c, tt, 1, 0x55555555);
  PERM_OP  (c, d, tt, 8, 0x00ff00ff);
  PERM_OP  (d, c, tt, 1, 0x55555555);

  d = ((d & 0x000000ff) << 16)
    | ((d & 0x0000ff00) <<  0)
    | ((d & 0x00ff0000) >> 16)
    | ((c & 0xf0000000) >>  4);

  c = c & 0x0fffffff;

  int i;

  for (i = 0; i < 16; i++)
  {
    const u32 shifts3s0[16] = {  1,  1,  2,  2,  2,  2,  2,  2,  1,  2,  2,  2,  2,  2,  2,  1 };
    const u32 shifts3s1[16] = { 27, 27, 26, 26, 26, 26, 26, 26, 27, 26, 26, 26, 26, 26, 26, 27 };

    c = c >> shifts3s0[i] | c << shifts3s1[i];
    d = d >> shifts3s0[i] | d << shifts3s1[i];

    c = c & 0x0fffffff;
    d = d & 0x0fffffff;

    u32 s = BOX ((( c >>  0) & 0x3f),  0, c_skb)
          | BOX ((((c >>  6) & 0x03)
                | ((c >>  7) & 0x3c)), 1, c_skb)
          | BOX ((((c >> 13) & 0x0f)
                | ((c >> 14) & 0x30)), 2, c_skb)
          | BOX ((((c >> 20) & 0x01)
                | ((c >> 21) & 0x06)
                | ((c >> 22) & 0x38)), 3, c_skb);

    u32 t = BOX ((( d >>  0) & 0x3f),  4, c_skb)
          | BOX ((((d >>  7) & 0x03)
                | ((d >>  8) & 0x3c)), 5, c_skb)
          | BOX ((((d >> 15) & 0x3f)), 6, c_skb)
          | BOX ((((d >> 21) & 0x0f)
                | ((d >> 22) & 0x30)), 7, c_skb);

    Kc[i] = ((t << 16) | (s & 0x0000ffff));
    Kd[i] = ((s >> 16) | (t & 0xffff0000));

    Kc[i] = rotl32 (Kc[i], 2u);
    Kd[i] = rotl32 (Kd[i], 2u);
  }
}

static void _des_encrypt (u32 data[2], u32 Kc[16], u32 Kd[16])
{
  u32 r = data[0];
  u32 l = data[1];

  u32 tt;

  IP (r, l, tt);

  r = rotl32 (r, 3u);
  l = rotl32 (l, 3u);

  int i;

  for (i = 0; i < 16; i++)
  {
    u32 u = Kc[i] ^ r;
    u32 t = Kd[i] ^ rotl32 (r, 28u);

    l ^= BOX (((u >>  2) & 0x3f), 0, c_SPtrans)
       | BOX (((u >> 10) & 0x3f), 2, c_SPtrans)
       | BOX (((u >> 18) & 0x3f), 4, c_SPtrans)
       | BOX (((u >> 26) & 0x3f), 6, c_SPtrans)
       | BOX (((t >>  2) & 0x3f), 1, c_SPtrans)
       | BOX (((t >> 10) & 0x3f), 3, c_SPtrans)
       | BOX (((t >> 18) & 0x3f), 5, c_SPtrans)
       | BOX (((t >> 26) & 0x3f), 7, c_SPtrans);

    tt = l;
    l  = r;
    r  = tt;
  }

  l = rotl32 (l, 29u);
  r = rotl32 (r, 29u);

  FP (r, l, tt);

  data[0] = l;
  data[1] = r;
}

static void transform_netntlmv1_key (const u8 *nthash, u8 *key)
{
  key[0] =                    (nthash[0] >> 0);
  key[1] = (nthash[0] << 7) | (nthash[1] >> 1);
  key[2] = (nthash[1] << 6) | (nthash[2] >> 2);
  key[3] = (nthash[2] << 5) | (nthash[3] >> 3);
  key[4] = (nthash[3] << 4) | (nthash[4] >> 4);
  key[5] = (nthash[4] << 3) | (nthash[5] >> 5);
  key[6] = (nthash[5] << 2) | (nthash[6] >> 6);
  key[7] = (nthash[6] << 1);

  key[0] |= 0x01;
  key[1] |= 0x01;
  key[2] |= 0x01;
  key[3] |= 0x01;
  key[4] |= 0x01;
  key[5] |= 0x01;
  key[6] |= 0x01;
  key[7] |= 0x01;
}

int main (int argc, char *argv[])
{
  u32 ct3_buf[2];
  u32 salt_buf[2];
  u32 chall_buf[6];

  if ((argc != 3) && (argc != 4))
  {
    fprintf (stderr, "usage: %s 8-byte-ct3-in-hex 8-byte-salt-in-hex [24-byte-ESS-in-hex]\n", argv[0]);

    return -1;
  }

  if (sscanf (argv[1], "%08x%08x", &ct3_buf[0],  &ct3_buf[1]) != 2)
  {
    fprintf (stderr, "Invalid data '%s'\n", argv[1]);

    return -1;
  }

  ct3_buf[0] = byte_swap_32 (ct3_buf[0]);
  ct3_buf[1] = byte_swap_32 (ct3_buf[1]);

  if (sscanf (argv[2], "%08x%08x", &salt_buf[0], &salt_buf[1]) != 2)
  {
    fprintf (stderr, "Invalid data '%s'\n", argv[2]);

    return -1;
  }

  salt_buf[0] = byte_swap_32 (salt_buf[0]);
  salt_buf[1] = byte_swap_32 (salt_buf[1]);

  if (argc == 4)
  {
    if (sscanf (argv[3], "%08x%08x%08x%08x%08x%08x", &chall_buf[0], &chall_buf[1], &chall_buf[2], &chall_buf[3], &chall_buf[4], &chall_buf[5]) != 6)
    {
      fprintf (stderr, "Invalid data '%s'\n", argv[3]);

      return -1;
    }

    chall_buf[0] = byte_swap_32 (chall_buf[0]);
    chall_buf[1] = byte_swap_32 (chall_buf[1]);
    chall_buf[2] = byte_swap_32 (chall_buf[2]);
    chall_buf[3] = byte_swap_32 (chall_buf[3]);
    chall_buf[4] = byte_swap_32 (chall_buf[4]);
    chall_buf[5] = byte_swap_32 (chall_buf[5]);

    if ((chall_buf[2] == 0) && (chall_buf[3] == 0) && (chall_buf[4] == 0) && (chall_buf[5] == 0))
    {
      u32 w[16] = { 0 };

      w[ 0] = salt_buf[0];
      w[ 1] = salt_buf[1];
      w[ 2] = chall_buf[0];
      w[ 3] = chall_buf[1];
      w[ 4] = 0x80;
      w[14] = 16 * 8;

      u32 dgst[4] = { 0 };

      dgst[0] = MD5M_A;
      dgst[1] = MD5M_B;
      dgst[2] = MD5M_C;
      dgst[3] = MD5M_D;

      md5_64 (w, dgst);

      salt_buf[0] = dgst[0];
      salt_buf[1] = dgst[1];
    }
  }

  for (u32 i = 0; i < 0x10000; i++)
  {
    u32 key_md4[2] = { i, 0 };
    u32 key_des[2] = { 0, 0 };

    transform_netntlmv1_key ((u8 *) key_md4, (u8 *) key_des);

    u32 Kc[16] = { 0 };
    u32 Kd[16] = { 0 };

    _des_keysetup (key_des, Kc, Kd);

    u32 pt3_buf[2] = { salt_buf[0], salt_buf[1] };

    _des_encrypt (pt3_buf, Kc, Kd);

    if (pt3_buf[0] != ct3_buf[0]) continue;
    if (pt3_buf[1] != ct3_buf[1]) continue;

    printf ("%02x%02x\n", (i >> 0) & 0xff, (i >> 8) & 0xff);

    return 0;
  }

  fprintf (stderr, "Key not found\n");

  return -1;
}