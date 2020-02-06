#pragma once
#pragma comment(lib, "Dwrite")
#pragma comment(lib, "Dwmapi.lib") 
#pragma comment(lib, "d2d1.lib")
#pragma comment (lib, "d3dx9.lib")

#include <msxml.h>    
#include <atomic>
#include <mutex>
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include <vector>
#include <array>
#include <random>
#include <memoryapi.h>
#include <string>
#include <thread>
#include <chrono>
#include <sstream>
#include <codecvt>
#include <iostream>

// overlay
#include <dcommon.h>
#include <d3dx9math.h>
#include <dwmapi.h> 
#include <d2d1.h>
#include <dwrite.h>
#include "Vectors.h"
#include "drawing/FOverlay.h"

// sdk
#include "driver.h"
#include "globals.h"

// string xor
#include "xor.h"