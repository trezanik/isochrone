#!/bin/sh

core_lines=$(awk -v s="#	define" 'index($0, s) == 1' ./src/core/definitions.h | awk '!/_API/' | sed 's/[ \t]*//')
imgui_lines=$(awk -v s="#	define" 'index($0, s) == 1' ./src/imgui/definitions.h | awk '!/_API/')
engine_lines=$(awk -v s="#	define" 'index($0, s) == 1' ./src/engine/definitions.h | awk '!/_API/')
app_lines=$(awk -v s="#	define" 'index($0, s) == 1' ./src/app/definitions.h | awk '!/_API/')

cat << EOF > configure.h
#pragma once

/*
 * This header is automatically generated and not included by default.
 *
 * It presents the items that can have an override to the defaults; force the
 * include via the CMakeLists option ISOCHRONE_FORCE_INCLUDE_CONFIGURE.
 *
 * See the associated headers for descriptions - and only modify if you know
 * what you're doing!
 */
 
//===========================
// core/definitions.h

$core_lines

//===========================
// imgui/definitions.h

$imgui_lines

//===========================
// engine/definitions.h

$engine_lines

//===========================
// app/definitions.h

$app_lines

EOF
