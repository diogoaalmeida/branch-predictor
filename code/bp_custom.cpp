/*
 * bp_custom.cpp
 * Computer Architecture - Lab 7: Branch Prediction
 * Derek Chiou
 * Alex Hsu, Chirag Sakhuja, Tommy Huynh
 * Spring 2016
 *
 * This file contains the implementation of the branch predictor.
 * YOU CAN CHANGE ANYTHING IN THIS FILE.
 * Implement a branch predictor of your own choice here. It starts off with the
 * same code as bp_btfn.cpp.
 */

#include "bp.h"
#include "bp_helper.h"

/*
 * Branch predictor state
 * Declare the global state needed by your branch predictor. All storage used
 * by your branch predictor should be here and cannot exceed 4 KiB.
 */
uintptr_t last_target;
// GAg, GAs, GAp, PAg, PAs, PAp, SAg, SAs, SAp
const char *b = "SAp";
const int k = 12;       // bits
const int i = 4;       // bits mais significativos
const int m = 4;       // bits mais significativos
const int a = 8;       // bits menos significativos
// GAg
bitset<k> gagBHR;
vector<bitset<2>> gagPHT;
// GAs
bitset<k> gasBHR;
vector<vector<bitset<2>>> gasPHT;
// GAp
bitset<k> gapBHR;
vector<vector<bitset<2>>> gapPHT;
// PAg
vector<bitset<k>> pagBHR;
vector<bitset<2>> pagPHT;
// PAs
vector<bitset<k>> pasBHR;
vector<vector<bitset<2>>> pasPHT;
// PAp
vector<bitset<k>> papBHR;
vector<vector<bitset<2>>> papPHT;
// SAg
vector<bitset<k>> sagBHR;
vector<bitset<2>> sagPHT;
// SAs
vector<bitset<k>> sasBHR;
vector<vector<bitset<2>>> sasPHT;
// SAp
vector<bitset<k>> sapBHR;
vector<vector<bitset<2>>> sapPHT;

bitset<k> shift(bitset<k> bit, int number)
{
    bit = bit << 1;
    if (number == 1)
    {
        bit = bit.to_ulong() + 1;
    }
    return bit;
}

bitset<2> shift_2_Bits(bitset<2> bit, int number)
{
    bit = bit << 1;
    if (number == 1)
    {
        bit = bit.to_ulong() + 1;
    }
    return bit;
}

int get_most_significant_bit(int number, int bits) {
    int mask = (1 << bits) - 1;
    int bitCount = (int) log2(number) + 1;
    number = number >> (bitCount - bits);

    if ((number & mask) == 0) {
        return 1;
    }
    return number & mask;
}

int get_least_significant_bit(int number, int bits) {
    int mask = (1 << bits) - 1;
    if ((number & mask) == 0) {
        return 1;
    }
    return number & mask;
}

/*
 * Initialize branch predictor
 * This function is called once at the beginning of the program to initialize
 * the global branch predictor state.
 */
void BP::init()
{
    /* The following flag can be helpful when debugging your branch predictor.
   * A record of the branches and your predictions can be written to the file
   * branch_trace.out. The flag can be set to the following values:
   *   TRACE_LEVEL_NONE             - do not record any branches
   *   TRACE_LEVEL_WRONG_TARGETS    - record when targets are mispredicted
   *   TRACE_LEVEL_WRONG_DIRECTIONS - record when directions are mispredicted
   *   TRACE_LEVEL_ALL              - record all branches
   * Additionally, you can also write to this file by outputting to br_trace.
   */

    if (strcmp(b, "GAg") == 0)
    {
        gagBHR = bitset<k>(0);
        gagPHT = vector<bitset<2>>(pow(2.0, k));
        cout << "GAg \n";
    }
    else if (strcmp(b, "GAs") == 0)
    {
        gasBHR = bitset<k>(0);
        gasPHT = vector<vector<bitset<2>>>(pow(2.0, m), vector<bitset<2>>(pow(2.0, k)));
        cout << "GAs \n";
    }
    else if (strcmp(b, "GAp") == 0)
    {
        gapBHR = bitset<k>(0);
        gapPHT = vector<vector<bitset<2>>>(pow(2.0, a), vector<bitset<2>>(pow(2.0, k)));
        cout << "GAp \n";
    }
    else if (strcmp(b, "PAg") == 0)
    {
        pagBHR = vector<bitset<k>>(pow(2.0, a));
        pagPHT = vector<bitset<2>>(pow(2.0, k));
        cout << "PAg \n";
    }
    else if (strcmp(b, "PAs") == 0)
    {
        pasBHR = vector<bitset<k>>(pow(2.0, a));
        pasPHT = vector<vector<bitset<2>>>(pow(2.0, m), vector<bitset<2>>(pow(2.0, k)));
        cout << "PAs \n";
    }
    else if (strcmp(b, "PAp") == 0)
    {
        papBHR = vector<bitset<k>>(pow(2.0, a));
        papPHT = vector<vector<bitset<2>>>(pow(2.0, a), vector<bitset<2>>(pow(2.0, k)));
        cout << "PAp \n";
    }
    else if (strcmp(b, "SAg") == 0)
    {
        sagBHR = vector<bitset<k>>(pow(2.0, i));
        sagPHT = vector<bitset<2>>(pow(2.0, k));
        cout << "SAg \n";
    }
    else if (strcmp(b, "SAs") == 0)
    {
        sasBHR = vector<bitset<k>>(pow(2.0, i));
        sasPHT = vector<vector<bitset<2>>>(pow(2.0, m), vector<bitset<2>>(pow(2.0, k)));
        cout << "SAs \n";
    }
    else if (strcmp(b, "SAp") == 0)
    {
        sapBHR = vector<bitset<k>>(pow(2.0, i));
        sapPHT = vector<vector<bitset<2>>>(pow(2.0, a), vector<bitset<2>>(pow(2.0, k)));
        cout << "SAp \n";
    }

    br_trace_level = TRACE_LEVEL_NONE;
    br_trace << "Custom Branch Predictor!" << endl;
}

/*
 * Predict branch
 * This function is called when a branch occurs and the argument struct
 * contains information used to predict the branch. This function returns a
 * struct that contains the prediction.
 * input:
 *   uintptr_t  br.inst_ptr      - program counter of the branch
 *   uintptr_t  br.next_inst_ptr - next program counter of the branch
 *   uintptr_t  br.target        - branch target if known (i.e. direct branch)
 *   bool       br.uncond        - true if branch is unconditional, else false
 *   bool       br.direct        - true if branch is direct, else false
 *   bool       br.call          - true if branch is a call instruction
 *   bool       br.ret           - true if branch is a return instruction
 *   string     br.dasm          - disassembly of instruction for debugging
 * output:
 *   bool       pred.taken       - predicted direction of the branch
 *                                   true = taken; false = not taken
 *   uintptr_t  pred.target      - predicted target of the branch
 *                                   this is the taken branch target regardless
 *                                   of predicted branch direction
 */
Prediction BP::predict(EntInfo br)
{
    bool taken;
    uintptr_t target;
    int phtIndex;
    int phtListIndex;
    int bhrIndex;
    int bhrListIndex;

    if (strcmp(b, "GAg") == 0)
    {
        phtIndex = gagBHR.to_ulong() - 1;
        taken = ((gagPHT[phtIndex] == 3) || (gagPHT[phtIndex] == 2));
    }
    else if (strcmp(b, "GAs") == 0)
    {
        phtListIndex = get_most_significant_bit(br.inst_ptr, m) - 1;
        phtIndex = gasBHR.to_ulong() - 1;
        taken = ((gasPHT[phtListIndex][phtIndex] == 3) || (gasPHT[phtListIndex][phtIndex] == 2));
    }
    else if (strcmp(b, "GAp") == 0)
    {
        phtListIndex = get_least_significant_bit(br.inst_ptr, a) - 1;
        phtIndex = gapBHR.to_ulong() - 1;
        taken = ((gapPHT[phtListIndex][phtIndex] == 3) || (gapPHT[phtListIndex][phtIndex] == 2));
    }
    else if (strcmp(b, "PAg") == 0)
    {
        bhrListIndex = get_least_significant_bit(br.inst_ptr, a) - 1;
        phtIndex = pagBHR[bhrListIndex].to_ulong() - 1;
        taken = ((pagPHT[phtIndex] == 3) || (pagPHT[phtIndex] == 2));
    }
    else if (strcmp(b, "PAs") == 0)
    {
        bhrListIndex = get_least_significant_bit(br.inst_ptr, a) - 1;
        bhrIndex = pasBHR[bhrListIndex].to_ulong() - 1;
        phtListIndex = get_most_significant_bit(br.inst_ptr, m) - 1;
        taken = ((pasPHT[phtListIndex][bhrIndex] == 3) || (pasPHT[phtListIndex][bhrIndex] == 2));
    }
    else if (strcmp(b, "PAp") == 0)
    {
        bhrListIndex = get_least_significant_bit(br.inst_ptr, a) - 1;
        bhrIndex = papBHR[bhrListIndex].to_ulong() - 1;
        phtListIndex = get_least_significant_bit(br.inst_ptr, a) - 1;
        taken = ((papPHT[phtListIndex][bhrIndex] == 3) || (papPHT[phtListIndex][bhrIndex] == 2));
    }
    else if (strcmp(b, "SAg") == 0)
    {
        bhrListIndex = get_most_significant_bit(br.inst_ptr, i) - 1;
        phtIndex = sagBHR[bhrListIndex].to_ulong() - 1;
        taken = ((sagPHT[phtIndex] == 3) || (sagPHT[phtIndex] == 2));
    }
    else if (strcmp(b, "SAs") == 0)
    {
        bhrListIndex = get_most_significant_bit(br.inst_ptr, i) - 1;
        bhrIndex = sasBHR[bhrListIndex].to_ulong() - 1;
        phtListIndex = get_most_significant_bit(br.inst_ptr, m) - 1;
        taken = ((sasPHT[phtListIndex][bhrIndex] == 3) || (sasPHT[phtListIndex][bhrIndex] == 2));
    }
    else if (strcmp(b, "SAp") == 0)
    {
        bhrListIndex = get_most_significant_bit(br.inst_ptr, i) - 1;
        bhrIndex = sapBHR[bhrListIndex].to_ulong() - 1;
        phtListIndex = get_least_significant_bit(br.inst_ptr, a) - 1;
        taken = ((sapPHT[phtListIndex][bhrIndex] == 3) || (sapPHT[phtListIndex][bhrIndex] == 2));
    }

    // Predict the taken target. If the branch is direct, just use the actual
    // target, otherwise, use the last target.
    if (br.direct)
    {
        target = br.target;
    }
    else
    {
        target = last_target;
    }

    return Prediction(taken, target);
}

/*
 * Update branch predictor
 * This function is called when a branch resolves and the argument struct
 * contains information used to update the branch predictor.
 * input:
 *   uintptr_t  br.inst_ptr      - program counter of the branch
 *   uintptr_t  br.next_inst_ptr - next program counter of the branch
 *   uintptr_t  br.target        - actual target of the branch
 *   bool       br.taken         - actual direction of the branch
 *                                   true = taken; false = not taken
 *   bool       br.uncond        - true if branch is unconditional, else false
 *   bool       br.direct        - true if branch is direct, else false
 *   bool       br.call          - true if branch is a call instruction
 *   bool       br.ret           - true if branch is a return instruction
 *   string     br.dasm          - disassembly of instruction for debugging
 */
void BP::update(ResInfo br)
{
    int phtIndex;
    int phtListIndex;
    int bhrIndex;
    int bhrListIndex;

    if (!br.direct)
    {
        last_target = br.target;
    }

    if (strcmp(b, "GAg") == 0)
    {
        phtIndex = gagBHR.to_ulong() - 1;
        if (br.taken)
        {
            gagPHT[phtIndex] = shift_2_Bits(gagPHT[phtIndex], 1);
            gagBHR = shift(gagBHR, 1);
        }
        else
        {
            gagPHT[phtIndex] = shift_2_Bits(gagPHT[phtIndex], 0);
            gagBHR = shift(gagBHR, 0);
        }
    }
    else if (strcmp(b, "GAs") == 0)
    {
        phtListIndex = get_most_significant_bit(br.inst_ptr, m) - 1;
        phtIndex = gasBHR.to_ulong() - 1;
        if (br.taken)
        {
            gasPHT[phtListIndex][phtIndex] = shift_2_Bits(gasPHT[phtListIndex][phtIndex], 1);
            gasBHR = shift(gasBHR, 1);
        }
        else
        {
            gasPHT[phtListIndex][phtIndex] = shift_2_Bits(gasPHT[phtListIndex][phtIndex], 0);
            gasBHR = shift(gasBHR, 0);
        }
    }
    else if (strcmp(b, "GAp") == 0)
    {
        phtListIndex = get_least_significant_bit(br.inst_ptr, a) - 1;
        phtIndex = gapBHR.to_ulong() - 1;
        if (br.taken)
        {
            gapPHT[phtListIndex][phtIndex] = shift_2_Bits(gapPHT[phtListIndex][phtIndex], 1);
            gapBHR = shift(gapBHR, 1);
        }
        else
        {
            gapPHT[phtListIndex][phtIndex] = shift_2_Bits(gapPHT[phtListIndex][phtIndex], 0);
            gapBHR = shift(gapBHR, 0);
        }
    }
    else if (strcmp(b, "PAg") == 0)
    {
        bhrListIndex = get_least_significant_bit(br.inst_ptr, a) - 1;
        phtIndex = pagBHR[bhrListIndex].to_ulong() - 1;
        if (br.taken)
        {
            pagPHT[phtIndex] = shift_2_Bits(pagPHT[phtIndex], 1);
            pagBHR[bhrListIndex] = shift(pagBHR[bhrListIndex], 1);
        }
        else
        {
            pagPHT[phtIndex] = shift_2_Bits(pagPHT[phtIndex], 0);
            pagBHR[bhrListIndex] = shift(pagBHR[bhrListIndex], 0);
        }
    }
    else if (strcmp(b, "PAs") == 0)
    {
        bhrListIndex = get_least_significant_bit(br.inst_ptr, a) - 1;
        bhrIndex = pasBHR[bhrListIndex].to_ulong() - 1;
        phtListIndex = get_most_significant_bit(br.inst_ptr, m) - 1;
        if (br.taken)
        {
            pasPHT[phtListIndex][bhrIndex] = shift_2_Bits(pasPHT[phtListIndex][bhrIndex], 1);
            pasBHR[bhrListIndex] = shift(pasBHR[bhrListIndex], 1);
        }
        else
        {
            pasPHT[phtListIndex][bhrIndex] = shift_2_Bits(pasPHT[phtListIndex][bhrIndex], 0);
            pasBHR[bhrListIndex] = shift(pasBHR[bhrListIndex], 0);
        }
    }
    else if (strcmp(b, "PAp") == 0)
    {
        bhrListIndex = get_least_significant_bit(br.inst_ptr, a) - 1;
        bhrIndex = papBHR[bhrListIndex].to_ulong() - 1;
        phtListIndex = get_least_significant_bit(br.inst_ptr, a) - 1;
        if (br.taken)
        {
            papPHT[phtListIndex][bhrIndex] = shift_2_Bits(papPHT[phtListIndex][bhrIndex], 1);
            papBHR[bhrListIndex] = shift(papBHR[bhrListIndex], 1);
        }
        else
        {
            papPHT[phtListIndex][bhrIndex] = shift_2_Bits(papPHT[phtListIndex][bhrIndex], 0);
            papBHR[bhrListIndex] = shift(papBHR[bhrListIndex], 0);
        }
    }
    else if (strcmp(b, "SAg") == 0)
    {
        bhrListIndex = get_most_significant_bit(br.inst_ptr, i) - 1;
        phtIndex = sagBHR[bhrListIndex].to_ulong() - 1;
        if (br.taken)
        {
            sagPHT[phtIndex] = shift_2_Bits(sagPHT[phtIndex], 1);
            sagBHR[bhrListIndex] = shift(sagBHR[bhrListIndex], 1);
        }
        else
        {
            sagPHT[phtIndex] = shift_2_Bits(sagPHT[phtIndex], 0);
            sagBHR[bhrListIndex] = shift(sagBHR[bhrListIndex], 0);
        }
    }
    else if (strcmp(b, "SAs") == 0)
    {
        bhrListIndex = get_most_significant_bit(br.inst_ptr, i) - 1;
        bhrIndex = sasBHR[bhrListIndex].to_ulong() - 1;
        phtListIndex = get_most_significant_bit(br.inst_ptr, m) - 1;
        if (br.taken)
        {
            sasPHT[phtListIndex][bhrIndex] = shift_2_Bits(sasPHT[phtListIndex][bhrIndex], 1);
            sasBHR[bhrListIndex] = shift(sasBHR[bhrListIndex], 1);
        }
        else
        {
            sasPHT[phtListIndex][bhrIndex] = shift_2_Bits(sasPHT[phtListIndex][bhrIndex], 0);
            sasBHR[bhrListIndex] = shift(sasBHR[bhrListIndex], 0);
        }
    }
    else if (strcmp(b, "SAp") == 0)
    {
        bhrListIndex = get_most_significant_bit(br.inst_ptr, i) - 1;
        bhrIndex = sapBHR[bhrListIndex].to_ulong() - 1;
        phtListIndex = get_least_significant_bit(br.inst_ptr, a) - 1;
        if (br.taken)
        {
            sapPHT[phtListIndex][bhrIndex] = shift_2_Bits(sapPHT[phtListIndex][bhrIndex], 1);
            sapBHR[bhrListIndex] = shift(sapBHR[bhrListIndex], 1);
        }
        else
        {
            sapPHT[phtListIndex][bhrIndex] = shift_2_Bits(sapPHT[phtListIndex][bhrIndex], 0);
            sapBHR[bhrListIndex] = shift(sapBHR[bhrListIndex], 0);
        }
    }
}