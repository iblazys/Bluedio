#ifndef FOverlay_H
#define FOverlay_H

class FOverlay
{
public:
	auto window_set_style()-> void;
	auto window_set_transparency()-> void;
	auto window_set_top_most()-> void;
	auto retrieve_window()->HWND;
	auto window_init()->BOOL;
	auto d2d_shutdown()-> void;
	auto init_d2d()->BOOL;
	auto begin_scene()-> void;
	auto end_scene()-> void;
	auto clear_scene()-> void;
	auto draw_text(D2D1_RECT_F rect, const char* str, ...)-> void;
	auto draw_box(float x, float y, float width, float height, ID2D1SolidColorBrush* color)-> void;
	auto draw_rect(D2D1_RECT_F rect)-> void;
	auto clear_screen()-> void;

	bool WorldToScreen(D3DXVECTOR3& point3D, D3DXVECTOR3& point2D);

	static ID2D1SolidColorBrush* brush;
	static ID2D1SolidColorBrush* red_brush;
	static ID2D1SolidColorBrush* black_brush;
};

#endif