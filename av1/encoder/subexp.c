/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */
#include "aom_dsp/bitwriter.h"

#include "av1/common/common.h"
#include "av1/common/entropy.h"
#include "av1/encoder/cost.h"
#include "av1/encoder/subexp.h"

static const uint8_t update_bits[255] = {
  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  6,  6,  6,
  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  8,  8,  8,  8,  8,  8,
  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
  8,  8,  8,  8,  8,  8,  8,  10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
  10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
  10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
  10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11,
  11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
  11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
  11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
  11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
  11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
  11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
  11, 11, 11, 11, 11, 11, 11, 0,
};
#define MIN_DELP_BITS 5

static int recenter_nonneg(int v, int m) {
  if (v > (m << 1))
    return v;
  else if (v >= m)
    return ((v - m) << 1);
  else
    return ((m - v) << 1) - 1;
}

static int remap_prob(int v, int m) {
  int i;
  static const uint8_t map_table[MAX_PROB - 1] = {
    // generated by:
    //   map_table[j] = split_index(j, MAX_PROB - 1, MODULUS_PARAM);
    20,  21,  22,  23,  24,  25,  0,   26,  27,  28,  29,  30,  31,  32,  33,
    34,  35,  36,  37,  1,   38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
    48,  49,  2,   50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,
    3,   62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  4,   74,
    75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  5,   86,  87,  88,
    89,  90,  91,  92,  93,  94,  95,  96,  97,  6,   98,  99,  100, 101, 102,
    103, 104, 105, 106, 107, 108, 109, 7,   110, 111, 112, 113, 114, 115, 116,
    117, 118, 119, 120, 121, 8,   122, 123, 124, 125, 126, 127, 128, 129, 130,
    131, 132, 133, 9,   134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144,
    145, 10,  146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 11,
    158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 12,  170, 171,
    172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 13,  182, 183, 184, 185,
    186, 187, 188, 189, 190, 191, 192, 193, 14,  194, 195, 196, 197, 198, 199,
    200, 201, 202, 203, 204, 205, 15,  206, 207, 208, 209, 210, 211, 212, 213,
    214, 215, 216, 217, 16,  218, 219, 220, 221, 222, 223, 224, 225, 226, 227,
    228, 229, 17,  230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241,
    18,  242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 19,
  };
  v--;
  m--;
  if ((m << 1) <= MAX_PROB)
    i = recenter_nonneg(v, m) - 1;
  else
    i = recenter_nonneg(MAX_PROB - 1 - v, MAX_PROB - 1 - m) - 1;

  i = map_table[i];
  return i;
}

static int prob_diff_update_cost(aom_prob newp, aom_prob oldp) {
  int delp = remap_prob(newp, oldp);
  return update_bits[delp] << AV1_PROB_COST_SHIFT;
}

static void encode_uniform(aom_writer *w, int v) {
  const int l = 8;
  const int m = (1 << l) - 190;
  if (v < m) {
    aom_write_literal(w, v, l - 1);
  } else {
    aom_write_literal(w, m + ((v - m) >> 1), l - 1);
    aom_write_literal(w, (v - m) & 1, 1);
  }
}

static INLINE int write_bit_gte(aom_writer *w, int word, int test) {
  aom_write_literal(w, word >= test, 1);
  return word >= test;
}

static void encode_term_subexp(aom_writer *w, int word) {
  if (!write_bit_gte(w, word, 16)) {
    aom_write_literal(w, word, 4);
  } else if (!write_bit_gte(w, word, 32)) {
    aom_write_literal(w, word - 16, 4);
  } else if (!write_bit_gte(w, word, 64)) {
    aom_write_literal(w, word - 32, 5);
  } else {
    encode_uniform(w, word - 64);
  }
}

void av1_write_prob_diff_update(aom_writer *w, aom_prob newp, aom_prob oldp) {
  const int delp = remap_prob(newp, oldp);
  encode_term_subexp(w, delp);
}

int av1_prob_diff_update_savings_search(const unsigned int *ct, aom_prob oldp,
                                        aom_prob *bestp, aom_prob upd,
                                        int probwt) {
  const uint32_t old_b = cost_branch256(ct, oldp);
  int bestsavings = 0;
  aom_prob newp, bestnewp = oldp;
  const int step = *bestp > oldp ? -1 : 1;
  const int upd_cost = av1_cost_one(upd) - av1_cost_zero(upd);

  if (old_b > (uint32_t)upd_cost + (MIN_DELP_BITS << AV1_PROB_COST_SHIFT)) {
    for (newp = *bestp; newp != oldp; newp += step) {
      const int new_b = cost_branch256(ct, newp);
      const int update_b = prob_diff_update_cost(newp, oldp) + upd_cost;
      const int savings = (int)((int64_t)old_b - new_b - update_b * probwt);
      if (savings > bestsavings) {
        bestsavings = savings;
        bestnewp = newp;
      }
    }
  }
  *bestp = bestnewp;
  return bestsavings;
}

int av1_prob_diff_update_savings_search_model(const unsigned int *ct,
                                              const aom_prob oldp,
                                              aom_prob *bestp, aom_prob upd,
                                              int stepsize, int probwt) {
  int i, old_b, new_b, update_b, savings, bestsavings;
  int newp;
  const int step_sign = *bestp > oldp ? -1 : 1;
  const int step = stepsize * step_sign;
  const int upd_cost = av1_cost_one(upd) - av1_cost_zero(upd);
  const aom_prob *newplist, *oldplist;
  aom_prob bestnewp;
  oldplist = av1_pareto8_full[oldp - 1];
  old_b = cost_branch256(ct + 2 * PIVOT_NODE, oldp);
  for (i = UNCONSTRAINED_NODES; i < ENTROPY_NODES; ++i)
    old_b += cost_branch256(ct + 2 * i, oldplist[i - UNCONSTRAINED_NODES]);

  bestsavings = 0;
  bestnewp = oldp;

  assert(stepsize > 0);

  if (old_b > upd_cost + (MIN_DELP_BITS << AV1_PROB_COST_SHIFT)) {
    for (newp = *bestp; (newp - oldp) * step_sign < 0; newp += step) {
      if (newp < 1 || newp > 255) continue;
      newplist = av1_pareto8_full[newp - 1];
      new_b = cost_branch256(ct + 2 * PIVOT_NODE, newp);
      for (i = UNCONSTRAINED_NODES; i < ENTROPY_NODES; ++i)
        new_b += cost_branch256(ct + 2 * i, newplist[i - UNCONSTRAINED_NODES]);
      update_b = prob_diff_update_cost(newp, oldp) + upd_cost;
      savings = old_b - new_b - update_b * probwt;
      if (savings > bestsavings) {
        bestsavings = savings;
        bestnewp = newp;
      }
    }
  }

  *bestp = bestnewp;
  return bestsavings;
}

#if CONFIG_ENTROPY
static int get_cost(unsigned int ct[][2], aom_prob p, int n) {
  int i, p0 = p;
  unsigned int total_ct[2] = { 0, 0 };
  int cost = 0;

  for (i = 0; i <= n; ++i) {
    cost += cost_branch256(ct[i], p);
    total_ct[0] += ct[i][0];
    total_ct[1] += ct[i][1];
    if (i < n)
      p = av1_merge_probs(p0, total_ct, COEF_COUNT_SAT_BITS,
                          COEF_MAX_UPDATE_FACTOR_BITS);
  }
  return cost;
}

int av1_prob_update_search_subframe(unsigned int ct[][2], aom_prob oldp,
                                    aom_prob *bestp, aom_prob upd, int n) {
  const int old_b = get_cost(ct, oldp, n);
  int bestsavings = 0;
  const int upd_cost = av1_cost_one(upd) - av1_cost_zero(upd);
  aom_prob newp, bestnewp = oldp;
  const int step = *bestp > oldp ? -1 : 1;

  for (newp = *bestp; newp != oldp; newp += step) {
    const int new_b = get_cost(ct, newp, n);
    const int update_b = prob_diff_update_cost(newp, oldp) + upd_cost;
    const int savings = old_b - new_b - update_b;
    if (savings > bestsavings) {
      bestsavings = savings;
      bestnewp = newp;
    }
  }
  *bestp = bestnewp;
  return bestsavings;
}

int av1_prob_update_search_model_subframe(unsigned int ct[ENTROPY_NODES]
                                                         [COEF_PROBS_BUFS][2],
                                          const aom_prob *oldp, aom_prob *bestp,
                                          aom_prob upd, int stepsize, int n) {
  int i, old_b, new_b, update_b, savings, bestsavings;
  int newp;
  const int step_sign = *bestp > oldp[PIVOT_NODE] ? -1 : 1;
  const int step = stepsize * step_sign;
  const int upd_cost = av1_cost_one(upd) - av1_cost_zero(upd);
  aom_prob bestnewp, newplist[ENTROPY_NODES], oldplist[ENTROPY_NODES];
  av1_model_to_full_probs(oldp, oldplist);
  memcpy(newplist, oldp, sizeof(aom_prob) * UNCONSTRAINED_NODES);
  for (i = UNCONSTRAINED_NODES, old_b = 0; i < ENTROPY_NODES; ++i)
    old_b += get_cost(ct[i], oldplist[i], n);
  old_b += get_cost(ct[PIVOT_NODE], oldplist[PIVOT_NODE], n);

  bestsavings = 0;
  bestnewp = oldp[PIVOT_NODE];

  assert(stepsize > 0);

  for (newp = *bestp; (newp - oldp[PIVOT_NODE]) * step_sign < 0; newp += step) {
    if (newp < 1 || newp > 255) continue;
    newplist[PIVOT_NODE] = newp;
    av1_model_to_full_probs(newplist, newplist);
    for (i = UNCONSTRAINED_NODES, new_b = 0; i < ENTROPY_NODES; ++i)
      new_b += get_cost(ct[i], newplist[i], n);
    new_b += get_cost(ct[PIVOT_NODE], newplist[PIVOT_NODE], n);
    update_b = prob_diff_update_cost(newp, oldp[PIVOT_NODE]) + upd_cost;
    savings = old_b - new_b - update_b;
    if (savings > bestsavings) {
      bestsavings = savings;
      bestnewp = newp;
    }
  }

  *bestp = bestnewp;
  return bestsavings;
}
#endif  // CONFIG_ENTROPY

void av1_cond_prob_diff_update(aom_writer *w, aom_prob *oldp,
                               const unsigned int ct[2], int probwt) {
  const aom_prob upd = DIFF_UPDATE_PROB;
  aom_prob newp = get_binary_prob(ct[0], ct[1]);
  const int savings =
      av1_prob_diff_update_savings_search(ct, *oldp, &newp, upd, probwt);
  assert(newp >= 1);
  if (savings > 0) {
    aom_write(w, 1, upd);
    av1_write_prob_diff_update(w, newp, *oldp);
    *oldp = newp;
  } else {
    aom_write(w, 0, upd);
  }
}

int av1_cond_prob_diff_update_savings(aom_prob *oldp, const unsigned int ct[2],
                                      int probwt) {
  const aom_prob upd = DIFF_UPDATE_PROB;
  aom_prob newp = get_binary_prob(ct[0], ct[1]);
  const int savings =
      av1_prob_diff_update_savings_search(ct, *oldp, &newp, upd, probwt);
  return savings;
}

void aom_write_primitive_symmetric(aom_writer *w, int word,
                                   unsigned int abs_bits) {
  if (word == 0) {
    aom_write_bit(w, 0);
  } else {
    const int x = abs(word);
    const int s = word < 0;
    aom_write_bit(w, 1);
    aom_write_bit(w, s);
    aom_write_literal(w, x - 1, abs_bits);
  }
}
