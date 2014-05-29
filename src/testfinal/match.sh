for i in 1 2 3 4 5 6 7 8 9 10; do
	cp ./$i[^0-9]*/nids*/*_cTos_tcp ./$i"_cTos_tcp"
	cp ./$i[^0-9]*/nids*/*_sToc_tcp ./$i"_sToc_tcp"
	cp ./$i[^0-9]*/nids*/*_cTos_udp ./$i"_cTos_udp"
	cp ./$i[^0-9]*/nids*/*_sToc_udp ./$i"_sToc_udp"
done

cat *_cTos_tcp > nids_cTos_tcp
cat *_sToc_tcp > nids_sToc_tcp
cat *_cTos_udp > nids_cTos_udp
cat *_sToc_udp > nids_sToc_udp

rm -rf [0-9]*_*_*

for i in 1 2 3 4 5 6 7 8 9 10; do
	echo "Match Result" >> ./$i[^0-9]*/matchresult
	if [ -e ./$i[^0-9]*/F_tcp_cs ]; then
		./exe/10ASM_F -r nids_cTos_tcp -f ./$i[^0-9]*/F_tcp_cs -n $i
		echo "	F_tcp_cs:" >> ./$i[^0-9]*/matchresult
		echo -n "		total: " >> ./$i[^0-9]*/matchresult
		cat MatchResult.dat | wc -l  	 >> ./$i[^0-9]*/matchresult
		echo -n "			0-0: " >> ./$i[^0-9]*/matchresult
		grep -c " 0 0" MatchResult.dat >> ./$i[^0-9]*/matchresult
		echo -n "			0-1: " >> ./$i[^0-9]*/matchresult
		grep -c " 0 1" MatchResult.dat >> ./$i[^0-9]*/matchresult
		echo -n "			1-0: " >> ./$i[^0-9]*/matchresult
		grep -c " 1 0" MatchResult.dat >> ./$i[^0-9]*/matchresult
		echo -n "			1-1: " >> ./$i[^0-9]*/matchresult
		grep -c " 1 1" MatchResult.dat >> ./$i[^0-9]*/matchresult
	fi
	if [ -e ./$i[^0-9]*/F_tcp_sc ]; then
		./exe/10ASM_F -r nids_sToc_tcp -f ./$i[^0-9]*/F_tcp_sc -n $i
		echo "	F_tcp_sc:" >> ./$i[^0-9]*/matchresult
		echo -n "		total: " >> ./$i[^0-9]*/matchresult
		cat MatchResult.dat | wc -l  	 >> ./$i[^0-9]*/matchresult
		echo -n "			0-0: " >> ./$i[^0-9]*/matchresult
		grep -c " 0 0" MatchResult.dat >> ./$i[^0-9]*/matchresult
		echo -n "			0-1: " >> ./$i[^0-9]*/matchresult
		grep -c " 0 1" MatchResult.dat >> ./$i[^0-9]*/matchresult
		echo -n "			1-0: " >> ./$i[^0-9]*/matchresult
		grep -c " 1 0" MatchResult.dat >> ./$i[^0-9]*/matchresult
		echo -n "			1-1: " >> ./$i[^0-9]*/matchresult
		grep -c " 1 1" MatchResult.dat >> ./$i[^0-9]*/matchresult
	fi
	if [ -e ./$i[^0-9]*/F_udp_cs ]; then
		./exe/10ASM_F -r nids_cTos_udp -f ./$i[^0-9]*/F_udp_cs -n $i
		echo "	F_udp_cs:" >> ./$i[^0-9]*/matchresult
		echo -n "		total: " >> ./$i[^0-9]*/matchresult
		cat MatchResult.dat | wc -l  	 >> ./$i[^0-9]*/matchresult
		echo -n "			0-0: " >> ./$i[^0-9]*/matchresult
		grep -c " 0 0" MatchResult.dat >> ./$i[^0-9]*/matchresult
		echo -n "			0-1: " >> ./$i[^0-9]*/matchresult
		grep -c " 0 1" MatchResult.dat >> ./$i[^0-9]*/matchresult
		echo -n "			1-0: " >> ./$i[^0-9]*/matchresult
		grep -c " 1 0" MatchResult.dat >> ./$i[^0-9]*/matchresult
		echo -n "			1-1: " >> ./$i[^0-9]*/matchresult
		grep -c " 1 1" MatchResult.dat >> ./$i[^0-9]*/matchresult
	fi
	if [ -e ./$i[^0-9]*/F_udp_sc ]; then
		./exe/10ASM_F -r nids_sToc_udp -f ./$i[^0-9]*/F_udp_sc -n $i
		echo "	F_udp_sc:" >> ./$i[^0-9]*/matchresult
		echo -n "		total: " >> ./$i[^0-9]*/matchresult
		cat MatchResult.dat | wc -l  	 >> ./$i[^0-9]*/matchresult
		echo -n "			0-0: " >> ./$i[^0-9]*/matchresult
		grep -c " 0 0" MatchResult.dat >> ./$i[^0-9]*/matchresult
		echo -n "			0-1: " >> ./$i[^0-9]*/matchresult
		grep -c " 0 1" MatchResult.dat >> ./$i[^0-9]*/matchresult
		echo -n "			1-0: " >> ./$i[^0-9]*/matchresult
		grep -c " 1 0" MatchResult.dat >> ./$i[^0-9]*/matchresult
		echo -n "			1-1: " >> ./$i[^0-9]*/matchresult
		grep -c " 1 1" MatchResult.dat >> ./$i[^0-9]*/matchresult
	fi
done
