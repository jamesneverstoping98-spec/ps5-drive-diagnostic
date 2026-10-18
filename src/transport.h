/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef DIAG_TRANSPORT_H
#define DIAG_TRANSPORT_H
#include <stdbool.h>
#include <stdint.h>
typedef struct { int fd, path, target, lun; } CdPass;
bool transport_open(unsigned unit, CdPass *pass);
void transport_close(CdPass *pass);
/* 0 success, -1 local request/allocation failure, -2 OS failure,
   -3 device status/short transfer. got allows variable-length replies. */
int transport_command(const CdPass *, const uint8_t *, int, void *, uint32_t,
                      uint32_t, uint32_t *, uint32_t *);
#endif
