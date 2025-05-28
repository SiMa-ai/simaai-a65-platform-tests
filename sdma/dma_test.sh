modprobe dmatest channel=dma0chan0 timeout=2000 iterations=1 run=1

modprobe dmatest

echo dma0chan0 > /sys/module/dmatest/parameters/channel
echo 2000 > /sys/module/dmatest/parameters/timeout
echo 2 > /sys/module/dmatest/parameters/iterations
echo 4 > /sys/module/dmatest/parameters/alignment
echo 1 > /sys/module/dmatest/parameters/run


sleep 2

dmesg_output=$(dmesg | grep -i 'dmatest')

if echo "$dmesg_output" | grep -q 'data error\|failures'; then
    echo "DMATest for dma0chan0 failed. Errors detected:"
    echo "$dmesg_output" | grep 'not copied!\|failures'
else
    echo "DMATest for dma0chan0 passed."
fi

echo dma1chan0 > /sys/module/dmatest/parameters/channel
echo 2000 > /sys/module/dmatest/parameters/timeout
echo 2 > /sys/module/dmatest/parameters/iterations
echo 4 > /sys/module/dmatest/parameters/alignmentss
echo 1 > /sys/module/dmatest/parameters/run

sleep 2

dmesg_output=$(dmesg | grep -i 'dmatest')

if echo "$dmesg_output" | grep -q 'data error\|failures'; then
    echo "DMATest for dma1chan0 failed. Errors detected:"
    echo "$dmesg_output" | grep 'not copied!\|failures'
else
    echo "DMATest for dma1chan0 passed."
fi