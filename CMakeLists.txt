cmake_minimum_required (VERSION 3.8.0 FATAL_ERROR)

project (RaspiStepperControl LANGUAGES CXX VERSION 1.0.0)

# We use C++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost REQUIRED system date_time filesystem program_options)
set(BOOST_DEPS
	Boost::system
	Boost::date_time
	Boost::filesystem
	Boost::program_options
)
find_package(WebSocketPP REQUIRED)

set(DEPENDENCIES
		${BOOST_DEPS})
set(DEFINITIONS
		BOOST_ALL_DYN_LINK
		BOOST_ALL_NO_LIB
		BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE
		BOOST_LOG_DYN_LINK
		_CRT_SECURE_NO_WARNINGS)

if(UNIX)
	set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
	list(APPEND DEPENDENCIES Threads::Threads)
elseif(WIN32)
	list(APPEND DEFINITIONS BOOST_ASIO_HAS_IOCP)
endif()


if(${CMAKE_HOST_SYSTEM_PROCESSOR} EQUAL "armv7l")
	message(STATUS "Adding wiringpi for raspberry pi gpio")
    find_library(wiringPi_LIB wiringPi)
    list(APPEND DEPENDENCIES wiringPi)
	list(APPEND DEFINITIONS RASPI=1)
else()
    message(STATUS "Using fake IO instead of wiringPI")
endif()


set(SOURCES
	# main
	"${CMAKE_CURRENT_LIST_DIR}/src/main.cpp"

	"${CMAKE_CURRENT_LIST_DIR}/src/MotorController.hpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/MotorController.cpp"

	"${CMAKE_CURRENT_LIST_DIR}/src/StopWatch.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/StopWatch.cpp"

	"${CMAKE_CURRENT_LIST_DIR}/src/Server.hpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/Server.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/Client.hpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/Client.cpp"

	"${CMAKE_CURRENT_LIST_DIR}/src/StepperMotor.hpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/StepperMotor.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/Focuser.hpp"
	"${CMAKE_CURRENT_LIST_DIR}/src/Focuser.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_compile_definitions(${PROJECT_NAME}  PUBLIC ${DEFINITIONS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${DEPENDENCIES})