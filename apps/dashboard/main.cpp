#include "DashboardBackend.hpp"

int main()
{
    DashboardBackend backend;

    if (!backend.Init())
    {
        return 1;
    }

    backend.Start();
    backend.Join();

    return 0;
}