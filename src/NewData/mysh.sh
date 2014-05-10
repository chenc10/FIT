cd ..
cd ..
make
cd src
cp -f TPTD ./NewData
cd NewData
rm -rf nids_*
./TPTD -r *thunder*.pcap 2>result
