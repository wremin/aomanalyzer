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

#ifndef AV1_COMMON_FILTER_H_
#define AV1_COMMON_FILTER_H_

#include "./aom_config.h"
#include "aom/aom_integer.h"
#include "aom_dsp/aom_filter.h"
#include "aom_ports/mem.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EIGHTTAP_REGULAR 0
#define EIGHTTAP_SMOOTH 1
#define MULTITAP_SHARP 2

#if CONFIG_DUAL_FILTER
#define EIGHTTAP_SMOOTH2 3

#define MAX_SUBPEL_TAPS 12

#define SWITCHABLE_FILTERS 4 /* Number of switchable filters */
#define LOG_SWITCHABLE_FILTERS \
  3 /* (1 << LOG_SWITCHABLE_FILTERS) > SWITCHABLE_FILTERS */

#define EIGHTTAP_SHARP (SWITCHABLE_FILTERS + 1)
#define FILTER_REGULAR_UV (SWITCHABLE_FILTERS + 2)
#define FILTER_SMOOTH_UV (SWITCHABLE_FILTERS + 3)
#define FILTER_SHARP_UV (SWITCHABLE_FILTERS + 4)
#define FILTER_SMOOTH2_UV (SWITCHABLE_FILTERS + 5)
#define EXTRA_FILTERS 6
#define SWITCHABLE_FILTER_CONTEXTS ((SWITCHABLE_FILTERS + 1) * 4)
#define INTER_FILTER_COMP_OFFSET (SWITCHABLE_FILTERS + 1)
#define INTER_FILTER_DIR_OFFSET ((SWITCHABLE_FILTERS + 1) * 2)

#else  // CONFIG_DUAL_FILTER

#define SWITCHABLE_FILTERS 3 /* Number of switchable filters */
#define LOG_SWITCHABLE_FILTERS \
  2 /* (1 << LOG_SWITCHABLE_FILTERS) > SWITCHABLE_FILTERS */
#define SWITCHABLE_FILTER_CONTEXTS (SWITCHABLE_FILTERS + 1)
#define EXTRA_FILTERS 1

#endif  // CONFIG_DUAL_FILTER

#define USE_TEMPORALFILTER_12TAP 1
#if USE_TEMPORALFILTER_12TAP
#define TEMPORALFILTER_12TAP (SWITCHABLE_FILTERS + EXTRA_FILTERS)
#endif

#define BILINEAR (SWITCHABLE_FILTERS)
#define SWITCHABLE (SWITCHABLE_FILTERS + 1) /* the last one */

typedef uint8_t InterpFilter;

#if CONFIG_EXT_INTRA
#if CONFIG_INTRA_INTERP
typedef enum {
  INTRA_FILTER_LINEAR,
  INTRA_FILTER_8TAP,
  INTRA_FILTER_8TAP_SHARP,
  INTRA_FILTER_8TAP_SMOOTH,
  INTRA_FILTERS,
} INTRA_FILTER;

extern const InterpKernel *av1_intra_filter_kernels[INTRA_FILTERS];
#endif  // CONFIG_INTRA_INTERP
#endif  // CONFIG_EXT_INTRA

typedef struct InterpFilterParams {
  const int16_t *filter_ptr;
  uint16_t taps;
  uint16_t subpel_shifts;
  InterpFilter interp_filter;
} InterpFilterParams;

InterpFilterParams av1_get_interp_filter_params(
    const InterpFilter interp_filter);

const int16_t *av1_get_interp_filter_kernel(const InterpFilter interp_filter);

static INLINE const int16_t *av1_get_interp_filter_subpel_kernel(
    const InterpFilterParams filter_params, const int subpel) {
  return filter_params.filter_ptr + filter_params.taps * subpel;
}

static INLINE int av1_is_interpolating_filter(
    const InterpFilter interp_filter) {
  const InterpFilterParams ip = av1_get_interp_filter_params(interp_filter);
  return (ip.filter_ptr[ip.taps / 2 - 1] == 128);
}

#if CONFIG_DUAL_FILTER
InterpFilter av1_get_plane_interp_filter(InterpFilter interp_filter, int plane);
#endif

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // AV1_COMMON_FILTER_H_
