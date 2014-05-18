#!/bin/bash
#program:
# input:data to save all pcap file. output:cTos_ASM sToc_ASM
#history:
PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin:~/bin
export PATH=$PATH:$PWD
while getopts "n:w:f:c:d:" arg
do
	case $arg in
		n)
			file=$OPTARG
			;;
		w)
			WThreshold=$OPTARG
			;;
		f)
			FThreshold=$OPTARG
			;;
		c)
			CoverThreshold=$OPTARG
			;;
		d)
			DiscardThreshold=$OPTARG
			;;
		?)
			echo "unknow argument"
			exit 1
			;;
	esac
done

readfile=$file".pcap"
out_tcp_cs=$file"_tcp_cs"
out_tcp_sc=$file"_tcp_sc"
out_udp=$file"udp"
sign_tcp_cs=$file"_sign_tcp_cs"
sign_tcp_sc=$file"_sign_tcp_sc"
sign_udp=$file"sign_udp"
./TPTD -r $readfile
if [ -e $out_tcp_cs ]; then
	./ASM -i out_tcp_cs -o sign_tcp_cs -w $WThreshold -f $FThreshold -c $CoverThreshold-d $DiscardThreshold 
	echo 'OK-cTos_ASM'
else
	echo 'No nids_cTos_out'

fi
if [ -e $out_tcp_sc ]; then
	./ASM -i out_tcp_sc -o sign_tcp_sc -w $WThreshold -f $FThreshold -c $CoverThreshold-d $DiscardThreshold 
	echo 'OK-sToc_ASM'
else
	echo 'No nids_sToc_out'
fi
if [ -e $out_udp ]; then
	./ASM -i out_tcp_sc -o sign_tcp_sc -w $WThreshold -f $FThreshold -c $CoverThreshold-d $DiscardThreshold
	echo 'OK-UDP_ASM'
else
	echo 'No udp data'
fi 
	
