/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "probe.h"
#include <string.h>
#include <errno.h>

static uint32_t be32(const uint8_t *p) {
    return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3];
}
int parse_toc(const uint8_t *data, size_t got, DiagToc *out) {
    memset(out, 0, sizeof(*out));
    if (got < 4) return -1;
    size_t length = ((size_t)data[0]<<8 | data[1])+2;
    if (length > got || length < 20 || (length-4)%8) return -1;
    unsigned first=data[2], last=data[3];
    if (first != 1 || last < first || last > 99) return -1;
    bool lead=false;
    for (size_t off=4; off<length; off+=8) {
        uint8_t track=data[off+2]; uint32_t lba=be32(data+off+4);
        if ((data[off+1] >> 4) != 1 || lba > 0x7fffffffu) return -1;
        if (track==0xaa) {
            if (lead || off+8!=length || out->count != last) return -1;
            out->leadout=lba; lead=true;
        } else {
            if (lead || out->count>=99 || track != out->count+1 || track>last) return -1;
            if (out->count && lba<=out->tracks[out->count-1].lba) return -1;
            out->tracks[out->count++]=(DiagTrack){track, (uint8_t)(data[off+1]&15), lba};
        }
    }
    if (!lead || !out->count || out->leadout <= out->tracks[out->count-1].lba) return -1;
    return 0;
}
static int command(CdPass *p, FILE *f, const char *stage, const uint8_t *cdb,
                   int cdb_len, void *buf, uint32_t requested, bool variable,
                   uint32_t *actual, uint32_t *sense) {
    errno=0;
    uint32_t got=0;
    int rc=transport_command(p,cdb,cdb_len,buf,requested,5000,sense,&got);
    int saved_errno=errno;
    if (rc==0 && (got>requested || (!variable && got!=requested))) rc=-3;
    *actual=got;
    fprintf(f,"{\"event\":\"command\",\"stage\":\"%s\",\"opcode\":%u,\"requested\":%u,\"actual\":%u,\"result\":%d,\"sense\":%u,\"os_errno\":%d}\n",
            stage,cdb[0],requested,got,rc,*sense,saved_errno);
    fflush(f);
    return rc;
}
static uint32_t crc32(const uint8_t *data,size_t n) {
    uint32_t crc=0xffffffffu;
    while(n--) { crc^=*data++; for(int i=0;i<8;i++) crc=(crc>>1)^(0xedb88320u & (0u-(crc&1))); }
    return ~crc;
}
int probe_drive(CdPass *p, FILE *f) {
    uint8_t inquiry[36]={0}, toc_data[804]={0}, raw[2352];
    uint32_t got=0,sense=0;
    const uint8_t inq[6]={0x12,0,0,0,36,0};
    if (command(p,f,"inquiry",inq,6,inquiry,36,false,&got,&sense)) return 1;
    /* Hex encoding avoids untrusted device strings breaking JSON. */
    fprintf(f,"{\"event\":\"inquiry\",\"identity_hex\":\"");
    for(int i=8;i<36;i++) fprintf(f,"%02x",inquiry[i]);
    fprintf(f,"\",\"device_type\":%u}\n",inquiry[0]&31);
    if ((inquiry[0]&31)!=5) return 1;
    const uint8_t tur[6]={0}; int ready=-1;
    for(int attempt=0;attempt<3;attempt++) {
        ready=command(p,f,"ready",tur,6,NULL,0,false,&got,&sense);
        if (!ready || ready==-2 || (sense>>8)==0x023a) break;
        /* Only retry unit attention, not arbitrary errors. */
        if ((sense>>16)!=6) break;
    }
    if (ready) return 1;
    const uint8_t toc_cmd[10]={0x43,0,0,0,0,0,0,3,36,0};
    if (command(p,f,"toc",toc_cmd,10,toc_data,sizeof(toc_data),true,&got,&sense)) return 1;
    DiagToc toc;
    if (parse_toc(toc_data,got,&toc)) {
        fprintf(f,"{\"event\":\"error\",\"reason\":\"invalid_or_unsupported_toc\"}\n"); return 1;
    }
    fprintf(f,"{\"event\":\"toc\",\"track_count\":%zu,\"leadout\":%u}\n",toc.count,toc.leadout);
    int failures=0;
    for(size_t i=0;i<toc.count;i++) {
        DiagTrack t=toc.tracks[i];
        fprintf(f,"{\"event\":\"track\",\"number\":%u,\"control\":%u,\"lba\":%u}\n",t.number,t.control,t.lba);
        /* Read only one sector at each INDEX 01, twice. No disc dump or writes. */
        uint8_t cdb[12]={0xbe,0,(uint8_t)(t.lba>>24),(uint8_t)(t.lba>>16),(uint8_t)(t.lba>>8),(uint8_t)t.lba,0,0,1,(t.control&4)?0xf8:0x10,0,0};
        uint32_t sums[2]={0}; bool ok=true;
        for(int repeat=0;repeat<2;repeat++) {
            memset(raw,0,sizeof(raw));
            if(command(p,f,"read_cd",cdb,12,raw,sizeof(raw),false,&got,&sense)) {ok=false; break;}
            sums[repeat]=crc32(raw,sizeof(raw));
        }
        bool stable=ok && sums[0]==sums[1];
        fprintf(f,"{\"event\":\"sample\",\"track\":%u,\"lba\":%u,\"read_ok\":%s,\"repeat_equal\":%s,\"crc32\":%u}\n",t.number,t.lba,ok?"true":"false",stable?"true":"false",sums[0]);
        fflush(f);
        if(!stable) failures++;
        if(!ok && sense==0) break; /* possible disconnect: stop issuing commands */
    }
    return failures ? 1 : 0;
}
