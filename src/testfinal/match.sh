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
rm -rf matchresult
rm -rf [0-9]*_*_*
for i in 1 2 3 4 5 6 7 8 9 10; do
	echo -n $i
	touch matchresult
	echo "Match Result" >> matchresult
	if [ -e ./$i[^0-9]*/F_tcp_cs ]; then
		./exe/8ASM_* -r nids_cTos_tcp -f ./$i[^0-9]*/F_tcp_cs -n $i
		echo "	F_tcp_cs:" >> matchresult

		echo -n "		total: " >> matchresult
		Total=`cat MatchResult.dat | wc -l`
		echo " $Total	" >> matchresult
		echo -n "			0-0: " >> matchresult
		Num00=`grep -c " 0 0" MatchResult.dat`
		echo " $Num00	" >> matchresult
		echo -n "			0-1: " >> matchresult
		Num01=`grep -c " 0 1" MatchResult.dat`
		echo " $Num01	" >> matchresult
		echo -n "			1-0: " >> matchresult
		Num10=`grep -c " 1 0" MatchResult.dat`
		echo " $Num10	" >> matchresult
		echo -n "			1-1: " >> matchresult
		Num11=`grep -c " 1 1" MatchResult.dat`
		echo " $Num11	" >> matchresult
		echo " " >> matchresult
		echo -n "			righteous rate: " >> matchresult
		echo "scale=4; ($Num11+$Num00)/$Total" | bc -l >> matchresult
		echo -n "			precision rate: " >> matchresult
		echo "scale=4; $Num11/($Num11+$Num01)" | bc -l >> matchresult
		echo -n "			recall rate: " >> matchresult
		echo "scale=4; $Num11/($Num11+$Num10)" | bc -l >> matchresult
	fi
	if [ -e ./$i[^0-9]*/F_tcp_sc ]; then
		./exe/8ASM_* -r nids_sToc_tcp -f ./$i[^0-9]*/F_tcp_sc -n $i
		echo "	F_tcp_sc:" >> matchresult
		
		echo -n "		total: " >> matchresult
		Total=`cat MatchResult.dat | wc -l`
		echo " $Total	" >> matchresult
		echo -n "			0-0: " >> matchresult
		Num00=`grep -c " 0 0" MatchResult.dat`
		echo " $Num00	" >> matchresult
		echo -n "			0-1: " >> matchresult
		Num01=`grep -c " 0 1" MatchResult.dat`
		echo " $Num01	" >> matchresult
		echo -n "			1-0: " >> matchresult
		Num10=`grep -c " 1 0" MatchResult.dat`
		echo " $Num10	" >> matchresult
		echo -n "			1-1: " >> matchresult
		Num11=`grep -c " 1 1" MatchResult.dat`
		echo " $Num11	" >> matchresult
		echo " " >> matchresult
		echo -n "			righteous rate: " >> matchresult
		echo "scale=4; ($Num11+$Num00)/$Total" | bc -l >> matchresult
		echo -n "			precision rate: " >> matchresult
		echo "scale=4; $Num11/($Num11+$Num01)" | bc -l >> matchresult
		echo -n "			recall rate: " >> matchresult
		echo "scale=4; $Num11/($Num11+$Num10)" | bc -l >> matchresult

	fi
	if [ -e ./$i[^0-9]*/F_udp_cs ]; then
		./exe/8ASM_* -r nids_cTos_udp -f ./$i[^0-9]*/F_udp_cs -n $i
		echo "	F_udp_cs:" >> matchresult
		
		echo -n "		total: " >> matchresult
		Total=`cat MatchResult.dat | wc -l`
		echo " $Total	" >> matchresult
		echo -n "			0-0: " >> matchresult
		Num00=`grep -c " 0 0" MatchResult.dat`
		echo " $Num00	" >> matchresult
		echo -n "			0-1: " >> matchresult
		Num01=`grep -c " 0 1" MatchResult.dat`
		echo " $Num01	" >> matchresult
		echo -n "			1-0: " >> matchresult
		Num10=`grep -c " 1 0" MatchResult.dat`
		echo " $Num10	" >> matchresult
		echo -n "			1-1: " >> matchresult
		Num11=`grep -c " 1 1" MatchResult.dat`
		echo " $Num11	" >> matchresult
		echo " " >> matchresult
		echo -n "			righteous rate: " >> matchresult
		echo "scale=4; ($Num11+$Num00)/$Total" | bc -l >> matchresult
		echo -n "			precision rate: " >> matchresult
		echo "scale=4; $Num11/($Num11+$Num01)" | bc -l >> matchresult
		echo -n "			recall rate: " >> matchresult
		echo "scale=4; $Num11/($Num11+$Num10)" | bc -l >> matchresult

	fi
	if [ -e ./$i[^0-9]*/F_udp_sc ]; then
		./exe/8ASM_* -r nids_sToc_udp -f ./$i[^0-9]*/F_udp_sc -n $i
		echo "	F_udp_sc:" >> matchresult
			
		echo -n "		total: " >> matchresult
		Total=`cat MatchResult.dat | wc -l`
		echo " $Total	" >> matchresult
		echo -n "			0-0: " >> matchresult
		Num00=`grep -c " 0 0" MatchResult.dat`
		echo " $Num00	" >> matchresult
		echo -n "			0-1: " >> matchresult
		Num01=`grep -c " 0 1" MatchResult.dat`
		echo " $Num01	" >> matchresult
		echo -n "			1-0: " >> matchresult
		Num10=`grep -c " 1 0" MatchResult.dat`
		echo " $Num10	" >> matchresult
		echo -n "			1-1: " >> matchresult
		Num11=`grep -c " 1 1" MatchResult.dat`
		echo " $Num11	" >> matchresult
		echo " " >> matchresult
		echo -n "			righteous rate: " >> matchresult
		echo "scale=4; ($Num11+$Num00)/$Total" | bc -l >> matchresult
		echo -n "			precision rate: " >> matchresult
		echo "scale=4; $Num11/($Num11+$Num01)" | bc -l >> matchresult
		echo -n "			recall rate: " >> matchresult
		echo "scale=4; $Num11/($Num11+$Num10)" | bc -l >> matchresult

	fi
# Packetlevel
	echo " " >> matchresult
	if [ -e ./$i[^0-9]*/P_tcp_cs ]; then
		./exe/11ASM_* -r nids_cTos_tcp -f ./$i[^0-9]*/P_tcp_cs -n $i
		echo "	P_tcp_cs:" >> matchresult
			
		echo -n "		total: " >> matchresult
		Total=`cat MatchResult.dat | wc -l`
		echo " $Total	" >> matchresult
		echo -n "			0-0: " >> matchresult
		Num00=`grep -c " 0 0" MatchResult.dat`
		echo " $Num00	" >> matchresult
		echo -n "			0-1: " >> matchresult
		Num01=`grep -c " 0 1" MatchResult.dat`
		echo " $Num01	" >> matchresult
		echo -n "			1-0: " >> matchresult
		Num10=`grep -c " 1 0" MatchResult.dat`
		echo " $Num10	" >> matchresult
		echo -n "			1-1: " >> matchresult
		Num11=`grep -c " 1 1" MatchResult.dat`
		echo " $Num11	" >> matchresult
		echo " " >> matchresult
		echo -n "			righteous rate: " >> matchresult
		echo "scale=4; ($Num11+$Num00)/$Total" | bc -l >> matchresult
		echo -n "			precision rate: " >> matchresult
		echo "scale=4; $Num11/($Num11+$Num01)" | bc -l >> matchresult
		echo -n "			recall rate: " >> matchresult
		echo "scale=4; $Num11/($Num11+$Num10)" | bc -l >> matchresult

	fi
	if [ -e ./$i[^0-9]*/P_tcp_sc ]; then
		./exe/11ASM_* -r nids_sToc_tcp -f ./$i[^0-9]*/P_tcp_sc -n $i
		echo "	P_tcp_sc:" >> matchresult
			
		echo -n "		total: " >> matchresult
		Total=`cat MatchResult.dat | wc -l`
		echo " $Total	" >> matchresult
		echo -n "			0-0: " >> matchresult
		Num00=`grep -c " 0 0" MatchResult.dat`
		echo " $Num00	" >> matchresult
		echo -n "			0-1: " >> matchresult
		Num01=`grep -c " 0 1" MatchResult.dat`
		echo " $Num01	" >> matchresult
		echo -n "			1-0: " >> matchresult
		Num10=`grep -c " 1 0" MatchResult.dat`
		echo " $Num10	" >> matchresult
		echo -n "			1-1: " >> matchresult
		Num11=`grep -c " 1 1" MatchResult.dat`
		echo " $Num11	" >> matchresult
		echo " " >> matchresult
		echo -n "			righteous rate: " >> matchresult
		echo "scale=4; ($Num11+$Num00)/$Total" | bc -l >> matchresult
		echo -n "			precision rate: " >> matchresult
		echo "scale=4; $Num11/($Num11+$Num01)" | bc -l >> matchresult
		echo -n "			recall rate: " >> matchresult
		echo "scale=4; $Num11/($Num11+$Num10)" | bc -l >> matchresult
	
	
	fi
	if [ -e ./$i[^0-9]*/P_udp_cs ]; then
		./exe/11ASM_* -r nids_cTos_udp -f ./$i[^0-9]*/P_udp_cs -n $i
		echo "	P_udp_cs:" >> matchresult
			
		echo -n "		total: " >> matchresult
		Total=`cat MatchResult.dat | wc -l`
		echo " $Total	" >> matchresult
		echo -n "			0-0: " >> matchresult
		Num00=`grep -c " 0 0" MatchResult.dat`
		echo " $Num00	" >> matchresult
		echo -n "			0-1: " >> matchresult
		Num01=`grep -c " 0 1" MatchResult.dat`
		echo " $Num01	" >> matchresult
		echo -n "			1-0: " >> matchresult
		Num10=`grep -c " 1 0" MatchResult.dat`
		echo " $Num10	" >> matchresult
		echo -n "			1-1: " >> matchresult
		Num11=`grep -c " 1 1" MatchResult.dat`
		echo " $Num11	" >> matchresult
		echo " " >> matchresult
		echo -n "			righteous rate: " >> matchresult
		echo "scale=4; ($Num11+$Num00)/$Total" | bc -l >> matchresult
		echo -n "			precision rate: " >> matchresult
		echo "scale=4; $Num11/($Num11+$Num01)" | bc -l >> matchresult
		echo -n "			recall rate: " >> matchresult
		echo "scale=4; $Num11/($Num11+$Num10)" | bc -l >> matchresult

	fi
	if [ -e ./$i[^0-9]*/P_udp_sc ]; then
		./exe/11ASM_* -r nids_sToc_udp -f ./$i[^0-9]*/P_udp_sc -n $i
		echo "	P_udp_sc:" >> matchresult
			
		echo -n "		total: " >> matchresult
		Total=`cat MatchResult.dat | wc -l`
		echo " $Total	" >> matchresult
		echo -n "			0-0: " >> matchresult
		Num00=`grep -c " 0 0" MatchResult.dat`
		echo " $Num00	" >> matchresult
		echo -n "			0-1: " >> matchresult
		Num01=`grep -c " 0 1" MatchResult.dat`
		echo " $Num01	" >> matchresult
		echo -n "			1-0: " >> matchresult
		Num10=`grep -c " 1 0" MatchResult.dat`
		echo " $Num10	" >> matchresult
		echo -n "			1-1: " >> matchresult
		Num11=`grep -c " 1 1" MatchResult.dat`
		echo " $Num11	" >> matchresult
		echo " " >> matchresult
		echo -n "			righteous rate: " >> matchresult
		echo "scale=4; ($Num11+$Num00)/$Total" | bc -l >> matchresult
		echo -n "			precision rate: " >> matchresult
		echo "scale=4; $Num11/($Num11+$Num01)" | bc -l >> matchresult
		echo -n "			recall rate: " >> matchresult
		echo "scale=4; $Num11/($Num11+$Num10)" | bc -l >> matchresult

	fi
	echo '1'
	mv matchresult $i"matchresult"
	echo '2'
	rm -rf $i[^0-9]*/*matchresult
			  echo '3'
	cp $i"matchresult" $i[^0-9]*/
	echo '4'
done
mkdir matchresult
mv ?*matchresult matchresult
