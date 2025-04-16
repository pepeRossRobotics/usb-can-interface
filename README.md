# usb-can-interface
Code to interface the USB-CAN Analyzer so that we can use it to develop and debug stuff

This code is heavily based on the work from https://github.com/kobolt/usb-can


## Instructions
For this to work we need to install Socket CAN
```
sudo apt-get update
sudo apt-get install can-utils
```

Then we need to create a virtual CAN interface
```
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
```

After this if you run `ifconfig` you should have this device:
```
vcan0: flags=193<UP,RUNNING,NOARP>  mtu 72
        unspec 00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00  txqueuelen 1000  (UNSPEC)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
```

Now compile the code with:
```
gcc -o tty_can_bridge tty_can_bridge.c
```

Run it with:
```
./tty_can_bridge /dev/ttyUSB0 vcan0
```

To check the output use candump. If you dont have it installed does it with:
```
sudo apt install can-utils
```

Run it with
```
candump vcan0
```
