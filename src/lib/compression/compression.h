/*
* Compression Transform
* (C) 2014 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_COMPRESSION_TRANSFORM_H_
#define BOTAN_COMPRESSION_TRANSFORM_H_

#include <botan/exceptn.h>
#include <botan/secmem.h>
#include <memory>
#include <string>

namespace Botan {

/**
* Interface for a compression algorithm.
*/
class BOTAN_PUBLIC_API(2, 0) Compression_Algorithm /* NOLINT(*-special-member-functions) */ {
   public:
      /**
      * Create an instance based on a name, or return null if the
      * algo combination cannot be found.
      */
      static std::unique_ptr<Compression_Algorithm> create(std::string_view algo_spec);

      /**
      * Create an instance based on a name
      * @param algo_spec algorithm name
      * Throws Lookup_Error if not found.
      */
      static std::unique_ptr<Compression_Algorithm> create_or_throw(std::string_view algo_spec);

      /**
      * Begin compressing. Most compression algorithms offer a tunable
      * time/compression tradeoff parameter generally represented by an integer
      * in the range of 1 to 9. Higher values typically imply better compression
      * and more memory and/or CPU time consumed by the compression process.
      *
      * If 0 or a value out of range is provided, a compression algorithm
      * specific default is used.
      *
      * @param comp_level the desired level of compression (typically from 1 to 9)
      */
      virtual void start(size_t comp_level = 0) = 0;

      /**
      * Process some data.
      *
      * The leading @p offset bytes of @p buf are ignored and remain untouched;
      * this can be useful for ignoring packet headers.  If @p flush is true,
      * the compression state is flushed, allowing the decompressor to recover
      * the entire message up to this point without having to see the rest of
      * the compressed stream.
      *
      * @param buf in/out parameter which will possibly be resized or swapped
      * @param offset an offset into blocks to begin processing
      * @param flush if true the compressor will be told to flush state
      */
      virtual void update(secure_vector<uint8_t>& buf, size_t offset = 0, bool flush = false) = 0;

      /**
      * Finish compressing
      *
      * The @p buf and @p offset parameters are treated as in update(). It is
      * acceptable to call start() followed by finish() with the entire message,
      * without any intervening call to update().
      *
      * @param final_block in/out parameter
      * @param offset an offset into final_block to begin processing
      */
      virtual void finish(secure_vector<uint8_t>& final_block, size_t offset = 0) = 0;

      /**
      * @return name of the compression algorithm
      */
      virtual std::string name() const = 0;

      /**
      * Reset the state and abort the current message; start can be
      * called again to process a new message.
      */
      virtual void clear() = 0;

      virtual ~Compression_Algorithm() = default;
};

/*
* Interface for a decompression algorithm.
*/
class BOTAN_PUBLIC_API(2, 0) Decompression_Algorithm /* NOLINT(*-special-member-functions) */ {
   public:
      /**
      * Create an instance based on a name, or return null if the
      * algo combination cannot be found.
      */
      static std::unique_ptr<Decompression_Algorithm> create(std::string_view algo_spec);

      /**
      * Create an instance based on a name
      * @param algo_spec algorithm name
      * Throws Lookup_Error if not found.
      */
      static std::unique_ptr<Decompression_Algorithm> create_or_throw(std::string_view algo_spec);

      /**
      * Begin decompressing.
      *
      * This initializes the decompression engine and must be done before
      * calling update() or finish(). No level is provided here; the
      * decompressor can accept input generated by any compression parameters.
      */
      virtual void start() = 0;

      /**
      * Process some data.
      * @param buf in/out parameter which will possibly be resized or swapped
      * @param offset an offset into blocks to begin processing
      */
      virtual void update(secure_vector<uint8_t>& buf, size_t offset = 0) = 0;

      /**
      * Finish decompressing
      *
      * Decompress the material in the in/out parameter @p buf. The leading
      * @p offset bytes of @p buf are ignored and remain untouched; this can
      * be useful for ignoring packet headers.
      *
      * This function may throw if the data seems to be invalid.
      *
      * @param final_block in/out parameter
      * @param offset an offset into final_block to begin processing
      */
      virtual void finish(secure_vector<uint8_t>& final_block, size_t offset = 0) = 0;

      /**
      * @return name of the decompression algorithm
      */
      virtual std::string name() const = 0;

      /**
      * Reset the state and abort the current message; start can be
      * called again to process a new message.
      */
      virtual void clear() = 0;

      virtual ~Decompression_Algorithm() = default;
};

BOTAN_DEPRECATED("Use Compression_Algorithm::create")

inline Compression_Algorithm* make_compressor(std::string_view type) {
   return Compression_Algorithm::create(type).release();
}

BOTAN_DEPRECATED("Use Decompression_Algorithm::create")

inline Decompression_Algorithm* make_decompressor(std::string_view type) {
   return Decompression_Algorithm::create(type).release();
}

/**
* An error that occurred during compression (or decompression)
*/
class BOTAN_PUBLIC_API(2, 9) Compression_Error final : public Exception {
   public:
      /**
      * @param func_name the name of the compression API that was called
      * (eg "BZ2_bzCompressInit" or "lzma_code")
      * @param type what library this came from
      * @param rc the error return code from the compression API. The
      * interpretation of this value will depend on the library.
      */
      Compression_Error(const char* func_name, ErrorType type, int rc);

      ErrorType error_type() const noexcept override { return m_type; }

      int error_code() const noexcept override { return m_rc; }

   private:
      ErrorType m_type;
      int m_rc;
};

/**
* Adapts a zlib style API
*/
class Compression_Stream /* NOLINT(*-special-member-functions) */ {
   public:
      virtual ~Compression_Stream() = default;

      virtual void next_in(uint8_t* b, size_t len) = 0;

      virtual void next_out(uint8_t* b, size_t len) = 0;

      virtual size_t avail_in() const = 0;

      virtual size_t avail_out() const = 0;

      virtual uint32_t run_flag() const = 0;
      virtual uint32_t flush_flag() const = 0;
      virtual uint32_t finish_flag() const = 0;

      virtual bool run(uint32_t flags) = 0;
};

/**
* Used to implement compression using Compression_Stream
*/
class Stream_Compression : public Compression_Algorithm {
   public:
      void update(secure_vector<uint8_t>& buf, size_t offset, bool flush) final;

      void finish(secure_vector<uint8_t>& buf, size_t offset) final;

      void clear() final;

   private:
      void start(size_t level) final;

      void process(secure_vector<uint8_t>& buf, size_t offset, uint32_t flags);

      virtual std::unique_ptr<Compression_Stream> make_stream(size_t level) const = 0;

      secure_vector<uint8_t> m_buffer;
      std::unique_ptr<Compression_Stream> m_stream;
};

/**
* Used to implement decompression using Compression_Stream
*/
class Stream_Decompression : public Decompression_Algorithm {
   public:
      void update(secure_vector<uint8_t>& buf, size_t offset) final;

      void finish(secure_vector<uint8_t>& buf, size_t offset) final;

      void clear() final;

   private:
      void start() final;

      void process(secure_vector<uint8_t>& buf, size_t offset, uint32_t flags);

      virtual std::unique_ptr<Compression_Stream> make_stream() const = 0;

      secure_vector<uint8_t> m_buffer;
      std::unique_ptr<Compression_Stream> m_stream;
};

}  // namespace Botan

#endif
