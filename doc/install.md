# Installation

## Configure Raspberry Pi
1. Open `/boot/cmdline.txt` and remove `console=ttyAMA0,115200` from the file. For Raspbian Jessie and newer the entry is `console=serial0,115200`
2. To fix the baud rate, execute this command (only required on Raspberry Pi 3): `sudo sh -c "echo dtoverlay=pi3-miniuart-bt >>/boot/config.txt"`
3. Reboot `sudo reboot now`

## Download agent source
1. Switch to home directory `cd ~`
2. Download agent `git clone https://git.informatik.uni-rostock.de/mj244/sindabus-agent.git`

## Install dependencies
1. Install dependencies `sudo apt install build-essential libsqlite3-dev`
2. Switch to home directory `cd ~`
3. Install kdriveExpress `sudo cp sindabus-agent/lib/kdriveExpress-17.2.0-raspbian/raspbian/libkdriveExpress.so /usr/local/lib/`
4. Reload shared libraries `sudo ldconfig`

## Compile
1. Switch directory `cd sindabus-agent/src`
2. Compile `make`

## Install agent
1. `sudo cp agent /usr/local/bin` (it might be necessary to create the directory first)
