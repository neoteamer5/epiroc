#include "CoolingControl.hpp"

#include <gtest/gtest.h>

#include <limits>

namespace
{
constexpr double DT = 0.1;
}


TEST(PIDControllerTest, ZeroErrorClampsToConfiguredMinimum)
{
    PIDController pid(2.0, 0.25, 0.10, 20.0, 100.0);

    EXPECT_DOUBLE_EQ(pid.Update(65.0, 65.0, DT), 20.0);
}

TEST(PIDControllerTest, PositiveCoolingErrorProducesPositiveOutput)
{
    PIDController pid(2.0, 0.0, 0.0, 0.0, 100.0);

    EXPECT_DOUBLE_EQ(pid.Update(65.0, 75.0, DT), 20.0);
}

TEST(PIDControllerTest, OutputIsClampedToMaximum)
{
    PIDController pid(10.0, 0.0, 0.0, 0.0, 100.0);

    EXPECT_DOUBLE_EQ(pid.Update(65.0, 100.0, DT), 100.0);
}

TEST(PIDControllerTest, OutputIsClampedToMinimum)
{
    PIDController pid(10.0, 0.0, 0.0, 20.0, 100.0);

    EXPECT_DOUBLE_EQ(pid.Update(65.0, 60.0, DT), 20.0);
}

TEST(PIDControllerTest, ConstructorNormalizesReversedOutputBounds)
{
    PIDController pid(1.0, 0.0, 0.0, 100.0, 20.0);

    EXPECT_DOUBLE_EQ(pid.Update(0.0, 1000.0, DT), 100.0);
    pid.Reset();
    EXPECT_DOUBLE_EQ(pid.Update(1000.0, 0.0, DT), 20.0);
}

TEST(PIDControllerTest, IntegralAccumulatesAcrossUpdates)
{
    PIDController pid(0.0, 1.0, 0.0, 0.0, 100.0);

    EXPECT_DOUBLE_EQ(pid.Update(0.0, 10.0, 1.0), 10.0);
    EXPECT_DOUBLE_EQ(pid.Update(0.0, 10.0, 1.0), 20.0);
}

TEST(PIDControllerTest, DerivativeIsSuppressedOnFirstUpdate)
{
    PIDController pid(0.0, 0.0, 1.0, 0.0, 100.0);

    EXPECT_DOUBLE_EQ(pid.Update(0.0, 10.0, 1.0), 0.0);
    EXPECT_DOUBLE_EQ(pid.Update(0.0, 20.0, 1.0), 10.0);
}

TEST(PIDControllerTest, ResetClearsIntegralAndDerivativeHistory)
{
    PIDController pid(0.0, 1.0, 1.0, 0.0, 100.0);

    EXPECT_DOUBLE_EQ(pid.Update(0.0, 10.0, 1.0), 10.0);
    EXPECT_DOUBLE_EQ(pid.Update(0.0, 20.0, 1.0), 40.0);

    pid.Reset();

    EXPECT_DOUBLE_EQ(pid.Update(0.0, 10.0, 1.0), 10.0);
}

TEST(PIDControllerTest, InvalidDtReturnsPreviousOutput)
{
    PIDController pid(2.0, 0.0, 0.0, 0.0, 100.0);
    ASSERT_DOUBLE_EQ(pid.Update(0.0, 10.0, 1.0), 20.0);

    EXPECT_DOUBLE_EQ(pid.Update(0.0, 50.0, 0.0), 20.0);
    EXPECT_DOUBLE_EQ(pid.Update(0.0, 50.0, -1.0), 20.0);
}

TEST(PIDControllerTest, NonFiniteInputReturnsPreviousOutput)
{
    PIDController pid(2.0, 0.0, 0.0, 0.0, 100.0);
    ASSERT_DOUBLE_EQ(pid.Update(0.0, 10.0, 1.0), 20.0);

    EXPECT_DOUBLE_EQ(pid.Update(std::numeric_limits<double>::quiet_NaN(), 10.0, 1.0), 20.0);
    EXPECT_DOUBLE_EQ(pid.Update(0.0, std::numeric_limits<double>::infinity(), 1.0), 20.0);
}

