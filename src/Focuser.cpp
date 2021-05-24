#include <cassert>
#include <cstdlib>
#include <cmath>
#include <algorithm>

#include "Focuser.hpp"
#include <iostream>

#if RASPI == 1
#include <wiringPi.h>
#include <wiringPiI2C.h>
#else
int wiringPiI2CSetup(int){return -1;}
int wiringPiI2CWriteReg16(int,int,int){return -1;}
int wiringPiI2CReadReg16(int,int){return -1;}
#endif

using namespace std;

int CHIP_I2C_ADDR = 0x0C;
int BUSY_REG_ADDR = 0x04;

int OPT_BASE    = 0x1000;
int OPT_FOCUS   = OPT_BASE | 0x01;
int OPT_ZOOM    = OPT_BASE | 0x02;
int OPT_MOTOR_X = OPT_BASE | 0x03;
int OPT_MOTOR_Y = OPT_BASE | 0x04;
int OPT_IRCUT   = OPT_BASE | 0x05;
int NONE = -1;

Focuser::~Focuser()
{
}

// Default constructor
Focuser::Focuser()
{
    // Setup i2c handle for communication with the driver
    m_chipAddr = wiringPiI2CSetup (CHIP_I2C_ADDR);
    if(m_chipAddr < 0)
    {
        std::cerr << "Error initializing i2c device (Zoom and Focus)\n";
        return;
    }


    // initial position is always zero
    m_Focus = m_Zoom = 0;
    m_IrCut = false;

    std::map<std::string, int> focus, zoom, motorx, motory, ircut;
    focus["REG_ADDR"] = 0x01;
    focus["MAX_VALUE"] = 18000;
    focus["RESET_ADDR"] = 0x01 + 0x0A;
    zoom["REG_ADDR"] = 0x00;
    zoom["MAX_VALUE"] = 18000;
    zoom["RESET_ADDR"] = 0x00 + 0x0A;
    motorx["REG_ADDR"] = 0x05;
    motorx["MAX_VALUE"] = 180;
    motorx["RESET_ADDR"] = NONE;
    motory["REG_ADDR"] = 0x06;
    motory["MAX_VALUE"] = 180;
    motory["RESET_ADDR"] = NONE;
    ircut["REG_ADDR"] = 0x0C;
    ircut["MAX_VALUE"] = 0x01; // #0x0001 open, 0x0000 close
    ircut["RESET_ADDR"] = NONE;

    m_opts[OPT_FOCUS] = focus;
    m_opts[OPT_ZOOM] = zoom;
    m_opts[OPT_MOTOR_X] = motorx;
    m_opts[OPT_MOTOR_Y] = motory;
    m_opts[OPT_IRCUT] = ircut;
}

void Focuser::setFocus(int value, bool blocking) {
    waitForFree();
    m_Focus += value;
    set(OPT_FOCUS, m_Focus, blocking);
}

void Focuser::setZoom(int value, bool blocking) {
    waitForFree();
    m_Zoom += value;
    set(OPT_ZOOM, m_Focus, blocking);
}
void Focuser::setIRCut(bool value, bool blocking)
{
    waitForFree();
    m_IrCut = value;
    set(OPT_IRCUT, (int)m_IrCut, blocking);
}

int Focuser::read(int reg_Addr)
{
    int value = wiringPiI2CReadReg16 (m_chipAddr, reg_Addr);
    value = ((value & 0x00FF)<< 8) | ((value & 0xFF00) >> 8);
    return value;
}

int Focuser::write(int reg_Addr, int value)
{
    if(value < 0)
        value = 0;
    value = ((value & 0x00FF)<< 8) | ((value & 0xFF00) >> 8);
    return wiringPiI2CWriteReg16 (m_chipAddr, reg_Addr, value);
}

bool Focuser::isBusy()
{
    return read(BUSY_REG_ADDR) != 0;
}

// wait until device is free again
void Focuser::waitForFree()
{
    int count = 0;
    while(isBusy() && count < (5 / 0.01))
    {
        count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Focuser::reset(int opt, bool blocking)
{
    waitForFree();
    auto& info = m_opts[opt];
    if(info.size() != 3 || info["RESET_ADDR"] <= 0) // no need to find()
        return;
    write(info["RESET_ADDR"], 0x0000);
    if(blocking)
        waitForFree();
}

int Focuser::get(int opt)
{
    waitForFree();
    auto& info = m_opts[opt];
    if(info.size() != 3) // no need to find()
        return -1;
    return read(info["REG_ADDR"]);
}

void Focuser::set(int opt, int value, bool blocking)
{
    waitForFree();
    auto& info = m_opts[opt];
    if(info.size() != 3) // no need to find()
        return;
    if(value > info["MAX_VALUE"])
        value = info["MAX_VALUE"];
    if(value < 0)
        value = 0;

    write(info["REG_ADDR"],value); // check return?

    if(blocking)
        waitForFree();
}


