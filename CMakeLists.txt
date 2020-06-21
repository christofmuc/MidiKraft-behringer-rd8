#
#  Copyright (c) 2019 Christof Ruch. All rights reserved.
#
#  Dual licensed: Distributed under Affero GPL license by default, an MIT license is available for purchase
#

cmake_minimum_required(VERSION 3.14)

project(MidiKraft-Behringer-RD8)

# Find JUCE, and add the components we need
IF(NOT DEFINED JUCE_LIBRARIES)
	SET(JUCE_DIR_HINT "${CMAKE_CURRENT_LIST_DIR}/../third_party")
	find_package(JUCE REQUIRED 
		COMPONENTS 
			juce_core		
			juce_events
			juce_audio_basics
			juce_audio_devices
			juce_data_structures
	)
ENDIF()

# Define the sources for the static library
set(Sources
	RD8.h RD8.cpp
	RD8Pattern.h RD8Pattern.cpp
	README.md
	LICENSE.md
)

# Setup library
add_library(midikraft-behringer-rd8 ${Sources})
target_include_directories(midikraft-behringer-rd8 PUBLIC ${CMAKE_CURRENT_LIST_DIR} PRIVATE ${JUCE_INCLUDES})
target_link_libraries(midikraft-behringer-rd8 juce-utils midikraft-base)

# Pedantic about warnings
if (MSVC)
    # warning level 4 and all warnings as errors
    target_compile_options(midikraft-behringer-rd8 PRIVATE /W4 /WX)
else()
    # lots of warnings and all warnings as errors
    #target_compile_options(midikraft-behringer-rd8 PRIVATE -Wall -Wextra -pedantic -Werror)
endif()
