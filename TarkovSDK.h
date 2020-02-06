#ifndef TARKOVSDK_H
#define TARKOVSDK_H

#include "Imports.h"

namespace Structs
{
	struct GameObjectManager
	{
		uint64_t	lastTaggedObject; //0x0000
		uint64_t	taggedObjects; //0x0008
		uint64_t	lastActiveObject; //0x0010
		uint64_t	activeObjects; //0x0018
	};

	struct BaseObject
	{
		uint64_t	previousBaseObject; //0x0000
		uint64_t	nextBaseObject; //0x0008
		uint64_t	gameObject; //0x0010
	};

	class ListInternal
	{
	public:
		char		pad_0x0000[0x20]; //0x0000
		uintptr_t*	firstEntry; //0x0020 
	}; //Size=0x0028

	class List
	{
	public:
		char			pad_0x0000[0x10]; //0x0000
		ListInternal*	listBase; //0x0010 
		__int32			itemCount; //0x0018 
		// 
	}; //Size=0x001C

	struct Player
	{
		uintptr_t		address;
		
		int				type;

		bool			isAiming;
		bool			isScoped = false;

		std::wstring	name;
		D3DXVECTOR3		headPosition;
		D3DXVECTOR3		fastPosition;
	};

	enum PType
	{
		SCAV,
		PLAYER_SCAV,
		PLAYER
	};

	enum Bones
	{
		HumanBase = 0,
		HumanPelvis = 14,
		HumanLThigh1 = 15,
		HumanLThigh2 = 16,
		HumanLCalf = 17,
		HumanLFoot = 18,
		HumanLToe = 19,
		HumanRThigh1 = 20,
		HumanRThigh2 = 21,
		HumanRCalf = 22,
		HumanRFoot = 23,
		HumanRToe = 24,
		HumanSpine1 = 29,
		HumanSpine2 = 36,
		HumanSpine3 = 37,
		HumanLCollarbone = 89,
		HumanLUpperarm = 90,
		HumanLForearm1 = 91,
		HumanLForearm2 = 92,
		HumanLForearm3 = 93,
		HumanLPalm = 94,
		HumanRCollarbone = 110,
		HumanRUpperarm = 111,
		HumanRForearm1 = 112,
		HumanRForearm2 = 113,
		HumanRForearm3 = 114,
		HumanRPalm = 115,
		HumanNeck = 132,
		HumanHead = 133
	};
}

class TarkovSDK 
{
	public:
		static TarkovSDK* Instance();

		bool ReadGameData();
		bool Init(uint64_t baseAddress);
		bool WorldToScreen(D3DXVECTOR3& point3D, D3DXVECTOR3& point2D, bool isScoped = false);

		bool GetCameraData(uint64_t address, bool isScoped = false);

		float GetDistance(D3DXVECTOR3 enemyPos, D3DXVECTOR3 playerPos);

		D3DXVECTOR3 GetPosition(uint64_t transform);

		uint64_t FindActiveObject(const char* name);
		uint64_t FindTaggedObject(const char* name);

		std::vector<Structs::Player> Players;
		D3DXMATRIX ViewMatrix;
		D3DXMATRIX OpticViewMatrix;
		Structs::Player LocalPlayer;
};

#endif

