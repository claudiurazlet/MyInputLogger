#pragma once

#include "GuiBase.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "version.h"

using namespace std;

constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

class InputLogger: public BakkesMod::Plugin::BakkesModPlugin
	,public SettingsWindowBase // Uncomment if you wanna render your own tab in the settings menu
	//,public PluginWindowBase // Uncomment if you want to render your own plugin window
{

	void onLoad() override;
	//void onUnload() override;

	void initVars();
	void initHooks();

	bool IsEverythingOk();
	void onTick();
	string doubleToString(double value, int precision);

public:
	void RenderSettings() override; // Uncomment if you wanna render your own tab in the settings menu
	//void RenderWindow() override; // Uncomment if you want to render your own plugin window
	void RenderCanvas(CanvasWrapper canvas);

private:

	shared_ptr<bool> enabled;
	string timeDifferences;
	string lastTimeDifferences;
	string commandList;

	void registerMove();
	bool shouldSaveCurrentMove(ControllerInput currentInputs, bool directionalAirRoll);
	chrono::system_clock::time_point lastToggle;
	chrono::system_clock::time_point startingPoint;
	optional<ControllerInput> lastInputs;
	optional<bool> lastDirectionalAirRoll;
	string lastCommandsLog;
	int saveKeyIndex;
	int airRollKeyIndex;
};
