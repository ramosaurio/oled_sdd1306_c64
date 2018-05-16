#!/bin/bash
echo SDD1306_i2c 0x3c | sudo tee /sys/class/i2c-adapter/i2c-1/new_device
dmesg
