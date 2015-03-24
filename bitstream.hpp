/**
 * @file bitstream.hpp
 * @author Robin Dietrich <me (at) invokr (dot) org>
 * @version 1.0
 * @par License Public Domain
 *
 * Inspired by Valve's and Raknet's bitstreams
 */

#ifndef _RD_BITSTREAM_HPP_
#define _RD_BITSTREAM_HPP_

#include <string>

#include <cassert>
#include <cstdint>
#include <cstddef>
#include <cstring>

/** Read / Write bitstream */
class bitstream {
public:
    /** Current I/O mode */
    enum class mode {
        io_unset  = 0,
        io_reader = 1,
        io_writer = 2
    };

    /** Different kinds of possible bitstream errors */
    enum class error {
        none  = 0, // Nothing wrong
        redef = 1, // Trying to redefine buffer or type
        size  = 2  // Buffer size would overflow
    };

    /** Underlying word type */
    typedef uint64_t word_t;

// Constructors / Destructors
public:
    /**
     * Constructs an empty bitstream.
     *
     * You are required to invoke set_buffer before calling any other functions
     */
    bitstream() :
        mError(error::none), mMode(mode::io_unset), mBuffer(nullptr), mBufferBytes(0),
        mBufferBits(0), mPos(0), mOwnsBuffer(false)
    {

    }

    /** Constructs bitstream from a buffer, defaults to reading mode */
    bitstream(word_t* buffer, size_t size, mode m = mode::io_reader) :
        mError(error::none), mMode(m), mBuffer(buffer), mBufferBytes(size), mBufferBits(size*8),
        mPos(0), mOwnsBuffer(false)
    {
        // Verify size
        if (!verify_size(size)) {
            mError = error::size;
            mBuffer = nullptr;
            mBufferBytes = 0;
            mBufferBits = 0;
        }
    }

    /** Constructs bitstream from std::string */
    bitstream(const std::string& data) :
        mError(error::none), mMode(mode::io_reader), mBuffer(nullptr), mBufferBytes(0),
        mBufferBits(0), mPos(0), mOwnsBuffer(true)
    {
        if (!verify_size(data.size())) {
            mError = error::size;
            return;
        }

        // +1 just to make sure :)
        mBufferBytes = (data.size() + 3) / 4 + 1;
        mBufferBits = mBufferBytes * 8;
        mBuffer = new word_t[mBufferBytes];
        memcpy(&mBuffer[0], data.c_str(), data.size());
    }

    /** Bitstreams should not support copying */
    bitstream(const bitstream&) = delete;

    /** Bitstreams should not support copy assignment */
    bitstream& operator=(const bitstream&) = delete;

    /** Default move constructor */
    bitstream(bitstream&&) = default;

    /** Default move assignment operator */
    bitstream& operator=(bitstream&&) = default;

    /** Destructor, frees buffer if owned */
    ~bitstream() {
        if (mOwnsBuffer) {
            delete[] mBuffer;
        }
    }

    /** Resets bitstream as if you used the default constructor */
    void reset() {
        if (mOwnsBuffer) {
            delete[] mBuffer;
        }

        // Default values
        mError = error::none;
        mMode = mode::io_unset;
        mBuffer = nullptr;
        mBufferBytes = 0;
        mBufferBits = 0;
        mPos = 0;
        mOwnsBuffer = false;
    }

// Shared functions and methods
public:
    /** Whether the bitstream is in a valid state */
    bool valid() {
        return (mError == error::none && mMode != mode::io_unset);
    }

    /** Returns error if any */
    error error() {
        return mError;
    }

    /** Sets I/O mode for this stream. */
    void set_mode(mode io_mode) {
        if (mMode == mode::io_unset) {
            mError = error::redef;
            return;
        }

        this->mMode = mode::io_unset;
    }

    /** Returns current I/O mode */
    mode mode() {
        return mMode;
    }

    /** Returns whether this bitstream is read only */
    bool is_reader() {
        return mMode == mode::io_reader;
    }

    /** Returns whether this bitstream is write only */
    bool is_writer() {
        return mMode == mode::io_writer;
    }

    /** Sets internal buffer */
    void set_buffer(word_t* buffer, uint32_t size) {
        if (mBuffer) {
            mError = error::redef;
            return;
        }

        mBuffer = buffer;
        mBufferBytes = size;
        mBufferBits = size*8;
        mPos = 0;
    }

    /** Returns size in bits */
    uint32_t size() {
        return mBufferBits;
    }

    /** Returns size in bytes */
    uint32_t size_bytes() {
        return mBufferBytes;
    }

    /** Returns current position in bits */
    uint32_t position() {
        return mPos;
    }

    /** Seek to a specific bit */
    void seek(uint32_t position);

// Write only functions
public:

// Read only functions
public:

private:
    enum error mError;
    enum mode mMode;
    word_t* mBuffer;
    uint32_t mBufferBytes;
    uint32_t mBufferBits;
    uint32_t mPos;
    bool mOwnsBuffer;

    /** Verifies buffer size */
    bool verify_size(uint32_t size) {
        static size_t size_bits_max = (2^32) - 1;
        return (size * 8) + 1 < size_bits_max;
    }
};

#endif /* _RD_BITSTREAM_HPP_ */