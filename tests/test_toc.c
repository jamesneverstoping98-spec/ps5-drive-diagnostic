/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "probe.h"
#include <assert.h>
#include <string.h>
int main(void) {
    uint8_t toc[28]={0,26,1,2,0,0x14,1,0,0,0,0,0,0,0x10,2,0,0,0,0,24,0,0x10,0xaa,0,0,0,0,40};
    DiagToc t;
    assert(parse_toc(toc,28,&t)==0 && t.count==2 && t.leadout==40 && t.tracks[1].lba==24);
    for(size_t i=0;i<28;i++) assert(parse_toc(toc,i,&t)!=0);
    uint8_t bad[28];
    memcpy(bad,toc,28); bad[1]=25; assert(parse_toc(bad,28,&t)!=0);
    memcpy(bad,toc,28); bad[2]=2; assert(parse_toc(bad,28,&t)!=0);
    memcpy(bad,toc,28); bad[14]=1; assert(parse_toc(bad,28,&t)!=0);
    memcpy(bad,toc,28); bad[19]=0; assert(parse_toc(bad,28,&t)!=0);
    memcpy(bad,toc,28); bad[22]=3; assert(parse_toc(bad,28,&t)!=0);
    memcpy(bad,toc,28); bad[27]=24; assert(parse_toc(bad,28,&t)!=0);
    return 0;
}
