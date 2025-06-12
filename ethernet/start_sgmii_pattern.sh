port=1
mode=0
pat=0
machine="davinci"
while getopts p:m:v:b:h flag
do
    case "${flag}" in
		p) port=${OPTARG};;
		m) mode=${OPTARG};;
		v) pat=${OPTARG};;
		b) machine=${OPTARG};;
		h) echo "Usage: start_sgmii_pattern.sh [Options]
		Options:
			-p <PORT>: PHY port number, [0 .. 3], default: 1
			-m <MODE>: Test pattern mode, [0 .. 15], default: 0
			-v <PAT0>: Value of PAT0, [0 .. 1023], default: 0
			-b <MACHINE>: davinci or michelangelo
			-h : Print this information, default: false";;
    esac
done
if [ "$port" -lt 0 ]; then
	port=0
fi
if [ "$port" -gt 3 ]; then
	port=3
fi
if [ "$mode" -lt 0 ]; then
	mode=0
fi
if [ "$mode" -gt 15 ]; then
	mode=15
fi
if [ "$pat" -lt 0 ]; then
	pat=0
fi
if [ "$pat" -gt 1023 ]; then
	pat=1023
fi
echo "port: $port"
echo "mode: $mode"
echo "pat0: $pat"
echo "machine: $machine"

regtaddr=$((0x1071 + port * 0x100))
regraddr=$((0x1093 + port * 0x100))
regval=$((mode + pat * 0x20))

case $machine in 
	davinci)
		off_regxaddr=0x11e0240;;
	michelangelo)
		off_regxaddr=0xa060240;;
	*)
        echo "Error: Invalid machine type '$machine'. Please use 'davinci' or 'michelangelo'."
        exit 1
esac

devmem2 $(((off_regxaddr & mask) | 0x84)) w "$regraddr" > /dev/null
devmem2 $(((off_regxaddr & mask) | 0x88)) w "$regval" > /dev/null
devmem2 $(((off_regxaddr & mask) | 0x80)) w 3 > /dev/null
sleep 0.1
devmem2 $(((off_regxaddr & mask) | 0x84)) w "$regtaddr" > /dev/null
devmem2 $(((off_regxaddr & mask) | 0x88)) w "$regval" > /dev/null
devmem2 $(((off_regxaddr & mask) | 0x80)) w 3 > /dev/null
echo "Pattern generation initiated!"
