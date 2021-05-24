#if RASPI == 1
#include <cassert>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include "StepperMotor.hpp"

using namespace std;

// Switching sequence for the 28BYJ48 (clockwise)
static const bool SEQUENCE[8][4] =
{
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
};

// stepAngle = (Step angle / gear reduction ratio) = (5.625 / 63.68395)
static const float stepAngle = 0.0883268076179f;

StepperMotor::~StepperMotor()
{
    if(thread && thread->joinable())
    {
        m_Run = false;
        moveVector = 0;
        thread->join();
    }
}

// Default constructor
StepperMotor::StepperMotor()
{
    running = false;
    threshold = 0;
    current_pos = 0;
    sequence = std::make_shared<std::vector<std::vector<bool>>>();
    rsequence = std::make_shared<std::vector<std::vector<bool>>>();
    sequence->resize(8);
    rsequence->resize(8);

    for(unsigned i = 0; i < sequence->size(); i++)
    {
        sequence->at(i).resize(4);
        rsequence->at(i).resize(4);
    }

    for (unsigned i = 0; i < sequence->size(); i++)
    {
        for(unsigned j = 0; j < sequence->at(i).size(); j++)
        {
            sequence->at(i).at(j) = SEQUENCE[i][j];
            rsequence->at(sequence->size() - 1 - i).at(j) = SEQUENCE[i][j];
        }
    }

    currentSequence = sequence;

    moveVector = 0;
    m_Run = true;

    thread = std::make_shared<std::thread>([this]()
    {
        int count = 0;

        while(m_Run)
        {
            count = 0;
            bool wasRunning = false;

            while(moveVector != 0)
            {
                wasRunning = true;
                digitalWrite(in1, (*currentSequence)[count][0] ? HIGH : LOW);
                digitalWrite(in2, (*currentSequence)[count][1] ? HIGH : LOW);
                digitalWrite(in3, (*currentSequence)[count][2] ? HIGH : LOW);
                digitalWrite(in4, (*currentSequence)[count][3] ? HIGH : LOW);

                if(++count == 8)
                {
                    count = 0;
                }

                delayMicroseconds(td); // minimum delay 5ms (speed 100%), maximum delay 25ms (speed 20%)
            }

            if (wasRunning)
            {
                // Cleanup (recommended in order to prevent stepper motor overheating)
                digitalWrite(in1, LOW);
                digitalWrite(in2, LOW);
                digitalWrite(in3, LOW);
                digitalWrite(in4, LOW);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
}

/// set the async move vector.
void StepperMotor::run_async(int vector)
{
    if (moveVector != vector)
    {
        moveVector = vector;
        td = (5 * 100 / (float)std::abs(vector)) * 1000;
        currentSequence = vector > 0 ? sequence : rsequence;
    }
}

// Returns the number of steps associated to a certain angle
unsigned StepperMotor::getSteps(unsigned angle) const
{
    return (unsigned) roundf(angle / stepAngle);
}


// Sets the GPIO outputs needed by inputs of the stepper motor driver (ULN2003APG)
// For more details concerning the wiringPi GPIO table conversion refere here:
// http://wiringpi.com/pins/
void StepperMotor::setGPIOutputs(unsigned in1, unsigned in2, unsigned in3, unsigned in4)
{
    this->in1 = in1;
    pinMode(in1, OUTPUT);
    this->in2 = in2;
    pinMode(in2, OUTPUT);
    this->in3 = in3;
    pinMode(in3, OUTPUT);
    this->in4 = in4;
    pinMode(in4, OUTPUT);
}


// Sets a maximum threshold angle for the motor rotation
void StepperMotor::setThreshold(unsigned threshold)
{
    assert(threshold < 180);
    this->threshold = threshold;
}


// Runs the stepper motor.
// * direction: 1 to go clockwise, -1 to go counterclockwise
// * angle: can assume values from 0 to 360 degrees
// * speed: from 20% (minimum speed) to 100% (maximum speed)
void StepperMotor::run(int direction, unsigned angle, unsigned speed)
{
    float td;
    unsigned nsteps, count, ndegrees;

    running = true;

    // Check the direction and angle values
    assert(direction == 1 || direction == -1);
    assert(angle <= 360);

    // Check the speed value (5 speed modes allowed, from 20% to 100%)
    /*  switch(speed) {
         case(20): break;
         case(40): break;
         case(60): break;
         case(80): break;
         case(100): break;
         default: return;
        }*/

    // Delay between each step of the switching sequence (in microseconds)
    td = (5 * 100 / (float) speed) * 1000;

    // Set the right number of steps to do, taking in account of the threshold
    if(abs(current_pos + direction * angle) > threshold && threshold != 0)
    {
        ndegrees = threshold - direction * current_pos;
    }
    else
    {
        ndegrees = angle;
    }

    nsteps = getSteps(ndegrees);

    // To go counterclockwise we need to reverse the switching sequence
    if(direction == -1)
    {
        currentSequence = rsequence;
    }

    count = 0;

    for(unsigned i = 0; i < nsteps; i++)
    {
        if(count == 8)
        {
            count = 0;
        }

        digitalWrite(in1, (*currentSequence)[count][0] ? HIGH : LOW);
        digitalWrite(in2, (*currentSequence)[count][1] ? HIGH : LOW);
        digitalWrite(in3, (*currentSequence)[count][2] ? HIGH : LOW);
        digitalWrite(in4, (*currentSequence)[count][3] ? HIGH : LOW);

        count++;
        delayMicroseconds(td); // minimum delay 5ms (speed 100%), maximum delay 25ms (speed 20%)
    }

    // Cleanup (recommended in order to prevent stepper motor overheating)
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    digitalWrite(in3, LOW);
    digitalWrite(in4, LOW);

    // Reverse again in order to restore the original vector for the next operations
    if(direction == -1)
    {
        currentSequence = sequence;
    }

    // Update the state
    this->nsteps += nsteps;
    current_pos += direction * ndegrees;
    running = false;
}


// Sends to sleep the stepper motor for a certain amount of time (in milliseconds)
void StepperMotor::wait(unsigned milliseconds) const
{
    delay(milliseconds);
}
#endif