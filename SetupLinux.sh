#!/bin/bash
set -e

echo "Updating package index..."
sudo apt update

echo "Installing SocketCAN tools and dependencies..."
sudo apt install -y \
    build-essential \
    git \
    pkg-config \
    libtool \
    autoconf \
    can-utils \
    #linux-modules-extra-$(uname -r)


echo "Creating virtual CAN interface (vcan0)..."
if ! ip link show vcan0 >/dev/null 2>&1; then
    echo "vcan0 not found, creating..."
    sudo ip link add dev vcan0 type vcan
else
    echo "vcan0 already exists, skipping creation."
fi

sudo ip link set up vcan0

echo "J1939 stack installed and vcan0 ready."
echo "Test with: j1939cat vcan0"

#!/bin/bash
set -e

echo "Updating system..."
sudo apt update

echo "Installing Python + CAN tools..."
sudo apt install -y python3 python3-pip python3-venv can-utils

echo "Creating Python virtual environment..."
python3 -m venv ~/j1939dash
source ~/j1939dash/bin/activate

echo "Installing PySide6 + CAN/J1939 libs into venv..."
pip install PySide6 python-can cantools j1939


echo "Run it with:"
echo "source ~/j1939dash/bin/activate && cd apps/dashboard && python3 main.py"
