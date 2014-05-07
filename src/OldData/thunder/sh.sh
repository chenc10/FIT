#!/bin/bash
#program:
# input:data to save all pcap file. output:cTos_ASM sToc_ASM
#history:
PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin:~/bin
export PATH=$PATH:$PWD
./TPTD -r $1 #we put pcap data into data file
mv nids_cTos_out Dat.dat
./ASM 0.1 
mv Result.result cTos_ASM
echo 'OK-cTos_ASM'
mv nids_sToc_out Dat.dat
./ASM
mv Result.result sToc_ASM
echo 'OK-sToc_ASM'
