#include <cstring>
#include <thread>
#include "can_reader.hpp"

int main(int argc, char** argv)
{
    bool use_demo = true;
    if (argc > 1 && strstr(argv[1], "can"))
        use_demo = false;

    if (use_demo) {
        std::thread(demo_loop).join();
    } else {
        init_socketcan();
        std::thread(read_loop).join();
    }

    return 0;
}
