for i in 1 2 3 4 5 6 7 8 9 10; do
	Name=`ls $i[^0-9]*.pcap`
	#Name=${Name%%.pcap}
	#Name=${Name##$i}
	#echo $Name
	sh ./exe/sh.sh -i $Name
done
	
