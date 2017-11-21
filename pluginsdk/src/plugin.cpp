#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#pragma warning (disable : 4100)  /* Disable Unreferenced parameter warning */
#include <Windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "teamspeak/public_errors.h"
#include "teamspeak/public_errors_rare.h"
#include "teamspeak/public_definitions.h"
#include "teamspeak/public_rare_definitions.h"
#include "teamspeak/clientlib_publicdefinitions.h"
#include "ts3_functions.h"
#include "plugin.h"
#include <tchar.h>
#include <Psapi.h>
#include <tlhelp32.h>

#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }

#include<Audiopolicy.h>
#include<Mmdeviceapi.h>

	static struct TS3Functions ts3Functions;

	struct IAudioSessionManager2;
	struct IAudioSessionEnumerator;
	struct IAudioSessionControl;
	struct IMMDevice;
	struct IMMDeviceEnumerator;

	using namespace std;

#define CHECK_HR(hr) if (hr != S_OK) { goto done; }

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); (dest)[destSize-1] = '\0'; }
#endif

#define PLUGIN_API_VERSION 22

#define PATH_BUFSIZE 512
#define COMMAND_BUFSIZE 128
#define INFODATA_BUFSIZE 128
#define SERVERINFO_BUFSIZE 256
#define CHANNELINFO_BUFSIZE 512
#define RETURNCODE_BUFSIZE 128

	static char* pluginID = NULL;

#ifdef _WIN32
	/* Helper function to convert wchar_T to Utf-8 encoded strings on Windows */
	static int wcharToUtf8(const wchar_t* str, char** result) {
		int outlen = WideCharToMultiByte(CP_UTF8, 0, str, -1, 0, 0, 0, 0);
		*result = (char*)malloc(outlen);
		if (WideCharToMultiByte(CP_UTF8, 0, str, -1, *result, outlen, 0, 0) == 0) {
			*result = NULL;
			return -1;
		}
		return 0;
	}
#endif

	/*********************************** Required functions ************************************/
	/*
	 * If any of these required functions is not implemented, TS3 will refuse to load the plugin
	 */

	 /* Unique name identifying this plugin */
	const char* ts3plugin_name() {
#ifdef _WIN32
		/* TeamSpeak expects UTF-8 encoded characters. Following demonstrates a possibility how to convert UTF-16 wchar_t into UTF-8. */
		static char* result = NULL;  /* Static variable so it's allocated only once */
		if (!result) {
			const wchar_t* name = L"Spotify Music Volume Ducking";
			if (wcharToUtf8(name, &result) == -1) {  /* Convert name into UTF-8 encoded result */
				result = "Spotify Music Volume Ducking";  /* Conversion failed, fallback here */
			}
		}
		return result;
#else
		return "Spotify Music Volume Ducking";
#endif
	}

	/* Plugin version */
	const char* ts3plugin_version() {
		return "1.0";
	}

	/* Plugin API version. Must be the same as the clients API major version, else the plugin fails to load. */
	int ts3plugin_apiVersion() {
		return PLUGIN_API_VERSION;
	}

	/* Plugin author */
	const char* ts3plugin_author() {
		/* If you want to use wchar_t, see ts3plugin_name() on how to use */
		return "DeltaTimo";
	}

	/* Plugin description */
	const char* ts3plugin_description() {
		/* If you want to use wchar_t, see ts3plugin_name() on how to use */
		return "This plugin automatically lowers the volume of music played by Spotify when someone is talking.";
	}

	/* Set TeamSpeak 3 callback functions */
	void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
		ts3Functions = funcs;
	}

	char pluginPath[PATH_BUFSIZE];

	/*
	 * Custom code called right after loading the plugin. Returns 0 on success, 1 on failure.
	 * If the function returns 1 on failure, the plugin will be unloaded again.
	 */
	int ts3plugin_init() {
		char appPath[PATH_BUFSIZE];
		char resourcesPath[PATH_BUFSIZE];
		char configPath[PATH_BUFSIZE];
		//char pluginPath[PATH_BUFSIZE];

		/* Your plugin init code here */
		//printf("PLUGIN: init\n");

		/* Example on how to query application, resources and configuration paths from client */
		/* Note: Console client returns empty string for app and resources path */
		ts3Functions.getAppPath(appPath, PATH_BUFSIZE);
		ts3Functions.getResourcesPath(resourcesPath, PATH_BUFSIZE);
		ts3Functions.getConfigPath(configPath, PATH_BUFSIZE);
		ts3Functions.getPluginPath(pluginPath, PATH_BUFSIZE, pluginID);

		//printf("PLUGIN: App path: %s\nResources path: %s\nConfig path: %s\nPlugin path: %s\n", appPath, resourcesPath, configPath, pluginPath);

		return 0;  /* 0 = success, 1 = failure, -2 = failure but client will not show a "failed to load" warning */
		/* -2 is a very special case and should only be used if a plugin displays a dialog (e.g. overlay) asking the user to disable
		 * the plugin again, avoiding the show another dialog by the client telling the user the plugin failed to load.
		 * For normal case, if a plugin really failed to load because of an error, the correct return value is 1. */
	}

	/* Custom code called right before the plugin is unloaded */
	void ts3plugin_shutdown() {
		/* Your plugin cleanup code here */
		//printf("PLUGIN: shutdown\n");

		/*
		 * Note:
		 * If your plugin implements a settings dialog, it must be closed and deleted here, else the
		 * TeamSpeak client will most likely crash (DLL removed but dialog from DLL code still open).
		 */
		 //LPDWORD exitCode;
		 //GetExitCodeThread(MainThread, &exitCode);
		 //ExitThread(exitCode);
		 //TerminateThread(MainThread, exitCode);
		 //exitPlugin = 1;

		 /* Free pluginID if we registered it */
		if (pluginID) {
			free(pluginID);
			pluginID = NULL;
		}
	}

	/****************************** Optional functions ********************************/
	/*
	 * Following functions are optional, if not needed you don't need to implement them.
	 */

	 /* Tell client if plugin offers a configuration window. If this function is not implemented, it's an assumed "does not offer" (PLUGIN_OFFERS_NO_CONFIGURE). */
	int ts3plugin_offersConfigure() {
		//printf("PLUGIN: offersConfigure\n");
		/*
		 * Return values:
		 * PLUGIN_OFFERS_NO_CONFIGURE         - Plugin does not implement ts3plugin_configure
		 * PLUGIN_OFFERS_CONFIGURE_NEW_THREAD - Plugin does implement ts3plugin_configure and requests to run this function in an own thread
		 * PLUGIN_OFFERS_CONFIGURE_QT_THREAD  - Plugin does implement ts3plugin_configure and requests to run this function in the Qt GUI thread
		 */
		return PLUGIN_OFFERS_NO_CONFIGURE;  /* In this case ts3plugin_configure does not need to be implemented */
	}

	/* Plugin might offer a configuration window. If ts3plugin_offersConfigure returns 0, this function does not need to be implemented. */
	void ts3plugin_configure(void* handle, void* qParentWidget) {
		//printf("PLUGIN: configure\n");
	}

	/*
	 * If the plugin wants to use error return codes, plugin commands, hotkeys or menu items, it needs to register a command ID. This function will be
	 * automatically called after the plugin was initialized. This function is optional. If you don't use these features, this function can be omitted.
	 * Note the passed pluginID parameter is no longer valid after calling this function, so you must copy it and store it in the plugin.
	 */
	void ts3plugin_registerPluginID(const char* id) {
		const size_t sz = strlen(id) + 1;
		pluginID = (char*)malloc(sz * sizeof(char));
		_strcpy(pluginID, sz, id);  /* The id buffer will invalidate after exiting this function */
		//printf("PLUGIN: registerPluginID: %s\n", pluginID);
	}

	/* Plugin command keyword. Return NULL or "" if not used. */
	const char* ts3plugin_commandKeyword() {
		return "spotifyducking";
	}

	static boolean autoduckingenabled = 1;
	int talkingClients = 0;

	int ts3plugin_processCommand(uint64 serverConnectionHandlerID, const char* command) {
		char buf[COMMAND_BUFSIZE];
		char *s, *param1 = NULL, *param2 = NULL;
		int i = 0;
		enum { CMD_NONE = 0, CMD_ENABLE, CMD_DISABLE } cmd = CMD_NONE;
#ifdef _WIN32
		char* context = NULL;
#endif


		_strcpy(buf, COMMAND_BUFSIZE, command);
#ifdef _WIN32
		s = strtok_s(buf, " ", &context);
#else
		s = strtok(buf, " ");
#endif
		while (s != NULL) {
			if (i == 0) {
				if (!strcmp(s, "toggle") || !strcmp(s, "onoff")) {
					if (autoduckingenabled) {
						cmd = CMD_DISABLE;
					}
					if (!autoduckingenabled) {
						cmd = CMD_ENABLE;
					}
				}
				else if (!strcmp(s, "enable") || !strcmp(s, "enabled") || !strcmp(s, "on")) {
					cmd = CMD_ENABLE;
				}
				else if (!strcmp(s, "disable") || !strcmp(s, "disabled") || !strcmp(s, "off")) {
					cmd = CMD_DISABLE;
				}
			}
			else if (i == 1) {
				param1 = s;
			}
			else {
				param2 = s;
			}
#ifdef _WIN32
			s = strtok_s(NULL, " ", &context);
#else
			s = strtok(NULL, " ");
#endif
			i++;
		}

		switch (cmd) {
		case CMD_NONE:
			return 1;
		case CMD_ENABLE:
			autoduckingenabled = 1;
		case CMD_DISABLE:
			autoduckingenabled = 0;
		}

		return 0;
	}

	void ts3plugin_currentServerConnectionChanged(uint64 serverConnectionHandlerID) {
	}

	const char* ts3plugin_infoTitle() {
		return "Spotify Music Volume Ducking";
	}

	void ts3plugin_freeMemory(void* data) {
		free(data);
	}

	int ts3plugin_requestAutoload() {
		return 0;  /* 1 = request autoloaded, 0 = do not request autoload */
	}

	void ts3plugin_onConnectStatusChangeEvent(uint64 serverConnectionHandlerID, int newStatus, unsigned int errorNumber) {

	}

	#define MAX_SPOTIFYS 10

	DWORD getProcessName(LPWSTR name, DWORD allSpotifys[MAX_SPOTIFYS])
	{
		int spotifys = 0;
		PROCESSENTRY32 entry;
		entry.dwSize = sizeof(PROCESSENTRY32);

		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

		if (Process32First(snapshot, &entry) == TRUE)
		{
			while (Process32Next(snapshot, &entry) == TRUE)
			{
				if (_wcsicmp(entry.szExeFile, name) == 0)
				{
					//return entry.th32ProcessID;
					if (spotifys >= MAX_SPOTIFYS) break;
					allSpotifys[spotifys] = entry.th32ProcessID;
					spotifys++;
				}
			}
		}

		CloseHandle(snapshot);

		return spotifys;
	}

	void changeSpotifyVolume(float volume)
	{
		DWORD allSpotifys[MAX_SPOTIFYS] = {};
		DWORD targetProcesses = getProcessName(L"spotify.exe", allSpotifys);
		if (targetProcesses == 0) {
			//printf("No spotify found!\n");
			return;
		}

		IMMDeviceEnumerator* deviceEnumerator = NULL;
		IMMDevice* device = NULL;
		IAudioSessionManager2* sessionManager = NULL;
		IAudioSessionEnumerator* sessionEnumerator = NULL;
		IAudioSessionControl* session = NULL;
		IAudioSessionControl2* sessionControl = NULL;

		int numberOfProcesses = 0;
		int numberOfActiveProcesses = 0;

		AudioSessionState state;
		
		//create device enumerator
		CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&deviceEnumerator);
		// get default device
		deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);

		// activate session manager
		device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL, (void**)&sessionManager);
		//// make enum
		sessionManager->GetSessionEnumerator(&sessionEnumerator);
		sessionEnumerator->GetCount(&numberOfProcesses);


		for (int i = 0; i < numberOfProcesses; i++)
		{
			sessionEnumerator->GetSession(i, &session);
			session->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&sessionControl);
			DWORD processId = 0;
			sessionControl->GetProcessId(&processId);
			if (!processId) {
				SAFE_RELEASE(sessionControl);
				SAFE_RELEASE(session);
				continue;
			}

			//if (processId != targetProcessID) {
			BOOLEAN isSpotify = 0;
			for (int j = 0; j < targetProcesses; j++)
			{
				if (processId == allSpotifys[j])
				{
					isSpotify = 1;
					break;
				}
			}
			if (!isSpotify) {
				SAFE_RELEASE(sessionControl);
				SAFE_RELEASE(session);
				continue;
			}

			session->GetState(&state);
			ISimpleAudioVolume* volumeControl;
			session->QueryInterface(_uuidof(ISimpleAudioVolume), (void**)&volumeControl);
			float currentVolume = 1.0f;
			volumeControl->GetMasterVolume(&currentVolume);
			//float endVolume = currentVolume + volume;

			float endVolume = 1.0f;
			if (volume > 0)
				endVolume = currentVolume * (1 / (1 - volume));
			else
				endVolume = currentVolume * (1 + volume);

			if (endVolume < 0)
				endVolume = 0;
			else if (endVolume > 1)
				endVolume = 1;


			volumeControl->SetMasterVolume(endVolume, NULL);
			SAFE_RELEASE(volumeControl);
			SAFE_RELEASE(sessionControl);
			SAFE_RELEASE(session);
		}
		//CLEANUP

		SAFE_RELEASE(deviceEnumerator);
		SAFE_RELEASE(device);
		SAFE_RELEASE(sessionEnumerator);
		SAFE_RELEASE(sessionManager);
	}

	#define DUCKINGPERCENTAGE 10
	#define DUCKINGSPOTIFY 0.5f

	void ts3plugin_onTalkStatusChangeEvent(uint64 serverConnectionHandlerID, int status, int isReceivedWhisper, anyID clientID) {
		if (talkingClients < 0) {
			talkingClients = 0;
		}

		//HWND hwndWinamp = FindWindow(L"Winamp v1.x", NULL);
		if (status == STATUS_TALKING) {
			talkingClients = talkingClients + 1;
			if (talkingClients == 1) {
				if (autoduckingenabled) {
					//for (int i = 1; i <= duckingPercentage; i = i + 1) {
					//	SendMessage(hwndWinamp, WM_COMMAND, 40059, 1);
					//}
					changeSpotifyVolume(-DUCKINGSPOTIFY);
				}
			}
		}
		else
		{
			if (status == STATUS_NOT_TALKING) {
				talkingClients = talkingClients - 1;
				if (talkingClients == 0) {
					if (autoduckingenabled) {
						//for (int i = 1; i <= duckingPercentage; i = i + 1) {
						//	SendMessage(hwndWinamp, WM_COMMAND, 40058, 1);
						//}
						changeSpotifyVolume(DUCKINGSPOTIFY);
					}
				}
			}
		}
	}

#ifdef __cplusplus
}
#endif