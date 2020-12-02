#pragma once
#pragma once
#include <Windows.h>
#include "..\code\M3DManager.h"

enum eGameMode {
	MODE_MANHUNT = 1,
	MODE_VICECITY,
	MODE_MANHUNT2
};

class eApp {
public:
	static HINSTANCE hInst;
	static HWND      hWindow;
	static HWND      hList;
	static HWND      hDataBox;
	static HWND      hTextureName;
	static HWND      hTable;
	static HMENU     hMenu;
	static DWORD     dwSel;
	static BOOL      bIsReady;
	static BOOL      bCombineModel;
	static BOOL	     bIsIni;
	static int       nGameMode;
	static M3DManager*    pM3DManager;
	static INT_PTR CALLBACK Process(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	static void Reset();
	static void CreateTooltip(HWND hWnd, LPCWSTR text);
	static void UpdateGameChange();
	static void Begin();
};