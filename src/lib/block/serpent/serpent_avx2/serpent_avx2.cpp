/*
* (C) 2018 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/internal/serpent.h>

#include <botan/internal/serpent_sbox.h>
#include <botan/internal/simd_avx2.h>

namespace Botan {

// TODO(Botan4) if minimum GCC is increased we can remove this
#if defined(__GNUG__) && !defined(__clang__) && (__GNUG__ < 13)

// These macros are redundant with the versions in serpent_sbox.h
// but unfortunately removing them seems to trigger a bug in GCC
// when building in amalgamation mode

   #define transform(B0, B1, B2, B3) \
      do {                           \
         B0 = B0.rotl<13>();         \
         B2 = B2.rotl<3>();          \
         B1 ^= B0 ^ B2;              \
         B3 ^= B2 ^ B0.shl<3>();     \
         B1 = B1.rotl<1>();          \
         B3 = B3.rotl<7>();          \
         B0 ^= B1 ^ B3;              \
         B2 ^= B3 ^ B1.shl<7>();     \
         B0 = B0.rotl<5>();          \
         B2 = B2.rotl<22>();         \
      } while(0)

   #define i_transform(B0, B1, B2, B3) \
      do {                             \
         B2 = B2.rotr<22>();           \
         B0 = B0.rotr<5>();            \
         B2 ^= B3 ^ B1.shl<7>();       \
         B0 ^= B1 ^ B3;                \
         B3 = B3.rotr<7>();            \
         B1 = B1.rotr<1>();            \
         B3 ^= B2 ^ B0.shl<3>();       \
         B1 ^= B0 ^ B2;                \
         B2 = B2.rotr<3>();            \
         B0 = B0.rotr<13>();           \
      } while(0)

#endif

void BOTAN_FN_ISA_AVX2 Serpent::avx2_encrypt_8(const uint8_t in[128], uint8_t out[128]) const {
   using namespace Botan::Serpent_F;

   SIMD_8x32::reset_registers();

   SIMD_8x32 B0 = SIMD_8x32::load_le(in);
   SIMD_8x32 B1 = SIMD_8x32::load_le(in + 32);
   SIMD_8x32 B2 = SIMD_8x32::load_le(in + 64);
   SIMD_8x32 B3 = SIMD_8x32::load_le(in + 96);

   SIMD_8x32::transpose(B0, B1, B2, B3);

   const Key_Inserter key_xor(m_round_key.data());

   key_xor(0, B0, B1, B2, B3);
   SBoxE0(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(1, B0, B1, B2, B3);
   SBoxE1(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(2, B0, B1, B2, B3);
   SBoxE2(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(3, B0, B1, B2, B3);
   SBoxE3(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(4, B0, B1, B2, B3);
   SBoxE4(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(5, B0, B1, B2, B3);
   SBoxE5(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(6, B0, B1, B2, B3);
   SBoxE6(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(7, B0, B1, B2, B3);
   SBoxE7(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);

   key_xor(8, B0, B1, B2, B3);
   SBoxE0(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(9, B0, B1, B2, B3);
   SBoxE1(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(10, B0, B1, B2, B3);
   SBoxE2(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(11, B0, B1, B2, B3);
   SBoxE3(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(12, B0, B1, B2, B3);
   SBoxE4(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(13, B0, B1, B2, B3);
   SBoxE5(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(14, B0, B1, B2, B3);
   SBoxE6(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(15, B0, B1, B2, B3);
   SBoxE7(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);

   key_xor(16, B0, B1, B2, B3);
   SBoxE0(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(17, B0, B1, B2, B3);
   SBoxE1(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(18, B0, B1, B2, B3);
   SBoxE2(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(19, B0, B1, B2, B3);
   SBoxE3(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(20, B0, B1, B2, B3);
   SBoxE4(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(21, B0, B1, B2, B3);
   SBoxE5(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(22, B0, B1, B2, B3);
   SBoxE6(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(23, B0, B1, B2, B3);
   SBoxE7(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);

   key_xor(24, B0, B1, B2, B3);
   SBoxE0(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(25, B0, B1, B2, B3);
   SBoxE1(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(26, B0, B1, B2, B3);
   SBoxE2(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(27, B0, B1, B2, B3);
   SBoxE3(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(28, B0, B1, B2, B3);
   SBoxE4(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(29, B0, B1, B2, B3);
   SBoxE5(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(30, B0, B1, B2, B3);
   SBoxE6(B0, B1, B2, B3);
   transform(B0, B1, B2, B3);
   key_xor(31, B0, B1, B2, B3);
   SBoxE7(B0, B1, B2, B3);
   key_xor(32, B0, B1, B2, B3);

   SIMD_8x32::transpose(B0, B1, B2, B3);
   B0.store_le(out);
   B1.store_le(out + 32);
   B2.store_le(out + 64);
   B3.store_le(out + 96);

   SIMD_8x32::zero_registers();
}

void BOTAN_FN_ISA_AVX2 Serpent::avx2_decrypt_8(const uint8_t in[128], uint8_t out[128]) const {
   using namespace Botan::Serpent_F;

   SIMD_8x32::reset_registers();

   SIMD_8x32 B0 = SIMD_8x32::load_le(in);
   SIMD_8x32 B1 = SIMD_8x32::load_le(in + 32);
   SIMD_8x32 B2 = SIMD_8x32::load_le(in + 64);
   SIMD_8x32 B3 = SIMD_8x32::load_le(in + 96);

   SIMD_8x32::transpose(B0, B1, B2, B3);

   const Key_Inserter key_xor(m_round_key.data());

   key_xor(32, B0, B1, B2, B3);
   SBoxD7(B0, B1, B2, B3);
   key_xor(31, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD6(B0, B1, B2, B3);
   key_xor(30, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD5(B0, B1, B2, B3);
   key_xor(29, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD4(B0, B1, B2, B3);
   key_xor(28, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD3(B0, B1, B2, B3);
   key_xor(27, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD2(B0, B1, B2, B3);
   key_xor(26, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD1(B0, B1, B2, B3);
   key_xor(25, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD0(B0, B1, B2, B3);
   key_xor(24, B0, B1, B2, B3);

   i_transform(B0, B1, B2, B3);
   SBoxD7(B0, B1, B2, B3);
   key_xor(23, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD6(B0, B1, B2, B3);
   key_xor(22, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD5(B0, B1, B2, B3);
   key_xor(21, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD4(B0, B1, B2, B3);
   key_xor(20, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD3(B0, B1, B2, B3);
   key_xor(19, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD2(B0, B1, B2, B3);
   key_xor(18, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD1(B0, B1, B2, B3);
   key_xor(17, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD0(B0, B1, B2, B3);
   key_xor(16, B0, B1, B2, B3);

   i_transform(B0, B1, B2, B3);
   SBoxD7(B0, B1, B2, B3);
   key_xor(15, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD6(B0, B1, B2, B3);
   key_xor(14, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD5(B0, B1, B2, B3);
   key_xor(13, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD4(B0, B1, B2, B3);
   key_xor(12, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD3(B0, B1, B2, B3);
   key_xor(11, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD2(B0, B1, B2, B3);
   key_xor(10, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD1(B0, B1, B2, B3);
   key_xor(9, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD0(B0, B1, B2, B3);
   key_xor(8, B0, B1, B2, B3);

   i_transform(B0, B1, B2, B3);
   SBoxD7(B0, B1, B2, B3);
   key_xor(7, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD6(B0, B1, B2, B3);
   key_xor(6, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD5(B0, B1, B2, B3);
   key_xor(5, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD4(B0, B1, B2, B3);
   key_xor(4, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD3(B0, B1, B2, B3);
   key_xor(3, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD2(B0, B1, B2, B3);
   key_xor(2, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD1(B0, B1, B2, B3);
   key_xor(1, B0, B1, B2, B3);
   i_transform(B0, B1, B2, B3);
   SBoxD0(B0, B1, B2, B3);
   key_xor(0, B0, B1, B2, B3);

   SIMD_8x32::transpose(B0, B1, B2, B3);

   B0.store_le(out);
   B1.store_le(out + 32);
   B2.store_le(out + 64);
   B3.store_le(out + 96);

   SIMD_8x32::zero_registers();
}

#undef transform
#undef i_transform

}  // namespace Botan
