#!/bin/bash

# 1. Fetch all paired Bluetooth MAC addresses
paired_devices=$(bluetoothctl devices | awk '{print $2}')

if [ -z "$paired_devices" ]; then
    echo "No paired Bluetooth devices found."
    exit 0
fi

echo "Scanning paired devices for Game Controllers..."
echo "--------------------------------------------------------"

# 2. Iterate through each paired device
for mac in $paired_devices; do
    # Fetch device details safely
    info=$(bluetoothctl info "$mac")
    name=$(echo "$info" | grep "Name:" | cut -d' ' -f2-)
    class_hex=$(echo "$info" | grep "Class:" | awk '{print $2}')

    # If the device has no declared Class ID, skip it
    if [ -z "$class_hex" ]; then
        continue
    fi

    # 3. Convert Hex Class ID to Decimal Integer for bitwise manipulation
    class_dec=$((class_hex))

    # 4. Deconstruct the architecture using Bash Bitwise Operators
    # Service Class: Bits 23 to 13
    service_class=$(( (class_dec >> 13) & 0x7FF ))
    
    # Major Class: Bits 12 to 8
    major_class=$(( (class_dec >> 8) & 0x1F ))
    
    # Minor Class: Bits 7 to 2
    minor_class=$(( (class_dec >> 2) & 0x3F ))

    # 5. Evaluate if the signatures point to a gamepad
    # Major 5 = Peripheral | Minor 2 = Gamepad 
    if [ "$major_class" -eq 5 ] && [ "$minor_class" -eq 2 ]; then
        # Check if the controller is currently active/online
        if echo "$info" | grep -q "Connected: yes"; then
            status="🟢 CONNECTED"
        else
            status="⚫ DISCONNECTED"
        fi
        
        # Check for Limited Discoverable Mode (Bit 13 of the entire CoD)
        lim_disc=$(( (class_dec >> 13) & 1 ))
        
        echo -e "Device Found: $name [$mac]"
        echo -e "  Status:     $status"
        echo -e "  Class ID:   $class_hex"
        echo -e "  Decoded:    Major Class: $major_class (Peripheral), Minor Class: $minor_class (Gamepad)"
        if [ "$lim_disc" -eq 1 ]; then
            echo -e "  Features:   Limited Discoverable Mode Flag is active."
        fi
        echo "--------------------------------------------------------"
    fi
done
