# ==============================================================================
# WOLS CA OUTLOOK ORGANISER (Qt6 NATIVE)
# ==============================================================================

cmake_minimum_required(VERSION 3.16)

# De projectnaam is nu correct ingesteld op Organiser
project(Wols_CA_OutlookOrganiser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)

if(MSVC)
    # /ZI lost de "Edit and Continue" Hot Reload waarschuwing op in Debug mode
    add_compile_options(/W3 $<$<CONFIG:Debug>:/ZI>)
endif()

# ==============================================================================
# QT6 CONFIGURATIE
# ==============================================================================
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_PREFIX_PATH "C:/Qt/6.11.1/msvc2022_64" ${CMAKE_PREFIX_PATH})
find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

# ==============================================================================
# BESTANDEN EN DOEL (TARGET)
# ==============================================================================
set(SOURCES
    # --- Hoofdbestand (Direct in src/) ---
    Wols_CA_OutlookOrganiser/src/Wols_CA_OutlookOrganiser.cpp

    # --- Core ---
    Wols_CA_OutlookOrganiser/src/Core/ConfigManager.cpp
    Wols_CA_OutlookOrganiser/src/Core/Logger.cpp


    # --- Outlook Engine (De classes heten intern nog Manager) ---
    Wols_CA_OutlookOrganiser/src/Outlook/OutlookManager_Core.cpp
    Wols_CA_OutlookOrganiser/src/Outlook/OutlookManager_Data.cpp
    Wols_CA_OutlookOrganiser/src/Outlook/OutlookManager_Engine.cpp
    Wols_CA_OutlookOrganiser/src/Outlook/OutlookManager_Router.cpp

    # --- Rules ---
    Wols_CA_OutlookOrganiser/src/Rules/RulesEngine.cpp
    Wols_CA_OutlookOrganiser/src/Rules/RulesManager.cpp

    # --- UI ---
    Wols_CA_OutlookOrganiser/src/UI/FolderConfigDialog.cpp
    Wols_CA_OutlookOrganiser/src/UI/MainWindow.cpp
    Wols_CA_OutlookOrganiser/src/UI/RuleEditorDialog.cpp
    
    # --- Headers (.hpp) ---
    Wols_CA_OutlookOrganiser/src/Include/ConfigManager.hpp
    Wols_CA_OutlookOrganiser/src/Include/FolderConfigDialog.hpp
    Wols_CA_OutlookOrganiser/src/Include/MainWindow.hpp
    Wols_CA_OutlookOrganiser/src/Include/OutlookManager.hpp
    Wols_CA_OutlookOrganiser/src/Include/resource.h
    Wols_CA_OutlookOrganiser/src/Include/RuleEditorDialog.hpp
    Wols_CA_OutlookOrganiser/src/Include/RuleModels.hpp
    Wols_CA_OutlookOrganiser/src/Include/RulesEngine.hpp
    Wols_CA_OutlookOrganiser/src/Include/RulesManager.hpp

    # --- Resources (.qrc) ---
    Wols_CA_OutlookOrganiser/src/resources.qrc
)

# Koppel de executable met de broncode en het Windows icoon (.rc) bestand
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} Wols_CA_OutlookOrganiser/src/Resource/resource.rc )
# Vertel CMake waar de header bestanden staan (Let op de hoofdletter I)
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/Wols_CA_OutlookOrganiser/src/Include
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets
    ole32
    oleaut32
    Qt6::AxContainer
)


# ==============================================================================
# AUTOMATISCHE DEPLOYMENT (POST-BUILD)
# ==============================================================================
# Voer windeployqt automatisch uit na een succesvolle build om de DLL's te kopiëren
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND "C:/Qt/6.11.1/msvc2022_64/bin/windeployqt.exe"
    ARGS "$<TARGET_FILE:${PROJECT_NAME}>"
    COMMENT "Wols CA Auto-Deploy: Kopiëren van vereiste Qt6 DLL's naar de build map..."
)