.PHONY: all test clean

all: cpuburn

# For Raspberry Pi 4
#CFLAGS=-O3 -march=armv8-a+fp+simd -mtune=cortex-a72 

# For Sima.ai Davinci board
CFLAGS=-O3 -march=armv8.2-a+fp+simd -mtune=cortex-a65 

# For old Raspberry Pi
# CFLAGS=-O3 -march=armv8-a -mtune=cortex-a53

# Uncomment to download the source code
#cpuburn-a53.S:
#	wget https://raw.githubusercontent.com/ssvb/cpuburn-arm/master/cpuburn-a53.S 
	
cpuburn: cpuburn-a65.S
	${CC} ${CFLAGS} -o $@ $^ ${LDFLAGS}
	#${STRIP} $@

# define dump_values = 
# 	while true; do 
# 		vcgencmd measure_clock arm
# 		vcgencmd measure_temp
# 		vcgencmd get_throttled
# 		sleep 10
# 	done
# endef

# throttling should happen at 80C
# see https://forums.raspberrypi.com/viewtopic.php?t=220061
#
# 1010000000000000101  <- 19 bits  rpi 3b+
# |||             |||_ under-voltage
# |||             ||_ currently throttled
# |||             |_ arm frequency capped
# |||_ under-voltage has occurred since last reboot
# ||_ throttling has occurred since last reboot
# |_ arm frequency capped has occurred since last reboot

test: cpuburn
	(trap 'kill 0' INT; ./monitor_pi.sh & ./cpuburn)

clean:
	rm -f cpuburn

# .ONESHELL:
