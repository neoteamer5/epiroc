# ---------------------------------------------------------------------------
# Start PLC simulator
#
# If this fails, it usually means the PLC module was not built.
# This can happen when SetupLinux has not completed successfully,
# or when the apps/plc target was skipped or failed during CMake build.
#
# Ensure that:
#   - SetupLinux finished without errors
#   - apps/common and apps/plc were built (check apps/build/plc/)
#   - the plc executable exists at: apps/build/plc/plc
#
# Note: You can Press any key to pause or continue the PLC loop
# ---------------------------------------------------------------------------

echo "Starting PLC simulator..."
./build/bin/plc
