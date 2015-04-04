/**
 * @file bitstream.cpp
 * @author Robin Dietrich <me (at) invokr (dot) org>
 * @version 1.0
 * @par License Public Domain
 *
 * Inspired by Valve's and Raknet's bitstreams
 */

#include "bitstream.hpp"

// mask definition
constexpr uint64_t bitstream::masks[];

// Default constructor
bitstream::bitstream() :
    mError(error::none), mMode(mode::io_unset), mBuffer(nullptr), mBufferBytes(0),
    mBufferBits(0), mPos(0), mOwnsBuffer(false) { }

bitstream::bitstream(word_t* buffer, size_t size, enum mode m) :
    mError(error::none), mMode(m), mBuffer(buffer), mBufferBytes(size), mBufferBits(size*8),
    mPos(0), mOwnsBuffer(false)
{
    if (!verify_size(size)) {
        mError = error::size;
        mBuffer = nullptr;
        mBufferBytes = 0;
        mBufferBits = 0;
    }
}

bitstream::bitstream(const std::string& data) :
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

bitstream::bitstream(const uint32_t size) :
    mError(error::none), mMode(mode::io_writer), mBuffer(new word_t[size]), mBufferBytes(size),
    mBufferBits(size * 8), mPos(0), mOwnsBuffer(true)
{
    if (!verify_size(size))
        mError = error::size;
}