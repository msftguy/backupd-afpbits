#!/bin/bash
rm backupd_anyafp.zip
set -e 
cp build/Debug/libbackupd_anyfs.dylib backupd_anyafp/
zip -r backupd_anyafp.zip backupd_anyafp -x '*.DS_Store'
