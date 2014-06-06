for i in 1 2 3 4 5 6 7 8 9 10; do
	cp ./$i[^0-9]*/nids*/*_cTos_tcp ./$i"_cTos_tcp"
	cp ./$i[^0-9]*/nids*/*_sToc_tcp ./$i"_sToc_tcp"
	cp ./$i[^0-9]*/nids*/*_cTos_udp ./$i"_cTos_udp"
	cp ./$i[^0-9]*/nids*/*_sToc_udp ./$i"_sToc_udp"
done
cat [0-9]*_cTos_tcp > nids_cTos_tcp
cat [0-9]*_sToc_tcp > nids_sToc_tcp
cat [0-9]*_cTos_udp > nids_cTos_udp
cat [0-9]*_sToc_udp > nids_sToc_udp

rm -rf [0-9]*_*_*
for i in 1 2 3 4 5 6 7 8 9 10; do
	touch matchresult
	echo "Match Result" >> matchresult
	if [ -e ./$i[^0-9]*/F_tcp_cs ]; then
		./exe/8ASM_* -r nids_cTos_tcp -f ./$i[^0-9]*/F_tcp_cs -n $i
		echo "	F_tcp_cs:" >> matchresult
		echo -n "		total: " >> matchresult
		cat MatchResult.dat | wc -l  	 >> matchresult
		echo -n "			0-0: " >> matchresult
		grep -c " 0 0" MatchResult.dat >> matchresult
		echo -n "			0-1: " >> matchresult
		grep -c " 0 1" MatchResult.dat >> matchresult
		echo -n "			1-0: " >> matchresult
		grep -c " 1 0" MatchResult.dat >> matchresult
		echo -n "			1-1: " >> matchresult
		grep -c " 1 1" MatchResult.dat >> matchresult
	fi
	if [ -e ./$i[^0-9]*/F_tcp_sc ]; then
		./exe/8ASM_* -r nids_sToc_tcp -f ./$i[^0-9]*/F_tcp_sc -n $i
		echo "	F_tcp_sc:" >> matchresult
		echo -n "		total: " >> matchresult
		cat MatchResult.dat | wc -l  	 >> matchresult
		echo -n "			0-0: " >> matchresult
		grep -c " 0 0" MatchResult.dat >> matchresult
		echo -n "			0-1: " >> matchresult
		grep -c " 0 1" MatchResult.dat >> matchresult
		echo -n "			1-0: " >> matchresult
		grep -c " 1 0" MatchResult.dat >> matchresult
		echo -n "			1-1: " >> matchresult
		grep -c " 1 1" MatchResult.dat >> matchresult
	fi
	if [ -e ./$i[^0-9]*/F_udp_cs ]; then
		./exe/8ASM_* -r nids_cTos_udp -f ./$i[^0-9]*/F_udp_cs -n $i
		echo "	F_udp_cs:" >> matchresult
		echo -n "		total: " >> matchresult
		cat MatchResult.dat | wc -l  	 >> matchresult
		echo -n "			0-0: " >> matchresult
		grep -c " 0 0" MatchResult.dat >> matchresult
		echo -n "			0-1: " >> matchresult
		grep -c " 0 1" MatchResult.dat >> matchresult
		echo -n "			1-0: " >> matchresult
		grep -c " 1 0" MatchResult.dat >> matchresult
		echo -n "			1-1: " >> matchresult
		grep -c " 1 1" MatchResult.dat >> matchresult
	fi
	if [ -e ./$i[^0-9]*/F_udp_sc ]; then
		./exe/8ASM_* -r nids_sToc_udp -f ./$i[^0-9]*/F_udp_sc -n $i
		echo "	F_udp_sc:" >> matchresult
		echo -n "		total: " >> matchresult
		cat MatchResult.dat | wc -l  	 >> matchresult
		echo -n "			0-0: " >> matchresult
		grep -c " 0 0" MatchResult.dat >> matchresult
		echo -n "			0-1: " >> matchresult
		grep -c " 0 1" MatchResult.dat >> matchresult
		echo -n "			1-0: " >> matchresult
		grep -c " 1 0" MatchResult.dat >> matchresult
		echo -n "			1-1: " >> matchresult
		grep -c " 1 1" MatchResult.dat >> matchresult
	fi
	mv matchresult $i"matchresult"
done
