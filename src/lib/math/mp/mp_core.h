/*
* MPI Algorithms
* (C) 1999-2010,2018,2024 Jack Lloyd
*     2006 Luca Piccarreta
*     2016 Matthias Gierlings
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_MP_CORE_OPS_H_
#define BOTAN_MP_CORE_OPS_H_

#include <botan/assert.h>
#include <botan/exceptn.h>
#include <botan/mem_ops.h>
#include <botan/types.h>
#include <botan/internal/ct_utils.h>
#include <botan/internal/mp_asmi.h>
#include <algorithm>
#include <array>
#include <span>

namespace Botan {

/*
* If cond == 0, does nothing.
* If cond > 0, swaps x[0:size] with y[0:size]
* Runs in constant time
*/
template <WordType W>
inline constexpr void bigint_cnd_swap(W cnd, W x[], W y[], size_t size) {
   const auto mask = CT::Mask<W>::expand(cnd);

   for(size_t i = 0; i != size; ++i) {
      const W a = x[i];
      const W b = y[i];
      x[i] = mask.select(b, a);
      y[i] = mask.select(a, b);
   }
}

/*
* If cond > 0 adds x[0:size] and y[0:size] and returns carry
* Runs in constant time
*/
template <WordType W>
inline constexpr W bigint_cnd_add(W cnd, W x[], const W y[], size_t size) {
   const auto mask = CT::Mask<W>::expand(cnd).value();

   W carry = 0;

   for(size_t i = 0; i != size; ++i) {
      x[i] = word_add(x[i], y[i] & mask, &carry);
   }

   return (mask & carry);
}

/*
* If cond > 0 subtracts y[0:size] from x[0:size] and returns borrow
* Runs in constant time
*/
template <WordType W>
inline constexpr auto bigint_cnd_sub(W cnd, W x[], const W y[], size_t size) -> W {
   const auto mask = CT::Mask<W>::expand(cnd).value();

   W carry = 0;

   for(size_t i = 0; i != size; ++i) {
      x[i] = word_sub(x[i], y[i] & mask, &carry);
   }

   return (mask & carry);
}

/*
* 2s complement absolute value
* If cond > 0 sets x to ~x + 1
* Runs in constant time
*/
template <WordType W>
inline constexpr void bigint_cnd_abs(W cnd, W x[], size_t size) {
   const auto mask = CT::Mask<W>::expand(cnd);

   W carry = mask.if_set_return(1);
   for(size_t i = 0; i != size; ++i) {
      const W z = word_add(~x[i], static_cast<W>(0), &carry);
      x[i] = mask.select(z, x[i]);
   }
}

/**
* Two operand addition with carry out
*/
template <WordType W>
inline constexpr auto bigint_add2(W x[], size_t x_size, const W y[], size_t y_size) -> W {
   W carry = 0;

   BOTAN_ASSERT(x_size >= y_size, "Expected sizes");

   const size_t blocks = y_size - (y_size % 8);

   for(size_t i = 0; i != blocks; i += 8) {
      carry = word8_add2(x + i, y + i, carry);
   }

   for(size_t i = blocks; i != y_size; ++i) {
      x[i] = word_add(x[i], y[i], &carry);
   }

   for(size_t i = y_size; i != x_size; ++i) {
      x[i] = word_add(x[i], static_cast<W>(0), &carry);
   }

   return carry;
}

/**
* Three operand addition with carry out
*/
template <WordType W>
inline constexpr auto bigint_add3(W z[], const W x[], size_t x_size, const W y[], size_t y_size) -> W {
   if(x_size < y_size) {
      return bigint_add3(z, y, y_size, x, x_size);
   }

   W carry = 0;

   const size_t blocks = y_size - (y_size % 8);

   for(size_t i = 0; i != blocks; i += 8) {
      carry = word8_add3(z + i, x + i, y + i, carry);
   }

   for(size_t i = blocks; i != y_size; ++i) {
      z[i] = word_add(x[i], y[i], &carry);
   }

   for(size_t i = y_size; i != x_size; ++i) {
      z[i] = word_add(x[i], static_cast<W>(0), &carry);
   }

   return carry;
}

/**
* Two operand subtraction
*/
template <WordType W>
inline constexpr auto bigint_sub2(W x[], size_t x_size, const W y[], size_t y_size) -> W {
   W borrow = 0;

   BOTAN_ASSERT(x_size >= y_size, "Expected sizes");

   const size_t blocks = y_size - (y_size % 8);

   for(size_t i = 0; i != blocks; i += 8) {
      borrow = word8_sub2(x + i, y + i, borrow);
   }

   for(size_t i = blocks; i != y_size; ++i) {
      x[i] = word_sub(x[i], y[i], &borrow);
   }

   for(size_t i = y_size; i != x_size; ++i) {
      x[i] = word_sub(x[i], static_cast<W>(0), &borrow);
   }

   return borrow;
}

/**
* Two operand subtraction, x = y - x; assumes y >= x
*/
template <WordType W>
inline constexpr void bigint_sub2_rev(W x[], const W y[], size_t y_size) {
   W borrow = 0;

   for(size_t i = 0; i != y_size; ++i) {
      x[i] = word_sub(y[i], x[i], &borrow);
   }

   BOTAN_ASSERT(borrow == 0, "y must be greater than x");
}

/**
* Three operand subtraction
*
* Expects that x_size >= y_size
*
* Writes to z[0:x_size] and returns borrow
*/
template <WordType W>
inline constexpr auto bigint_sub3(W z[], const W x[], size_t x_size, const W y[], size_t y_size) -> W {
   W borrow = 0;

   BOTAN_ASSERT(x_size >= y_size, "Expected sizes");

   const size_t blocks = y_size - (y_size % 8);

   for(size_t i = 0; i != blocks; i += 8) {
      borrow = word8_sub3(z + i, x + i, y + i, borrow);
   }

   for(size_t i = blocks; i != y_size; ++i) {
      z[i] = word_sub(x[i], y[i], &borrow);
   }

   for(size_t i = y_size; i != x_size; ++i) {
      z[i] = word_sub(x[i], static_cast<W>(0), &borrow);
   }

   return borrow;
}

/**
* Conditional subtraction for Montgomery reduction
*
* This function assumes that (x0 || x) is less than 2*p
*
* Computes z[0:N] = (x0 || x[0:N]) - p[0:N]
*
* If z would be positive, returns z[0:N]
* Otherwise returns original input x
*/
template <WordType W>
inline constexpr void bigint_monty_maybe_sub(size_t N, W z[], W x0, const W x[], const W p[]) {
   W borrow = 0;

   const size_t blocks = N - (N % 8);

   for(size_t i = 0; i != blocks; i += 8) {
      borrow = word8_sub3(z + i, x + i, p + i, borrow);
   }

   for(size_t i = blocks; i != N; ++i) {
      z[i] = word_sub(x[i], p[i], &borrow);
   }

   borrow = (x0 - borrow) > x0;

   CT::conditional_assign_mem(borrow, z, x, N);
}

/**
* Conditional subtraction for Montgomery reduction
*
* This function assumes that (x0 || x) is less than 2*p
*
* Computes z[0:N] = (x0 || x[0:N]) - p[0:N]
*
* If z would be positive, returns z[0:N]
* Otherwise returns original input x
*/
template <size_t N, WordType W>
inline constexpr void bigint_monty_maybe_sub(W z[N], W x0, const W x[N], const W y[N]) {
   W borrow = 0;

   for(size_t i = 0; i != N; ++i) {
      z[i] = word_sub(x[i], y[i], &borrow);
   }

   borrow = (x0 - borrow) > x0;

   CT::conditional_assign_mem(borrow, z, x, N);
}

/**
* Return abs(x-y), ie if x >= y, then compute z = x - y
* Otherwise compute z = y - x
* No borrow is possible since the result is always >= 0
*
* Returns a Mask: |1| if x >= y or |0| if x < y
* @param z output array of at least N words
* @param x input array of N words
* @param y input array of N words
* @param N length of x and y
* @param ws array of at least 2*N words
*/
template <WordType W>
inline constexpr auto bigint_sub_abs(W z[], const W x[], const W y[], size_t N, W ws[]) -> CT::Mask<W> {
   // Subtract in both direction then conditional copy out the result

   W* ws0 = ws;
   W* ws1 = ws + N;

   W borrow0 = 0;
   W borrow1 = 0;

   const size_t blocks = N - (N % 8);

   for(size_t i = 0; i != blocks; i += 8) {
      borrow0 = word8_sub3(ws0 + i, x + i, y + i, borrow0);
      borrow1 = word8_sub3(ws1 + i, y + i, x + i, borrow1);
   }

   for(size_t i = blocks; i != N; ++i) {
      ws0[i] = word_sub(x[i], y[i], &borrow0);
      ws1[i] = word_sub(y[i], x[i], &borrow1);
   }

   return CT::conditional_copy_mem(borrow0, z, ws1, ws0, N);
}

/*
* Shift Operations
*/
template <WordType W>
inline constexpr void bigint_shl1(W x[], size_t x_size, size_t x_words, size_t shift) {
   const size_t word_shift = shift / WordInfo<W>::bits;
   const size_t bit_shift = shift % WordInfo<W>::bits;

   copy_mem(x + word_shift, x, x_words);
   clear_mem(x, word_shift);

   const auto carry_mask = CT::Mask<W>::expand(bit_shift);
   const W carry_shift = carry_mask.if_set_return(WordInfo<W>::bits - bit_shift);

   W carry = 0;
   for(size_t i = word_shift; i != x_size; ++i) {
      const W w = x[i];
      x[i] = (w << bit_shift) | carry;
      carry = carry_mask.if_set_return(w >> carry_shift);
   }
}

template <WordType W>
inline constexpr void bigint_shr1(W x[], size_t x_size, size_t shift) {
   const size_t word_shift = shift / WordInfo<W>::bits;
   const size_t bit_shift = shift % WordInfo<W>::bits;

   const size_t top = x_size >= word_shift ? (x_size - word_shift) : 0;

   if(top > 0) {
      copy_mem(x, x + word_shift, top);
   }
   clear_mem(x + top, std::min(word_shift, x_size));

   const auto carry_mask = CT::Mask<W>::expand(bit_shift);
   const W carry_shift = carry_mask.if_set_return(WordInfo<W>::bits - bit_shift);

   W carry = 0;

   for(size_t i = 0; i != top; ++i) {
      const W w = x[top - i - 1];
      x[top - i - 1] = (w >> bit_shift) | carry;
      carry = carry_mask.if_set_return(w << carry_shift);
   }
}

template <WordType W>
inline constexpr void bigint_shl2(W y[], const W x[], size_t x_size, size_t shift) {
   const size_t word_shift = shift / WordInfo<W>::bits;
   const size_t bit_shift = shift % WordInfo<W>::bits;

   copy_mem(y + word_shift, x, x_size);

   const auto carry_mask = CT::Mask<W>::expand(bit_shift);
   const W carry_shift = carry_mask.if_set_return(WordInfo<W>::bits - bit_shift);

   W carry = 0;
   for(size_t i = word_shift; i != x_size + word_shift + 1; ++i) {
      const W w = y[i];
      y[i] = (w << bit_shift) | carry;
      carry = carry_mask.if_set_return(w >> carry_shift);
   }
}

template <WordType W>
inline constexpr void bigint_shr2(W y[], const W x[], size_t x_size, size_t shift) {
   const size_t word_shift = shift / WordInfo<W>::bits;
   const size_t bit_shift = shift % WordInfo<W>::bits;
   const size_t new_size = x_size < word_shift ? 0 : (x_size - word_shift);

   if(new_size > 0) {
      copy_mem(y, x + word_shift, new_size);
   }

   const auto carry_mask = CT::Mask<W>::expand(bit_shift);
   const W carry_shift = carry_mask.if_set_return(WordInfo<W>::bits - bit_shift);

   W carry = 0;
   for(size_t i = new_size; i > 0; --i) {
      W w = y[i - 1];
      y[i - 1] = (w >> bit_shift) | carry;
      carry = carry_mask.if_set_return(w << carry_shift);
   }
}

/*
* Linear Multiply - returns the carry
*/
template <WordType W>
[[nodiscard]] inline constexpr auto bigint_linmul2(W x[], size_t x_size, W y) -> W {
   W carry = 0;

   for(size_t i = 0; i != x_size; ++i) {
      x[i] = word_madd2(x[i], y, &carry);
   }

   return carry;
}

template <WordType W>
inline constexpr void bigint_linmul3(W z[], const W x[], size_t x_size, W y) {
   const size_t blocks = x_size - (x_size % 8);

   W carry = 0;

   for(size_t i = 0; i != blocks; i += 8) {
      carry = word8_linmul3(z + i, x + i, y, carry);
   }

   for(size_t i = blocks; i != x_size; ++i) {
      z[i] = word_madd2(x[i], y, &carry);
   }

   z[x_size] = carry;
}

/**
* Compare x and y
* Return -1 if x < y
* Return 0 if x == y
* Return 1 if x > y
*/
template <WordType W>
inline constexpr int32_t bigint_cmp(const W x[], size_t x_size, const W y[], size_t y_size) {
   static_assert(sizeof(W) >= sizeof(uint32_t), "Size assumption");

   const W LT = static_cast<W>(-1);
   const W EQ = 0;
   const W GT = 1;

   const size_t common_elems = std::min(x_size, y_size);

   W result = EQ;  // until found otherwise

   for(size_t i = 0; i != common_elems; i++) {
      const auto is_eq = CT::Mask<W>::is_equal(x[i], y[i]);
      const auto is_lt = CT::Mask<W>::is_lt(x[i], y[i]);

      result = is_eq.select(result, is_lt.select(LT, GT));
   }

   if(x_size < y_size) {
      W mask = 0;
      for(size_t i = x_size; i != y_size; i++) {
         mask |= y[i];
      }

      // If any bits were set in high part of y, then x < y
      result = CT::Mask<W>::is_zero(mask).select(result, LT);
   } else if(y_size < x_size) {
      W mask = 0;
      for(size_t i = y_size; i != x_size; i++) {
         mask |= x[i];
      }

      // If any bits were set in high part of x, then x > y
      result = CT::Mask<W>::is_zero(mask).select(result, GT);
   }

   CT::unpoison(result);
   BOTAN_DEBUG_ASSERT(result == LT || result == GT || result == EQ);
   return static_cast<int32_t>(result);
}

/**
* Compare x and y
* Returns a Mask: |1| if x[0:x_size] < y[0:y_size] or |0| otherwise
* If lt_or_equal is true, returns |1| also for x == y
*/
template <WordType W>
inline constexpr auto bigint_ct_is_lt(const W x[], size_t x_size, const W y[], size_t y_size, bool lt_or_equal = false)
   -> CT::Mask<W> {
   const size_t common_elems = std::min(x_size, y_size);

   auto is_lt = CT::Mask<W>::expand(lt_or_equal);

   for(size_t i = 0; i != common_elems; i++) {
      const auto eq = CT::Mask<W>::is_equal(x[i], y[i]);
      const auto lt = CT::Mask<W>::is_lt(x[i], y[i]);
      is_lt = eq.select_mask(is_lt, lt);
   }

   if(x_size < y_size) {
      W mask = 0;
      for(size_t i = x_size; i != y_size; i++) {
         mask |= y[i];
      }
      // If any bits were set in high part of y, then is_lt should be forced true
      is_lt |= CT::Mask<W>::expand(mask);
   } else if(y_size < x_size) {
      W mask = 0;
      for(size_t i = y_size; i != x_size; i++) {
         mask |= x[i];
      }

      // If any bits were set in high part of x, then is_lt should be false
      is_lt &= CT::Mask<W>::is_zero(mask);
   }

   return is_lt;
}

template <WordType W>
inline constexpr auto bigint_ct_is_eq(const W x[], size_t x_size, const W y[], size_t y_size) -> CT::Mask<W> {
   const size_t common_elems = std::min(x_size, y_size);

   W diff = 0;

   for(size_t i = 0; i != common_elems; i++) {
      diff |= (x[i] ^ y[i]);
   }

   // If any bits were set in high part of x/y, then they are not equal
   if(x_size < y_size) {
      for(size_t i = x_size; i != y_size; i++) {
         diff |= y[i];
      }
   } else if(y_size < x_size) {
      for(size_t i = y_size; i != x_size; i++) {
         diff |= x[i];
      }
   }

   return CT::Mask<W>::is_zero(diff);
}

/**
* Compute ((n1<<bits) + n0) / d
*/
template <WordType W>
inline constexpr auto bigint_divop_vartime(W n1, W n0, W d) -> W {
   BOTAN_ARG_CHECK(d != 0, "Division by zero");

   if constexpr(WordInfo<W>::dword_is_native) {
      typename WordInfo<W>::dword n = n1;
      n <<= WordInfo<W>::bits;
      n |= n0;
      return static_cast<W>(n / d);
   } else {
      W high = n1 % d;
      W quotient = 0;

      for(size_t i = 0; i != WordInfo<W>::bits; ++i) {
         const W high_top_bit = high >> (WordInfo<W>::bits - 1);

         high <<= 1;
         high |= (n0 >> (WordInfo<W>::bits - 1 - i)) & 1;
         quotient <<= 1;

         if(high_top_bit || high >= d) {
            high -= d;
            quotient |= 1;
         }
      }

      return quotient;
   }
}

/**
* Compute ((n1<<bits) + n0) % d
*/
template <WordType W>
inline constexpr auto bigint_modop_vartime(W n1, W n0, W d) -> W {
   BOTAN_ARG_CHECK(d != 0, "Division by zero");

   W z = bigint_divop_vartime(n1, n0, d);
   W carry = 0;
   z = word_madd2(z, d, &carry);
   return (n0 - z);
}

/*
* Compute an integer x such that (a*x) == -1 (mod 2^n)
*
* Throws an exception if input is even, since in that case no inverse
* exists. If input is odd, then input and 2^n are relatively prime and
* the inverse exists.
*/
template <WordType W>
inline constexpr auto monty_inverse(W a) -> W {
   BOTAN_ARG_CHECK(a % 2 == 1, "Cannot compute Montgomery inverse of an even integer");

   /*
   * From "A New Algorithm for Inversion mod p^k" by Çetin Kaya Koç
   * https://eprint.iacr.org/2017/411.pdf sections 5 and 7.
   */

   W b = 1;
   W r = 0;

   for(size_t i = 0; i != WordInfo<W>::bits; ++i) {
      const W bi = b % 2;
      r >>= 1;
      r += bi << (WordInfo<W>::bits - 1);

      b -= a * bi;
      b >>= 1;
   }

   // Now invert in addition space
   r = (WordInfo<W>::max - r) + 1;

   return r;
}

template <size_t S, WordType W, size_t N>
inline constexpr W shift_left(std::array<W, N>& x) {
   static_assert(S < WordInfo<W>::bits, "Shift too large");

   W carry = 0;
   for(size_t i = 0; i != N; ++i) {
      const W w = x[i];
      x[i] = (w << S) | carry;
      carry = w >> (WordInfo<W>::bits - S);
   }

   return carry;
}

template <size_t S, WordType W, size_t N>
inline constexpr W shift_right(std::array<W, N>& x) {
   static_assert(S < WordInfo<W>::bits, "Shift too large");

   W carry = 0;
   for(size_t i = 0; i != N; ++i) {
      const W w = x[N - 1 - i];
      x[N - 1 - i] = (w >> S) | carry;
      carry = w << (WordInfo<W>::bits - S);
   }

   return carry;
}

// Should be consteval but this triggers a bug in Clang 14
template <WordType W, size_t N>
constexpr auto hex_to_words(const char (&s)[N]) {
   // Char count includes null terminator which we ignore
   const constexpr size_t C = N - 1;

   // Number of nibbles that a word can hold
   const constexpr size_t NPW = (WordInfo<W>::bits / 4);

   // Round up to the next number of words that will fit the input
   const constexpr size_t S = (C + NPW - 1) / NPW;

   auto hex2int = [](char c) -> int8_t {
      if(c >= '0' && c <= '9') {
         return static_cast<int8_t>(c - '0');
      } else if(c >= 'a' && c <= 'f') {
         return static_cast<int8_t>(c - 'a' + 10);
      } else if(c >= 'A' && c <= 'F') {
         return static_cast<int8_t>(c - 'A' + 10);
      } else {
         return -1;
      }
   };

   std::array<W, S> r = {0};

   for(size_t i = 0; i != C; ++i) {
      const int8_t c = hex2int(s[i]);
      if(c >= 0) {
         shift_left<4>(r);
         r[0] += c;
      }
   }

   return r;
}

/*
* Comba Multiplication / Squaring
*/
BOTAN_FUZZER_API void bigint_comba_mul4(word z[8], const word x[4], const word y[4]);
BOTAN_FUZZER_API void bigint_comba_mul6(word z[12], const word x[6], const word y[6]);
BOTAN_FUZZER_API void bigint_comba_mul7(word z[14], const word x[7], const word y[7]);
BOTAN_FUZZER_API void bigint_comba_mul8(word z[16], const word x[8], const word y[8]);
BOTAN_FUZZER_API void bigint_comba_mul9(word z[18], const word x[9], const word y[9]);
BOTAN_FUZZER_API void bigint_comba_mul16(word z[32], const word x[16], const word y[16]);
BOTAN_FUZZER_API void bigint_comba_mul24(word z[48], const word x[24], const word y[24]);

BOTAN_FUZZER_API void bigint_comba_sqr4(word out[8], const word in[4]);
BOTAN_FUZZER_API void bigint_comba_sqr6(word out[12], const word in[6]);
BOTAN_FUZZER_API void bigint_comba_sqr7(word out[14], const word x[7]);
BOTAN_FUZZER_API void bigint_comba_sqr8(word out[16], const word in[8]);
BOTAN_FUZZER_API void bigint_comba_sqr9(word out[18], const word in[9]);
BOTAN_FUZZER_API void bigint_comba_sqr16(word out[32], const word in[16]);
BOTAN_FUZZER_API void bigint_comba_sqr24(word out[48], const word in[24]);

/*
* Comba Fixed Length Multiplication
*/
template <size_t N, WordType W>
constexpr inline void comba_mul(W z[2 * N], const W x[N], const W y[N]) {
   if(!std::is_constant_evaluated()) {
      if constexpr(std::same_as<W, word> && N == 4) {
         return bigint_comba_mul4(z, x, y);
      }
      if constexpr(std::same_as<W, word> && N == 6) {
         return bigint_comba_mul6(z, x, y);
      }
      if constexpr(std::same_as<W, word> && N == 7) {
         return bigint_comba_mul7(z, x, y);
      }
      if constexpr(std::same_as<W, word> && N == 8) {
         return bigint_comba_mul8(z, x, y);
      }
      if constexpr(std::same_as<W, word> && N == 9) {
         return bigint_comba_mul9(z, x, y);
      }
      if constexpr(std::same_as<W, word> && N == 16) {
         return bigint_comba_mul16(z, x, y);
      }
   }

   word3<W> accum;

   for(size_t i = 0; i != 2 * N; ++i) {
      const size_t start = i + 1 < N ? 0 : i + 1 - N;
      const size_t end = std::min(N, i + 1);

      for(size_t j = start; j != end; ++j) {
         accum.mul(x[j], y[i - j]);
      }
      z[i] = accum.extract();
   }
}

template <size_t N, WordType W>
constexpr inline void comba_sqr(W z[2 * N], const W x[N]) {
   if(!std::is_constant_evaluated()) {
      if constexpr(std::same_as<W, word> && N == 4) {
         return bigint_comba_sqr4(z, x);
      }
      if constexpr(std::same_as<W, word> && N == 6) {
         return bigint_comba_sqr6(z, x);
      }
      if constexpr(std::same_as<W, word> && N == 7) {
         return bigint_comba_sqr7(z, x);
      }
      if constexpr(std::same_as<W, word> && N == 8) {
         return bigint_comba_sqr8(z, x);
      }
      if constexpr(std::same_as<W, word> && N == 9) {
         return bigint_comba_sqr9(z, x);
      }
      if constexpr(std::same_as<W, word> && N == 16) {
         return bigint_comba_sqr16(z, x);
      }
   }

   word3<W> accum;

   for(size_t i = 0; i != 2 * N; ++i) {
      const size_t start = i + 1 < N ? 0 : i + 1 - N;
      const size_t end = std::min(N, i + 1);

      for(size_t j = start; j != end; ++j) {
         accum.mul(x[j], x[i - j]);
      }
      z[i] = accum.extract();
   }
}

/*
* Montgomery reduction
*
* Sets r to the Montgomery reduction of z using parameters p / p_dash
*
* The workspace should be of size equal to the prime
*/
BOTAN_FUZZER_API void bigint_monty_redc_4(word r[4], const word z[8], const word p[4], word p_dash, word ws[4]);
BOTAN_FUZZER_API void bigint_monty_redc_6(word r[6], const word z[12], const word p[6], word p_dash, word ws[6]);
BOTAN_FUZZER_API void bigint_monty_redc_8(word r[8], const word z[16], const word p[8], word p_dash, word ws[8]);
BOTAN_FUZZER_API void bigint_monty_redc_12(word r[12], const word z[24], const word p[12], word p_dash, word ws[12]);
BOTAN_FUZZER_API void bigint_monty_redc_16(word r[16], const word z[32], const word p[16], word p_dash, word ws[16]);
BOTAN_FUZZER_API void bigint_monty_redc_24(word r[24], const word z[48], const word p[24], word p_dash, word ws[24]);
BOTAN_FUZZER_API void bigint_monty_redc_32(word r[32], const word z[64], const word p[32], word p_dash, word ws[32]);

BOTAN_FUZZER_API
void bigint_monty_redc_generic(
   word r[], const word z[], size_t z_size, const word p[], size_t p_size, word p_dash, word ws[]);

/**
* Montgomery Reduction
* @param r result of exactly p_size words
* @param z integer to reduce, of size exactly 2*p_size.
* @param p modulus
* @param p_size size of p
* @param p_dash Montgomery value
* @param ws array of at least p_size words
* @param ws_size size of ws in words
*
* It is allowed to set &r[0] == &z[0] however in this case note that only the
* first p_size words of r will be written to and the high p_size words of r/z
* will still hold the original inputs, these must be cleared after use.
* See bigint_monty_redc_inplace
*/
inline void bigint_monty_redc(
   word r[], const word z[], const word p[], size_t p_size, word p_dash, word ws[], size_t ws_size) {
   const size_t z_size = 2 * p_size;

   BOTAN_ARG_CHECK(ws_size >= p_size, "Montgomery reduction workspace too small");

   if(p_size == 4) {
      bigint_monty_redc_4(r, z, p, p_dash, ws);
   } else if(p_size == 6) {
      bigint_monty_redc_6(r, z, p, p_dash, ws);
   } else if(p_size == 8) {
      bigint_monty_redc_8(r, z, p, p_dash, ws);
   } else if(p_size == 12) {
      bigint_monty_redc_12(r, z, p, p_dash, ws);
   } else if(p_size == 16) {
      bigint_monty_redc_16(r, z, p, p_dash, ws);
   } else if(p_size == 24) {
      bigint_monty_redc_24(r, z, p, p_dash, ws);
   } else if(p_size == 32) {
      bigint_monty_redc_32(r, z, p, p_dash, ws);
   } else {
      bigint_monty_redc_generic(r, z, z_size, p, p_size, p_dash, ws);
   }
}

inline void bigint_monty_redc_inplace(word z[], const word p[], size_t p_size, word p_dash, word ws[], size_t ws_size) {
   bigint_monty_redc(z, z, p, p_size, p_dash, ws, ws_size);
   clear_mem(z + p_size, p_size);
}

/**
* Basecase O(N^2) multiplication
*/
BOTAN_FUZZER_API
void basecase_mul(word z[], size_t z_size, const word x[], size_t x_size, const word y[], size_t y_size);

/**
* Basecase O(N^2) squaring
*/
BOTAN_FUZZER_API
void basecase_sqr(word z[], size_t z_size, const word x[], size_t x_size);

/*
* High Level Multiplication/Squaring Interfaces
*/
void bigint_mul(word z[],
                size_t z_size,
                const word x[],
                size_t x_size,
                size_t x_sw,
                const word y[],
                size_t y_size,
                size_t y_sw,
                word workspace[],
                size_t ws_size);

void bigint_sqr(word z[], size_t z_size, const word x[], size_t x_size, size_t x_sw, word workspace[], size_t ws_size);

/**
* Reduce z modulo p = 2**B - C where C is small
*
* z is assumed to be at most (p-1)**2
*
* For details on the algorithm see
* - Handbook of Applied Cryptography, Algorithm 14.47
* - Guide to Elliptic Curve Cryptography, Algorithm 2.54 and Note 2.55
*
*/
template <WordType W, size_t N, W C>
constexpr std::array<W, N> redc_crandall(std::span<const W, 2 * N> z) {
   static_assert(N >= 2);

   std::array<W, N> hi = {};

   // hi = hi * c + lo

   W carry = 0;
   for(size_t i = 0; i != N; ++i) {
      hi[i] = word_madd3(z[i + N], C, z[i], &carry);
   }

   // hi += carry * C
   word carry_c[2] = {0};
   carry_c[0] = word_madd2(carry, C, &carry_c[1]);

   carry = bigint_add2(hi.data(), N, carry_c, 2);

   constexpr W P0 = WordInfo<W>::max - (C - 1);

   std::array<W, N> r = {};

   W borrow = 0;

   /*
   * For undetermined reasons, on GCC (only) removing this asm block causes
   * massive (up to 20%) performance regressions in secp256k1.
   *
   * The generated code without the asm seems quite reasonable, and timing
   * repeated calls to redc_crandall with the cycle counter show that GCC
   * computes it in about the same number of cycles with or without the asm.
   *
   * So the cause of the regression is unclear. But it is reproducible across
   * machines and GCC versions.
   */
#if defined(BOTAN_MP_USE_X86_64_ASM) && defined(__GNUC__) && !defined(__clang__)
   if constexpr(N == 4 && std::same_as<W, uint64_t>) {
      if(!std::is_constant_evaluated()) {
         asm volatile(R"(
                      movq 0(%[x]), %[borrow]
                      subq %[p0], %[borrow]
                      movq %[borrow], 0(%[r])
                      movq 16(%[x]), %[borrow]
                      sbbq $-1, %[borrow]
                      movq %[borrow], 8(%[r])
                      movq 16(%[x]), %[borrow]
                      sbbq $-1, %[borrow]
                      movq %[borrow], 16(%[r])
                      movq 24(%[x]), %[borrow]
                      sbbq $-1, %[borrow]
                      movq %[borrow], 24(%[r])
                      sbbq %[borrow],%[borrow]
                      negq %[borrow]
                      )"
                      : [borrow] "=r"(borrow)
                      : [x] "r"(hi.data()), [p0] "r"(P0), [r] "r"(r.data()), "0"(borrow)
                      : "cc", "memory");
      }

      borrow = (carry - borrow) > carry;
      CT::conditional_assign_mem(borrow, r.data(), hi.data(), N);
      return r;
   }
#endif

   r[0] = word_sub(hi[0], P0, &borrow);
   for(size_t i = 1; i != N; ++i) {
      r[i] = word_sub(hi[i], WordInfo<W>::max, &borrow);
   }

   borrow = (carry - borrow) > carry;

   CT::conditional_assign_mem(borrow, r.data(), hi.data(), N);

   return r;
}

// Extract a WindowBits sized window out of s, depending on offset.
template <size_t WindowBits, typename W, size_t N>
constexpr size_t read_window_bits(std::span<const W, N> words, size_t offset) {
   static_assert(WindowBits >= 1 && WindowBits <= 7);

   constexpr uint8_t WindowMask = static_cast<uint8_t>(1 << WindowBits) - 1;

   constexpr size_t W_bits = sizeof(W) * 8;
   const auto bit_shift = offset % W_bits;
   const auto word_offset = words.size() - 1 - (offset / W_bits);

   const bool single_byte_window = bit_shift <= (W_bits - WindowBits) || word_offset == 0;

   const auto w0 = words[word_offset];

   if(single_byte_window) {
      return (w0 >> bit_shift) & WindowMask;
   } else {
      // Otherwise we must join two words and extract the result
      const auto w1 = words[word_offset - 1];
      const auto combined = ((w0 >> bit_shift) | (w1 << (W_bits - bit_shift)));
      return combined & WindowMask;
   }
}

}  // namespace Botan

#endif
