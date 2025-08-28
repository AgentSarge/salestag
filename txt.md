# 🚀 Automated Build & Flash (RECOMMENDED)
./build_and_flash.sh

# OR Manual Steps Below:

# 1. Navigate to project
cd /Users/self/Desktop/salestag/new_componet/softwareV3

# 2. Set up environment
source /Users/self/esp/esp-idf/export.sh

# 3. Build (clean first if needed)
idf.py clean
idf.py fullclean # Optional but recommended
idf.py build

# 4. Flash
idf.py flash --port /dev/cu.usbmodem101

# 5. Monitor (optional)
idf.py monitor --port /dev/cu.usbmodem101
