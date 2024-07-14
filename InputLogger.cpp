#include "pch.h"
#include "InputLogger.h"

#include <fstream>
#include <filesystem>
#include <chrono>

using namespace std;
using namespace chrono;

BAKKESMOD_PLUGIN(InputLogger, "Rocket League Input Logger", plugin_version, 0)

shared_ptr<CVarManagerWrapper> _globalCvarManager;

#define CVAR_PLUGIN_ENABLED ("enabled")

void InputLogger::onLoad()
{
	_globalCvarManager = cvarManager;

	initVars();
	initHooks();
}

void InputLogger::initVars()
{
	enabled = std::make_shared<bool>(true);
	cvarManager->registerCvar(CVAR_PLUGIN_ENABLED, "0", "Enable the plugin", true, true, 0, true, 1).bindTo(enabled);

	lastCommandsLog = "";

	lastToggle = system_clock::now();

	for (auto& binding : gameWrapper->GetSettings().GetAllGamepadBindings()) {
		if (binding.second == "ToggleRoll") {
			airRollKeyIndex = gameWrapper->GetFNameIndexByString(binding.first);
		}
	}
}

bool InputLogger::IsEverythingOk() 
{
	if (!*enabled) { return false; }

	bool customTraining = gameWrapper->IsInCustomTraining();
	bool freeplay = gameWrapper->IsInFreeplay();
	if (!customTraining && !freeplay) { return false; }

	ServerWrapper server = gameWrapper->GetCurrentGameState();
	if (!server) { LOG("[MyCPS] Server undefined"); return false; }

	BallWrapper ball = server.GetBall();
	if (!ball) { LOG("[MyCPS] Ball undefined"); return false; }

	CarWrapper car = gameWrapper->GetLocalCar();
	if (!car) { LOG("[MyCPS] Car undefined"); return false; }

	return true;
}

void InputLogger::initHooks()
{
	gameWrapper->HookEvent("Function Engine.GameViewportClient.Tick", bind(&InputLogger::onTick, this));
}

void InputLogger::onTick()
{
	if (!IsEverythingOk()) return;

	auto now = system_clock::now();
	const chrono::duration<double> elapsed_seconds = now - lastToggle;

	// DON'T OVERREACT. IF USER STAYS ON THE SAME KEY, WAIT 500ms BEFORE TOGGLING
	//if (elapsed_seconds < duration_cast<system_clock::duration>(duration<double>(0.1))) { return; };

	//if (elapsed_seconds > duration_cast<system_clock::duration>(duration<double>(1.0))) { commandList = ""; };

	lastToggle = now;
	startingPoint = now;

	registerMove();
}

void InputLogger::registerMove()
{
	const auto inputs = gameWrapper->GetPlayerController().GetVehicleInput();
	const auto isDirectionalAirRoll = gameWrapper->IsKeyPressed(airRollKeyIndex);

	if (!shouldSaveCurrentMove(inputs, isDirectionalAirRoll)) { return; }

	// COMPUTE TIME SINCE REGISTERING STARTED
	//const auto time = to_string((system_clock::now() - startingPoint).count());

	CarWrapper car = gameWrapper->GetLocalCar();

	bool isDodgingForward = inputs.DodgeForward == 1;
	bool isDodgingBackward = inputs.DodgeForward == -1;
	bool isDriveForward = inputs.Throttle > 0;
	bool isDriveBackward = inputs.Throttle < 0;
	bool isBoost = inputs.ActivateBoost > 0;
	bool isJump = (car.GetInput().Jumped > 0);
	bool isHandbrake = inputs.Handbrake > 0;
	bool isRollRight = inputs.Roll > 0;
	bool isRollLeft = inputs.Roll < 0;
	bool isAnalogueDown = inputs.Pitch > 0;
	bool isAnalogueUp = inputs.Pitch < 0;
	bool isAnalogueRight = inputs.Steer > 0;
	bool isAnalogueLeft = inputs.Steer < 0;

	const auto roll = doubleToString(inputs.Roll, 1);
	const auto analogueVertical = doubleToString(inputs.Pitch, 1);
	const auto analogueHorizontal = doubleToString(inputs.Steer, 1);

	//string newCommandsLog = "time (" + time + "), ";
	string newCommandsLog = "";
	if (isDriveForward) { newCommandsLog += "DriveForward, "; }
	if (isDriveBackward) { newCommandsLog += "DriveBackward, "; }
	if (isDodgingForward) { newCommandsLog += "DodgingForward, "; }
	if (isDodgingBackward) { newCommandsLog += "DodgingBackward, "; }
	if (isBoost) { newCommandsLog += "Boost, "; }
	if (isAnalogueDown) { newCommandsLog += "AnalogueDown (" + analogueVertical + "), "; }
	if (isAnalogueUp) { newCommandsLog += "AnalogueUp (" + analogueVertical + "), "; }
	if (isRollRight) { newCommandsLog += "RollRight (" + roll + "), "; }
	if (isRollLeft) { newCommandsLog += "RollLeft (" + roll + "), "; }
	if (isAnalogueRight) { newCommandsLog += "AnalogueRight (" + analogueHorizontal + "), "; }
	if (isAnalogueLeft) { newCommandsLog += "AnalogueLeft (" + analogueHorizontal + "), "; }
	if (isHandbrake) { newCommandsLog += "Handbrake, "; }
	if (isDirectionalAirRoll) { newCommandsLog += "DirectionalAirRoll, "; }
	if (isJump) { newCommandsLog += "Jump, "; }

	if (lastCommandsLog == newCommandsLog || newCommandsLog == "") { return; }

	lastCommandsLog = newCommandsLog;

	LOG("[InputLogger] lastInputs == {0}", newCommandsLog);
}

string InputLogger::doubleToString(double value, int precision)
{
	// Create a stringstream object
	ostringstream oss;
	// Set the desired precision and fixed format
	oss << std::fixed << std::setprecision(precision) << value;
	return oss.str();
}

bool InputLogger::shouldSaveCurrentMove(ControllerInput currentInputs, bool directionalAirRoll)
{
	// When registering 1st inputs save them directly
	if (!lastInputs) {
		lastInputs = currentInputs;
		lastDirectionalAirRoll = directionalAirRoll;
		return true;
	}

	// Otherwise check if last inputs are the same, if so don't save them
	if (currentInputs.Jump == lastInputs.value().Jump
		&& currentInputs.ActivateBoost == lastInputs.value().ActivateBoost
		&& currentInputs.DodgeForward == lastInputs.value().DodgeForward
		&& currentInputs.DodgeStrafe == lastInputs.value().DodgeStrafe
		&& currentInputs.Handbrake == lastInputs.value().Handbrake
		&& currentInputs.Pitch == lastInputs.value().Pitch
		&& currentInputs.Roll == lastInputs.value().Roll
		&& currentInputs.Steer == lastInputs.value().Steer
		&& currentInputs.Throttle == lastInputs.value().Throttle
		&& currentInputs.Yaw == lastInputs.value().Yaw
		&& directionalAirRoll == lastDirectionalAirRoll.value())
	{
		return false;
	};

	return true;
}

void InputLogger::RenderCanvas(CanvasWrapper canvas)
{
	//if (!IsEverythingOk()) return;

	// THIS VALUES SHOULD BE VARIABLES BUT HAVING SO FEW, I JUST ADDED THEM FIXED
	//canvas.SetColor(0, 255, 0, 255);
	//canvas.SetPosition(Vector2{ 7, 200 });
	//canvas.DrawString(lastCommandsLog, 1.5f, 1.5f);
}

void InputLogger::RenderSettings() {
	static auto cvarPluginEnabled = cvarManager->getCvar(CVAR_PLUGIN_ENABLED);
	auto pluginEnabled = cvarPluginEnabled.getBoolValue();

	if (ImGui::Checkbox("Enabled", &pluginEnabled)) {
		cvarPluginEnabled.setValue(pluginEnabled);
		cvarManager->executeCommand("writeconfig", false);
	}
}
