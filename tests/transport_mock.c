/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "transport.h"
#include <stdlib.h>
#include <string.h>
static const char *scenario(void) { const char *s=getenv("DIAG_SCENARIO"); return s?s:"ok"; }
static int is(const char *s) { return strcmp(scenario(),s)==0; }
bool transport_open(unsigned unit,CdPass *p) { p->fd=0; return unit==0 && !is("absent"); }
void transport_close(CdPass *p) { p->fd=-1; }
int transport_command(const CdPass *p,const uint8_t *cdb,int n,void *buf,uint32_t len,uint32_t timeout,uint32_t *sense,uint32_t *got) {
    (void)p; (void)n; (void)timeout; *sense=0; *got=0;
    if(is("disconnect")) return -2;
    if(cdb[0]==0x12) {
        memset(buf,0,len); ((uint8_t *)buf)[0]=5;
        memcpy((uint8_t *)buf+8,"VIRTUAL DIAGNOSTIC DRIVE     ",28); *got=len; return 0;
    }
    if(cdb[0]==0) {
        if(is("empty")) { *sense=0x023a00; return -3; }
        static int ready_calls;
        if(is("attention") && ready_calls++==0) { *sense=0x062800; return -3; }
        return 0;
    }
    if(cdb[0]==0x43) {
        uint8_t toc[28]={0,26,1,2, 0,0x14,1,0,0,0,0,0,
                                    0,0x10,2,0,0,0,0,24,
                                    0,0x10,0xaa,0,0,0,0,40};
        memcpy(buf,toc,sizeof(toc)); *got=is("truncated_toc")?10:28; return 0;
    }
    if(cdb[0]==0xbe) {
        if(is("bad_sector")) { *sense=0x031100; return -3; }
        if(is("short")) { *got=16; return 0; }
        static int calls;
        memset(buf,is("unstable")?++calls:42,len); *got=len; return 0;
    }
    return -3;
}
