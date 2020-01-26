#pragma comment(lib, "d3d9")
#pragma comment(lib, "d3dx9")

#include <Windows.h>
#include "detours.h"
#include "Patternscanning.h"
#include <string>
#include <d3d9.h>
#include <d3dx9.h>
#include <iostream>

typedef HRESULT(__stdcall* f_EndScene)(IDirect3DDevice9* pDevice); // our function prototype 
f_EndScene oEndScene; // original endscene

ID3DXFont* Font;

D3DCOLOR MenuBackgroundColor = D3DCOLOR_ARGB(255, 50, 50, 50);
D3DCOLOR MenuHighLightedColor = D3DCOLOR_ARGB(255, 90, 90, 90);
D3DCOLOR MenuBorderColor = D3DCOLOR_ARGB(255, 180, 0, 0);
D3DCOLOR TextColor = D3DCOLOR_ARGB(255, 255, 255, 255);
D3DCOLOR White = D3DCOLOR_ARGB(255, 255, 255, 255);
D3DCOLOR Black = D3DCOLOR_ARGB(255, 0, 0, 0);

void SolidRect(int x, int y, int w, int h, D3DCOLOR color, IDirect3DDevice9* pDevice);
void BorderedRect(int x, int y, int w, int h, int thiccness, D3DCOLOR color, IDirect3DDevice9* pDevice);
void CreateFontB(IDirect3DDevice9* pDevice, int size, std::string choiceFont);
void WriteText(LPCSTR Text, int x, int y, int w, int h, D3DCOLOR color);
bool CheckBox(int x, int y, bool var, std::string Name, IDirect3DDevice9* pDevice, POINT Mouse);
void DrawMenu(IDirect3DDevice9* pDevice);

bool MenuOpen;

int MenuX = 500;
int MenuY = 500;

HRESULT __stdcall Hooked_EndScene(IDirect3DDevice9* pDevice) // our hooked endscene
{
	static bool init = false;

	if (!init) {
		init = true;
		CreateFontB(pDevice, 14, "Times New Roman");
	}

	DrawMenu(pDevice);
	return oEndScene(pDevice); // call original ensdcene so the game can draw
}

DWORD WINAPI MainThread(LPVOID param) // our main thread
{
	static DWORD DirectXDevice = NULL; // our device address. made it static but you can make it just standard DWORD doesent mather.

	while (!DirectXDevice) // while loop so it tries to get the device untill it gets it.
		DirectXDevice = **(DWORD**)(FindPattern("shaderapidx9.dll", "A1 ?? ?? ?? ?? 50 8B 08 FF 51 0C") + 0x1); // geting the device by patternscaning

	void** pVTable = *reinterpret_cast<void***>(DirectXDevice); // getting the vtable array
	oEndScene = (f_EndScene)DetourFunction((PBYTE)pVTable[42], (PBYTE)Hooked_EndScene); // hooking the endscene and putting a jump to our function and then storing original in oEndscene

	return false;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH: // gets runned when injected
		CreateThread(0, 0, MainThread, hModule, 0, 0); // creates our thread 
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

void SolidRect(int x, int y, int w, int h, D3DCOLOR color, IDirect3DDevice9* pDevice) {
	D3DRECT rect = { x, y, x + w, y + h };
	pDevice->Clear(1, &rect, D3DCLEAR_TARGET, color, 0.0f, 0);
}

void BorderedRect(int x, int y, int w, int h, int thiccness, D3DCOLOR color, IDirect3DDevice9* pDevice) {
	SolidRect(x, y, w, thiccness, color, pDevice);
	SolidRect(x, y, thiccness, h, color, pDevice);
	SolidRect(x + (w - thiccness), y, thiccness, h, color, pDevice);
	SolidRect(x, y + (h - thiccness), w, thiccness, color, pDevice);
}

void CreateFontB(IDirect3DDevice9* pDevice, int size, std::string choiceFont) {
	D3DXCreateFontA(pDevice, size, 0, FW_BOLD, 0, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, choiceFont.c_str(), &Font);
}

void WriteText(LPCSTR Text, int x, int y, int w, int h, D3DCOLOR color) {
	RECT rect = { x, y, x + w, y + h };
	Font->DrawTextA(NULL, Text, -1, &rect, DT_CENTER, color);
}

bool CheckBox(int x, int y, bool var, std::string Name, IDirect3DDevice9* pDevice, POINT Mouse) {
	BorderedRect(x, y, 10, 10, 1, White, pDevice);
	if (Mouse.x >= x && Mouse.x <= x + 111 && Mouse.y >= y && Mouse.y <= y + 10) {
		SolidRect(x + 1, y + 1, 8, 8, MenuHighLightedColor, pDevice);
		if (GetAsyncKeyState(VK_LBUTTON) & 1) {
			var = !var;
			return var;
		}
	}

	if (var) {
		SolidRect(x + 1, y + 1, 8, 8, White, pDevice);
	}
	WriteText(Name.c_str(), x + 11, y, 100, 14, White);

	return var;
}

void DrawMenu(IDirect3DDevice9* pDevice) {
	static POINT Mouse;
	GetCursorPos(&Mouse);

	if (Mouse.x >= MenuX - 2 && Mouse.x <= MenuX + 204 && Mouse.y >= MenuY - 20 && Mouse.y <= MenuY) {
		if (GetAsyncKeyState(VK_LBUTTON)) {
			MenuX = Mouse.x - 100;
			MenuY = Mouse.y + 10;
		}
	}

	if (GetAsyncKeyState(VK_INSERT) & 1) {
		MenuOpen = !MenuOpen;
	}

	if (MenuOpen) {
		SolidRect(MenuX, MenuY, 200, 200, MenuBackgroundColor, pDevice);
		SolidRect(MenuX - 2, MenuY - 20, 204, 20, MenuBorderColor, pDevice);
		BorderedRect(MenuX - 2, MenuY - 2, 204, 204, 2, MenuBorderColor, pDevice);
		WriteText("My Menu", MenuX, MenuY - 17, 200, 14, Black);

		static bool Test;
		Test = CheckBox(MenuX + 5, MenuY + 5, Test, "Dump Nerds", pDevice, Mouse);

		if (Test) {
			SolidRect(50, 50, 150, 150, Black, pDevice);
			WriteText("Now Dumping Nerds", 50, 70, 150, 20, White);
		}
	}
}
