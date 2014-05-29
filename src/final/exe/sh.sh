#!/bin/bash
#program:
# input:data to save all pcap file. output:cTos_ASM sToc_ASM
#history:
Option=" "
while getopts "i:w:c:d:" arg
do
	case $arg in
		i)
			readfile=$OPTARG
			;;
		w)
			WThreshold=$OPTARG
			Option=$Option" -w "$WThreshold
			;;
		c)
			CoverThreshold=$OPTARG
			Option=$Option" -c "$CoverThreshold
			;;
		d)
			DiscardThreshold=$OPTARG
			Option=$Option" -d "$DiscardThreshold
			;;
		?)
			echo "unknow argument"
			exit 1
			;;
	esac
done
echo $readfile

./exe/TPTD -r $readfile 2>result

AppType=${readfile%%.pcap}
echo $AppType
F_tcp_cs=$AppType"_F_tcp_cs"
F_tcp_sc=$AppType"_F_tcp_sc"
F_udp_cs=$AppType"_F_udp_cs"
F_udp_sc=$AppType"_F_udp_sc"

P_tcp_cs=$AppType"_P_tcp_cs"
P_tcp_sc=$AppType"_P_tcp_sc"
P_udp_cs=$AppType"_P_udp_cs"
P_udp_sc=$AppType"_P_udp_sc"


if [ -e nids_cTos_tcp ]; then
	./exe/10ASM_F -i nids_cTos_tcp -o F_tcp_cs $Option
	./exe/9ASM_P -i nids_cTos_tcp -o P_tcp_cs $Option
	echo "OK-"$AppType"-tcp_cTos"
else
	echo 'No nids_cTos_tcp'

fi

if [ -e nids_sToc_tcp ]; then
	./exe/10ASM_F -i nids_sToc_tcp -o F_tcp_sc $Option
	./exe/9ASM_P -i nids_sToc_tcp -o P_tcp_sc $Option
	echo "OK-"$AppType"-tcp_sToc"
else
	echo 'No nids_sToc_tcp'

fi

if [ -e nids_cTos_udp ]; then
	./exe/10ASM_F -i nids_cTos_udp -o F_udp_cs $Option
	./exe/9ASM_P -i nids_cTos_udp -o P_udp_cs $Option
	echo "OK-"$AppType"-udp_cTos"
else
	echo 'No nids_cTos_udp'

fi

if [ -e nids_sToc_udp ]; then
	./exe/10ASM_F -i nids_sToc_udp -o F_udp_sc $Option
	./exe/9ASM_P -i nids_sToc_udp -o P_udp_sc $Option
	echo "OK-"$AppType"-udp_sToc"
else
	echo 'No nids_sToc_udp'

fi

echo $AppType
mkdir -p $AppType"/nids_data"
mv nids_* $AppType"/nids_data"
mv *_*_* $AppType
rm -rf result


