#!/bin/bash

while true; do
    # 1. Initialize empty arrays on every loop refresh
    mac_addresses=()
    device_names=()

    # 2. Populate the arrays
    while read -r mac name; do
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
            mac_addresses+=("$mac")
            device_names+=("$name")
        fi
    done < <(bluetoothctl devices | awk '{print $2, substr($0, index($0,$3))}')

    # Check if any devices were found
    if [ ${#mac_addresses[@]} -eq 0 ]; then
        echo "No Bluetooth game controller devices found."
        exit 1
    fi

    # 3. Display the numbered menu
    echo -e "\n========================================"
    echo "Select a Bluetooth game controller device to view info:"
    echo "========================================"
    for i in "${!mac_addresses[@]}"; do
        echo "[$((i + 1))] ${device_names[$i]}"
    done
    echo "[q] Quit script"
    echo "----------------------------------------"

    # 4. Prompt for user input
    read -p "Enter selection: " choice

    # Exit condition
    if [[ "$choice" == "q" || "$choice" == "Q" ]]; then
        echo "Exiting."
        break
    fi

    # 5. Validate input and extract specific device info
    if [[ "$choice" =~ ^[0-9]+$ ]] && [ "$choice" -ge 1 ] && [ "$choice" -le "${#mac_addresses[@]}" ]; then
        target_index=$((choice - 1))
        target_mac="${mac_addresses[$target_index]}"
        target_name="${device_names[$target_index]}"

        # Query bluetoothctl info once and save it to a variable
        raw_info=$(bluetoothctl info "$target_mac")

        # --- Strict Paired Logic ---
        # Checks if Paired, Bonded, and Trusted are ALL set to "yes"
        if echo "$raw_info" | grep -q "Paired: yes" && \
           echo "$raw_info" | grep -q "Bonded: yes" && \
           echo "$raw_info" | grep -q "Trusted: yes"; then
            paired_status="yes"
        else
            paired_status="no"
        fi
        
        # Extract 'Class' property
        device_class=$(echo "$raw_info" | grep -i "Class:" | awk '{print $2}')
        if [ -z "$device_class" ]; then
            device_class="Unknown"
        fi

        # 6. Print the formatted output exactly as requested
        echo -e "\n----------------------------------------"
        echo "Name: $target_name"
        echo "MAC address: $target_mac"
        echo "Paired?: $paired_status"
        echo "Class: $device_class"
        echo "----------------------------------------"
        
        # Pause so the user can read the info before the menu refreshes
        read -n 1 -s -r -p "Press any key to return to the menu..."
        echo ""
    else
        echo -e "\n[!] Invalid selection. Please choose a valid number or 'q'."
        sleep 1.5
    fi
done
