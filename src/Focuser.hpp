// Copyright (c) 2016, Cristiano Urban (http://crish4cks.net)
//
// A simple C++ class created to provide an easily interaction with
// the geared stepper motor 28BYJ48 through the ULN2003APG driver.
//

#ifndef FOCUSER_HPP
#define FOCUSER_HPP

#include <vector>
#include <thread>
#include <atomic>
#include <map>
#include <string>


using namespace std;

class Focuser
{
public:
    Focuser();
    ~Focuser();

    void setFocus(int value, bool blocking);
    void setZoom(int value, bool blocking);
    void setIRCut(bool value, bool blocking);
private:
    int read(int reg_Addr);
    int write(int reg_Addr, int value);

    // DEvice is busy
    bool isBusy();
    // wait until device is free again
    void waitForFree();

    void reset(int opt, bool blocking = true);
    int get(int opt);
    void set(int opt, int value, bool blocking = false);

    int m_chipAddr;
    int m_Focus;
    int m_Zoom;
    bool m_IrCut;
    std::map<int, std::map<std::string, int>> m_opts;
};

#endif
