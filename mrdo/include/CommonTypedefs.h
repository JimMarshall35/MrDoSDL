#pragma once
#include <stdint.h>

#define A_SMALL_NUMBER 0.1f
//#define ReplayValidator // uncomment to build a headless replay validator app - takes file name of replay in replays folder and expected score.
						  // returns 0 if expected score matches after running the replay and 1 if it doesn't
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;