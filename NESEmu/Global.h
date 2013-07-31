#ifndef Global_h__
#define Global_h__

#include <iostream>
#include <fstream>
#include <SDL.h>
#undef main

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef signed char int8;
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

#endif // Global_h__
