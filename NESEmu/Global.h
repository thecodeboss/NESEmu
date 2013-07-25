#ifndef Global_h__
#define Global_h__

#include <iostream>
#include <fstream>
#include <SDL.h>
#undef main

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef unsigned int u32;

template<u32 bitnum, u32 numbits=1, typename T=u8>
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
