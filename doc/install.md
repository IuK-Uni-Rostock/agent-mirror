# Installation on Raspberry Pi (any model)

## 1. Configure Raspberry Pi
1. Open `/boot/cmdline.txt` and remove `console=ttyAMA0,115200` from the file. For Raspbian Jessie and newer the entry is `console=serial0,115200`
2. To fix the baud rate, execute this command (only required on Raspberry Pi 3): `sudo sh -c "echo dtoverlay=pi3-miniuart-bt >>/boot/config.txt"`
3. Reboot `sudo reboot now`

## 2. Download agent source
1. Switch to home directory `cd ~`
2. Download agent `git clone https://git.informatik.uni-rostock.de/iuk/security-projects/software/building-automation/agent.git`

## 3. Make KNX USB devices accessible for non-root users
1. Add group KNX `sudo groupadd --system knx`
2. Add user to group `sudo gpasswd -a pi knx` (all users in group KNX will have access to KNX USB devices)
3. Copy udev-rules `sudo cp agent/lib/kdriveExpress-17.2.0-raspbian/90-knxusb.rules /etc/udev/rules.d/`
4. Reload udev rules or reboot `sudo reboot now`

## 4. Install dependencies
1. Install dependencies `sudo apt install build-essential libsqlite3-dev`
2. Switch to home directory `cd ~`
3. Install kdriveExpress `sudo cp agent/lib/kdriveExpress-17.2.0-raspbian/raspbian/libkdriveExpress.so /usr/local/lib/`
4. Reload shared libraries `sudo ldconfig`

## 5. Compile
1. Switch directory `cd agent/src`
2. Compile `make`

## 6. Install agent
1. `sudo cp agent /usr/local/bin` (it might be necessary to create the directory first)
2. Agent can now be run from any working directory, simply execute `agent`
