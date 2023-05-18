port=1
loopback=0
while getopts p:l:h flag
do
    case "${flag}" in
        p) port=${OPTARG};;
        l) loopback=${OPTARG};;
        h) echo "Usage: set_sgmii_rxtx_loopback.sh [Options]
        Options:
                -p <PORT>: PHY port number, [0 .. 3], default: 1
                -l <VAL> : 1 to enable loopback mode, 0 to disable loopback mode, default: 0
                -h : Print this information, default: false";;
    esac
done
if ((port < 0)); then
    port=0
fi
if ((port > 3)); then
    port=3
fi
if ((loopback < 0)); then
    loopback=0
fi
if ((loopback > 1)); then
    loopback=1
fi
echo "port: $port"
echo "loopback: $loopback"
regxaddr=$((0x11e0240 + port*0x0200000))
regaddr=$((0x1000 + port*0x100))
regval=$((0x5100 + loopback*0x10))
devmem2 $regxaddr w $regval > /dev/null
sleep 0.1
regval=$((loopback*0x6))
devmem2 0x11e0284 w $regaddr > /dev/null
devmem2 0x11e0288 w $regval > /dev/null
devmem2 0x11e0280 w 3 > /dev/null
echo "Set SGMII RX to TX loopback!"
