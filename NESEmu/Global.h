#ifndef Global_h__
#define Global_h__

#define _USE_MATH_DEFINES

#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include <cstdlib>
#include <stdint.h>
#include <SDL.h>
#undef main

typedef uint_least8_t uint8;
typedef uint_least16_t uint16;
typedef uint_least32_t uint32;
typedef unsigned long long uint64;

typedef int_least8_t int8;
typedef signed short int16;
typedef signed int int32;
typedef signed long long int64;

template<uint32 bitnum, uint32 numbits=1, typename T=uint8>
struct Bit {
	T data;
	enum {
		mask = (1u << numbits) - 1u
	};
	template<typename T2>
	Bit& operator=(T2 value) {
		data = (data & ~(mask << bitnum)) | ((numbits > 1 ? value&mask : !!value) << bitnum);
		return *this;
	}
	operator unsigned() const { return (data >> bitnum) & mask; }
	Bit& operator++ () { return *this = *this + 1; }
	unsigned operator++ (int) { unsigned r = *this; ++*this; return r; }
};

char ToHex(uint8 in);

void PrintHex(uint8 in);

void ReadHex(std::ifstream& input, uint8& output);

#endif // Global_h__
