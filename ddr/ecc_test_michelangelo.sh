devmem2 0x10010c80 w 0x0
devmem2 0x10010604 w 0x303
devmem2 0x10010c80 w 0x1
sleep 1

for i in {1..86400}
do
   echo "Iteration $i"
   devmem2 0x1e00000000 w 0xAAAAAAA8
   devmem2 0x1000000000 w
   sleep 1
done

ce=$(cat /sys/devices/system/edac/mc/mc0/ce_count)
ue=$(cat /sys/devices/system/edac/mc/mc0/ue_count)
echo "DDRC:0 report CE: $ce UE: $ue"

echo "Total errors report"
edac-util --report=ce
edac-util --report=ue
