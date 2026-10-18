/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "probe.h"
#include <errno.h>
#include <sys/stat.h>
#include <time.h>

int main(int argc,char **argv) {
    const char *path=argc>1?argv[1]:"/data/PS5DriveDiagnostic/report.jsonl";
#if defined(__PROSPERO__)
    mkdir("/data/PS5DriveDiagnostic",0777);
#endif
    FILE *f=fopen(path,"w");
    if(!f) { fprintf(stderr,"Cannot create diagnostic report: errno=%d\n",errno); return 2; }
#ifdef DIAG_MOCK
    const char *backend="host_mock";
#else
    const char *backend="ps5_cam_unvalidated";
#endif
    fprintf(f,"{\"event\":\"session\",\"schema_version\":1,\"backend\":\"%s\",\"unix_time\":%lld,\"target_scope\":[\"PS5 original\",\"PS5 Slim\"],\"model\":\"unrecorded\",\"games_tested\":false,\"emulator_tested\":false}\n",backend,(long long)time(NULL));
    fflush(f);
    unsigned found=0,passed=0;
    for(unsigned unit=0;unit<8;unit++) {
        CdPass pass;
        errno=0; bool opened=transport_open(unit,&pass); int error=errno;
        fprintf(f,"{\"event\":\"open\",\"unit\":%u,\"opened\":%s,\"os_errno\":%d}\n",unit,opened?"true":"false",error);
        fflush(f);
        if(!opened) continue;
        found++;
        if(!probe_drive(&pass,f)) passed++;
        transport_close(&pass);
    }
    fprintf(f,"{\"event\":\"summary\",\"opened\":%u,\"sample_passed\":%u,\"sample_failed\":%u}\n",found,passed,found-passed);
    int write_failed=ferror(f);
    if(fclose(f)!=0) write_failed=1;
    return write_failed?2:(found && found==passed?0:1);
}
