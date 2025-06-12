port=1
loopback=0
machine="davinci";
while getopts p:l:m:h flag
do
    case "${flag}" in
        p) port=${OPTARG};;
        l) loopback=${OPTARG};;
        m) machine=${OPTARG};;
        h) echo "Usage: set_sgmii_rxtx_loopback.sh [Options]
        Options:
                -p <PORT>: PHY port number, [0 .. 3], default: 1
                -l <VAL> : 1 to enable loopback mode, 0 to disable loopback mode, default: 0
                -m <MACHINE> : davinci or michelangelo
                -h : Print this information, default: false";;
    esac
done
if [ "$port" -lt 0 ]; then
    port=0
fi
if [ "$port" -gt 3 ]; then
    port=3
fi
if [ "$loopback" -lt 0 ]; then
    loopback=0
fi
if [ "$loopback" -gt 1 ]; then
    loopback=1
fi
echo "port: $port"
echo "loopback: $loopback"
echo "machine: $machine"

case $machine in
	davinci)
		off_regxaddr=0x11e0240
		off_regval=0x5100
		;;
	michelangelo)
		off_regxaddr=0xa060240
		off_regval=0x4000
		;;
    *)
        echo "Error: Invalid machine type '$machine'. Please use 'davinci' or 'michelangelo'."
        exit 1
esac

mask=$((0xFFFFFF00))
regxaddr=$((off_regxaddr + port * 0x0200000))
regaddr=$((0x1000 + port * 0x100))
regval=$((off_regval + loopback * 0x10))
devmem2 "$regxaddr" w "$regval" > /dev/null
sleep 0.1
regval=$((loopback * 0x6))
devmem2 $(((off_regxaddr & mask) | 0x84)) w "$regaddr" > /dev/null
devmem2 $(((off_regxaddr & mask) | 0x88)) w "$regval" > /dev/null
devmem2 $(((off_regxaddr & mask) | 0x80)) w 3 > /dev/null
echo "Set SGMII RX to TX loopback!"
