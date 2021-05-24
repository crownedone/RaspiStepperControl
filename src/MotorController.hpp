#pragma once
#include <thread>
#include <atomic>
class StepperMotor;
class Focuser;

class MotorController
{
public:
    MotorController();
    ~MotorController();

    void setPitch(int vector);
    void setYaw(int vector);

    void setFocus(int vector);
    void setZoom(int vector);
    void setIR(bool vector);
private:

    // Stepper motor handle
    std::shared_ptr<StepperMotor> m_Stepper1;
    std::shared_ptr<StepperMotor> m_Stepper2;

    // Focuser handle
    std::shared_ptr<Focuser> m_Focuser;
};
