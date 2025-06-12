MMC_DEVICE=""
SD_CARD="/dev/mmcblk1"

insertions=0
removals=0
state=false

echo "Enter 0 - /dev/mmcblk0 (eMMC) or  1 /dev/mmcblk1 (SD card)"
read -p "Choice: " choice
case $choice in
  0)
    MMC_DEVICE=/dev/mmcblk0
    ;;
  1)
    MMC_DEVICE=/dev/mmcblk1
    ;;
  *)
    echo "Invalid choice. Exiting."
    exit 1
    ;;
esac

test1() {
  local partition="${MMC_DEVICE}p6"
  dd if=/dev/urandom of="${partition}" bs=1M count=1024 
  sync
  dd if="${partition}" of=/tmp/readback.bin bs=1M count=1024
}

test2() {
  local partition="${MMC_DEVICE}p6"
  exec_time=$( { time dd if=/dev/urandom of="${partition}" bs=1M count=1024 status=none; } 2>&1 | awk '/real/ {print $2}' )
  elapsed_time=$(echo "$exec_time" | awk -F'[ms]' '{print $1 * 60 + $2}')
  file_size_mb=1024
  throughput=$(echo "$file_size_mb $elapsed_time" | awk '{print $1 / $2}')
  echo "Throughput: $throughput MB/s"
}

test3() {
  dd if=/dev/urandom of="${MMC_DEVICE}" bs=1M count=1024 
  sync
  dd if="${MMC_DEVICE}" of=/tmp/readback.bin bs=1M count=1024
}

test4() {
  while true; do
    if [ -e "$SD_CARD" ]; then
      if [ "$state" = false ]; then
        ((insertions++))
        echo "SD Card is inserted"
        echo " Insertions: $insertions and Removals: $removals"
      fi
      state=true  
    else
      if [ "$state" = true ]; then
        ((removals++))
        echo "SD Card is removed"
        echo " Insertions: $insertions and Removals: $removals"
      fi
      state=false
    fi

    if [ "$insertions" -eq 3 ] && [ "$removals" -eq 3 ]; then
      echo "Test passed"
      break
    fi

    sleep 2

done
}

echo "1) Test 1: Memory Operations"
echo "2) Test 2: Throughput Measurement"
echo "3) Test 3: Whole memory read and write"
echo "4) Test 4: sd_card insertion and connection back"
read -p "Enter your choice (1 or 2 or 3 or 4): " test_choice

case $test_choice in
  1)
    test1
    ;;
  2)
    test2
    ;;
  3)
    test3
    ;;
  4)
    test4
    ;;
  *)
    echo "Invalid choice. Exiting."
    exit 1
    ;;
esac
