#ifndef __MURMURHASH_H_
#define __MURMURHASH_H_

#include <stdint.h>
#include <functional>

const static int MURMUR_SEED = 184333;

template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

extern uint64_t murmurhash( const void * key, int len, unsigned int seed);

#endif
