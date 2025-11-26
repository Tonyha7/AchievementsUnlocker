// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"

#include <windows.h>
#include <commctrl.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <format>

#pragma comment(lib, "comctl32.lib")

typedef void* (*tSteamAPI_SteamUserStats_Accessor)();
typedef bool (*tSteamAPI_ISteamUserStats_SetAchievement)(void* instancePtr, const char* pchName);
typedef bool (*tSteamAPI_ISteamUserStats_StoreStats)(void* instancePtr);
typedef uint32_t(*tSteamAPI_ISteamUserStats_GetNumAchievements)(void* instancePtr);
typedef const char* (*tSteamAPI_ISteamUserStats_GetAchievementName)(void* instancePtr, uint32_t iAchievement);
typedef bool (*tSteamAPI_ISteamUserStats_GetAchievement)(void* instancePtr, const char* pchName, bool* pbAchieved);
typedef void* (*tSteamAPI_SteamUser)();
typedef void (*tSteamAPI_ISteamUser_GetSteamID)(void* instancePtr, void* pSteamID);
typedef uint64_t(*tSteamAPI_ISteamUser_GetSteamID_Func)(void* instancePtr);
typedef uint64_t(*tSteamAPI_ISteamUserStats_RequestUserStats)(void* instancePtr, uint64_t steamID);

// GUI
HMODULE g_hModule = NULL;
HWND hMainWindow = NULL;
HWND hListView = NULL;
HWND hLogEdit = NULL;
HWND hBtnUnlock = NULL;
HWND hLabelSteamID = NULL;

tSteamAPI_ISteamUserStats_SetAchievement g_fSetAchievement = nullptr;
tSteamAPI_ISteamUserStats_StoreStats g_fStoreStats = nullptr;
tSteamAPI_ISteamUserStats_GetNumAchievements g_fGetNumAchievements = nullptr;
tSteamAPI_ISteamUserStats_GetAchievementName g_fGetAchievementName = nullptr;
tSteamAPI_ISteamUserStats_GetAchievement g_fGetAchievement = nullptr;
tSteamAPI_ISteamUserStats_RequestUserStats g_fRequestUserStats = nullptr;
tSteamAPI_ISteamUser_GetSteamID_Func g_fGetSteamID = nullptr;

void* g_pUserStats = nullptr;
void* g_pUser = nullptr;

void Log(const std::string& msg) {
    if (hLogEdit) {
        std::string text = msg + "\r\n";
        int len = GetWindowTextLengthA(hLogEdit);
        SendMessageA(hLogEdit, EM_SETSEL, (WPARAM)len, (LPARAM)len);
        SendMessageA(hLogEdit, EM_REPLACESEL, 0, (LPARAM)text.c_str());
    }
}

bool InitSteamAPI() {
#ifdef _WIN64
    const char* steamApiDllName = "steam_api64.dll";
#else
    const char* steamApiDllName = "steam_api.dll";
#endif

    HMODULE hSteamApi = GetModuleHandleA(steamApiDllName);
    if (!hSteamApi) {
        Log("[-] " + std::string(steamApiDllName) + " not found in process. Make sure the game is running and Steam is loaded.");
        return false;
    }
    Log("[+] Found " + std::string(steamApiDllName) + " at " + std::format("0x{:X}", (uintptr_t)hSteamApi));

    tSteamAPI_SteamUserStats_Accessor fSteamUserStats = (tSteamAPI_SteamUserStats_Accessor)GetProcAddress(hSteamApi, "SteamAPI_SteamUserStats_v013");
    if (!fSteamUserStats) fSteamUserStats = (tSteamAPI_SteamUserStats_Accessor)GetProcAddress(hSteamApi, "SteamAPI_SteamUserStats_v012");
    if (!fSteamUserStats) fSteamUserStats = (tSteamAPI_SteamUserStats_Accessor)GetProcAddress(hSteamApi, "SteamAPI_SteamUserStats_v011");
    if (!fSteamUserStats) fSteamUserStats = (tSteamAPI_SteamUserStats_Accessor)GetProcAddress(hSteamApi, "SteamAPI_SteamUserStats_v010");

    tSteamAPI_SteamUser fSteamUser = (tSteamAPI_SteamUser)GetProcAddress(hSteamApi, "SteamAPI_SteamUser_v023");
    if (!fSteamUser) fSteamUser = (tSteamAPI_SteamUser)GetProcAddress(hSteamApi, "SteamAPI_SteamUser_v022");
    if (!fSteamUser) fSteamUser = (tSteamAPI_SteamUser)GetProcAddress(hSteamApi, "SteamAPI_SteamUser_v021");
    if (!fSteamUser) fSteamUser = (tSteamAPI_SteamUser)GetProcAddress(hSteamApi, "SteamAPI_SteamUser_v020");

    if (!fSteamUserStats || !fSteamUser) {
        Log("[-] Could not find Interface accessors (SteamUserStats or SteamUser).");
        return false;
    }

    g_fSetAchievement = (tSteamAPI_ISteamUserStats_SetAchievement)GetProcAddress(hSteamApi, "SteamAPI_ISteamUserStats_SetAchievement");
    g_fStoreStats = (tSteamAPI_ISteamUserStats_StoreStats)GetProcAddress(hSteamApi, "SteamAPI_ISteamUserStats_StoreStats");
    g_fGetNumAchievements = (tSteamAPI_ISteamUserStats_GetNumAchievements)GetProcAddress(hSteamApi, "SteamAPI_ISteamUserStats_GetNumAchievements");
    g_fGetAchievementName = (tSteamAPI_ISteamUserStats_GetAchievementName)GetProcAddress(hSteamApi, "SteamAPI_ISteamUserStats_GetAchievementName");
    g_fGetAchievement = (tSteamAPI_ISteamUserStats_GetAchievement)GetProcAddress(hSteamApi, "SteamAPI_ISteamUserStats_GetAchievement");
    g_fRequestUserStats = (tSteamAPI_ISteamUserStats_RequestUserStats)GetProcAddress(hSteamApi, "SteamAPI_ISteamUserStats_RequestUserStats");
    g_fGetSteamID = (tSteamAPI_ISteamUser_GetSteamID_Func)GetProcAddress(hSteamApi, "SteamAPI_ISteamUser_GetSteamID");

    if (!g_fSetAchievement || !g_fStoreStats || !g_fGetNumAchievements || !g_fGetAchievementName || !g_fRequestUserStats || !g_fGetSteamID || !g_fGetAchievement) {
        Log("[-] Could not find all Flat API exports.");
        return false;
    }

    g_pUserStats = fSteamUserStats();
    g_pUser = fSteamUser();

    if (!g_pUserStats || !g_pUser) {
        Log("[-] Interface accessors returned NULL.");
        return false;
    }
    Log("[+] Got ISteamUserStats interface.");
    Log("[+] Got ISteamUser interface.");

    uint64_t steamID = g_fGetSteamID(g_pUser);
    Log("[+] Current SteamID: " + std::to_string(steamID));

    if (hLabelSteamID) {
        std::string idStr = "SteamID: " + std::to_string(steamID);
        SetWindowTextA(hLabelSteamID, idStr.c_str());
    }

    Log("[*] Requesting stats for current user...");
    g_fRequestUserStats(g_pUserStats, steamID);

    return true;
}

void RefreshAchievementList() {
    ListView_DeleteAllItems(hListView);
    if (!g_pUserStats || !g_fGetNumAchievements) return;

    uint32_t numAchievements = g_fGetNumAchievements(g_pUserStats);
    if (numAchievements == 0) return;

    Log("[+] Found " + std::to_string(numAchievements) + " achievements.");

    for (uint32_t i = 0; i < numAchievements; i++) {
        const char* achName = g_fGetAchievementName(g_pUserStats, i);
        if (!achName) continue;

        bool bAchieved = false;
        g_fGetAchievement(g_pUserStats, achName, &bAchieved);

        LVITEMA lvItem = { 0 };
        lvItem.mask = LVIF_TEXT | LVIF_PARAM;
        lvItem.iItem = i;
        lvItem.iSubItem = 0;
        lvItem.pszText = (LPSTR)achName;
        lvItem.lParam = bAchieved ? 1 : 0;
        int index = ListView_InsertItem(hListView, &lvItem);

        ListView_SetCheckState(hListView, index, bAchieved);
    }
}

void UnlockSelectedAchievements() {
    if (!g_pUserStats || !g_fSetAchievement) return;

    int count = ListView_GetItemCount(hListView);
    bool anySuccess = false;
    int successCount = 0;

    for (int i = 0; i < count; i++) {
        if (ListView_GetCheckState(hListView, i)) {
            LVITEMA lvItem = { 0 };
            lvItem.mask = LVIF_PARAM | LVIF_TEXT;
            lvItem.iItem = i;
            lvItem.iSubItem = 0;
            char buf[256];
            lvItem.pszText = buf;
            lvItem.cchTextMax = 256;
            ListView_GetItem(hListView, &lvItem);

            if (lvItem.lParam == 1) continue;

            bool result = g_fSetAchievement(g_pUserStats, lvItem.pszText);
            if (result) {
                Log("[+] Unlocked: " + std::string(lvItem.pszText));
                anySuccess = true;
                successCount++;

                lvItem.mask = LVIF_PARAM;
                lvItem.lParam = 1;
                ListView_SetItem(hListView, &lvItem);
            }
            else {
                Log("[-] Failed to unlock: " + std::string(lvItem.pszText));
            }
        }
    }

    if (anySuccess) {
        g_fStoreStats(g_pUserStats);
        Log("[+] Successfully set " + std::to_string(successCount) + " achievements!");
        MessageBoxA(hMainWindow, "Selected achievements unlocked!", "Success", MB_OK | MB_ICONINFORMATION);
    }
    else {
        Log("[-] No new achievements unlocked.");
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
    {
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_LISTVIEW_CLASSES;
        InitCommonControlsEx(&icex);

        hLabelSteamID = CreateWindowA("STATIC", "SteamID: Fetching...",
            WS_CHILD | WS_VISIBLE,
            10, 15, 300, 20, hWnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        hListView = CreateWindowExA(0, "SysListView32", "",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_NOCOLUMNHEADER | WS_BORDER,
            10, 50, 560, 300, hWnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
        ListView_SetExtendedListViewStyle(hListView, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);

        LVCOLUMNA lvc;
        lvc.mask = LVCF_WIDTH | LVCF_TEXT;
        lvc.cx = 530;
        lvc.pszText = (LPSTR)"Achievement";
        ListView_InsertColumn(hListView, 0, &lvc);

        hBtnUnlock = CreateWindowA("BUTTON", "Unlock",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            480, 10, 90, 30, hWnd, (HMENU)1, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        hLogEdit = CreateWindowExA(0, "EDIT", "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_BORDER,
            10, 360, 560, 190, hWnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        SendMessage(hListView, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hBtnUnlock, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hLogEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hLabelSteamID, WM_SETFONT, (WPARAM)hFont, TRUE);
    }
    break;
    case WM_COMMAND:
        if (LOWORD(wParam) == 1) {
            UnlockSelectedAchievements();
        }
        break;
    case WM_NOTIFY:
    {
        LPNMHDR lpnmHdr = (LPNMHDR)lParam;
        if (lpnmHdr->hwndFrom == hListView) {
            if (lpnmHdr->code == NM_CUSTOMDRAW) {
                LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
                switch (lplvcd->nmcd.dwDrawStage) {
                case CDDS_PREPAINT:
                    return CDRF_NOTIFYITEMDRAW;
                case CDDS_ITEMPREPAINT:
                    if (lplvcd->nmcd.lItemlParam == 1) {
                        lplvcd->clrText = RGB(0, 180, 0);
                    }
                    return CDRF_NEWFONT;
                }
            }
            else if (lpnmHdr->code == LVN_ITEMCHANGING) {
                LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
                if (pnmv->uChanged & LVIF_STATE) {
                    UINT oldState = (pnmv->uOldState & LVIS_STATEIMAGEMASK) >> 12;
                    UINT newState = (pnmv->uNewState & LVIS_STATEIMAGEMASK) >> 12;
                    if (oldState != newState) {
                        LVITEMA lvItem;
                        lvItem.mask = LVIF_PARAM;
                        lvItem.iItem = pnmv->iItem;
                        lvItem.iSubItem = 0;
                        ListView_GetItem(hListView, &lvItem);

                        if (lvItem.lParam == 1) {
                            if (newState != 2) {
                                return TRUE;
                            }
                        }
                    }
                }
            }
        }
    }
    break;
    case WM_TIMER:
        if (wParam == 1) {
            if (g_pUserStats && g_fGetNumAchievements) {
                uint32_t num = g_fGetNumAchievements(g_pUserStats);
                if (num > 0) {
                    KillTimer(hWnd, 1);
                    RefreshAchievementList();
                }
                else {
                    Log("[-] Waiting for stats...");
                }
            }
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

DWORD WINAPI MainThread(LPVOID lpParam) {
    WNDCLASSEXA wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = g_hModule;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = "AchievementsUnlockerClass";
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassExA(&wcex)) {
        MessageBoxA(NULL, "Call to RegisterClassEx failed!", "Error", NULL);
        return 1;
    }

    hMainWindow = CreateWindowA(
        "AchievementsUnlockerClass",
        "AchievementsUnlocker",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        600, 600,
        NULL, NULL, g_hModule, NULL
    );

    if (!hMainWindow) {
        MessageBoxA(NULL, "Call to CreateWindow failed!", "Error", NULL);
        return 1;
    }

    ShowWindow(hMainWindow, SW_SHOW);
    UpdateWindow(hMainWindow);

    Log("[*] Initializing Steam API...");
    if (InitSteamAPI()) {
        Log("[+] Steam API Initialized.");
        SetTimer(hMainWindow, 1, 1000, NULL);
    }
    else {
        Log("[-] Failed to initialize Steam API.");
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        g_hModule = hModule;
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MainThread, NULL, 0, NULL);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
