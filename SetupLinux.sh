#!/bin/bash
set -e

# Move to the directory where the script is located
cd "$(dirname "$0")"

# ---------------------------------------------------------------------------
# Install system dependencies
# ---------------------------------------------------------------------------
echo "Updating package index..."
sudo apt update

echo "Installing build, CAN, Python, and GoogleTest dependencies..."
sudo apt install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libtool \
    autoconf \
    can-utils \
    python3 \
    python3-pip \
    python3-venv \
    libgtest-dev

# If required on a native Ubuntu installation:
# sudo apt install -y linux-modules-extra-$(uname -r)

# ---------------------------------------------------------------------------
# Configure virtual CAN
# ---------------------------------------------------------------------------
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

# ---------------------------------------------------------------------------
# Python environment
# ---------------------------------------------------------------------------
echo "Creating Python virtual environment..."

if [ ! -d "$HOME/j1939dash" ]; then
    python3 -m venv "$HOME/j1939dash"
else
    echo "Python virtual environment already exists."
fi

source "$HOME/j1939dash/bin/activate"

echo "Installing PySide6 + CAN/J1939 libraries..."
pip install --upgrade pip
pip install PySide6 python-can cantools j1939

# ---------------------------------------------------------------------------
# Build C++ project
# ---------------------------------------------------------------------------
echo "Building project..."

mkdir -p build

cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTING=ON

cmake --build build -j"$(nproc)"

# ---------------------------------------------------------------------------
# Run GoogleTest tests through CTest
# ---------------------------------------------------------------------------
echo
echo "Running GoogleTest unit tests..."

ctest \
    --test-dir build \
    --output-on-failure

echo
echo "All unit tests passed."

# ---------------------------------------------------------------------------
# Runtime selection
# ---------------------------------------------------------------------------
echo
echo "Note:"
echo "Open a new Linux terminal and run 'StartPLC.sh' when PLC mode is used."
echo

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

        source "$HOME/j1939dash/bin/activate"
        cd apps/dashboard/qt-app

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
        ./build/bin/can_reader_demo
        ;;

    *)
        echo "Invalid choice"
        exit 1
        ;;
esac