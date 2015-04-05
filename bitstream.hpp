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

/** This class provides functions to read and write data as a stream of bits. */
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
    bitstream();

    /**
     * Constructs bitstream from a buffer, defaults to reading mode.
     *
     * \param buffer Buffer to read or write from
     * \param size Size of buffer
     * \param m I/O Mode
     */
    bitstream(word_t* buffer, size_t size, mode m = mode::io_reader);

    /**
     * Constructs bitstream from std::string, read-only
     *
     * \param data String to read from
     */
    bitstream(const std::string& data);

    /**
     * Constuct bitstream with given size in writing mode.
     *
     * \param size Number of bytes to allocate for internal buffer
     */
    bitstream(const uint32_t size);

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
        if (mOwnsBuffer)
            delete[] mBuffer;
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

    /**
     * Sets I/O mode for this stream.
     *
     * \param io_mode I/O Mode to set
     */
    void set_mode(mode io_mode) {
        if (mMode != mode::io_unset) {
            mError = error::redef;
            return;
        }

        this->mMode = io_mode;
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

    /**
     * Sets buffer used for reading / writing.
     *
     * \param buffer Buffer to use internally
     * \param size Size of buffer
     */
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

    /** Returns number of bits left in the stream */
    uint32_t left() {
        return mBufferBits - mPos;
    }

    /** Returns pointer to buffer */
    word_t* buffer() {
        return mBuffer;
    }

    /**
     * Seek to a specific bit.
     * Does a boundary check against the maximum position possible.
     *
     * \param position Position in bits to seek to.
     */
    void seek(uint32_t position) {
        assert(position < mBufferBits);
        mPos = position;
    }

// Write only functions
public:
    /**
     * Writes up to 32 bits from data to the buffer.
     * If bits is not 32, only writes the lower bits.
     *
     * \param bits Number of bits to write
     * \param data Data to write
     */
    void write(uint8_t bits, uint32_t data) {
        assert(mError == error::none);
        assert(mMode == mode::io_writer);
        assert(bits <= 32);

        static constexpr uint8_t bitSize = sizeof(word_t) << 3;  // size of a single chunk in bits
        const uint32_t start = mPos / bitSize;                  // active chunk
        const uint32_t end = (mPos + bits - 1) / bitSize;       // last chunk
        const uint32_t shift = mPos % bitSize;                  // shift amount

        if (start == end) {
            mBuffer[start] = (mBuffer[start] & masks[shift]) | (data << shift);
        } else {
            mBuffer[start] = (mBuffer[start] & masks[shift]) | (data << shift);
            mBuffer[end] = (data >> (bitSize - shift)) & masks[bits - (bitSize - shift)];
        }

        mPos += bits;
    }

    /**
     * Writes specified number of bytes read from data.
     *
     * \param data Data to read from
     * \param size Size of data
     */
    void write_bytes(const char* data, uint32_t size) {
        if ((mPos & 7) == 0) {
            memcpy(&(reinterpret_cast<char*>(mBuffer)[mPos >> 3]), data, size);
        } else {
            for (uint32_t i = 0; i < size; ++i) {
                write(8, data[i]);
            }
        }
    }

// Read only functions
public:
    /** 
     * Reads up to 32 bits from the stream
     * 
     * \param bits Number of bits to read
     */
    uint32_t read(uint8_t bits) {
        assert(mError == error::none);
        assert(mMode == mode::io_reader);
        assert(bits <= 32);

        static constexpr uint8_t bitSize = sizeof(word_t) << 3;  // size of a single chunk in bits
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

    /** 
     * Reads number of bytes into buffer
     * 
     * \param bytes Number of bytes to read
     * \param dest Where to copy the read bytes
     */
    void read_bytes(uint32_t bytes, char* dest) {
        if ((mPos & 7) == 0) {
            memcpy(dest, &(reinterpret_cast<char*>(mBuffer)[mPos >> 3]), bytes);
        } else {
            for (uint32_t i = 0; i < bytes; ++i) {
                dest[i] = static_cast<int8_t>(read(8));
            }
        }
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