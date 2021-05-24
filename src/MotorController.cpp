#include "MotorController.hpp"
#include <boost/filesystem.hpp>
#include <future>

#include "Focuser.hpp"

#if RASPI == 1
#include <wiringPi.h>
#include "StepperMotor.hpp"

#else
// fake class
class StepperMotor
{
public:
    void setGPIOutputs(int, int, int, int) {};
    bool isRunning() const
    {
        return false;
    }
    unsigned getThreshold() const
    {
        return 0;
    }
    int getCurrentPosition() const
    {
        return 0;
    }
    unsigned getSteps(unsigned angle) const
    {
        return 0;
    };
    unsigned getNumOfSteps() const
    {
        return 0;
    }
    void setGPIOutputs(unsigned in1, unsigned in2, unsigned in3, unsigned in4) {};
    void setThreshold(unsigned threshold) {};
    void run(int direction, unsigned angle, unsigned speed) {};
    void wait(unsigned milliseconds) const {};

    void run_async(int vector) {};
};
#endif

MotorController::MotorController()
{
#if RASPI == 1
    wiringPiSetup();
#endif

    m_Stepper1 = std::make_shared<StepperMotor>();
    m_Stepper2 = std::make_shared<StepperMotor>();
    m_Focuser = std::make_shared<Focuser>();

    // Yaw Motor
    m_Stepper1->setGPIOutputs(7, 0, 2, 3);
    // Pitch Motor
    m_Stepper2->setGPIOutputs(22, 23, 24, 25);
}

MotorController::~MotorController()
{
}

void MotorController::setPitch(int vector)
{
    m_Stepper1->run_async(vector);
}

void MotorController::setYaw(int vector)
{
    m_Stepper2->run_async(vector);
}

void MotorController::setFocus(int vector)
{
    m_Focuser->setFocus(vector, true);
}
void MotorController::setZoom(int vector)
{
    m_Focuser->setZoom(vector, true);
}
void MotorController::setIR(bool vector)
{
    m_Focuser->setIRCut(vector, true);
}
