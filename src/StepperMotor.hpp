// Copyright (c) 2016, Cristiano Urban (http://crish4cks.net)
//
// A simple C++ class created to provide an easily interaction with
// the geared stepper motor 28BYJ48 through the ULN2003APG driver.
//
#pragma once
#if RASPI == 1
#include <vector>
#include <thread>
#include <atomic>

using namespace std;

class StepperMotor
{
public:
    StepperMotor();
    ~StepperMotor();
    bool isRunning() const
    {
        return running;
    }
    unsigned getThreshold() const
    {
        return threshold;
    }
    int getCurrentPosition() const
    {
        return current_pos;
    }
    unsigned getSteps(unsigned angle) const;
    unsigned getNumOfSteps() const
    {
        return nsteps;
    }
    void setGPIOutputs(unsigned in1, unsigned in2, unsigned in3, unsigned in4);
    void setThreshold(unsigned threshold);
    void run(int direction, unsigned angle, unsigned speed);
    void wait(unsigned milliseconds) const;

    // run in the custom thread, can be called mutliple times to adjust vector = (direction and velocity).
    void run_async(int vector);
private:
    std::shared_ptr<vector<vector<bool>>> sequence;          // the switching sequence
    std::shared_ptr<vector<vector<bool>>> rsequence;         // the switching sequence backwards
    std::shared_ptr<std::vector<std::vector<bool>>> currentSequence;
    bool running;                           // state of the stepper motor
    unsigned threshold;                     // symmetric threshold in degrees
    int current_pos;                        // current position in degrees
    unsigned nsteps;                        // total number of steps from the beginning
    unsigned in1, in2, in3, in4;            // stepper motor driver inputs

    std::atomic<bool> m_Run;
    std::atomic<int> moveVector;
    float td;
    std::shared_ptr<std::thread> thread;
};

#endif
