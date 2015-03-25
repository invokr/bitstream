/**
 * @file bitstream.cpp
 * @author Robin Dietrich <me (at) invokr (dot) org>
 * @version 1.0
 * @par License Public Domain
 *
 * Inspired by Valve's and Raknet's bitstreams
 */

#include "bitstream.hpp"

// Pre-computed bitmasks
const uint64_t bitstream::masks[64] = {
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