#!/usr/bin/env python3
"""Wipe Arduino Nano memory via Serial (Soft reset & buffer clear)."""

import serial
import time
import sys

def wipe_port(port, baud=9600):
    print(f"Connecting to {port} at {baud} baud...")
    try:
        with serial.Serial(port, baud, timeout=1) as ser:
            time.sleep(2)  # Wait for Arduino reset
            
            # Send a command to overwrite EEPROM if you have a specific sketch for that,
            # but via standard Serial, we can only flush buffers and restart.
            
            # Hard reset via DTR/RTS toggle
            ser.dtr = False
            ser.rts = False
            time.sleep(0.2)
            ser.dtr = True
            ser.rts = True
            time.sleep(1)
            
            # Flood input buffer with junk to trigger any potential overflow resets
            # and clear any stuck state
            ser.write(b"\x00" * 256)
            time.sleep(0.5)
            
            # Flush everything
            ser.reset_input_buffer()
            ser.reset_output_buffer()
            
            print("Memory/Buffer flushed and controller reset.")
            print("To fully erase Flash memory, you need to upload an 'Empty Sketch' via Arduino IDE.")
            
    except serial.SerialException as e:
        print(f"Error: {e}")
        print("Make sure the port is correct and no other program (like Arduino IDE) is using it.")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        port = sys.argv[1]
    else:
        port = "COM4"  # Default port for Windows
        
    wipe_port(port)
