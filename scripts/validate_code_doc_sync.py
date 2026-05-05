#!/usr/bin/env python3
import re,sys,os
root='.'
errors=[]
mk='app/src/main/cpp/Android.mk'
if os.path.exists(mk):
  txt=open(mk,encoding='utf-8',errors='ignore').read()
  for m in re.findall(r'([\w/.-]+\.c)',txt):
    p=os.path.join('app/src/main/cpp',m) if not m.startswith('app/') else m
    if not os.path.exists(p): errors.append(f'missing source in Android.mk: {p}')
c='app/src/main/cpp/lowlevel/rafaelia_jni_direct.c'
if os.path.exists(c):
  t=open(c,encoding='utf-8',errors='ignore').read()
  if 'GetDirectBufferAddress' in t and 'GetDirectBufferCapacity' not in t:
    errors.append('JNI uses GetDirectBufferAddress without capacity checks')
for doc in ['docs/RAFAELIA_CODE_DOC_SYNC.md','docs/RAFAELIA_TOP42_BENCHMARKS.md']:
  if not os.path.exists(doc): errors.append(f'missing doc {doc}')
if errors:
  print('\n'.join(errors)); sys.exit(1)
print('validate_code_doc_sync: ok')
