"""Build and execute real C host tests against a mock transport, never PS5 hardware."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

p=argparse.ArgumentParser()
p.add_argument('--cc',nargs='+',default=['cc'],help='compiler command, e.g. --cc zig cc')
a=p.parse_args()
root=Path(__file__).resolve().parents[1]
os.chdir(root)
(root/'build').mkdir(exist_ok=True)
(root/'results').mkdir(exist_ok=True)
(root/'build/tmp').mkdir(exist_ok=True)
os.environ['TMP']=os.environ['TEMP']=str(root/'build/tmp')
flags=['-std=c11','-Wall','-Wextra','-Werror','-Isrc']
ext='.exe' if os.name=='nt' else ''
exe=root/'build'/('diagnostic-mock'+ext)
toc=root/'build'/('test-toc'+ext)
logs=[]
def execute(cmd,**kwargs):
    r=subprocess.run([str(s) for s in cmd],capture_output=True,text=True,**kwargs)
    logs.append({'command':[str(s) for s in cmd],'exit_code':r.returncode,'stdout':r.stdout,'stderr':r.stderr})
    return r
for cmd in [a.cc+flags+['-DDIAG_MOCK','src/main.c','src/probe.c','tests/transport_mock.c','-o',str(exe)],
            a.cc+flags+['tests/test_toc.c','src/probe.c','tests/transport_mock.c','-o',str(toc)]]:
    r=execute(cmd)
    if r.returncode:
        (root/'results/host-tests.json').write_text(json.dumps({'scope':'compiled_C_host_mock_only','status':'build_failed','hardware_tested':False,'ps5_build_tested':False,'commands':logs},indent=2)+'\n')
        print(r.stderr); sys.exit(2)
r=execute([toc])
assert r.returncode==0, r.stderr
cases=[{'id':'toc_parser_boundaries','status':'pass'}]
for scenario in ['ok','attention','absent','disconnect','empty','truncated_toc','bad_sector','short','unstable']:
    env=dict(os.environ,DIAG_SCENARIO=scenario)
    report=root/'results'/('mock-'+scenario+'.jsonl')
    r=execute([exe,report],env=env)
    expected=0 if scenario in ['ok','attention'] else 1
    assert r.returncode==expected,(scenario,r.returncode,r.stderr)
    events=[json.loads(s) for s in report.read_text().splitlines()]
    assert events[0]['backend']=='host_mock'
    assert events[-1]['event']=='summary'
    if scenario in ['ok','attention']:
        assert events[-1]['sample_passed']==1
        assert [x['track'] for x in events if x['event']=='sample']==[1,2]
        assert all(x['repeat_equal'] for x in events if x['event']=='sample')
    if scenario=='short':
        assert any(x.get('actual')==16 and x.get('result')==-3 for x in events)
    if scenario=='unstable':
        assert any(x['read_ok'] and not x['repeat_equal'] for x in events if x['event']=='sample')
    if scenario=='empty':
        assert any(x.get('sense')==0x023a00 for x in events)
    cases.append({'id':scenario,'status':'pass','exit_code':r.returncode})
result={'scope':'compiled_C_host_mock_only','compiler':a.cc,'hardware_tested':False,'ps5_build_tested':False,
        'games_tested':False,'cases':cases,'passed':len(cases),'failed':0,'commands':logs}
(root/'results/host-tests.json').write_text(json.dumps(result,indent=2)+'\n')
print(f'{len(cases)} compiled C host checks passed; no PS5 build or hardware test performed.')
