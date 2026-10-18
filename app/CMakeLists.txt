cmake_minimum_required(VERSION 3.24)

project(PzAnimator VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.10 REQUIRED COMPONENTS Gui Quick QuickControls2 Quick3D Network)
find_package(assimp CONFIG REQUIRED)

qt_standard_project_setup(REQUIRES 6.10)

# Shared, GUI-free code: protocol, take files, PZ mesh loading, GLB export.
add_library(pzcore STATIC
    src/protocol/Protocol.h
    src/protocol/Protocol.cpp
    src/scene/PzSpace.h
    src/export/TakeFile.h
    src/export/TakeFile.cpp
    src/export/BodyMesh.h
    src/export/BodyMesh.cpp
    src/export/GlbExporter.h
    src/export/GlbExporter.cpp
)
target_include_directories(pzcore PUBLIC src)
target_link_libraries(pzcore PUBLIC Qt6::Gui PRIVATE assimp::assimp)

# Command-line exporter: pzexport <take.pzrec> [out.glb] ...
qt_add_executable(pzexport tools/pzexport/main.cpp)
target_link_libraries(pzexport PRIVATE pzcore)

qt_add_executable(PzAnimator WIN32
    src/main.cpp
)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(PzAnimator
    URI PzAnimator
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/TakesPane.qml
        qml/AeroButton.qml
        qml/AeroCheckBox.qml
        qml/AeroRadioButton.qml
        qml/AeroTextField.qml
        qml/AeroGroupBox.qml
        qml/AeroProgressBar.qml
        qml/AeroIcon.qml
    SOURCES
        src/net/AgentClient.h
        src/net/AgentClient.cpp
        src/scene/PoseState.h
        src/scene/PoseState.cpp
        src/scene/BoneGeometry.h
        src/scene/BoneGeometry.cpp
        src/assets/PzAssets.h
        src/assets/PzAssets.cpp
        src/app/AppSettings.h
        src/app/AppSettings.cpp
        src/app/GameLauncher.h
        src/app/GameLauncher.cpp
        src/app/TakeExporter.h
        src/app/TakeExporter.cpp
        src/app/TakeLibrary.h
        src/app/TakeLibrary.cpp
        src/platform/GlobalHotkey.h
        src/platform/GlobalHotkey.cpp
)

# The generated QML type registrations include QML_ELEMENT headers by bare file name.
target_include_directories(PzAnimator PRIVATE src src/app src/assets src/net src/scene)
target_compile_definitions(PzAnimator PRIVATE PROJECT_VERSION="${PROJECT_VERSION}")

target_link_libraries(PzAnimator PRIVATE
    pzcore
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Quick3D
    Qt6::Network
)

if(WIN32)
    target_link_libraries(PzAnimator PRIVATE user32)
    target_compile_definitions(PzAnimator PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    # Copy the Qt runtime and QML imports next to the exe so it runs straight from the build folder.
    add_custom_command(TARGET PzAnimator POST_BUILD
        COMMAND Qt6::windeployqt --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/qml" --no-translations
                --no-compiler-runtime "$<TARGET_FILE:PzAnimator>"
        VERBATIM)
    # pzexport sits next to PzAnimator.exe and reuses its deployed Qt/Assimp DLLs.
    set_target_properties(pzexport PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release"
                                              RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug")
endif()
