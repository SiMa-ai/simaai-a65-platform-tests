for c in {0..3}
do
   devmem2 0x58$((c))0320 w 0x0
   devmem2 0x58$((c))0074 w 0x303
   devmem2 0x58$((c))0320 w 0x1
   sleep 1
done

for i in {1..86400}
do
   echo "Iteration $i"

   for c in {0..3}
   do
      devmem2 0x$((c+1))60000000 w 0xAAAAAAA8
      devmem2 0x$((c))80000000 w
   done

   sleep 1
done

for c in {0..3}
do
   ce=$(cat /sys/devices/system/edac/mc/mc$c/ce_count)
   ue=$(cat /sys/devices/system/edac/mc/mc$c/ue_count)
   echo "DDRC$c report CE: $ce UE: $ue"
done

echo "Total errors report"
edac-util --report=ce
edac-util --report=ue
