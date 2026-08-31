#include "CoolingControl.hpp"

#include <gtest/gtest.h>

namespace
{
constexpr double DT = 0.1;

void MoveControllerToCooling(CoolingController& controller)
{
    ASSERT_TRUE(controller.Init());
    controller.Update(60.0, true, false, true, true, DT);
    ASSERT_EQ(controller.GetState(), CoolingState::Standby);
    controller.Update(72.0, true, false, true, true, DT);
    ASSERT_EQ(controller.GetState(), CoolingState::Cooling);
}
}


TEST(CoolingControllerTest, UpdateBeforeInitKeepsSafeOutput)
{
    CoolingController controller;

    controller.Update(90.0, true, false, true, true, DT);

    EXPECT_EQ(controller.GetState(), CoolingState::Off);
    EXPECT_DOUBLE_EQ(controller.GetFanSpeed(), 0.0);
}

TEST(CoolingControllerTest, InitStartsInOffWithZeroFan)
{
    CoolingController controller;

    ASSERT_TRUE(controller.Init());
    EXPECT_EQ(controller.GetState(), CoolingState::Off);
    EXPECT_DOUBLE_EQ(controller.GetFanSpeed(), 0.0);
}

TEST(CoolingControllerTest, StandbyCommandsZeroFan)
{
    CoolingController controller;
    ASSERT_TRUE(controller.Init());

    controller.Update(60.0, true, false, true, true, DT);

    EXPECT_EQ(controller.GetState(), CoolingState::Standby);
    EXPECT_DOUBLE_EQ(controller.GetFanSpeed(), 0.0);
}

TEST(CoolingControllerTest, CoolingUsesPidOutput)
{
    CoolingController controller;
    MoveControllerToCooling(controller);

    EXPECT_GT(controller.GetFanSpeed(), 0.0);
    EXPECT_LT(controller.GetFanSpeed(), 100.0);
}

TEST(CoolingControllerTest, HighCoolingCommandsFullFan)
{
    CoolingController controller;
    MoveControllerToCooling(controller);

    controller.Update(86.0, true, false, true, true, DT);

    EXPECT_EQ(controller.GetState(), CoolingState::HighCooling);
    EXPECT_DOUBLE_EQ(controller.GetFanSpeed(), 100.0);
}

TEST(CoolingControllerTest, PostRunUsesPidOutput)
{
    CoolingController controller;
    MoveControllerToCooling(controller);

    controller.Update(75.0, true, true, true, true, DT);

    EXPECT_EQ(controller.GetState(), CoolingState::PostRun);
    EXPECT_GT(controller.GetFanSpeed(), 0.0);
    EXPECT_LT(controller.GetFanSpeed(), 100.0);
}

TEST(CoolingControllerTest, FaultCommandsSafeOutput)
{
    CoolingController controller;
    MoveControllerToCooling(controller);

    controller.Update(75.0, true, false, false, true, DT);

    EXPECT_EQ(controller.GetState(), CoolingState::Fault);
    EXPECT_DOUBLE_EQ(controller.GetFanSpeed(), CoolingController::FAULT_FAN_SPEED);
}

TEST(CoolingControllerTest, NormalTemperatureSequenceMatchesDldExample)
{
    CoolingController controller;
    ASSERT_TRUE(controller.Init());

    controller.Update(60.0, true, false, true, true, DT);
    EXPECT_EQ(controller.GetState(), CoolingState::Standby);

    controller.Update(72.0, true, false, true, true, DT);
    EXPECT_EQ(controller.GetState(), CoolingState::Cooling);

    controller.Update(78.0, true, false, true, true, DT);
    EXPECT_EQ(controller.GetState(), CoolingState::Cooling);

    controller.Update(86.0, true, false, true, true, DT);
    EXPECT_EQ(controller.GetState(), CoolingState::HighCooling);

    controller.Update(82.0, true, false, true, true, DT);
    EXPECT_EQ(controller.GetState(), CoolingState::HighCooling);

    controller.Update(77.0, true, false, true, true, DT);
    EXPECT_EQ(controller.GetState(), CoolingState::Cooling);

    controller.Update(64.0, true, false, true, true, DT);
    EXPECT_EQ(controller.GetState(), CoolingState::Standby);
    EXPECT_DOUBLE_EQ(controller.GetFanSpeed(), 0.0);
}

TEST(CoolingControllerTest, ReInitReturnsControllerToSafeOffState)
{
    CoolingController controller;
    MoveControllerToCooling(controller);
    ASSERT_GT(controller.GetFanSpeed(), 0.0);

    ASSERT_TRUE(controller.Init());

    EXPECT_EQ(controller.GetState(), CoolingState::Off);
    EXPECT_DOUBLE_EQ(controller.GetFanSpeed(), CoolingController::OFF_FAN_SPEED);
}
