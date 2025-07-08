#include <bitset>
#include <cstdint>
#include <cassert>

template <size_t N>
class BloomFilter1 {
private:
    std::bitset<N> bitset;
    int hash_count;
    uint64_t insertion_counter;
    const uint64_t clear_threshold;

    // Simple 64-bit hash function inspired by xxHash/MurmurHash3
    uint64_t simpleHash64(uint64_t key, uint64_t seed) const {
        key ^= seed;
        key ^= key >> 33;
        key *= 0xff51afd7ed558ccdULL;
        key ^= key >> 33;
        key *= 0xc4ceb9fe1a85ec53ULL;
        key ^= key >> 33;
        return key;
    }

    // Use seed to simulate independent hash functions
    size_t hash_i(uint64_t key, int i) const {
        uint64_t seed = static_cast<uint64_t>(i) * 0x9e3779b185ebca87ULL;
        return simpleHash64(key, seed) % N;
    }

public:
    BloomFilter1(int hash_count, uint64_t threshold)
        : hash_count(hash_count), insertion_counter(0), clear_threshold(threshold) { printf("BLOOM FILTER INSTANTIATED with hash_count %d, threshold %lu\n", hash_count, threshold); }

    void insert(uint64_t key) {
        for (int i = 0; i < hash_count; ++i) {
            bitset.set(hash_i(key, i));
        }
        insertion_counter++; // Increment counter on insertion

        /*
        // Check if full and clear
        if (insertion_counter >= clear_threshold) {
            printf("clearing bloom filter at %lu insertions\n", insertion_counter);
            std::fflush(stdout);
            clear();
            insertion_counter = 0; // Reset counter after clearing
        }
        */
    }

    bool possiblyContains(uint64_t key) const {
        for (int i = 0; i < hash_count; ++i) {
            if (!bitset.test(hash_i(key, i))) return false;
        }
        return true;
    }

    void clear() {
        bitset.reset();
    }
};

template <size_t N>
class BloomFilter2 {
private:
    std::bitset<N> bitset;
    int hash_count;

    // Simple 64-bit hash function inspired by xxHash/MurmurHash3
    uint64_t simpleHash64(uint64_t key, uint64_t seed) const {
        key ^= seed;
        key ^= key >> 33;
        key *= 0xff51afd7ed558ccdULL;
        key ^= key >> 33;
        key *= 0xc4ceb9fe1a85ec53ULL;
        key ^= key >> 33;
        return key;
    }

    // Use seed to simulate independent hash functions
    size_t hash_i(uint64_t key, int i) const {
        i = i + 2;
        uint64_t seed = static_cast<uint64_t>(i) * 0x9e3779b185ebca87ULL;
        return simpleHash64(key, seed) % N;
    }

public:
    BloomFilter2(int hash_count) : hash_count(hash_count) {}

    void insert(uint64_t key) {
        for (int i = 0; i < hash_count; ++i) {
            bitset.set(hash_i(key, i));
        }
    }

    bool possiblyContains(uint64_t key) const {
        for (int i = 0; i < hash_count; ++i) {
            if (!bitset.test(hash_i(key, i))) return false;
        }
        return true;
    }

    void clear() {
        bitset.reset();
    }
};

