#/bin/bash
bluetoothctl list | grep -Eio "([0-9a-f]{2}[:]){5}[0-9a-f]{2}"
