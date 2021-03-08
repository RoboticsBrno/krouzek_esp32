cmake_minimum_required(VERSION 3.10)

# set the project name
project(HelloWorld
    DESCRIPTION "Super projekt!"
    VERSION 1.0.0)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(MOJE_VARIABLE "Nic" STRING "SuperNejPromena")

#Options
option(TEST_OPTION "Jste skveli?" OFF)

# configure a header file to pass the version number only
configure_file(Config.h.in Config.h)

# add the executable
add_executable(main hello.cpp)

# add Config.h directory to build
target_include_directories(main PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           )
