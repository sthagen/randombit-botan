/*
* (C) 2015,2016,2018,2021 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/
#include "fuzzers.h"

#include <botan/internal/divide.h>

void fuzz(std::span<const uint8_t> in) {
   if(in.size() > 2 * 4096 / 8) {
      return;
   }

   // Save on allocations by making these static
   static Botan::BigInt x;
   static Botan::BigInt y;
   static Botan::BigInt q;
   static Botan::BigInt r;
   static Botan::BigInt ct_q;
   static Botan::BigInt ct_r;
   static Botan::BigInt z;

   x = Botan::BigInt::from_bytes(in.subspan(0, in.size() / 2));
   y = Botan::BigInt::from_bytes(in.subspan(in.size() / 2, in.size() - in.size() / 2));

   if(y == 0) {
      return;
   }

   Botan::vartime_divide(x, y, q, r);

   FUZZER_ASSERT_TRUE(r < y);

   z = q * y + r;

   FUZZER_ASSERT_EQUAL(z, x);

   Botan::ct_divide(x, y, ct_q, ct_r);

   FUZZER_ASSERT_EQUAL(q, ct_q);
   FUZZER_ASSERT_EQUAL(r, ct_r);

   // Now divide by just low word of y

   y = y.word_at(0);
   if(y == 0) {
      return;
   }

   Botan::vartime_divide(x, y, q, r);

   FUZZER_ASSERT_TRUE(r < y);
   z = q * y + r;
   FUZZER_ASSERT_EQUAL(z, x);

   Botan::word rw = 0;
   Botan::ct_divide_word(x, y.word_at(0), ct_q, rw);
   FUZZER_ASSERT_EQUAL(ct_q, q);
   FUZZER_ASSERT_EQUAL(rw, r.word_at(0));
}
