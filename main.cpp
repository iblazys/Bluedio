#include "imports.h"

#define _FPS_ 75

TarkovSDK*			SDK;

std::uint32_t process_id = 0; // GLOBAL

void render_overlay() 
{
	const wchar_t* playerType = L"PLAYER";

	// Initialize the window
	if (!Globals::m_Overlay->window_init())
		return;

	// D2D Failed to initialize?
	if (!Globals::m_Overlay->init_d2d())
		return;

	// Jump into the main render - cleared and redrawn each frame
	while (Globals::RunRenderLoop)
	{
		SDK->ReadGameData();

		// For match ending
		if (SDK->Players.size() == 0) 
		{
			int tries = 0;

			for (; tries < 0; tries++)
				SDK->ReadGameData();

			break; // End rendering
		}
		
		Globals::m_Overlay->begin_scene();
		Globals::m_Overlay->clear_scene();

		auto& local_player = SDK->LocalPlayer;
		for (auto& player : SDK->Players)
		{
			if (player.address == local_player.address)
				continue;

			D3DXVECTOR3 screen_pos;
			D3DXVECTOR3 screen_pos_head;

			// get proper view matrix if aiming
			if (local_player.isAiming) 
			{

			}

			// Box ESP
			if (SDK->WorldToScreen(player.fastPosition, screen_pos, local_player.isScoped))
			{
				// Get Distance
				int distance = SDK->GetDistance(player.fastPosition, local_player.fastPosition);

				if (player.type == Structs::PLAYER_SCAV)
					playerType = L"SCAV";

				if (player.type == Structs::SCAV)
					playerType = L"AI";

				if (player.type == Structs::PLAYER)
					playerType = player.name.c_str();

				Globals::m_Overlay->draw_text(D2D1::RectF(screen_pos.x - 30, screen_pos.y, screen_pos.x + 100, screen_pos.y + 20), "%S - %dm", playerType, distance);

				// Working
				if (SDK->WorldToScreen(player.headPosition, screen_pos_head, local_player.isScoped))
				{
					float BoxHeight1 = screen_pos.y - screen_pos_head.y;
					float BoxHeight2 = std::abs(screen_pos.x - screen_pos_head.x);
					float BoxHeight = (BoxHeight1 > BoxHeight2) ? BoxHeight1 : BoxHeight2;
					float BoxWidth = BoxHeight / 1.15f;

					Globals::m_Overlay->draw_box(screen_pos_head.x - (BoxWidth / 2) - 1, screen_pos_head.y - (BoxHeight / 5) - 1, BoxWidth + 2, BoxHeight + 12, Globals::m_Overlay->black_brush);
					Globals::m_Overlay->draw_box(screen_pos_head.x - (BoxWidth / 2), screen_pos_head.y - (BoxHeight / 5), BoxWidth, BoxHeight + 10, Globals::m_Overlay->red_brush);
				}
			}

			// Bone Esp - TODO

			// Box on head
			if (SDK->WorldToScreen(player.headPosition, screen_pos, local_player.isScoped))
			{
				Globals::m_Overlay->draw_rect(D2D1::RectF(screen_pos.x - 2, screen_pos.y - 2, screen_pos.x + 2, screen_pos.y + 2));
			}
		}

		// item esp



		// exit esp

		Globals::m_Overlay->end_scene();
	}

	// Shutdown
	Globals::m_Overlay->clear_screen();
	Globals::m_Overlay->d2d_shutdown();

	return;
}

int main()
{
	process_id = get_process_id(xorstr_("EscapeFromTarkov.exe"));
	//process_id = get_process_id("EscapeFromTarkov_BE.exe");

	if (process_id == 0) 
	{
		std::puts(xorstr_("[+] Unable to find process id. Is it running? Press anything."));
		system("pause");
		return 0;
	}

	Globals::m_BaseAddress = get_module_base_address(xorstr_("UnityPlayer.dll"));

	if (Globals::m_BaseAddress == 0)
	{
		std::puts(xorstr_("[+] Unable to retrieve base address. Press anything."));
		system("pause");
		return 0;
	}

	printf("%s: %d\n", xorstr_("[+] Process ID:"), process_id); // COMMENT OUT IN PROD
	printf("%s: 0x%I64X\n", xorstr_("[+] Base Addr:"), Globals::m_BaseAddress);

	std::puts(xorstr_("[+] Initializing..."));

	SDK = TarkovSDK::Instance();

	if (!SDK->Init(Globals::m_BaseAddress))
		return 0;

	std::thread render_thread(render_overlay);
	render_thread.detach();

	while (TRUE)
	{
		if (GetAsyncKeyState(VK_END)) 
		{
			std::puts(xorstr_("[+] SHUTTING DOWN"));

			Globals::RunReaderLoop = false; // stop reading
			Globals::RunRenderLoop = false; // stop rendering

			std::puts(xorstr_("[+] SHUTDOWN COMPLETE"));

			Sleep(500); // time for loops to end and d2d to shutdown

			exit(0);
		}

		Sleep(50);
	}
}
