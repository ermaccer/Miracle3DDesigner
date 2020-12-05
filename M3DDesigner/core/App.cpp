#include "App.h"
#include "..\code\M3DManager.h"
#include "..\resource.h"
#include <CommCtrl.h>
#include <windowsx.h>

HINSTANCE        eApp::hInst;
HWND             eApp::hWindow;
HWND             eApp::hList;
HWND             eApp::hDataBox;
HWND             eApp::hTextureName;
HWND             eApp::hTable;
HMENU            eApp::hMenu;
DWORD            eApp::dwSel;
BOOL             eApp::bIsReady;
BOOL			 eApp::bIsIni;
BOOL		     eApp::bCombineModel;
int              eApp::nGameMode;
M3DManager*  eApp::pM3DManager;

INT_PTR CALLBACK eApp::Process(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	hWindow = hDlg;
	HANDLE hicon = 0;

	if (!bIsReady)
	{
		if (bIsIni || !bIsReady)
		{
			EnableWindow(GetDlgItem(hDlg, M3D_COMBINE_MDL), FALSE);
			EnableWindow(GetDlgItem(hDlg, M3D_DECOMPILE), FALSE);
			EnableMenuItem(hMenu, ID_EXPORT_SMD, MF_DISABLED);
			EnableMenuItem(hMenu, ID_EXPORT_OBJ, MF_DISABLED);
		}

		EnableMenuItem(hMenu, ID_FILE_CLOSE, MF_DISABLED);
		EnableMenuItem(hMenu, ID_EXPORT_SMD, MF_DISABLED);
		EnableMenuItem(hMenu, ID_EXPORT_OBJ, MF_DISABLED);
		//EnableMenuItem(hMenu, ID_SAVEAS_GTA3, MF_DISABLED);
	}
	else
	{
		if (bIsIni)
		{
			EnableWindow(GetDlgItem(hDlg, M3D_COMBINE_MDL), FALSE);
			EnableWindow(GetDlgItem(hDlg, M3D_DECOMPILE), FALSE);
			EnableMenuItem(hMenu, ID_EXPORT_SMD, MF_DISABLED);
			EnableMenuItem(hMenu, ID_EXPORT_OBJ, MF_DISABLED);
		}
		if (!bIsIni)
		{
			EnableWindow(GetDlgItem(hDlg, M3D_COMBINE_MDL), TRUE);
			EnableWindow(GetDlgItem(hDlg, M3D_DECOMPILE), TRUE);

			EnableMenuItem(hMenu, ID_EXPORT_SMD, MF_ENABLED);
			EnableMenuItem(hMenu, ID_EXPORT_OBJ, MF_ENABLED);
		}

		EnableMenuItem(hMenu, ID_FILE_CLOSE, MF_ENABLED);

	}


	if (bIsIni)
	{
		EnableWindow(GetDlgItem(hDlg, M3D_COMPILE), TRUE);
	}
	else
	{
		EnableWindow(GetDlgItem(hDlg, M3D_COMPILE), FALSE);
	}

	switch (message)
	{
	case WM_INITDIALOG:
		Reset();
    	hTextureName = GetDlgItem(hDlg, M3D_TEXTURE_NAME);
		hDataBox = GetDlgItem(hDlg,M3D_GROUPS);
		hTable = GetDlgItem(hDlg, M3D_FILE_NAME);
		hList = GetDlgItem(hDlg, M3D_LOG);
		Button_SetCheck(GetDlgItem(hDlg, M3D_COMBINE_MDL), TRUE);
		Button_SetCheck(GetDlgItem(hDlg, M3D_NOFLIPUV), TRUE);
		CreateTooltip(GetDlgItem(hDlg, M3D_COMBINE_MDL), L"Attaches all seperate objects to main mesh and rigs them");
		PushLogMessage(hList, L"Miracle3D Designer v" + (std::wstring)M3D_DESIGNER_VERSION + L" ready!");
		hMenu = GetMenu(hDlg);
		hicon = LoadImage(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDI_M3DDESIGNER), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE);
		SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hicon);
		return (INT_PTR)TRUE;
	case WM_CLOSE:
		EndDialog(hDlg, LOWORD(wParam));
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == ID_OPEN_MODEL)
		{
			pM3DManager = new M3DManager();
			pM3DManager->Init(&hList, &hDataBox, &hTextureName, &hTable);
			pM3DManager->OpenFile(SetPathFromButton(L"Miracle3D Model (*.sam)(*.gsm)\0*.sam;*.gsm\0All Files (*.*)\0*.*\0", L"sam", hDlg));
		}
		if (LOWORD(wParam) == ID_OPEN_INI)
		{
			pM3DManager = new M3DManager();
			pM3DManager->Init(&hList, &hDataBox, &hTextureName, &hTable);
			pM3DManager->OpenINI(SetPathFromButton(L"Configuration File\0*.ini\0All Files (*.*)\0*.*\0", L"ini", hDlg));
		}
		if (LOWORD(wParam) == ID_EXPORT_SMD)
			pM3DManager->ExportToSMD(SetFolderFromButton(eApp::hWindow), IsDlgButtonChecked(hDlg, M3D_COMBINE_MDL), IsDlgButtonChecked(hDlg, M3D_NOFLIPUV));
		if (LOWORD(wParam) == ID_EXPORT_OBJ)
			pM3DManager->ExportToOBJ(SetFolderFromButton(eApp::hWindow), IsDlgButtonChecked(hDlg, M3D_COMBINE_MDL), IsDlgButtonChecked(hDlg, M3D_NOFLIPUV));

		if (LOWORD(wParam) == M3D_DECOMPILE)
			pM3DManager->Decompile(SetFolderFromButton(eApp::hWindow));


		if (LOWORD(wParam) == M3D_COMPILE)
		{
			pM3DManager->Compile(SetSavePathFromButton(L"Miracle3D Model (*.sam)(*.gsm)\0*.sam;*.gsm\0All Files (*.*)\0*.*\0", L"sam", hDlg), IsDlgButtonChecked(hDlg, M3D_FLIP_UV));
		}

		if (wParam == IDM_ABOUT)
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hDlg, About);

		if (LOWORD(wParam) == ID_FILE_CLOSE)
			pM3DManager->Close();

		if (LOWORD(wParam) == IDM_EXIT)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}

	}
	return (INT_PTR)FALSE;
}

INT_PTR eApp::About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

void eApp::Reset()
{
	nGameMode = 0;
}

void eApp::CreateTooltip(HWND hWnd, LPCWSTR text)
{
	HWND hwndTT = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP |
		TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, NULL,
		hInst, NULL);
	SetWindowPos(hwndTT, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

	TOOLINFO ti;
	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
	ti.hwnd = hWindow;
	ti.hinst = NULL;
	ti.uId = (UINT_PTR)hWnd;
	ti.lpszText = (LPWSTR)text;

	RECT rect;
	GetClientRect(hWnd, &rect);

	ti.rect.left = rect.left;
	ti.rect.top = rect.top;
	ti.rect.right = rect.right;
	ti.rect.bottom = rect.bottom;

	SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)&ti);
}

void eApp::UpdateGameChange()
{

}

void eApp::Begin()
{
	pM3DManager = nullptr;
	DialogBox(hInst, MAKEINTRESOURCE(IDD_DESIGN), 0, Process);
}
