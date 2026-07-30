#include "CANReader.hpp"
#include "can_reader_c_api.hpp"
/**
 * Python → C wrapper → C++ CANReader → atomic CANData
 *
 */

static CANReader* reader = nullptr;

void start_can_reader_demo() {
    reader = new CANReader(true);
}

void start_can_reader_real() {
    reader = new CANReader(false);
}

int get_can_data(CANData_C* out) {
    if (!reader || !out) return -1;

    out->spd  = reader->data.spd.load();
    out->rpm  = reader->data.rpm.load();
    out->fuel = reader->data.fuel.load();
    out->temp = reader->data.temp.load();
    out->warn = reader->data.warn.load() ? 1 : 0;

    return 0;
}
