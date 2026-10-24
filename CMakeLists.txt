cmake_minimum_required(VERSION 3.24)
project(Pomodoro VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 6.11 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Widgets Multimedia Network Test)
qt_standard_project_setup(REQUIRES 6.11)
if(MSVC)
    add_compile_options(/utf-8 /W4 /permissive-)
endif()
qt_add_library(pomodoro_core STATIC
    src/TimerEngine.cpp src/TimerEngine.h src/DataStore.cpp src/DataStore.h
    src/DailyStatistics.cpp src/DailyStatistics.h)
target_link_libraries(pomodoro_core PUBLIC Qt6::Core)
target_include_directories(pomodoro_core PUBLIC src)
qt_add_executable(Pomodoro WIN32
    src/main.cpp src/AppController.cpp src/AppController.h
    src/PowerMonitor.cpp src/PowerMonitor.h src/ChimePlayer.cpp src/ChimePlayer.h assets/app.rc)
target_link_libraries(Pomodoro PRIVATE pomodoro_core Qt6::Gui Qt6::Qml Qt6::Quick
    Qt6::QuickControls2 Qt6::Widgets Qt6::Multimedia Qt6::Network)
qt_add_qml_module(Pomodoro URI PomodoroApp VERSION 1.0
    QML_FILES qml/Main.qml qml/AppButton.qml qml/WindowControlButton.qml qml/ResizeHandle.qml qml/PhaseTabs.qml qml/RoundedSpinBox.qml qml/TomatoDial.qml
)
qt_add_resources(Pomodoro "media" PREFIX "/assets" BASE "assets"
    FILES assets/chime.wav assets/app-icon.png assets/app.ico assets/cycle-tomato.png assets/cycle-tomato-pending.png)

# Qt Creator ships the matching Qt 6.11.1 shader baking tool.
set(POMODORO_QSB "E:/Qt/Tools/QtCreator/bin/qsb.exe" CACHE FILEPATH "Qt shader baker")
if(NOT EXISTS "${POMODORO_QSB}")
    message(FATAL_ERROR "Qt qsb tool is required; set POMODORO_QSB to its path.")
endif()
set(POMODORO_GLASS_SHADER "${CMAKE_CURRENT_BINARY_DIR}/shaders/liquidglass.frag.qsb")
add_custom_command(OUTPUT "${POMODORO_GLASS_SHADER}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
    COMMAND "${POMODORO_QSB}" --glsl "100 es,120,150" --hlsl 50 --msl 12
        -o "${POMODORO_GLASS_SHADER}" "${CMAKE_CURRENT_SOURCE_DIR}/shaders/liquidglass.frag"
    DEPENDS shaders/liquidglass.frag
    VERBATIM)
qt_add_resources(Pomodoro "glass_shaders" PREFIX "/shaders"
    BASE "${CMAKE_CURRENT_BINARY_DIR}/shaders"
    FILES "${POMODORO_GLASS_SHADER}")
set_source_files_properties(assets/app.rc PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/app.ico")
target_include_directories(Pomodoro PRIVATE assets)
set_target_properties(Pomodoro PROPERTIES OUTPUT_NAME Pomodoro)
include(CTest)
if(BUILD_TESTING)
    qt_add_executable(PomodoroTests tests/tst_pomodoro.cpp)
    target_link_libraries(PomodoroTests PRIVATE pomodoro_core Qt6::Test)
    add_test(NAME pomodoro-core COMMAND PomodoroTests)
    set_tests_properties(pomodoro-core PROPERTIES ENVIRONMENT "POMODORO_TEST_ROOT=E:/alarm/work/test-data")
    qt_add_executable(PomodoroUiTests tests/tst_ui.cpp src/AppController.cpp src/AppController.h
        src/PowerMonitor.cpp src/PowerMonitor.h src/ChimePlayer.cpp src/ChimePlayer.h)
    target_link_libraries(PomodoroUiTests PRIVATE pomodoro_core Qt6::Gui Qt6::Qml Qt6::Quick
        Qt6::QuickControls2 Qt6::Widgets Qt6::Multimedia Qt6::Network Qt6::Test)
    target_compile_definitions(PomodoroUiTests PRIVATE POMODORO_SOURCE_DIR="E:/alarm")
    qt_add_resources(PomodoroUiTests "test_media" PREFIX "/assets" BASE "assets"
        FILES assets/chime.wav assets/app-icon.png assets/app.ico assets/cycle-tomato.png assets/cycle-tomato-pending.png)
    qt_add_resources(PomodoroUiTests "test_glass_shaders" PREFIX "/shaders"
        BASE "${CMAKE_CURRENT_BINARY_DIR}/shaders"
        FILES "${POMODORO_GLASS_SHADER}")
    add_test(NAME pomodoro-ui COMMAND PomodoroUiTests interfaceFlow)
    set_tests_properties(pomodoro-ui PROPERTIES ENVIRONMENT
        "POMODORO_TEST_ROOT=E:/alarm/work/test-data;QPA_PLATFORM=offscreen;QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")
    qt_add_executable(PomodoroAudioTests tests/tst_audio.cpp
        src/AppController.cpp src/AppController.h src/ChimePlayer.cpp src/ChimePlayer.h)
    target_link_libraries(PomodoroAudioTests PRIVATE pomodoro_core Qt6::Gui Qt6::Qml Qt6::Quick
        Qt6::QuickControls2 Qt6::Widgets Qt6::Multimedia Qt6::Network Qt6::Test)
    qt_add_resources(PomodoroAudioTests "audio_test_media" PREFIX "/assets" BASE "assets"
        FILES assets/chime.wav assets/app.ico)
    add_test(NAME pomodoro-audio COMMAND PomodoroAudioTests)
    set_tests_properties(pomodoro-audio PROPERTIES ENVIRONMENT
        "POMODORO_TEST_ROOT=E:/alarm/work/audio-test-data")
endif()
install(TARGETS Pomodoro RUNTIME DESTINATION .)
