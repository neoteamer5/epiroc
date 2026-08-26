# J1939 Dashboard

This project provides a Python/Qt dashboard for monitoring J1939 CAN communication.

## Prerequisites

Run the commands from the project root directory:

```bash
cd ~/epiroc
```

Activate the Python virtual environment:

```bash
source ~/j1939dash/bin/activate
```

After activation, the shell should show:

```text
(j1939dash) neomo@ohio:~/epiroc$
```

## Run the C API Test

To test the J1939 C API integration:

```bash
python apps/dashboard/qt-app/test_c_api.py
```

## Run the Qt Dashboard

To start the dashboard application:

```bash
cd apps/dashboard/qt-app && python main.py
```

## Quick Start

```bash
cd ~/epiroc
source ~/j1939dash/bin/activate

# Test the C API
python apps/dashboard/qt-app/test_c_api.py

# Or start the dashboard
python apps/dashboard/qt-app/main.py
```

Only one of the last two commands is required depending on whether you want to test the C API or run the dashboard.
