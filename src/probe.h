/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef DIAG_PROBE_H
#define DIAG_PROBE_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "transport.h"
typedef struct { uint8_t number, control; uint32_t lba; } DiagTrack;
typedef struct { DiagTrack tracks[99]; size_t count; uint32_t leadout; } DiagToc;
int parse_toc(const uint8_t *, size_t, DiagToc *);
int probe_drive(CdPass *, FILE *);
#endif
