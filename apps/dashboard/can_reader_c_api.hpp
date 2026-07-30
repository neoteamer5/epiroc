// can_reader_c_api.hpp
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int spd;
    int rpm;
    int fuel;
    int temp;
    int warn;
} CANData_C;

void start_can_reader_demo();
void start_can_reader_real();
int get_can_data(CANData_C* out);

#ifdef __cplusplus
}
#endif