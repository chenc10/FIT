for i in 1 2 3 4 5 6 7; do
	echo "voice"$i".pcap"
	TPTD -r "voice"$i".pcap" 2>result
	if [ -e nids_cTos_udp ]; then
		mv nids_cTos_udp "v"$i"_cTos_udp"
	fi
	if [ -e nids_sToc_udp ]; then
		mv nids_sToc_udp "v"$i"_sToc_udp"
	fi
	if [ -e nids_cTos_tcp ]; then
		mv nids_cTos_tcp "v"$i"_cTos_tcp"
	fi
	if [ -e nids_sToc_tcp ]; then
		mv nids_sToc_tcp "v"$i"_sToc_tcp"
	fi
	#rm -rf nids_*
done
