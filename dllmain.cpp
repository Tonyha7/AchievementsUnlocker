// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"

#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

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

void UnlockAllAchievements() {
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    std::cout << "[+] Unlocker DLL Injected" << std::endl;

#ifdef _WIN64
    const char* steamApiDllName = "steam_api64.dll";
#else
    const char* steamApiDllName = "steam_api.dll";
#endif

    HMODULE hSteamApi = GetModuleHandleA(steamApiDllName);
    if (!hSteamApi) {
        std::cout << "[-] " << steamApiDllName << " not found in process. Make sure the game is running and Steam is loaded." << std::endl;
        return;
    }
    std::cout << "[+] Found " << steamApiDllName << " at " << hSteamApi << std::endl;

    tSteamAPI_SteamUserStats_Accessor fSteamUserStats = (tSteamAPI_SteamUserStats_Accessor)GetProcAddress(hSteamApi, "SteamAPI_SteamUserStats_v013");
    if (!fSteamUserStats) {
        fSteamUserStats = (tSteamAPI_SteamUserStats_Accessor)GetProcAddress(hSteamApi, "SteamAPI_SteamUserStats_v012");
    }
    if (!fSteamUserStats) {
        fSteamUserStats = (tSteamAPI_SteamUserStats_Accessor)GetProcAddress(hSteamApi, "SteamAPI_SteamUserStats_v011");
    }
    if (!fSteamUserStats) {
        fSteamUserStats = (tSteamAPI_SteamUserStats_Accessor)GetProcAddress(hSteamApi, "SteamAPI_SteamUserStats_v010");
    }


    tSteamAPI_SteamUser fSteamUser = (tSteamAPI_SteamUser)GetProcAddress(hSteamApi, "SteamAPI_SteamUser_v023");
    if (!fSteamUser) {
        fSteamUser = (tSteamAPI_SteamUser)GetProcAddress(hSteamApi, "SteamAPI_SteamUser_v022");
    }
    if (!fSteamUser) {
        fSteamUser = (tSteamAPI_SteamUser)GetProcAddress(hSteamApi, "SteamAPI_SteamUser_v021");
    }
    if (!fSteamUser) {
        fSteamUser = (tSteamAPI_SteamUser)GetProcAddress(hSteamApi, "SteamAPI_SteamUser_v020");
    }


    if (!fSteamUserStats || !fSteamUser) {
        std::cout << "[-] Could not find Interface accessors (SteamUserStats or SteamUser)." << std::endl;
        return;
    }

    tSteamAPI_ISteamUserStats_SetAchievement fSetAchievement = (tSteamAPI_ISteamUserStats_SetAchievement)GetProcAddress(hSteamApi, "SteamAPI_ISteamUserStats_SetAchievement");
    tSteamAPI_ISteamUserStats_StoreStats fStoreStats = (tSteamAPI_ISteamUserStats_StoreStats)GetProcAddress(hSteamApi, "SteamAPI_ISteamUserStats_StoreStats");
    tSteamAPI_ISteamUserStats_GetNumAchievements fGetNumAchievements = (tSteamAPI_ISteamUserStats_GetNumAchievements)GetProcAddress(hSteamApi, "SteamAPI_ISteamUserStats_GetNumAchievements");
    tSteamAPI_ISteamUserStats_GetAchievementName fGetAchievementName = (tSteamAPI_ISteamUserStats_GetAchievementName)GetProcAddress(hSteamApi, "SteamAPI_ISteamUserStats_GetAchievementName");
    tSteamAPI_ISteamUserStats_GetAchievement fGetAchievement = (tSteamAPI_ISteamUserStats_GetAchievement)GetProcAddress(hSteamApi, "SteamAPI_ISteamUserStats_GetAchievement");
    tSteamAPI_ISteamUserStats_RequestUserStats fRequestUserStats = (tSteamAPI_ISteamUserStats_RequestUserStats)GetProcAddress(hSteamApi, "SteamAPI_ISteamUserStats_RequestUserStats");
    tSteamAPI_ISteamUser_GetSteamID_Func fGetSteamID = (tSteamAPI_ISteamUser_GetSteamID_Func)GetProcAddress(hSteamApi, "SteamAPI_ISteamUser_GetSteamID");

    if (!fSetAchievement || !fStoreStats || !fGetNumAchievements || !fGetAchievementName || !fRequestUserStats || !fGetSteamID || !fGetAchievement) {
        std::cout << "[-] Could not find all Flat API exports." << std::endl;
        return;
    }

    void* pUserStats = fSteamUserStats();
    void* pUser = fSteamUser();

    if (!pUserStats || !pUser) {
        std::cout << "[-] Interface accessors returned NULL." << std::endl;
        return;
    }
    std::cout << "[+] Got ISteamUserStats interface at " << pUserStats << std::endl;
    std::cout << "[+] Got ISteamUser interface at " << pUser << std::endl;

    uint64_t steamID = fGetSteamID(pUser);
    std::cout << "[+] Current SteamID: " << steamID << std::endl;

    std::cout << "[*] Requesting stats for current user..." << std::endl;
    fRequestUserStats(pUserStats, steamID);

    int maxRetries = 3;
    for (int retry = 0; retry < maxRetries; retry++) {
        uint32_t numAchievements = fGetNumAchievements(pUserStats);

        if (numAchievements == 0) {
            std::cout << "[-] No achievements found yet. Waiting..." << std::endl;
            Sleep(1000);
            continue;
        }

        bool anySuccess = false;
        int successCount = 0;

        std::cout << "\n[+] Found " << numAchievements << " achievements. Checking status..." << std::endl;

        for (uint32_t i = 0; i < numAchievements; i++) {
            const char* achName = fGetAchievementName(pUserStats, i);
            if (!achName) continue;

            bool bAchieved = false;
            bool getResult = fGetAchievement(pUserStats, achName, &bAchieved);

            if (!getResult) {
                continue;
            }

            if (bAchieved) {
                std::cout << "[*] " << achName << " is already unlocked." << std::endl;
                continue;
            }

            bool result = fSetAchievement(pUserStats, achName);
            if (result) {
                std::cout << "[+] Unlocked: " << achName << std::endl;
                anySuccess = true;
                successCount++;
            }
            else {
                std::cout << "[-] Failed to unlock: " << achName << std::endl;
            }
        }

        if (anySuccess) {
            std::cout << "\n[+] Successfully set " << successCount << " achievements!" << std::endl;
            break;
        }
        else {
            if (successCount == 0) {
                bool allUnlocked = true;
                for (uint32_t i = 0; i < numAchievements; i++) {
                    const char* achName = fGetAchievementName(pUserStats, i);
                    bool bAchieved = false;
                    fGetAchievement(pUserStats, achName, &bAchieved);
                    if (!bAchieved) { allUnlocked = false; break; }
                }

                if (allUnlocked) {
                    std::cout << "[+] All achievements are already unlocked!" << std::endl;
                    break;
                }
            }

            std::cout << "[-] Waiting for stats/unlocks (Retry " << retry + 1 << "/" << maxRetries << ")..." << std::endl;
            Sleep(2000);
        }
    }

    bool storeResult = fStoreStats(pUserStats);
    if (storeResult) {
        std::cout << "[+] SUCCESS." << std::endl;
    }
    else {
        std::cout << "[-] FAILED." << std::endl;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)UnlockAllAchievements, NULL, 0, NULL);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
