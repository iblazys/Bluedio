#include "Imports.h"
#include "TarkovSDK.h"

TarkovSDK* TarkovSDK::Instance()
{
	static TarkovSDK instance;
	return &instance;
}

bool TarkovSDK::ReadGameData()
{
	this->Players.clear();
	{
		uint64_t registeredPlayers = Read<uint64_t>(Globals::m_LocalGameWorld + _OFFSET_LGW_REGISTEREDPLAYERS_); // TODO: LGW Struct

		if (!registeredPlayers)
			return false;

		uint64_t list_base = Read<uint64_t>(registeredPlayers + offsetof(Structs::List, listBase));
		uint32_t player_count = Read<uint32_t>(registeredPlayers + offsetof(Structs::List, itemCount));

		if (player_count <= 0 || !list_base)
			return false;

		constexpr auto BUFFER_SIZE = 128;
		if (player_count > BUFFER_SIZE)
			std::runtime_error(xorstr_("Too many players, extend buffer."));

		uint64_t player_buffer[BUFFER_SIZE];

		ReadToBuffer(list_base + offsetof(Structs::ListInternal, firstEntry), player_buffer, sizeof(uint64_t) * player_count);

		Structs::Player player;
		for (uint32_t i = 0; i < player_count; i++)
		{
			player.address = player_buffer[i];

			// Get bone position
			uint64_t body = Read<uint64_t>(player.address + 0x88);
			uint64_t skeleton = Read<uint64_t>(body + 0x28);
			uint64_t boneEnum = Read<uint64_t>(skeleton + 0x28);
			uint64_t transformArray = Read<uint64_t>(boneEnum + 0x10);
			uint64_t bone_transform = Read<uint64_t>(transformArray + (0x20 + Structs::Bones::HumanHead * 0x8));
			uint64_t bone_transform_internal = Read<uint64_t>(bone_transform + 0x10);
			uint64_t matrices = Read<uint64_t>(bone_transform_internal + 0x38);
			
			if(matrices)
			{
				player.fastPosition = ReadStruct<D3DXVECTOR3>(matrices + 0xB0);
				player.headPosition = GetPosition(bone_transform);

				//printf(xorstr_("[+] FastPos: X: %f, Y: %f, Z: %f\n"), player.fastPosition.x, player.fastPosition.y, player.fastPosition.z);
			}

			uint64_t playerProfile = Read<uint64_t>(player.address + 0x3A0);
			uint64_t playerInfo = Read<uint64_t>(playerProfile + 0x28);
			uint64_t playerName = Read<uint64_t>(playerInfo + 0x10);

			if (!playerName)
				continue;

			// Get name
			uint32_t nameLength = Read<uint32_t>(playerName + 0x10);

			player.name.resize(nameLength + 1);

			ReadToBuffer(playerName + 0x14, (uint64_t*)player.name.data(), nameLength * 2 + 2);

			// Get player type
			uint32_t creationDate = Read<uint32_t>(playerInfo + 0x54);
			uint64_t playerId = Read<uint64_t>(playerProfile + 0x20);
			uint32_t isPlayer = Read<uint32_t>(playerId + 0x10);

			if (playerId == 0x0 && creationDate == 0x0)
				player.type = Structs::SCAV;

			if (creationDate != 0x0 && playerId == 0x0)
				player.type = Structs::PLAYER_SCAV;

			if (playerId != 0x0)
				player.type = Structs::PLAYER;

			// LOCAL_PLAYER
			if (Read<uint64_t>(player.address + _OFFSET_PLAYER_LOCALCHECK_) != 0x0) 
			{
				// IsAimingCheck
				uint64_t pProceduralWeaponAnimation = Read<uint64_t>(player.address + _OFFSET_PLAYER_PROCEDWEAPONANIM_);
				uint64_t pBreathEffector = Read<uint64_t>(pProceduralWeaponAnimation + 0x28);
				player.isAiming = Read<bool>(pBreathEffector + 0x88);

				if (player.isAiming)
				{
					uint64_t opticSightArray = Read<uint64_t>(pProceduralWeaponAnimation + 0x88);
					uint64_t opticCamera = Read<uint64_t>(opticSightArray + 0x20);
					uint64_t opticCameraRaw = Read<uint64_t>(opticCamera + 0x28);

					Globals::m_OpticCamera = opticCameraRaw;

					if (opticCamera)
						player.isScoped = true;
				}

				if (Globals::EnableNoRecoilOrSway) 
				{
					Write<uint32_t>(pProceduralWeaponAnimation + 0xE0, (uint32_t)4);
				}

				// Store local player after collecting data
				this->LocalPlayer = player;
			}

			this->Players.emplace_back(player);
		};
	}

	// Camera shit - READ VIEW MATRIXES and fov
	{
		this->GetCameraData(Globals::m_FpsCamera);

		if (this->LocalPlayer.isAiming && this->LocalPlayer.isScoped)
			this->GetCameraData(Globals::m_OpticCamera, true);
	}

	return true;
}

/*
 * Initialization will only complete when we are in the game / loading screen.
 */
bool TarkovSDK::Init(uint64_t baseAddress)
{
	Globals::m_ObjectManager = Read<uint64_t>(baseAddress + _OFFSET_GAMEOBJMANAGER_);

	if (!Globals::m_ObjectManager)
		return false;
	
	printf(xorstr_("[+] Found GameObjectManager: 0x%I64X\n"), Globals::m_ObjectManager);

	// Two reads at once
	auto active_objects = ReadStruct<std::array<uint64_t, 2>>(Globals::m_ObjectManager + offsetof(Structs::GameObjectManager, lastActiveObject));

	if (!active_objects[0] || !active_objects[1])
		return false;

	// Constantly search for game world every 3 seconds.
	do 
	{
		Globals::m_GameWorld = FindActiveObject(xorstr_("GameWorld"));
		printf(xorstr_("[+] Waiting for GameWorld....\n"));
		Sleep(1000);
	} 
	while (Globals::m_GameWorld == 0x0);

	printf(xorstr_("[+] Found GameWorld: 0x%I64X\n"), Globals::m_GameWorld);

	do
	{
		Sleep(1000);

		auto temp = Read<uint64_t>(Globals::m_GameWorld + _OFFSET_LGW_CHAIN_1);
		temp = Read<uint64_t>(temp + _OFFSET_LGW_CHAIN_2);
		Globals::m_LocalGameWorld = Read<uint64_t>(temp + _OFFSET_LGW_CHAIN_3);
		
		printf(xorstr_("[+] Waiting for LocalGameWorld....\n"));
	} while (Globals::m_LocalGameWorld == 0x0);

	printf(xorstr_("[+] Found LocalGameWorld: 0x%I64X\n"), Globals::m_LocalGameWorld);

	auto tagged_objects = ReadStruct<std::array<uint64_t, 2>>(Globals::m_ObjectManager + offsetof(Structs::GameObjectManager, lastTaggedObject));

	if (!tagged_objects[0] || !tagged_objects[1])
		return false;

	if (!(Globals::m_FpsCamera = FindTaggedObject(xorstr_("FPS Camera"))))
		return false;
	
	printf(xorstr_("[+] Found FPS Camera: 0x%I64X\n"), Globals::m_FpsCamera);

	/* Camera is read in ReadData() - only exists if we have a scope
	Globals::m_OpticCamera = FindTaggedObject(xorstr_("optic_camera"));
	printf(xorstr_("[+] Optic Camera: 0x%I64X\n"), Globals::m_OpticCamera);
	*/

	return true;
}

float TarkovSDK::GetDistance(D3DXVECTOR3 enemyPos, D3DXVECTOR3 playerPos)
{
	float length;
	float distanceX;
	float distanceZ;

	distanceX = enemyPos.x - playerPos.x;
	distanceZ = enemyPos.z - playerPos.z;

	length = sqrt((distanceX * distanceX) + (distanceZ * distanceZ));

	return length;
}

uint64_t TarkovSDK::FindActiveObject(const char* name)
{
	auto active_objects = ReadStruct<std::array<uint64_t, 2>>(Globals::m_ObjectManager + offsetof(Structs::GameObjectManager, lastActiveObject));

	if (!active_objects[0] || !active_objects[1])
		return false;

	Structs::BaseObject activeObject = ReadStruct<Structs::BaseObject>(active_objects[1]);
	Structs::BaseObject lastObject = ReadStruct<Structs::BaseObject>(active_objects[0]);

	char objectName[64];
	uint64_t objectNamePtr = 0x0;

	if (activeObject.gameObject == 0x0)
	{
		return uint64_t();
	}

	while (activeObject.gameObject != 0 && activeObject.gameObject != lastObject.gameObject)
	{
		objectNamePtr = Read<uint64_t>(activeObject.gameObject + _OFFSET_GAMEOBJECT_NAME_);

		ReadToBuffer(objectNamePtr, objectName, 64);

		//printf(xorstr_("[+] Active Object: %s\n"), objectName);

		if (strcmp(objectName, name) == 0)
		{
			return activeObject.gameObject;
		}

		activeObject = ReadStruct<Structs::BaseObject>(activeObject.nextBaseObject);
	}

	return uint64_t();
}

uint64_t TarkovSDK::FindTaggedObject(const char* name)
{
	auto tagged_objects = ReadStruct<std::array<uint64_t, 2>>(Globals::m_ObjectManager + offsetof(Structs::GameObjectManager, lastTaggedObject));
	auto active_objects = ReadStruct<std::array<uint64_t, 2>>(Globals::m_ObjectManager + offsetof(Structs::GameObjectManager, lastActiveObject));

	if (!tagged_objects[0] || !tagged_objects[1])
		return false;

	char objectName[64];
	uint64_t objectNamePtr = 0x0;

	Structs::BaseObject taggedObject = ReadStruct<Structs::BaseObject>(tagged_objects[1]);
	Structs::BaseObject lastObject = ReadStruct<Structs::BaseObject>(tagged_objects[0]);
	Structs::BaseObject firstActiveObject = ReadStruct<Structs::BaseObject>(active_objects[1]);

	while (taggedObject.gameObject != firstActiveObject.gameObject) 
	{
		objectNamePtr = Read<uint64_t>(taggedObject.gameObject + _OFFSET_GAMEOBJECT_NAME_);

		ReadToBuffer(objectNamePtr, objectName, 64);

		//printf(xorstr_("[+] Tagged Object: %s\n"), objectName);

		if (strcmp(objectName, name) == 0)
		{
			return taggedObject.gameObject;
		}

		taggedObject = ReadStruct<Structs::BaseObject>(taggedObject.nextBaseObject);
	}

	return uint64_t();
}

D3DXVECTOR3 TarkovSDK::GetPosition(uint64_t boneTransform)
{
	__m128 result;

	const __m128 mulVec0 = { -2.000, 2.000, -2.000, 0.000 };
	const __m128 mulVec1 = { 2.000, -2.000, -2.000, 0.000 };
	const __m128 mulVec2 = { -2.000, -2.000, 2.000, 0.000 };

	uint64_t internalTransform = Read<uint64_t>(boneTransform + 0x10);

	uint64_t pTransformAccessReadOnly = Read<uint64_t>(internalTransform + 0x38); // matrices

	int transformAccessReadOnlyIndex = Read<int>(internalTransform + 0x40);

	uint64_t transformDataArray = Read<uint64_t>(pTransformAccessReadOnly + 0x18);
	uint64_t transformDataIndices = Read<uint64_t>(pTransformAccessReadOnly + 0x20);


	SIZE_T sizeMatriciesBuf = sizeof(Matrix34) * transformAccessReadOnlyIndex + sizeof(Matrix34);
	SIZE_T sizeIndicesBuf = sizeof(int) * transformAccessReadOnlyIndex + sizeof(int);

	// Allocate memory for storing large amounts of data (matricies and indicies)
	PVOID pMatriciesBuf = malloc(sizeMatriciesBuf);
	PVOID pIndicesBuf = malloc(sizeIndicesBuf);

	if (pMatriciesBuf && pIndicesBuf)
	{
		// Read Matricies array into the buffer
		ReadToBuffer(transformDataArray, pMatriciesBuf, sizeMatriciesBuf);
		// Read Indices array into the buffer
		ReadToBuffer(transformDataIndices, pIndicesBuf, sizeIndicesBuf);

		result = *(__m128*)((ULONGLONG)pMatriciesBuf + 0x30 * transformAccessReadOnlyIndex);
		int transformIndex = *(int*)((ULONGLONG)pIndicesBuf + 0x4 * transformAccessReadOnlyIndex);

		while (transformIndex >= 0)
		{
			Matrix34 matrix34 = *(Matrix34*)((ULONGLONG)pMatriciesBuf + 0x30 * transformIndex);

			__m128 xxxx = _mm_castsi128_ps(_mm_shuffle_epi32(*(__m128i*)(&matrix34.vec1), 0x00));	// xxxx
			__m128 yyyy = _mm_castsi128_ps(_mm_shuffle_epi32(*(__m128i*)(&matrix34.vec1), 0x55));	// yyyy
			__m128 zwxy = _mm_castsi128_ps(_mm_shuffle_epi32(*(__m128i*)(&matrix34.vec1), 0x8E));	// zwxy
			__m128 wzyw = _mm_castsi128_ps(_mm_shuffle_epi32(*(__m128i*)(&matrix34.vec1), 0xDB));	// wzyw
			__m128 zzzz = _mm_castsi128_ps(_mm_shuffle_epi32(*(__m128i*)(&matrix34.vec1), 0xAA));	// zzzz
			__m128 yxwy = _mm_castsi128_ps(_mm_shuffle_epi32(*(__m128i*)(&matrix34.vec1), 0x71));	// yxwy
			__m128 tmp7 = _mm_mul_ps(*(__m128*)(&matrix34.vec2), result);

			result = _mm_add_ps(
				_mm_add_ps(
					_mm_add_ps(
						_mm_mul_ps(
							_mm_sub_ps(
								_mm_mul_ps(_mm_mul_ps(xxxx, mulVec1), zwxy),
								_mm_mul_ps(_mm_mul_ps(yyyy, mulVec2), wzyw)),
							_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp7), 0xAA))),
						_mm_mul_ps(
							_mm_sub_ps(
								_mm_mul_ps(_mm_mul_ps(zzzz, mulVec2), wzyw),
								_mm_mul_ps(_mm_mul_ps(xxxx, mulVec0), yxwy)),
							_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp7), 0x55)))),
					_mm_add_ps(
						_mm_mul_ps(
							_mm_sub_ps(
								_mm_mul_ps(_mm_mul_ps(yyyy, mulVec0), yxwy),
								_mm_mul_ps(_mm_mul_ps(zzzz, mulVec1), zwxy)),
							_mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp7), 0x00))),
						tmp7)), *(__m128*)(&matrix34.vec0));

			transformIndex = *(int*)((ULONGLONG)pIndicesBuf + 0x4 * transformIndex);
		}

		free(pMatriciesBuf);
		free(pIndicesBuf);
	}

	return D3DXVECTOR3(result.m128_f32[0], result.m128_f32[1], result.m128_f32[2]);
}

bool TarkovSDK::WorldToScreen(D3DXVECTOR3& point3D, D3DXVECTOR3& point2D, bool isScoped)
{
	D3DXMATRIX temp;

	temp = this->ViewMatrix;

	//if(!isScoped)
		D3DXMatrixTranspose(&temp, &this->ViewMatrix);

	D3DXVECTOR3 translationVector = D3DXVECTOR3(temp._41, temp._42, temp._43);
	D3DXVECTOR3 up = D3DXVECTOR3(temp._21, temp._22, temp._23);
	D3DXVECTOR3 right = D3DXVECTOR3(temp._11, temp._12, temp._13);

	float w = D3DXVec3Dot(&translationVector, &point3D) + temp._44;

	//if (w < 0.098f)
		//return  D3DXVECTOR3(0,0,0);

	if (w < 1.f)
		w = 1.f;

	float y = D3DXVec3Dot(&up, &point3D) + temp._24;
	float x = D3DXVec3Dot(&right, &point3D) + temp._14;

	if(isScoped)
	{
		x /= Globals::m_FpsFovX;
		y /= Globals::m_FpsFovY;
	}

	float ScreenX = (Globals::Window_width / 2) * (1.f + x / w);
	float ScreenY = (Globals::Window_height / 2) * (1.f - y / w);
	float ScreenZ = w;

	point2D = D3DXVECTOR3(ScreenX, ScreenY, ScreenZ);

	return true;
}

bool TarkovSDK::GetCameraData(uint64_t camera, bool isScoped)
{
	if (!isScoped)
	{
		if (!(camera = Read<uint64_t>(camera + _OFFSET_CAMERA_OBJECT_)) ||
			!(camera = Read<uint64_t>(camera + _OFFSET_CAMERA_ENTITY)))
		{
			return false;
		}

		Globals::m_FpsFovX = Read<float>(camera + _OFFSET_CAMERA_FOV_X);
		Globals::m_FpsFovY = Read<float>(camera + _OFFSET_CAMERA_FOV_X);
	}
	else
	{
		if (!(camera = Read<uint64_t>(camera + _OFFSET_CAMERA_ENTITY)))
		{
			return false;
		}

	}

	ReadToBuffer(camera + _OFFSET_CAMERA_VIEWMATRIX, &this->ViewMatrix, sizeof(D3DMATRIX));

	return true;
}
