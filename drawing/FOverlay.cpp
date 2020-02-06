#pragma once
#include "../imports.h"

static HWND win;

/*

Window functions
*/

auto FOverlay::window_set_style() -> void {
	int i = 0;

	i = (int)GetWindowLong(win, -20);

	SetWindowLongPtr(win, -20, (LONG_PTR)(i | 0x20));
}

auto FOverlay::window_set_transparency()-> void {
	MARGINS margin;
	UINT opacity_flag, color_key_flag, color_key = 0;
	UINT opacity = 0;

	margin.cyBottomHeight = -1;
	margin.cxLeftWidth = -1;
	margin.cxRightWidth = -1;
	margin.cyTopHeight = -1;

	DwmExtendFrameIntoClientArea(win, &margin);

	opacity_flag = 0x02;
	color_key_flag = 0x01;
	color_key = 0x000000;
	opacity = 0xFF;

	SetLayeredWindowAttributes(win, color_key, opacity, opacity_flag);
}

auto FOverlay::window_set_top_most()-> void {
	SetWindowPos(win, HWND_TOPMOST, 0, 0, 0, 0, 0x0002 | 0x0001);
}

auto FOverlay::retrieve_window()->HWND {
	return win;
}

// Hijacking method down here.

auto FOverlay::window_init()->BOOL {
	win = FindWindow("CEF-OSC-WIDGET", "NVIDIA GeForce Overlay");
	if (!win)
		return FALSE;

	FOverlay::window_set_style();
	FOverlay::window_set_transparency();
	FOverlay::window_set_top_most(); // set top every frame instead

	ShowWindow(win, SW_SHOW);

	return TRUE;
}


/*

Overlay functions

*/



ID2D1Factory* d2d_factory;
ID2D1HwndRenderTarget* tar;
IDWriteFactory* write_factory;
ID2D1SolidColorBrush* FOverlay::brush;
ID2D1SolidColorBrush* FOverlay::red_brush;
ID2D1SolidColorBrush* FOverlay::black_brush;
IDWriteTextFormat* format;


auto FOverlay::d2d_shutdown() ->void {
	// Release
	tar->Release();
	write_factory->Release();
	brush->Release();
	d2d_factory->Release();
}

auto FOverlay::init_d2d()-> BOOL {
	HRESULT ret;
	RECT rc;

	// Initialize D2D here
	ret = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2d_factory);
	if (FAILED(ret))
		return FALSE;

	ret = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown * *)(&write_factory));
	if (FAILED(ret))
		return FALSE;

	write_factory->CreateTextFormat(L"Verdana", NULL, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
		13.0,
		L"en-us",
		&format);

	GetClientRect(FOverlay::retrieve_window(), &rc);

	ret = d2d_factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED)), D2D1::HwndRenderTargetProperties(FOverlay::retrieve_window(), D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top)), &tar);
	if (FAILED(ret))
		return FALSE;

	//brush = new ID2D1SolidColorBrush();

	tar->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &brush);
	tar->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &red_brush);
	tar->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &black_brush);

	return TRUE;
}


auto FOverlay::begin_scene()-> void {
	tar->BeginDraw();
}

auto FOverlay::end_scene()-> void {
	tar->EndDraw();
}

auto FOverlay::clear_scene()-> void {
	tar->Clear();
}


auto FOverlay::draw_text(D2D1_RECT_F rect, const char* str, ...)-> void {
	char buf[4096];
	int len = 0;
	wchar_t b[256];

	va_list arg_list;
	va_start(arg_list, str);
	vsnprintf(buf, sizeof(buf), str, arg_list);
	va_end(arg_list);

	len = strlen(buf);
	mbstowcs(b, buf, len);

	tar->DrawText(b, len, format, rect, brush, D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);

}

auto FOverlay::draw_box(float x, float y, float width, float height, ID2D1SolidColorBrush* color) -> void
{
	 tar->DrawRectangle(D2D1::RectF(x, y, x + width, y + height), color);
}

auto FOverlay::draw_rect(D2D1_RECT_F rect) -> void
{
	tar->DrawRectangle(rect, brush);
}

auto FOverlay::clear_screen()-> void {
	FOverlay::begin_scene();
	FOverlay::clear_scene();
	FOverlay::end_scene();
}

bool FOverlay::WorldToScreen(D3DXVECTOR3& point3D, D3DXVECTOR3& point2D)
{
	//D3DXMATRIX temp;

	auto& temp = TarkovSDK::Instance()->ViewMatrix;

	//D3DXMatrixTranspose(&temp, &viewMatrix);

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

	float ScreenX = (2560 / 2) * (1.f + x / w);
	float ScreenY = (1080 / 2) * (1.f - y / w);
	float ScreenZ = w;

	point2D = D3DXVECTOR3(ScreenX, ScreenY, ScreenZ);

	return true;
}