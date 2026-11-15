cmake_minimum_required(VERSION 3.16)
project(SCAPMS VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Core library: model, controller and utilities (plain C++, no GUI code)
add_library(scapms_core STATIC
    model/Appointment.h model/Appointment.cpp
    model/Appointable.h
    model/PatientRecord.h model/PatientRecord.cpp
    model/GeneralPatient.h model/GeneralPatient.cpp
    model/EmergencyPatient.h model/EmergencyPatient.cpp
    model/FollowUpPatient.h model/FollowUpPatient.cpp
    model/ClinicDatabase.h model/ClinicDatabase.cpp
    controller/Sorter.h controller/Sorter.cpp
    controller/SearchEngine.h controller/SearchEngine.cpp
    controller/ClinicManager.h controller/ClinicManager.cpp
    controller/AppointmentController.h controller/AppointmentController.cpp
    controller/ReportGenerator.h controller/ReportGenerator.cpp
    utils/Exceptions.h utils/StringUtils.h utils/IDGenerator.h
    utils/FileHandler.h utils/FileHandler.cpp
)
target_include_directories(scapms_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# GUI application (Qt Widgets)
add_executable(SCAPMS
    main.cpp
    gui/GuiUtils.h gui/BasePanel.h
    gui/MainWindow.h gui/MainWindow.cpp
    gui/DashboardPanel.h gui/DashboardPanel.cpp
    gui/PatientPanel.h gui/PatientPanel.cpp
    gui/PatientDialog.h gui/PatientDialog.cpp
    gui/AppointmentPanel.h gui/AppointmentPanel.cpp
    gui/QueuePanel.h gui/QueuePanel.cpp
    gui/SearchPanel.h gui/SearchPanel.cpp
    gui/ReportPanel.h gui/ReportPanel.cpp
    gui/AdminPanel.h gui/AdminPanel.cpp
)
target_link_libraries(SCAPMS PRIVATE scapms_core Qt6::Widgets)

if(MSVC)
    target_compile_options(scapms_core PRIVATE /W4)
else()
    target_compile_options(scapms_core PRIVATE -Wall -Wextra)
    target_compile_options(SCAPMS PRIVATE -Wall -Wextra)
endif()

# Copy the sample data file next to the executable
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
