/*
  Copyright (C) ARM Limited 2008-2025  All rights reserved.                                                                                                                                                                                                                        

  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// This file provides a sample predictor integration based on the interface provided.

#include "lib/sim_common_structs.h"
#include "my_cond_branch_predictor.h"
#include <cassert>

//
// beginCondDirPredictor()
// 
// This function is called by the simulator before the start of simulation.
// It can be used for arbitrary initialization steps for the contestant's code.
//
void beginCondDirPredictor()
{
    cbp2025_RUNLTS.setup();

    /*-----------------------------------------------*/
    // ADDING ON CSC
    knobs.clear();
    knobs.reserve(Knobs::Size);
    knobs.insert(knobs.end(), Knobs::Size, 0);
    // setup sample_predictor

    assert(knob_enable.size() == knob_names.size());
    assert(knob_names.size() == Knobs::Size);
    correlator.init_pred(knob_names, knob_enable);
    epoch = 0;
    num_unique_elements_epoch = 0;
    /*-----------------------------------------------*/
}

//
// notify_instr_fetch(uint64_t seq_no, uint8_t piece, uint64_t pc, const uint64_t fetch_cycle)
// 
// This function is called when any instructions(not just branches) gets fetched.
// Along with the unique identifying ids(seq_no, piece), PC of the instruction and fetch_cycle are also provided as inputs
//
void notify_instr_fetch(uint64_t seq_no, uint8_t piece, uint64_t pc, const uint64_t fetch_cycle)
{
}

//
// get_cond_dir_prediction(uint64_t seq_no, uint8_t piece, uint64_t pc, const uint64_t pred_cycle)
// 
// This function is called by the simulator for predicting conditional branches.
// input values are unique identifying ids(seq_no, piece) and PC of the branch.
// return value is the predicted direction. 
//
bool get_cond_dir_prediction(uint64_t seq_no, uint8_t piece, uint64_t pc, const uint64_t pred_cycle)
{
    const bool runlts_pred = cbp2025_RUNLTS.predict(seq_no, piece, pc);
    //return runlts_pred; FOR RUNLTS ONLY

    /*-----------------------------------------------*/
    // ADDING ON CSC
    knobs[IndirectInLast100] = last_100_indirects.size();
    knobs[IndirectInLast1K] = last_1K_indirects.size();
    knobs[LongBrsInLast100] = last_100_longbrs.size();
    knobs[LongBrsInLast1K] = last_1K_longbrs.size();

    const bool csc_pred = correlator.pred(knobs, uniq(seq_no, piece));

    bool final_pred;
    #ifdef ORACLE_BLOOM
    if (!oracle_bloom.count(pc))
    #else
    if (!bloom1.possiblyContains(pc))
    #endif
    {
        final_pred = csc_pred;
        preds_from_csc++;
    }
    else
    {
        final_pred = runlts_pred;
        preds_from_tage++;
    }

    return final_pred;
    /*-----------------------------------------------*/
}

//
// spec_update(uint64_t seq_no, uint8_t piece, uint64_t pc, InstClass inst_class, const bool resolve_dir, const bool pred_dir, const uint64_t next_pc)
// 
// This function is called by the simulator for updating the history vectors and any state that needs to be updated speculatively.
// The function is called for all the branches (not just conditional branches). To faciliate accurate history updates, spec_update is called right
// after a prediction is made.
// input values are unique identifying ids(seq_no, piece), PC of the instruction, instruction class, predicted/resolve direction and the next_pc 
//
void spec_update(uint64_t seq_no, uint8_t piece, uint64_t pc, InstClass inst_class, const bool resolve_dir, const bool pred_dir, const uint64_t next_pc)
{
    assert(is_br(inst_class));

    /*-----------------------------------------------*/
    // ADDING ON CSC
    csc_spec_update_front(seq_no, piece, pc, next_pc);
    /*-----------------------------------------------*/

    int br_type = 0;
    switch (inst_class) {
    case InstClass::condBranchInstClass:

	/*-----------------------------------------------*/
	// ADDING ON CSC
        if (resolve_dir)
        {
            if (knobs[Knobs::GlobalBias] != 7)
                ++knobs[GlobalBias];
        }
        else
        {
            if (knobs[GlobalBias] != 0)
                --knobs[GlobalBias];
        }
	/*-----------------------------------------------*/

        br_type = 1; // cond(1)
        break;
    case InstClass::uncondDirectBranchInstClass:
        br_type = 0;
        break;
    case InstClass::uncondIndirectBranchInstClass:

	/*-----------------------------------------------*/
	// ADDING ON CSC
        last_100_indirects.push_back(seq_no);
        last_1K_indirects.push_back(seq_no);
	/*-----------------------------------------------*/

        br_type = 2; // indirect(2)
        break;
    case InstClass::callDirectInstClass:

	/*-----------------------------------------------*/
	// ADDING ON CSC
        ++knobs[CallDepth];

        last_100_calls.push_back(seq_no);
        last_1K_calls.push_back(seq_no);
	/*-----------------------------------------------*/

        br_type = 4; // call(4)
        break;
    case InstClass::callIndirectInstClass:

	/*-----------------------------------------------*/
	// ADDING ON CSC
        ++knobs[CallDepth];

        last_100_calls.push_back(seq_no);
        last_1K_calls.push_back(seq_no);
	/*-----------------------------------------------*/

        br_type = 6; // call(4) + indirect(2)
        break;
    case InstClass::ReturnInstClass:

	/*-----------------------------------------------*/
	// ADDING ON CSC
        if (knobs[CallDepth] != 0)
            --knobs[CallDepth];
	/*-----------------------------------------------*/

        br_type = 10; // ret(8) + indirect(2)
        break;
    default:
        assert(false);
    }

    if (inst_class == InstClass::condBranchInstClass) {
        cbp2025_RUNLTS.history_update(seq_no, piece, pc, br_type, pred_dir, resolve_dir, next_pc);
    } else {
        cbp2025_RUNLTS.TrackOtherInst(pc, br_type, pred_dir, resolve_dir, next_pc);
    }

    /*-----------------------------------------------*/
    // ADDING ON CSC
    csc_spec_update_back(seq_no);
    /*-----------------------------------------------*/
}

//
// notify_instr_decode(uint64_t seq_no, uint8_t piece, uint64_t pc, const DecodeInfo& _decode_info, const uint64_t decode_cycle)
// 
// This function is called when any instructions(not just branches) gets decoded.
// Along with the unique identifying ids(seq_no, piece), PC of the instruction, decode info and cycle are also provided as inputs
//
// For the sample predictor implementation, we do not leverage decode information
void notify_instr_decode(uint64_t seq_no, uint8_t piece, uint64_t pc, const DecodeInfo& _decode_info, const uint64_t decode_cycle)
{
    const std::optional<uint64_t>& dst_reg = _decode_info.dst_reg_info;
    if (dst_reg.has_value() && 0 <= *dst_reg && *dst_reg <= 64) {
        cbp2025_RUNLTS.decode_notify(seq_no, piece, *dst_reg);
    }
}

//
// notify_agen_complete(uint64_t seq_no, uint8_t piece, uint64_t pc, const DecodeInfo& _decode_info, const uint64_t mem_va, const uint64_t mem_sz, const uint64_t agen_cycle)
// 
// This function is called when any load/store instructions complete agen.
// Along with the unique identifying ids(seq_no, piece), PC of the instruction, decode info, mem_va and mem_sz and agen_cycle are also provided as inputs
//
void notify_agen_complete(uint64_t seq_no, uint8_t piece, uint64_t pc, const DecodeInfo& _decode_info, const uint64_t mem_va, const uint64_t mem_sz, const uint64_t agen_cycle)
{
}

//
// notify_instr_execute_resolve(uint64_t seq_no, uint8_t piece, uint64_t pc, const bool pred_dir, const ExecuteInfo& _exec_info, const uint64_t execute_cycle)
// 
// This function is called when any instructions(not just branches) gets executed.
// Along with the unique identifying ids(seq_no, piece), PC of the instruction, execute info and cycle are also provided as inputs
//
// For conditional branches, we use this information to update the predictor.
// At the moment, we do not consider updating any other structure, but the contestants are allowed to  update any other predictor state.
void notify_instr_execute_resolve(uint64_t seq_no, uint8_t piece, uint64_t pc, const bool pred_dir, const ExecuteInfo& _exec_info, const uint64_t execute_cycle)
{
    const std::optional<uint64_t>& dst_reg = _exec_info.dec_info.dst_reg_info;
    if (dst_reg.has_value() && 0 <= *dst_reg && *dst_reg <= 64) {
        cbp2025_RUNLTS.execute_notify(seq_no, piece, *dst_reg, _exec_info.dst_reg_value.value());
    }

    const bool is_branch = is_br(_exec_info.dec_info.insn_class);
    if (is_branch) {
        if (is_cond_br(_exec_info.dec_info.insn_class)) {
            const bool _resolve_dir = _exec_info.taken.value();
            const uint64_t _next_pc = _exec_info.next_pc;

			/*-----------------------------------------------*/
			// ADDING ON CSC
			bool taken = _resolve_dir;
            bool misp = _resolve_dir != pred_dir;

            if (misp)
            {
                #ifdef ORACLE_BLOOM
				oracle_bloom.insert(pc); // oracle implementation
                #else
                // if(!bloom1.possiblyContains(pc) || !bloom2.possiblyContains(pc)){
                if (!bloom1.possiblyContains(pc) && epoch % 2 == 0)
                {
                    num_unique_elements_epoch++;
                    bloom1.insert(pc);
                }
                #endif
            }

			correlator.update(taken, pred_dir, uniq(seq_no, piece));

            #ifdef ORACLE_BLOOM
            if (oracle_bloom.count(pc))
            #else
			if (bloom1.possiblyContains(pc) /* || bloom2.possiblyContains(pc))*/ /* || cbp2016_tage_sc_l.HitBank > 0*/)
            #endif
            {
				cbp2025_RUNLTS.update(seq_no, piece, pc, _resolve_dir, pred_dir, _next_pc);
            }
            else
            {
                cbp2025_RUNLTS.kill_checkpoint(seq_no, piece);
            }
			/*-----------------------------------------------*/

        } else {
            assert(pred_dir);
        }
    }
}

//
// notify_instr_commit(uint64_t seq_no, uint8_t piece, uint64_t pc, const bool pred_dir, const ExecuteInfo& _exec_info, const uint64_t commit_cycle)
// 
// This function is called when any instructions(not just branches) gets committed.
// Along with the unique identifying ids(seq_no, piece), PC of the instruction, execute info and cycle are also provided as inputs
//
// For the sample predictor implementation, we do not leverage commit information
void notify_instr_commit(uint64_t seq_no, uint8_t piece, uint64_t pc, const bool pred_dir, const ExecuteInfo& _exec_info, const uint64_t commit_cycle)
{
}

//
// endCondDirPredictor()
//
// This function is called by the simulator at the end of simulation.
// It can be used by the contestant to print out other contestant-specific measurements.
//
void endCondDirPredictor()
{
    cbp2025_RUNLTS.terminate();
    std::fflush(stdout);
}
