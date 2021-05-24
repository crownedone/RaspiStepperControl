#include <ctime>
#include <iostream>

#include "StopWatch.hpp"

#include "MotorController.hpp"

//#include <boost/filesystem.hpp>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <signal.h>
#include "Client.hpp"

using namespace std;

std::atomic<bool> running(true);

static void on_close(int signal) {
    std::cerr << "Stopping\n";
    running = false;
}

int main(int argc, char **argv) {
    try {
        MotorController mcd;

        WebSocketClient acs;

        std::cout << "Websocket Protocol:" << std::endl;
        std::cout << "-----------------------------------------------------" << std::endl;
        std::cout << "\"pXXXX\" - Pitch   X = [-100,100]" << std::endl;
        std::cout << "\"yXXXX\" - Yaw     X = [-100,100]" << std::endl;
        std::cout << "\"fX\"    - Focus   X = 0 - stop, 1 - left, 2 - right" << std::endl;
        std::cout << "\"zX\"    - Zoom    X = 0 - stop, 1 - left, 2 - right" << std::endl;
        std::cout << "\"iX\"    - IR Cut  X = 0 - off, 1 - on" << std::endl;
        std::cout << "-----------------------------------------------------" << std::endl;

        std::cout << "Trying to connect to 192.168.1.99:9876" << std::endl;
        // try connect
        while(!acs.connect("192.168.1.99", 9876) && running)
            std::this_thread::sleep_for(std::chrono::seconds(3));

        //mcd.setPitch(rh->message.CameraSettings.motorPitch);
        //mcd.setYaw(rh->message.CameraSettings.motorYaw);

        acs.onMessage.connect([&mcd](const std::string& msg){
            if(msg.empty())
                return;

            char code = msg[0];
            int value = 0;

            try
            {
                value = std::stoi(msg.substr(1));
            }
            catch(const std::exception& e)
            {
                std::cerr << "Exception: " << e.what() << std::endl;
            }

            switch(code)
            {
                case 'p':// 0 - 200 = [-100,100]
                    std::cout << "setting Pitch " << value << std::endl;
                    mcd.setPitch(value);
                    break;
                case 'y': // 0 - 200 = [-100,100]
                    std::cout << "setting Yaw " << value << std::endl;
                    mcd.setYaw(value);
                    break;
                case 'f':
                    std::cout << "setting Focus " << ((value == 0) ? "stop" : ((value == 1) ? "left" : "right")) << std::endl;
                    if(value == 0)
                        mcd.setFocus(0);
                    else if(value == 1)
                        mcd.setFocus(100);
                    else if(value == 2)
                        mcd.setFocus(-100);
                    break;
                case 'z':
                    std::cout << "setting Zoom " << ((value == 0) ? "stop" : ((value == 1) ? "left" : "right")) << std::endl;
                    if(value == 0)
                        mcd.setZoom(0);
                    else if(value == 1)
                        mcd.setZoom(100);
                    else if(value == 2)
                        mcd.setZoom(-100);
                    break;
                case 'i':
                    std::cout << "setting IR " << value << std::endl;
                    mcd.setIR(value > 0);
                    break;
            }
        });

        signal(SIGINT, on_close);

        // loop until shutdown
        while(running)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
    return 0;
}