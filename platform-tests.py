import subprocess
import re

def run_command(command, test_number, test_type, check_bytes=False):
    try:
        result = subprocess.run(command, shell=True, text=True, capture_output=True)
        output = result.stdout + result.stderr

        if check_bytes:
            match = re.search(r'Total bytes\s*:\s*(\d+)', output)
            if match:
                bytes_written = match.group(1)
                print(f"{test_type} Test {test_number} Passed - Total bytes: {bytes_written}")
            else:
                print(f"{test_type} Test {test_number}: Failed")
        else:
            if "Pattern Loaded" in output:
                print(f"{test_type} Test {test_number}: Passed")
            else:
                print(f"{test_type} Test {test_number}: Failed")
        
    except subprocess.CalledProcessError as e:
        print(f"{test_type} Test {test_number}: Failed")
        print("Error output:\n", e.stderr)

def run_script(choice, test): 
    cmd = f'echo -e "{choice}\n{test}" | /usr/bin/simaai_pt/emmc_sd_test.sh'
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)

    if result.returncode == 0:
        output = result.stdout
        if test == "2":
            throughput_match = re.search(r"Throughput: \s*([\d.]+)\s*MB/s", output)
            if throughput_match:
                throughput = throughput_match.group(1)
                print(f"Device {choice} Test {test}: Number of bytes written in 60 seconds: {throughput} MB/s")
            else:
                print(f"Device {choice} Test {test}: Failed")
        else:
            print(f"Device {choice} Test {test}: Passed")
    else:
        print(f"Device {choice} Test {test}: Failed")

def run_sdma_test():
    command = "/usr/bin/simaai_pt/dma_test.sh"
    result = subprocess.run(command, shell=True, capture_output=True, text=True)
    
    if "summary" in result.stdout and "0 failures" in result.stdout:
        print("SDMA Test: Passed")
    else:
        print("SDMA Test: Failed")

def main():
    
    ocm_commands = [
        "/usr/bin/simaai_pt/ddr_test -d 0x10 -p 9 -b -s 0x800000",  # Test 1
        "/usr/bin/simaai_pt/ddr_test -d 0x10 -p 10 -b -s 0x800000", # Test 2
        "/usr/bin/simaai_pt/ddr_test -d 0x10 -r -b -s 0x800000",    # Test 3
        "/usr/bin/simaai_pt/ddr_test -d 0x10 -p 3 -b -s 0x800000",  # Test 4
        "/usr/bin/simaai_pt/ddr_test -d 0x10 -p 11 -b -s 0x800000", # Test 5
        "/usr/bin/simaai_pt/ddr_test -d 0x10 -f -t 60"              # Test 6 (Check Total Bytes)
    ]

    ddr_commands = [
        "/usr/bin/simaai_pt/ddr_test -d 0x8 -p 9 -b",                # DDR Test 1
        "/usr/bin/simaai_pt/ddr_test -d 0x8 -p 10 -b",               # DDR Test 2
        "/usr/bin/simaai_pt/ddr_test -d 0x8 -r -b",                  # DDR Test 3
        "/usr/bin/simaai_pt/ddr_test -d 0x8 -p 11 -b",               # DDR Test 4
        "/usr/bin/simaai_pt/ddr_test -d 0x8 -f -t 60"                # DDR Test 5 (Check Total Bytes)
    ]

    print("Tests for OCM:")
    for i, command in enumerate(ocm_commands, start=1):
        run_command(command, i, "OCM", check_bytes=True if i == 6 else False)
    
    print("Tests for DDR:")
    for i, command in enumerate(ddr_commands, start=1):
        run_command(command, i, "DDR", check_bytes=True if i == 5 else False)

    eMMC_tests = ["1", "2", "3"]
    SD_card_tests = ["1", "2", "3"]

    print("Tests for eMMC:")
    for test in eMMC_tests:
        run_script("0", test)

    print("Tests for SD Card:")
    for test in SD_card_tests:
        run_script("1", test)
    
    print("Test for SDMA:")
    run_sdma_test()

if __name__ == "__main__":
    main()
