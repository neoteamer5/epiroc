#include "CoolingControl.hpp"

#include <gtest/gtest.h>

#include <limits>

namespace
{
CoolingThresholds ValidThresholds()
{
    //return CoolingThresholds{70.0, 65.0, 85.0, 78.0};
    return CoolingThresholds{};
}

void MoveToCooling(StateMachine& stateMachine)
{
    ASSERT_TRUE(stateMachine.Init(ValidThresholds()));
    EXPECT_EQ(stateMachine.Update(60.0, true, false, true, true), CoolingState::Standby);
    EXPECT_EQ(stateMachine.Update(70.0, true, false, true, true), CoolingState::Cooling);
}
}

TEST(CoolingCalibrationTest, LoadCalibrationReturnsExpectedDefaults)
{
    const CoolingThresholds thresholds = LoadCalibration();

    EXPECT_DOUBLE_EQ(thresholds.CoolOn, 70.0);
    EXPECT_DOUBLE_EQ(thresholds.CoolOff, 65.0);
    EXPECT_DOUBLE_EQ(thresholds.HighCoolOn, 85.0);
    EXPECT_DOUBLE_EQ(thresholds.HighCoolOff, 78.0);
}


TEST(StateMachineTest, StartsInOffState)
{
    StateMachine stateMachine;

    EXPECT_EQ(stateMachine.GetState(), CoolingState::Off);
}

TEST(StateMachineTest, RejectsInvalidCoolHysteresis)
{
    StateMachine stateMachine;
    CoolingThresholds thresholds = ValidThresholds();
    thresholds.CoolOff = thresholds.CoolOn;

    EXPECT_FALSE(stateMachine.Init(thresholds));
    EXPECT_EQ(stateMachine.GetState(), CoolingState::Off);
}

TEST(StateMachineTest, RejectsInvalidHighCoolHysteresis)
{
    StateMachine stateMachine;
    CoolingThresholds thresholds = ValidThresholds();
    thresholds.HighCoolOff = thresholds.HighCoolOn;

    EXPECT_FALSE(stateMachine.Init(thresholds));
}

TEST(StateMachineTest, RejectsCoolOnAtOrAboveHighCoolOn)
{
    StateMachine stateMachine;
    CoolingThresholds thresholds = ValidThresholds();
    thresholds.CoolOn = thresholds.HighCoolOn;

    EXPECT_FALSE(stateMachine.Init(thresholds));
}

TEST(StateMachineTest, RejectsNonFiniteThreshold)
{
    StateMachine stateMachine;
    CoolingThresholds thresholds = ValidThresholds();
    thresholds.CoolOn = std::numeric_limits<double>::quiet_NaN();

    EXPECT_FALSE(stateMachine.Init(thresholds));
}

TEST(StateMachineTest, UpdateBeforeInitForcesFault)
{
    StateMachine stateMachine;

    EXPECT_EQ(stateMachine.Update(60.0, true, false, true, true), CoolingState::Fault);
    EXPECT_EQ(stateMachine.GetState(), CoolingState::Fault);
}

TEST(StateMachineTest, OffTransitionsToStandbyWhenEnabled)
{
    StateMachine stateMachine;
    ASSERT_TRUE(stateMachine.Init(ValidThresholds()));

    EXPECT_EQ(stateMachine.Update(60.0, true, false, true, true), CoolingState::Standby);
}

TEST(StateMachineTest, StandbyTransitionsToCoolingAtCoolOnThreshold)
{
    StateMachine stateMachine;
    ASSERT_TRUE(stateMachine.Init(ValidThresholds()));
    ASSERT_EQ(stateMachine.Update(60.0, true, false, true, true), CoolingState::Standby);

    EXPECT_EQ(stateMachine.Update(70.0, true, false, true, true), CoolingState::Cooling);
}

TEST(StateMachineTest, CoolingTransitionsToHighCoolingAtHighCoolOnThreshold)
{
    StateMachine stateMachine;
    MoveToCooling(stateMachine);

    EXPECT_EQ(stateMachine.Update(85.0, true, false, true, true), CoolingState::HighCooling);
}

TEST(StateMachineTest, HighCoolingTransitionsBackToCoolingAtHighCoolOffThreshold)
{
    StateMachine stateMachine;
    MoveToCooling(stateMachine);
    ASSERT_EQ(stateMachine.Update(86.0, true, false, true, true), CoolingState::HighCooling);

    EXPECT_EQ(stateMachine.Update(78.0, true, false, true, true), CoolingState::Cooling);
}

TEST(StateMachineTest, CoolingTransitionsBackToStandbyAtCoolOffThreshold)
{
    StateMachine stateMachine;
    MoveToCooling(stateMachine);

    EXPECT_EQ(stateMachine.Update(65.0, true, false, true, true), CoolingState::Standby);
}

TEST(StateMachineTest, HysteresisKeepsCoolingBetweenCoolOffAndCoolOn)
{
    StateMachine stateMachine;
    MoveToCooling(stateMachine);

    EXPECT_EQ(stateMachine.Update(67.0, true, false, true, true), CoolingState::Cooling);
    EXPECT_EQ(stateMachine.Update(69.9, true, false, true, true), CoolingState::Cooling);
}

TEST(StateMachineTest, HysteresisKeepsHighCoolingAboveHighCoolOff)
{
    StateMachine stateMachine;
    MoveToCooling(stateMachine);
    ASSERT_EQ(stateMachine.Update(86.0, true, false, true, true), CoolingState::HighCooling);

    EXPECT_EQ(stateMachine.Update(82.0, true, false, true, true), CoolingState::HighCooling);
    EXPECT_EQ(stateMachine.Update(78.1, true, false, true, true), CoolingState::HighCooling);
}

TEST(StateMachineTest, ShutdownFromCoolingTransitionsToPostRun)
{
    StateMachine stateMachine;
    MoveToCooling(stateMachine);

    EXPECT_EQ(stateMachine.Update(75.0, true, true, true, true), CoolingState::PostRun);
}

TEST(StateMachineTest, ShutdownFromHighCoolingTransitionsToPostRun)
{
    StateMachine stateMachine;
    MoveToCooling(stateMachine);
    ASSERT_EQ(stateMachine.Update(90.0, true, false, true, true), CoolingState::HighCooling);

    EXPECT_EQ(stateMachine.Update(90.0, true, true, true, true), CoolingState::PostRun);
}

TEST(StateMachineTest, PostRunTransitionsToOffAtCoolOffThreshold)
{
    StateMachine stateMachine;
    MoveToCooling(stateMachine);
    ASSERT_EQ(stateMachine.Update(75.0, true, true, true, true), CoolingState::PostRun);

    EXPECT_EQ(stateMachine.Update(65.0, false, true, true, true), CoolingState::Off);
}

TEST(StateMachineTest, InvalidSensorForcesFault)
{
    StateMachine stateMachine;
    ASSERT_TRUE(stateMachine.Init(ValidThresholds()));

    EXPECT_EQ(stateMachine.Update(60.0, true, false, false, true), CoolingState::Fault);
}

TEST(StateMachineTest, UnhealthyPumpForcesFault)
{
    StateMachine stateMachine;
    ASSERT_TRUE(stateMachine.Init(ValidThresholds()));

    EXPECT_EQ(stateMachine.Update(60.0, true, false, true, false), CoolingState::Fault);
}

TEST(StateMachineTest, NonFiniteTemperatureForcesFault)
{
    StateMachine stateMachine;
    ASSERT_TRUE(stateMachine.Init(ValidThresholds()));

    EXPECT_EQ(stateMachine.Update(std::numeric_limits<double>::infinity(), true, false, true, true),
              CoolingState::Fault);
}

TEST(StateMachineTest, FaultRecoversToOffWhenHealthyAndDisabled)
{
    StateMachine stateMachine;
    ASSERT_TRUE(stateMachine.Init(ValidThresholds()));
    ASSERT_EQ(stateMachine.Update(60.0, true, false, false, true), CoolingState::Fault);

    EXPECT_EQ(stateMachine.Update(60.0, false, false, true, true), CoolingState::Off);
}

TEST(StateMachineTest, FaultDoesNotRecoverWhileEnabled)
{
    StateMachine stateMachine;
    ASSERT_TRUE(stateMachine.Init(ValidThresholds()));
    ASSERT_EQ(stateMachine.Update(60.0, true, false, false, true), CoolingState::Fault);

    EXPECT_EQ(stateMachine.Update(60.0, true, false, true, true), CoolingState::Fault);
}

