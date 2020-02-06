#ifndef Globals_H
#define Globals_H

#include "Imports.h"
#include "TarkovSDK.h"

namespace Globals
{
	static int Window_width = 2560;
	static int Window_height = 1080;

	static bool RunReaderLoop = true;
	static bool RunRenderLoop = true;
	static bool EnablePlayerESP = true;
	static bool EnableNoRecoilOrSway = true;

	static uintptr_t m_BaseAddress = 0;

	static FOverlay* m_Overlay;
	static uint64_t m_ObjectManager = 0;
	static uint64_t m_GameWorld = 0;
	static uint64_t m_LocalGameWorld = 0;
	static uint64_t m_FpsCamera = 0;
	static uint64_t m_OpticCamera = 0;
	
	static float m_FpsFovX = 0.f;
	static float m_FpsFovY = 0.f;

	static D3DXMATRIX m_FpsMatrix { };
	static D3DXMATRIX m_OpticMatrix { };
}

namespace Offsets
{
	// Globals
	#define _OFFSET_GAMEOBJMANAGER_ 0x15181E8


	// GameObject
	#define _OFFSET_GAMEOBJECT_NAME_ 0x60

	// LocalGameWorld
	#define _OFFSET_LGW_CHAIN_1 0x30
	#define _OFFSET_LGW_CHAIN_2 0x18
	#define _OFFSET_LGW_CHAIN_3 0x28

	#define _OFFSET_LGW_REGISTEREDPLAYERS_ 0x68 // 0x68??

	// Camera
	#define _OFFSET_CAMERA_OBJECT_ 0x30
	#define _OFFSET_CAMERA_ENTITY 0x18
	#define _OFFSET_CAMERA_VIEWMATRIX 0xD8

	#define _OFFSET_CAMERA_FOV_Y 0x118
	#define _OFFSET_CAMERA_FOV_X 0x12C

	// Player
	#define _OFFSET_PLAYER_LOCALCHECK_ 0x18
	#define _OFFSET_PLAYER_PROCEDWEAPONANIM_ 0x70
	#define _OFFSET_PLAYER_AIMINDEX_ 0x108
}

struct TransformAccessReadOnly
{
	ULONGLONG	pTransformData;
	int			index;
};

struct TransformData
{
	char pad[24];
	ULONGLONG pTransformArray;
	ULONGLONG pTransformIndices;
};

struct Matrix34
{
	Vector4 vec0;
	Vector4 vec1;
	Vector4 vec2;
};
#endif