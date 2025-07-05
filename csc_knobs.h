#ifndef __CSC_KNOBS_H__
#define __CSC_KNOBS_H__
// CSC KNOBS
#define LOGFT 12 
#define TCB 9 
#define BLS 174745
#define BLB 2
#define LOGWS 12 
#define CSC_USE_THETA false
#define ORACLE_BLOOM
#define CSC_CTR_MAX ((1 << LOGWS - 1) - 1)
//#define USE_BLOOM
#define USE_CSC false

//#define BLOOM_SIZE_INDIV 40960
//#define BLOOM_SIZE 195227 * 2

//#define PERPC_STATS
//#define BRANCH_LATENCY
//#define PRINT_WPS 
//#define PRINT_ALL_LATENCIES
//#define ENTRY_STATS
//#define PRINT_AGG_ENTRY_STATS
//#define PRINT_PER_ENTRY_STATS
//#define RAS_STUDY
//#define GHIST_STUDY
//#define BANK_USAGE
//#define PRINT_PRED_CASES
//#define BRANCH_WP_CSV

// HIGH LATENCY CLASSIFICATION METHODS
//#define HIGH_LATENCY_TABLE TODO: IMPLEMENT THIS!!! MAKE SURE IT'S NOT JUST MISP AS WELL
//#define MEM_AND_UP_ALL // all branches > 215
//#define MEM_AND_UP_ON_MISP // all misp branches > 215 
//#define L3_AND_UP_ALL
//#define L3_AND_UP // just flag all branches with latency 66 and up as high latency
//#define LOW_LATENCY_TABLE
//#define HIGH_LATENCY_HEAP
//#define LAST1KPERC_LATENCY
#endif // __CSC_KNOBS_H__
