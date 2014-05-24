for i in 3 4 5 6 7 8 9; do
	TPTD -r $i*.pcap -n $i 2>result
	mv nids_cTos_udp $i"nids_cTos_udp"
	rm -rf nids_*
done
