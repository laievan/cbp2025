#ifndef __CSC_H__
#define __CSC_H__
#include "correlation.h"
#include "bloom.h"
#include <deque>
#include <unordered_set>
#include <numeric>
#include <set>

using u64 = uint64_t;
using i64 = int64_t;

static std::set<uint64_t> seen_pcs;
static std::deque<uint64_t> last_100_indirects;
static std::deque<uint64_t> last_100_longbrs;
static std::deque<uint64_t> last_1K_indirects;
static std::deque<uint64_t> last_1K_longbrs;
static std::deque<u64> last_100_calls;
static std::deque<u64> last_100_forward_brs;
static std::deque<u64> last_100_backward_brs;
static std::deque<u64> last_100_bb_sz;
static std::deque<u64> last_100_bb_count;
static std::deque<u64> last_1K_calls;
static std::deque<u64> last_1K_forward_brs;
static std::deque<u64> last_1K_backward_brs;
static std::deque<u64> last_1K_bb_sz;
static std::deque<u64> last_1K_bb_count;
static constexpr uint64_t uniq(uint64_t const seq_no, uint8_t const piece) __attribute((pure));
static u64 avg(std::deque<u64> const &) __attribute((pure));

static constexpr uint64_t uniq(uint64_t const seq_no, uint8_t const piece)
{
    assert(piece < 16);
    return (seq_no << 4) | (piece & 0x000F);
}

u64 avg(std::deque<u64> const &q)
{
    u64 const count = q.size();
    assert(count > 0 && "must be non-zero range");
    u64 const sum = std::accumulate(q.begin(), q.end(), 0UL);
    return sum / count;
}

static uint64_t preds_from_csc = 0;
static uint64_t preds_from_tage = 0;

static Correlation correlator;
static std::vector<uint64_t> knobs;
namespace
{
    enum Knobs
    {
        CallDepth,

        GlobalBias,

        IndirectInLast100,
        IndirectInLast1K,

        // long br is more than 4k (page sz)
        LongBrsInLast100,
        LongBrsInLast1K,

        Size
    };
}
static std::vector<std::string> const knob_names = {
    /*[Knobs::CallDepth] = */ "call_depth",
    /*[Knobs::GlobalBias] = */ "global_bias",
    /*[Knobs::IndirectInLast100] = */ "indirect_100",
    /*[Knobs::IndirectInLast1K] = */ "indirect_1K",
    /*[Knobs::LongBrsInLast100] = */ "long_br_100",
    /*[Knobs::LongBrsInLast1K] = */ "long_br_1K",

};

static std::vector<bool> const knob_enable = {
    /*[Knobs::CallDepth] = */ true,
    /*[Knobs::GlobalBias] = */ true,
    /*[Knobs::IndirectInLast100] = */ true,
    /*[Knobs::IndirectInLast1K] = */ true,
    /*[Knobs::LongBrsInLast100] = */ true,
    /*[Knobs::LongBrsInLast1K] = */ true,

};

// Bloom setup
#define BLOOM_THRESHOLD 40 * 1024

static int bloom_filter_switch_threshold = BLOOM_THRESHOLD;
static int epoch;
static int num_unique_elements_epoch = 0;

/// this is num of bits
// BloomFilter1<BLOOM_SIZE> bloom1 = BloomFilter1<BLOOM_SIZE>(2);
// BloomFilter2<BLOOM_SIZE> bloom2 = BloomFilter2<BLOOM_SIZE>(2);
static BloomFilter1<BLS> bloom1 = BloomFilter1<BLS>(BLB);
// static BloomFilter2<BLS> bloom2 = BloomFilter2<BLS>(BLB);
static std::unordered_set<uint64_t> oracle_bloom;


// CSC Features spec_update
inline void csc_spec_update_front(uint64_t seq_no, uint8_t piece, uint64_t pc, uint64_t next_pc) {
    if (piece == 0)
    {
        u64 const bb_start_seq_no = last_100_bb_count.empty() ? 0 : last_100_bb_count.back();
        last_100_bb_count.push_back(seq_no);
        last_1K_bb_count.push_back(seq_no);

        u64 const bb_sz = seq_no - bb_start_seq_no;
        last_100_bb_sz.push_back(bb_sz);
        last_1K_bb_sz.push_back(bb_sz);
    }

    i64 const delta = next_pc - pc;
    if (delta > 0)
    {
        // forward br
        last_100_forward_brs.push_back(seq_no);
        last_1K_forward_brs.push_back(seq_no);
    }

    if (delta < 0)
    {
        // backward br
        last_100_backward_brs.push_back(seq_no);
        last_1K_backward_brs.push_back(seq_no);
    }

    if (piece == 0 && abs(delta) > 4096)
    {
        last_100_longbrs.push_back(seq_no);
        last_1K_longbrs.push_back(seq_no);
    }
}

inline void csc_spec_update_back(uint64_t seq_no) {
    int pop_count = 0;

    while (!last_100_indirects.empty() && last_100_indirects.front() < (seq_no - 100))
    {
        last_100_indirects.pop_front();
        pop_count++;
    }
    // assert(pop_count <= 1);
    pop_count = 0;

    while (!last_100_longbrs.empty() && last_100_longbrs.front() < (seq_no - 100))
    {
        last_100_longbrs.pop_front();
        pop_count++;
    }
    // assert(pop_count <= 1);
    pop_count = 0;

    while (!last_100_calls.empty() && last_100_calls.front() < (seq_no - 100))
    {
        last_100_calls.pop_front();
    }

    // XXX: this is sus
    while (!last_100_bb_count.empty() && last_100_bb_count.front() < (seq_no - 100))
    {
        assert(last_100_bb_sz.size() == last_100_bb_count.size());
        last_100_bb_count.pop_front();
        last_100_bb_sz.pop_front();
    }

    while (!last_100_backward_brs.empty() && last_100_backward_brs.front() < (seq_no - 100))
    {
        last_100_backward_brs.pop_front();
    }

    while (!last_100_forward_brs.empty() && last_100_forward_brs.front() < (seq_no - 100))
    {
        last_100_forward_brs.pop_front();
    }

    while (!last_1K_indirects.empty() && last_1K_indirects.front() < (seq_no - 1000))
    {
        last_1K_indirects.pop_front();
        pop_count++;
    }
    // assert(pop_count <= 1);
    pop_count = 0;

    while (!last_1K_longbrs.empty() && last_1K_longbrs.front() < (seq_no - 1000))
    {
        last_1K_longbrs.pop_front();
        pop_count++;
    }
    // assert(pop_count <= 1);
    pop_count = 0;

    while (!last_1K_calls.empty() && last_1K_calls.front() < (seq_no - 1000))
    {
        last_1K_calls.pop_front();
    }

    // XXX: this is also sus
    while (!last_1K_bb_count.empty() && last_1K_bb_count.front() < (seq_no - 1000))
    {
        assert(last_1K_bb_count.size() == last_1K_bb_sz.size());
        last_1K_bb_count.pop_front();
        last_1K_bb_sz.pop_front();
    }

    while (!last_1K_backward_brs.empty() && last_1K_backward_brs.front() < (seq_no - 1000))
    {
        last_1K_backward_brs.pop_front();
    }

    while (!last_1K_forward_brs.empty() && last_1K_forward_brs.front() < (seq_no - 1000))
    {
        last_1K_forward_brs.pop_front();
    }
}

#endif // __CSC_H__
