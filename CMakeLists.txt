cmake_minimum_required(VERSION 3.12)

project(DragonFlyReverbSuperColliderPort LANGUAGES CXX)

option(SCSYNTH "Build the scsynth plugin target" ON)
option(SUPERNOVA "Build the supernova plugin target" OFF)
option(STRICT "Enable strict compiler warnings" OFF)
option(NATIVE "Enable -march=native where supported" OFF)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT SC_PATH)
    set(SC_PATH "../chow dsp to sc codex/supercollider")
endif()

get_filename_component(SC_PATH_FOR_MODULES "${SC_PATH}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

list(APPEND CMAKE_MODULE_PATH "${SC_PATH_FOR_MODULES}/tools/cmake_gen")

include(SuperColliderServerPlugin)
sc_do_initial_compiler_config()
sc_check_sc_path("${SC_PATH}")

set(dragonflyreverbdf_cpp_files
    src/DragonflyReverbDF.cpp
    upstream/dragonfly-reverb/common/freeverb/allpass.cpp
    upstream/dragonfly-reverb/common/freeverb/biquad.cpp
    upstream/dragonfly-reverb/common/freeverb/comb.cpp
    upstream/dragonfly-reverb/common/freeverb/delay.cpp
    upstream/dragonfly-reverb/common/freeverb/delayline.cpp
    upstream/dragonfly-reverb/common/freeverb/earlyref.cpp
    upstream/dragonfly-reverb/common/freeverb/efilter.cpp
    upstream/dragonfly-reverb/common/freeverb/nrev.cpp
    upstream/dragonfly-reverb/common/freeverb/nrevb.cpp
    upstream/dragonfly-reverb/common/freeverb/progenitor.cpp
    upstream/dragonfly-reverb/common/freeverb/progenitor2.cpp
    upstream/dragonfly-reverb/common/freeverb/revbase.cpp
    upstream/dragonfly-reverb/common/freeverb/slot.cpp
    upstream/dragonfly-reverb/common/freeverb/strev.cpp
    upstream/dragonfly-reverb/common/freeverb/utils.cpp
    upstream/dragonfly-reverb/common/freeverb/zrev.cpp
    upstream/dragonfly-reverb/common/freeverb/zrev2.cpp
)

set(dragonflyreverbdf_sc_files
    Classes/EarlyReflectionsDF.sc
    Classes/HallDF.sc
    Classes/PlateDF.sc
    Classes/RoomDF.sc
)

set(dragonflyreverbdf_schelp_files
    HelpSource/Classes/EarlyReflectionsDF.schelp
    HelpSource/Classes/HallDF.schelp
    HelpSource/Classes/PlateDF.schelp
    HelpSource/Classes/RoomDF.schelp
)

sc_add_server_plugin(
    "DragonflyReverbDF"
    "DragonflyReverbDF"
    "${dragonflyreverbdf_cpp_files}"
    "${dragonflyreverbdf_sc_files}"
    "${dragonflyreverbdf_schelp_files}"
)

if(TARGET DragonflyReverbDF_scsynth)
    set_target_properties(DragonflyReverbDF_scsynth PROPERTIES OUTPUT_NAME "DragonflyReverbDF")
    target_include_directories(DragonflyReverbDF_scsynth PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/upstream/dragonfly-reverb/common"
    )
    target_compile_definitions(DragonflyReverbDF_scsynth PRIVATE LIBFV3_FLOAT)
endif()

if(TARGET DragonflyReverbDF_supernova)
    set_target_properties(DragonflyReverbDF_supernova PROPERTIES OUTPUT_NAME "DragonflyReverbDF_supernova")
    target_include_directories(DragonflyReverbDF_supernova PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/upstream/dragonfly-reverb/common"
    )
    target_compile_definitions(DragonflyReverbDF_supernova PRIVATE LIBFV3_FLOAT)
endif()

install(FILES HelpSource/Guides/DragonFlyReverb.schelp DESTINATION DragonflyReverbDF/HelpSource/Guides)
