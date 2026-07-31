#!/bin/bash
set -e

# Move to the directory where the script is located
cd "$(dirname "$0")"


echo "Updating package index..."
sudo apt update

echo "Installing SocketCAN tools and dependencies..."
sudo apt install -y \
    build-essential \
    cmake \
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

echo "Updating system..."
sudo apt update

echo "Installing Python + CAN tools..."
sudo apt install -y python3 python3-pip python3-venv can-utils



echo "Creating Python virtual environment..."
python3 -m venv ~/j1939dash
source ~/j1939dash/bin/activate

echo "Installing PySide6 + CAN/J1939 libs into venv..."
pip install PySide6 python-can cantools j1939

echo "Building Project..."
mkdir -p build
cd build
cmake ..
make

echo -e "\n\n\n\nNote:"
echo "Open a new Linux terminal and run 'StartPLC.sh' after you proceed following interaction to answer 'n' for the question: 'Use demo data? (y/n)'"
echo -e "\n\n\n\n..."

#!/bin/bash

echo "Select run mode:"
echo "1) Run with Qt (Python Qt dashboard)"
echo "2) Run without Qt (core-only CLI mode)"
read -p "Enter choice [1/2]: " choice

case "$choice" in
    1)
        echo "Qt mode selected."
        echo "Select data source:"
        echo "1) Demo data"
        echo "2) PLC"
        read -p "Enter choice [1/2]: " datasource

        source ~/j1939dash/bin/activate
        cd ../apps/dashboard/qt-app

        case "$datasource" in
            1)
                echo "Running Qt dashboard with demo data..."
                python3 main.py demo
                ;;
            2)
                echo "Running Qt dashboard with PLC data..."
                python3 main.py PLC
                ;;
            *)
                echo "Invalid data source choice"
                exit 1
                ;;
        esac
        ;;
    2)
        echo "Starting non-Qt mode..."
        ./bin/can_reader_demo
        ;;
    *)
        echo "Invalid choice"
        exit 1
        ;;
esac

