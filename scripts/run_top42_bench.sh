#!/usr/bin/env bash
set -euo pipefail
out_dir="${1:-.}"
mkdir -p "$out_dir"
sha="$(git rev-parse --short HEAD 2>/dev/null || echo unknown)"
ts="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
python - "$out_dir" "$sha" "$ts" <<'PY'
import csv,json,os,sys
out,sha,ts=sys.argv[1:]
names=[
"memcpy MB/s","memset MB/s","crc32c MiB/s","crc32_sw MiB/s","EMA ops/sec","entropy ops/sec","DirectByteBuffer process ops/sec","JNI call latency ns","vCPU step ops/sec","stepAllVcpus ops/sec","target_hz","actual_hz_q16","jitter_ns","missed_ticks","period_ns","L1 state bytes","L2 input bytes","L2 output bytes","JNI arena used","baremetal arena used","sequential read MB/s","sequential write MB/s","random read 4K IOPS","random write 4K IOPS","fsync latency","shell spawn latency","command throughput","matrix create ops/sec","vector dot ops/sec","vector add ops/sec","BitRAF encode ops/sec","BitRAF decode ops/sec","GP small sample time","Java heap KB","native heap KB","RSS idle MB","RSS after benchmark MB","APK size",".so size","page size","cache line","battery / energy estimate when available"]
metrics=[{"name":n,"value":None,"unit":"raw","status":"NEEDS_ARTIFACT","source":"CI_ARTIFACT","command":"scripts/run_top42_bench.sh","timestamp":ts} for n in names]
d={"schema_version":"1.0.0","git_sha":sha,"device":"github-actions-generic","abi":"unknown","page_size":None,"cache_line":None,"target_hz":None,"actual_hz_q16":None,"jitter_ns":None,"metrics":metrics}
with open(os.path.join(out,'top42.json'),'w') as f: json.dump(d,f,indent=2)
with open(os.path.join(out,'top42.csv'),'w',newline='') as f:
 w=csv.writer(f);w.writerow(['name','value','unit','status','source','command','timestamp'])
 [w.writerow([m['name'],m['value'],m['unit'],m['status'],m['source'],m['command'],m['timestamp']]) for m in metrics]
with open(os.path.join(out,'top42_env.txt'),'w') as f: f.write(f'git_sha={sha}\ntimestamp={ts}\nenv=ci-generic\n')
with open(os.path.join(out,'top42_readme.md'),'w') as f: f.write('# TOP42\nGerado em CI genérico: status NEEDS_ARTIFACT para métricas dependentes de device.\n')
PY
