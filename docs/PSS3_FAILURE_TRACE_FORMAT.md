# PSS3 Failure Trace Format

Arquivo: `out/failure_trace.csv`.

CSV simples (split por vírgula), **sem suporte** a aspas com vírgulas internas.

Obrigatórias: timestamp,run_id,domain,layer,file,function,line,signature_hash,event_type,severity,recurring,stable_after_fix,rollback,exit_code,signal,errno,latency_ms,delta_ms,gate,notes

Opcionais: android_api,abi,device_model,commit_sha,workflow_run,test_name,artifact_sha256,module,thread_id,pid,ppid,zombie_detected
