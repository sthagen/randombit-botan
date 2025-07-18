/*
* AES using POWER8/POWER9 crypto extensions
*
* Contributed by Jeffrey Walton
*
* Further changes
* (C) 2018,2019 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/internal/aes.h>

#include <botan/internal/isa_extn.h>
#include <bit>

#include <altivec.h>
#undef vector
#undef bool

namespace Botan {

// NOLINTBEGIN(readability-container-data-pointer)

typedef __vector unsigned long long Altivec64x2;
typedef __vector unsigned int Altivec32x4;
typedef __vector unsigned char Altivec8x16;

namespace {

static_assert(std::endian::native == std::endian::big || std::endian::native == std::endian::little);

BOTAN_FORCE_INLINE Altivec8x16 reverse_vec(Altivec8x16 src) {
   if constexpr(std::endian::native == std::endian::little) {
      const Altivec8x16 mask = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
      const Altivec8x16 zero = {0};
      return vec_perm(src, zero, mask);
   } else {
      return src;
   }
}

BOTAN_FN_ISA_AES BOTAN_FORCE_INLINE Altivec64x2 load_key(const uint32_t key[]) {
   return reinterpret_cast<Altivec64x2>(reverse_vec(reinterpret_cast<Altivec8x16>(vec_vsx_ld(0, key))));
}

BOTAN_FN_ISA_AES BOTAN_FORCE_INLINE Altivec64x2 load_block(const uint8_t src[]) {
   return reinterpret_cast<Altivec64x2>(reverse_vec(vec_vsx_ld(0, src)));
}

BOTAN_FN_ISA_AES BOTAN_FORCE_INLINE void store_block(Altivec64x2 src, uint8_t dest[]) {
   vec_vsx_st(reverse_vec(reinterpret_cast<Altivec8x16>(src)), 0, dest);
}

BOTAN_FORCE_INLINE void store_blocks(Altivec64x2 B0, Altivec64x2 B1, Altivec64x2 B2, Altivec64x2 B3, uint8_t out[]) {
   store_block(B0, out);
   store_block(B1, out + 16);
   store_block(B2, out + 16 * 2);
   store_block(B3, out + 16 * 3);
}

BOTAN_FN_ISA_AES BOTAN_FORCE_INLINE void xor_blocks(
   Altivec64x2& B0, Altivec64x2& B1, Altivec64x2& B2, Altivec64x2& B3, Altivec64x2 K) {
   B0 = vec_xor(B0, K);
   B1 = vec_xor(B1, K);
   B2 = vec_xor(B2, K);
   B3 = vec_xor(B3, K);
}

BOTAN_FN_ISA_AES BOTAN_FORCE_INLINE void aes_vcipher(Altivec64x2& B, Altivec64x2 K) {
#if defined(__clang__)
   B = reinterpret_cast<Altivec64x2>(
      __builtin_crypto_vcipher(reinterpret_cast<Altivec8x16>(B), reinterpret_cast<Altivec8x16>(K)));
#else
   B = __builtin_crypto_vcipher(B, K);
#endif
}

BOTAN_FN_ISA_AES BOTAN_FORCE_INLINE void aes_vcipherlast(Altivec64x2& B, Altivec64x2 K) {
#if defined(__clang__)
   B = reinterpret_cast<Altivec64x2>(
      __builtin_crypto_vcipherlast(reinterpret_cast<Altivec8x16>(B), reinterpret_cast<Altivec8x16>(K)));
#else
   B = __builtin_crypto_vcipherlast(B, K);
#endif
}

BOTAN_FN_ISA_AES BOTAN_FORCE_INLINE void aes_vncipher(Altivec64x2& B, Altivec64x2 K) {
#if defined(__clang__)
   B = reinterpret_cast<Altivec64x2>(
      __builtin_crypto_vncipher(reinterpret_cast<Altivec8x16>(B), reinterpret_cast<Altivec8x16>(K)));
#else
   B = __builtin_crypto_vncipher(B, K);
#endif
}

BOTAN_FN_ISA_AES BOTAN_FORCE_INLINE void aes_vncipherlast(Altivec64x2& B, Altivec64x2 K) {
#if defined(__clang__)
   B = reinterpret_cast<Altivec64x2>(
      __builtin_crypto_vncipherlast(reinterpret_cast<Altivec8x16>(B), reinterpret_cast<Altivec8x16>(K)));
#else
   B = __builtin_crypto_vncipherlast(B, K);
#endif
}

BOTAN_FN_ISA_AES BOTAN_FORCE_INLINE void aes_vcipher(
   Altivec64x2& B0, Altivec64x2& B1, Altivec64x2& B2, Altivec64x2& B3, Altivec64x2 K) {
   aes_vcipher(B0, K);
   aes_vcipher(B1, K);
   aes_vcipher(B2, K);
   aes_vcipher(B3, K);
}

BOTAN_FN_ISA_AES BOTAN_FORCE_INLINE void aes_vcipherlast(
   Altivec64x2& B0, Altivec64x2& B1, Altivec64x2& B2, Altivec64x2& B3, Altivec64x2 K) {
   aes_vcipherlast(B0, K);
   aes_vcipherlast(B1, K);
   aes_vcipherlast(B2, K);
   aes_vcipherlast(B3, K);
}

BOTAN_FN_ISA_AES BOTAN_FORCE_INLINE void aes_vncipher(
   Altivec64x2& B0, Altivec64x2& B1, Altivec64x2& B2, Altivec64x2& B3, Altivec64x2 K) {
   aes_vncipher(B0, K);
   aes_vncipher(B1, K);
   aes_vncipher(B2, K);
   aes_vncipher(B3, K);
}

BOTAN_FN_ISA_AES BOTAN_FORCE_INLINE void aes_vncipherlast(
   Altivec64x2& B0, Altivec64x2& B1, Altivec64x2& B2, Altivec64x2& B3, Altivec64x2 K) {
   aes_vncipherlast(B0, K);
   aes_vncipherlast(B1, K);
   aes_vncipherlast(B2, K);
   aes_vncipherlast(B3, K);
}

}  // namespace

BOTAN_FN_ISA_AES void AES_128::hw_aes_encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const {
   const Altivec64x2 K0 = load_key(&m_EK[0]);
   const Altivec64x2 K1 = load_key(&m_EK[4]);
   const Altivec64x2 K2 = load_key(&m_EK[8]);
   const Altivec64x2 K3 = load_key(&m_EK[12]);
   const Altivec64x2 K4 = load_key(&m_EK[16]);
   const Altivec64x2 K5 = load_key(&m_EK[20]);
   const Altivec64x2 K6 = load_key(&m_EK[24]);
   const Altivec64x2 K7 = load_key(&m_EK[28]);
   const Altivec64x2 K8 = load_key(&m_EK[32]);
   const Altivec64x2 K9 = load_key(&m_EK[36]);
   const Altivec64x2 K10 = load_key(&m_EK[40]);

   while(blocks >= 4) {
      Altivec64x2 B0 = load_block(in);
      Altivec64x2 B1 = load_block(in + 16);
      Altivec64x2 B2 = load_block(in + 16 * 2);
      Altivec64x2 B3 = load_block(in + 16 * 3);

      xor_blocks(B0, B1, B2, B3, K0);
      aes_vcipher(B0, B1, B2, B3, K1);
      aes_vcipher(B0, B1, B2, B3, K2);
      aes_vcipher(B0, B1, B2, B3, K3);
      aes_vcipher(B0, B1, B2, B3, K4);
      aes_vcipher(B0, B1, B2, B3, K5);
      aes_vcipher(B0, B1, B2, B3, K6);
      aes_vcipher(B0, B1, B2, B3, K7);
      aes_vcipher(B0, B1, B2, B3, K8);
      aes_vcipher(B0, B1, B2, B3, K9);
      aes_vcipherlast(B0, B1, B2, B3, K10);

      store_blocks(B0, B1, B2, B3, out);

      out += 4 * 16;
      in += 4 * 16;
      blocks -= 4;
   }

   for(size_t i = 0; i != blocks; ++i) {
      Altivec64x2 B = load_block(in);

      B = vec_xor(B, K0);
      aes_vcipher(B, K1);
      aes_vcipher(B, K2);
      aes_vcipher(B, K3);
      aes_vcipher(B, K4);
      aes_vcipher(B, K5);
      aes_vcipher(B, K6);
      aes_vcipher(B, K7);
      aes_vcipher(B, K8);
      aes_vcipher(B, K9);
      aes_vcipherlast(B, K10);

      store_block(B, out);

      out += 16;
      in += 16;
   }
}

BOTAN_FN_ISA_AES void AES_128::hw_aes_decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const {
   const Altivec64x2 K0 = load_key(&m_EK[40]);
   const Altivec64x2 K1 = load_key(&m_EK[36]);
   const Altivec64x2 K2 = load_key(&m_EK[32]);
   const Altivec64x2 K3 = load_key(&m_EK[28]);
   const Altivec64x2 K4 = load_key(&m_EK[24]);
   const Altivec64x2 K5 = load_key(&m_EK[20]);
   const Altivec64x2 K6 = load_key(&m_EK[16]);
   const Altivec64x2 K7 = load_key(&m_EK[12]);
   const Altivec64x2 K8 = load_key(&m_EK[8]);
   const Altivec64x2 K9 = load_key(&m_EK[4]);
   const Altivec64x2 K10 = load_key(&m_EK[0]);

   while(blocks >= 4) {
      Altivec64x2 B0 = load_block(in);
      Altivec64x2 B1 = load_block(in + 16);
      Altivec64x2 B2 = load_block(in + 16 * 2);
      Altivec64x2 B3 = load_block(in + 16 * 3);

      xor_blocks(B0, B1, B2, B3, K0);
      aes_vncipher(B0, B1, B2, B3, K1);
      aes_vncipher(B0, B1, B2, B3, K2);
      aes_vncipher(B0, B1, B2, B3, K3);
      aes_vncipher(B0, B1, B2, B3, K4);
      aes_vncipher(B0, B1, B2, B3, K5);
      aes_vncipher(B0, B1, B2, B3, K6);
      aes_vncipher(B0, B1, B2, B3, K7);
      aes_vncipher(B0, B1, B2, B3, K8);
      aes_vncipher(B0, B1, B2, B3, K9);
      aes_vncipherlast(B0, B1, B2, B3, K10);

      store_blocks(B0, B1, B2, B3, out);

      out += 4 * 16;
      in += 4 * 16;
      blocks -= 4;
   }

   for(size_t i = 0; i != blocks; ++i) {
      Altivec64x2 B = load_block(in);

      B = vec_xor(B, K0);
      aes_vncipher(B, K1);
      aes_vncipher(B, K2);
      aes_vncipher(B, K3);
      aes_vncipher(B, K4);
      aes_vncipher(B, K5);
      aes_vncipher(B, K6);
      aes_vncipher(B, K7);
      aes_vncipher(B, K8);
      aes_vncipher(B, K9);
      aes_vncipherlast(B, K10);

      store_block(B, out);

      out += 16;
      in += 16;
   }
}

BOTAN_FN_ISA_AES void AES_192::hw_aes_encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const {
   const Altivec64x2 K0 = load_key(&m_EK[0]);
   const Altivec64x2 K1 = load_key(&m_EK[4]);
   const Altivec64x2 K2 = load_key(&m_EK[8]);
   const Altivec64x2 K3 = load_key(&m_EK[12]);
   const Altivec64x2 K4 = load_key(&m_EK[16]);
   const Altivec64x2 K5 = load_key(&m_EK[20]);
   const Altivec64x2 K6 = load_key(&m_EK[24]);
   const Altivec64x2 K7 = load_key(&m_EK[28]);
   const Altivec64x2 K8 = load_key(&m_EK[32]);
   const Altivec64x2 K9 = load_key(&m_EK[36]);
   const Altivec64x2 K10 = load_key(&m_EK[40]);
   const Altivec64x2 K11 = load_key(&m_EK[44]);
   const Altivec64x2 K12 = load_key(&m_EK[48]);

   while(blocks >= 4) {
      Altivec64x2 B0 = load_block(in);
      Altivec64x2 B1 = load_block(in + 16);
      Altivec64x2 B2 = load_block(in + 16 * 2);
      Altivec64x2 B3 = load_block(in + 16 * 3);

      xor_blocks(B0, B1, B2, B3, K0);
      aes_vcipher(B0, B1, B2, B3, K1);
      aes_vcipher(B0, B1, B2, B3, K2);
      aes_vcipher(B0, B1, B2, B3, K3);
      aes_vcipher(B0, B1, B2, B3, K4);
      aes_vcipher(B0, B1, B2, B3, K5);
      aes_vcipher(B0, B1, B2, B3, K6);
      aes_vcipher(B0, B1, B2, B3, K7);
      aes_vcipher(B0, B1, B2, B3, K8);
      aes_vcipher(B0, B1, B2, B3, K9);
      aes_vcipher(B0, B1, B2, B3, K10);
      aes_vcipher(B0, B1, B2, B3, K11);
      aes_vcipherlast(B0, B1, B2, B3, K12);

      store_blocks(B0, B1, B2, B3, out);

      out += 4 * 16;
      in += 4 * 16;
      blocks -= 4;
   }

   for(size_t i = 0; i != blocks; ++i) {
      Altivec64x2 B = load_block(in);

      B = vec_xor(B, K0);
      aes_vcipher(B, K1);
      aes_vcipher(B, K2);
      aes_vcipher(B, K3);
      aes_vcipher(B, K4);
      aes_vcipher(B, K5);
      aes_vcipher(B, K6);
      aes_vcipher(B, K7);
      aes_vcipher(B, K8);
      aes_vcipher(B, K9);
      aes_vcipher(B, K10);
      aes_vcipher(B, K11);
      aes_vcipherlast(B, K12);

      store_block(B, out);

      out += 16;
      in += 16;
   }
}

BOTAN_FN_ISA_AES void AES_192::hw_aes_decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const {
   const Altivec64x2 K0 = load_key(&m_EK[48]);
   const Altivec64x2 K1 = load_key(&m_EK[44]);
   const Altivec64x2 K2 = load_key(&m_EK[40]);
   const Altivec64x2 K3 = load_key(&m_EK[36]);
   const Altivec64x2 K4 = load_key(&m_EK[32]);
   const Altivec64x2 K5 = load_key(&m_EK[28]);
   const Altivec64x2 K6 = load_key(&m_EK[24]);
   const Altivec64x2 K7 = load_key(&m_EK[20]);
   const Altivec64x2 K8 = load_key(&m_EK[16]);
   const Altivec64x2 K9 = load_key(&m_EK[12]);
   const Altivec64x2 K10 = load_key(&m_EK[8]);
   const Altivec64x2 K11 = load_key(&m_EK[4]);
   const Altivec64x2 K12 = load_key(&m_EK[0]);

   while(blocks >= 4) {
      Altivec64x2 B0 = load_block(in);
      Altivec64x2 B1 = load_block(in + 16);
      Altivec64x2 B2 = load_block(in + 16 * 2);
      Altivec64x2 B3 = load_block(in + 16 * 3);

      xor_blocks(B0, B1, B2, B3, K0);
      aes_vncipher(B0, B1, B2, B3, K1);
      aes_vncipher(B0, B1, B2, B3, K2);
      aes_vncipher(B0, B1, B2, B3, K3);
      aes_vncipher(B0, B1, B2, B3, K4);
      aes_vncipher(B0, B1, B2, B3, K5);
      aes_vncipher(B0, B1, B2, B3, K6);
      aes_vncipher(B0, B1, B2, B3, K7);
      aes_vncipher(B0, B1, B2, B3, K8);
      aes_vncipher(B0, B1, B2, B3, K9);
      aes_vncipher(B0, B1, B2, B3, K10);
      aes_vncipher(B0, B1, B2, B3, K11);
      aes_vncipherlast(B0, B1, B2, B3, K12);

      store_blocks(B0, B1, B2, B3, out);

      out += 4 * 16;
      in += 4 * 16;
      blocks -= 4;
   }

   for(size_t i = 0; i != blocks; ++i) {
      Altivec64x2 B = load_block(in);

      B = vec_xor(B, K0);
      aes_vncipher(B, K1);
      aes_vncipher(B, K2);
      aes_vncipher(B, K3);
      aes_vncipher(B, K4);
      aes_vncipher(B, K5);
      aes_vncipher(B, K6);
      aes_vncipher(B, K7);
      aes_vncipher(B, K8);
      aes_vncipher(B, K9);
      aes_vncipher(B, K10);
      aes_vncipher(B, K11);
      aes_vncipherlast(B, K12);

      store_block(B, out);

      out += 16;
      in += 16;
   }
}

BOTAN_FN_ISA_AES void AES_256::hw_aes_encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const {
   const Altivec64x2 K0 = load_key(&m_EK[0]);
   const Altivec64x2 K1 = load_key(&m_EK[4]);
   const Altivec64x2 K2 = load_key(&m_EK[8]);
   const Altivec64x2 K3 = load_key(&m_EK[12]);
   const Altivec64x2 K4 = load_key(&m_EK[16]);
   const Altivec64x2 K5 = load_key(&m_EK[20]);
   const Altivec64x2 K6 = load_key(&m_EK[24]);
   const Altivec64x2 K7 = load_key(&m_EK[28]);
   const Altivec64x2 K8 = load_key(&m_EK[32]);
   const Altivec64x2 K9 = load_key(&m_EK[36]);
   const Altivec64x2 K10 = load_key(&m_EK[40]);
   const Altivec64x2 K11 = load_key(&m_EK[44]);
   const Altivec64x2 K12 = load_key(&m_EK[48]);
   const Altivec64x2 K13 = load_key(&m_EK[52]);
   const Altivec64x2 K14 = load_key(&m_EK[56]);

   while(blocks >= 4) {
      Altivec64x2 B0 = load_block(in);
      Altivec64x2 B1 = load_block(in + 16);
      Altivec64x2 B2 = load_block(in + 16 * 2);
      Altivec64x2 B3 = load_block(in + 16 * 3);

      xor_blocks(B0, B1, B2, B3, K0);
      aes_vcipher(B0, B1, B2, B3, K1);
      aes_vcipher(B0, B1, B2, B3, K2);
      aes_vcipher(B0, B1, B2, B3, K3);
      aes_vcipher(B0, B1, B2, B3, K4);
      aes_vcipher(B0, B1, B2, B3, K5);
      aes_vcipher(B0, B1, B2, B3, K6);
      aes_vcipher(B0, B1, B2, B3, K7);
      aes_vcipher(B0, B1, B2, B3, K8);
      aes_vcipher(B0, B1, B2, B3, K9);
      aes_vcipher(B0, B1, B2, B3, K10);
      aes_vcipher(B0, B1, B2, B3, K11);
      aes_vcipher(B0, B1, B2, B3, K12);
      aes_vcipher(B0, B1, B2, B3, K13);
      aes_vcipherlast(B0, B1, B2, B3, K14);

      store_blocks(B0, B1, B2, B3, out);

      out += 4 * 16;
      in += 4 * 16;
      blocks -= 4;
   }

   for(size_t i = 0; i != blocks; ++i) {
      Altivec64x2 B = load_block(in);

      B = vec_xor(B, K0);
      aes_vcipher(B, K1);
      aes_vcipher(B, K2);
      aes_vcipher(B, K3);
      aes_vcipher(B, K4);
      aes_vcipher(B, K5);
      aes_vcipher(B, K6);
      aes_vcipher(B, K7);
      aes_vcipher(B, K8);
      aes_vcipher(B, K9);
      aes_vcipher(B, K10);
      aes_vcipher(B, K11);
      aes_vcipher(B, K12);
      aes_vcipher(B, K13);
      aes_vcipherlast(B, K14);

      store_block(B, out);

      out += 16;
      in += 16;
   }
}

BOTAN_FN_ISA_AES void AES_256::hw_aes_decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const {
   const Altivec64x2 K0 = load_key(&m_EK[56]);
   const Altivec64x2 K1 = load_key(&m_EK[52]);
   const Altivec64x2 K2 = load_key(&m_EK[48]);
   const Altivec64x2 K3 = load_key(&m_EK[44]);
   const Altivec64x2 K4 = load_key(&m_EK[40]);
   const Altivec64x2 K5 = load_key(&m_EK[36]);
   const Altivec64x2 K6 = load_key(&m_EK[32]);
   const Altivec64x2 K7 = load_key(&m_EK[28]);
   const Altivec64x2 K8 = load_key(&m_EK[24]);
   const Altivec64x2 K9 = load_key(&m_EK[20]);
   const Altivec64x2 K10 = load_key(&m_EK[16]);
   const Altivec64x2 K11 = load_key(&m_EK[12]);
   const Altivec64x2 K12 = load_key(&m_EK[8]);
   const Altivec64x2 K13 = load_key(&m_EK[4]);
   const Altivec64x2 K14 = load_key(&m_EK[0]);

   while(blocks >= 4) {
      Altivec64x2 B0 = load_block(in);
      Altivec64x2 B1 = load_block(in + 16);
      Altivec64x2 B2 = load_block(in + 16 * 2);
      Altivec64x2 B3 = load_block(in + 16 * 3);

      xor_blocks(B0, B1, B2, B3, K0);
      aes_vncipher(B0, B1, B2, B3, K1);
      aes_vncipher(B0, B1, B2, B3, K2);
      aes_vncipher(B0, B1, B2, B3, K3);
      aes_vncipher(B0, B1, B2, B3, K4);
      aes_vncipher(B0, B1, B2, B3, K5);
      aes_vncipher(B0, B1, B2, B3, K6);
      aes_vncipher(B0, B1, B2, B3, K7);
      aes_vncipher(B0, B1, B2, B3, K8);
      aes_vncipher(B0, B1, B2, B3, K9);
      aes_vncipher(B0, B1, B2, B3, K10);
      aes_vncipher(B0, B1, B2, B3, K11);
      aes_vncipher(B0, B1, B2, B3, K12);
      aes_vncipher(B0, B1, B2, B3, K13);
      aes_vncipherlast(B0, B1, B2, B3, K14);

      store_blocks(B0, B1, B2, B3, out);

      out += 4 * 16;
      in += 4 * 16;
      blocks -= 4;
   }

   for(size_t i = 0; i != blocks; ++i) {
      Altivec64x2 B = load_block(in);

      B = vec_xor(B, K0);
      aes_vncipher(B, K1);
      aes_vncipher(B, K2);
      aes_vncipher(B, K3);
      aes_vncipher(B, K4);
      aes_vncipher(B, K5);
      aes_vncipher(B, K6);
      aes_vncipher(B, K7);
      aes_vncipher(B, K8);
      aes_vncipher(B, K9);
      aes_vncipher(B, K10);
      aes_vncipher(B, K11);
      aes_vncipher(B, K12);
      aes_vncipher(B, K13);
      aes_vncipherlast(B, K14);

      store_block(B, out);

      out += 16;
      in += 16;
   }
}

// NOLINTEND(readability-container-data-pointer)

}  // namespace Botan
