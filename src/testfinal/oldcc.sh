for i in 1 2 3 4 5 6 7 8 9 10; do
	rm -rf $i[^0-9]*/matchresult
	cp matchresult/$i"matchresult" $i[^0-9]*
	echo "ok"
	echo "$i"matchresult""
	
done
	
