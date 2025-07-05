struct BranchStats {
    uint64_t taken;
    uint64_t non_taken;
    uint64_t total_mispred;
    uint64_t total_wrong_path;

    uint64_t pred_with_csc;
    uint64_t pred_with_runlts;

    // There are 4 pred outcomes
    // 0. Used CSC, Correct
    // 1. Used CSC, Incorrect
    // 2. Used RUNLTS, Correct
    // 3. Used RUNLTS, Incorrect
    std::array<uint64_t, 4> pred_outcomes{};

    /*------------------------------------------------------------------------------------------*/
    // FROM BEFORE, PROBABLY NOT GOING TO USE ANYMORE AT LEAST FOR NOW
    uint64_t pred_with_tage;
    uint64_t pred_with_loop; // aka loop override TAGE
    uint64_t pred_with_sc; // aka SC override TAGE/loop
    std::pair<uint64_t, uint64_t> loop_disagree_and_override;

    // SC override specific profiling counters
    // all the cases where sc overrides should add up to pred_with_sc
    uint64_t sc_tagel_conflict; // number of times sc disagrees with tage/loop
    
    /* Trying something fancy:
     * There are 9 possibilities on each prediction (bundling Tage and Loop together):
     * 0. SC and TageL agree
     * 1. Tage has high conf, SC is high threshold (SC override)
     * 2. Tage has high conf, SC is med threshold, 2nd conf counter has low conf (SC override)
     * 3. Tage has high conf, SC is med threshold, 2nd conf counter has high conf (keep TageL)
     * 4. Tage has high conf, SC is low threshold (keep TageL)
     * 5. Tage has med conf, SC is med/high threshold (SC override)
     * 6. Tage has med conf, SC is low threshold, 1st conf counter has low conf (SC override)
     * 7. Tage has med conf, SC is low threshold, 1st conf counter has high conf (keep TageL)
     * 8. Tage has low conf (SC override)
     *
     * I'm going to create an array of pairs of size 9 for each possibility
     * The pair will be (# of times this case happens, # of times mispred using this case)
    */
    std::array<std::pair<uint64_t, uint64_t>, 9> pred_cases;
#ifdef BANK_USAGE
    std::unordered_map<uint16_t, uint64_t> bank_usage; // for TAGE bank distribution
#endif

    std::unordered_map<uint64_t, std::pair<uint64_t, uint64_t>> latencies; // [# of cycles, [# of tot occur, # of misp]]

    uint64_t pred_long_is_long;
    uint64_t pred_long_is_short;
    uint64_t pred_long_is_med;
    uint64_t pred_short_is_long;
    uint64_t pred_short_is_short;
    uint64_t pred_short_is_med;

#ifdef RAS_STUDY
    std::unordered_map<RAS1, std::unordered_map<uint64_t, uint64_t>> ras1_wps; // [first address on RAS, [# of cycles, freq]]
    std::unordered_map<RAS2, std::unordered_map<uint64_t, uint64_t>> ras2_wps; // [first and second address on RAS, [# of cycles, freq]]
    std::unordered_map<RAS3, std::unordered_map<uint64_t, uint64_t>> ras3_wps; // [first and second and third address on RAS, [# of cycles, freq]]
#endif
    /*------------------------------------------------------------------------------------------*/

    BranchStats() : taken(0), non_taken(0), total_mispred(0), total_wrong_path(0), pred_with_tage(0), pred_with_loop(0), pred_with_sc(0), 
    sc_tagel_conflict(0), pred_long_is_long(0), pred_long_is_short(0), pred_long_is_med(0), pred_short_is_long(0), pred_short_is_short(0), pred_short_is_med(0) {}
};

std::unordered_map<uint64_t, BranchStats> branch_stats_map; // PER PC STATS
std::unordered_map<uint64_t, uint8_t> pred_used; // FOR BLAMING CORRECT PRED 0 = RUNLTS, 1 = CSC

// Global stats
static uint64_t pred_from_csc = 0;
static uint64_t pred_from_runlts = 0;
uint64_t used_csc_right;
uint64_t used_csc_wrong;
uint64_t used_runlts_right;
uint64_t used_runlts_wrong;
