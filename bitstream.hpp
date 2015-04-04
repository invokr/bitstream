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
    typedef uint32_t word_t;

    /** Pre-computed bitmasks */
    static constexpr uint64_t masks[64] = {
        0x0,             0x1,              0x3,              0x7,
        0xf,             0x1f,             0x3f,             0x7f,
        0xff,            0x1ff,            0x3ff,            0x7ff,
        0xfff,           0x1fff,           0x3fff,           0x7fff,
        0xffff,          0x1ffff,          0x3ffff,          0x7ffff,
        0xfffff,         0x1fffff,         0x3fffff,         0x7fffff,
        0xffffff,        0x1ffffff,        0x3ffffff,        0x7ffffff,
        0xfffffff,       0x1fffffff,       0x3fffffff,       0x7fffffff,
        0xffffffff,      0x1ffffffff,      0x3ffffffff,      0x7ffffffff,
        0xfffffffff,     0x1fffffffff,     0x3fffffffff,     0x7fffffffff,
        0xffffffffff,    0x1ffffffffff,    0x3ffffffffff,    0x7ffffffffff,
        0xfffffffffff,   0x1fffffffffff,   0x3fffffffffff,   0x7fffffffffff,
        0xffffffffffff,  0x1ffffffffffff,  0x3ffffffffffff,  0x7ffffffffffff,
        0xfffffffffffff, 0x1fffffffffffff, 0x3fffffffffffff, 0x7fffffffffffff
    };

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

    /** Constuct bitstream with given size in writing mode */
    bitstream(const uint32_t size) :
        mError(error::none), mMode(mode::io_writer), mBuffer(new word_t[size]), mBufferBytes(size),
        mBufferBits(size * 8), mPos(0), mOwnsBuffer(true)
    {
        if (!verify_size(size)) {
            mError = error::size;
            return;
        }
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
        if (mMode != mode::io_unset) {
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

    /** Returns pointer to buffer */
    word_t* buffer() {
        return mBuffer;
    }

    /** Seek to a specific bit */
    void seek(uint32_t position);

// Write only functions
public:
    void write(uint8_t bits, uint32_t data) {
        assert(mError == error::none);
        assert(mMode == mode::io_writer);
        assert(bits <= 32);

        static constexpr uint8_t bitSize = sizeof(word_t) * 8;  // size of a single chunk in bits
        const uint32_t start = mPos / bitSize;                  // active chunk
        const uint32_t end = (mPos + bits - 1) / bitSize;       // last chunk
        const uint32_t shift = mPos % bitSize;                  // shift amount

        if (start == end) {
            mBuffer[start] = (mBuffer[start] & masks[shift]) | (data << shift);
        } else {
            mBuffer[start] = (mBuffer[start] & masks[shift]) | ((data & masks[bitSize - shift]) << shift);
            mBuffer[end] = data << (bitSize - shift);
        }

        mPos += bits;
    }

// Read only functions
public:
    /** Reads up to 32 bits */
    uint32_t read(uint8_t bits) {
        assert(mError == error::none);
        assert(mMode == mode::io_reader);
        assert(bits <= 32);

        static constexpr uint8_t bitSize = sizeof(word_t) * 8;  // size of a single chunk in bits
        const uint32_t start = mPos / bitSize;                // current active chunk
        const uint32_t end   = (mPos + bits - 1) / bitSize;   // next chunk in case of wrapping
        const uint32_t shift = mPos % bitSize;                // shift amount

        uint32_t ret; // return value
        if (start == end) {
            ret = (mBuffer[start] >> shift) & masks[bits];
        } else {
            ret = ((mBuffer[start] >> shift) | (mBuffer[end] << (bitSize - shift))) & masks[bits];
        }

        mPos += bits;
        return ret;
    }
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
        return (size * 8) + 1 > size_bits_max;
    }
};

#endif /* _RD_BITSTREAM_HPP_ */