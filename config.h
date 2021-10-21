#ifndef __SYS_H__

#define __PC__      1
#define PICO        0
#define XIAO        1
#define __SERIAL__  1
#define TIB_SZ     80

#if __PC__
	#define USER_SZ       (512*1024)
	#define NUM_FUNCS     (MAX_FUNCS)
	#define NUM_REGS      (MAX_REGS)
	#ifdef _WIN32
			#define _CRT_SECURE_NO_WARNINGS
			#include <Windows.h>
			#include <conio.h>
		#else
			#define __LINUX__
			#include <unistd.h>
		#endif
#else
	#include <Arduino.h>
	#if PICO
		#define mySerial SerialUSB
		#define USER_SZ      (64*1024)
		#define NUM_FUNCS    (26*26)
		#define NUM_REGS     (26*26)
		#define ILED          25
	#elif XIAO
		#define mySerial SerialUSB
		#define USER_SZ      (24*1024)
		#define NUM_FUNCS    (26*3)
		#define NUM_REGS     (26*3)
		#define ILED          13
	#endif
#endif // __PC__

#endif // __SYS_H__
