port=1
mode=0
pat=0
while getopts p:m:v:h flag
do
    case "${flag}" in
        p) port=${OPTARG};;
        m) mode=${OPTARG};;
        v) pat=${OPTARG};;
	h) echo "Usage: start_sgmii_pattern.sh [Options]
	Options:
		-p <PORT>: PHY port number, [0 .. 3], default: 1
		-m <MODE>: Test pattern mode, [0 .. 15], default: 0
		-v <PAT0>: Value of PAT0, [0 .. 1023], default: 0
		-h : Print this information, default: false";;
    esac
done
if ((port < 0)); then
	port=0
fi
if ((port > 3)); then
	port=3
fi
if ((mode < 0)); then
        mode=0
fi
if ((mode > 15)); then
        mode=15
fi
if ((pat < 0)); then
        pat=0
fi
if ((pat > 1023)); then
        pat=1023
fi
echo "port: $port"
echo "mode: $mode"
echo "pat0: $pat"
regtaddr=$((0x102b + port*0x100))
regraddr=$((0x1051 + port*0x100))
regval=$((mode + pat*0x20))
devmem2 0x11e0284 w $regraddr > /dev/null
devmem2 0x11e0288 w $regval > /dev/null
devmem2 0x11e0280 w 3 > /dev/null
sleep 0.1
devmem2 0x11e0284 w $regtaddr > /dev/null
devmem2 0x11e0288 w $regval > /dev/null
devmem2 0x11e0280 w 3 > /dev/null
echo "Pattern generation initiated!"
