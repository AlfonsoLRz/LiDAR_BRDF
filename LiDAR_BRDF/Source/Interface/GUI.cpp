#include "stdafx.h"
#include "GUI.h"

#include <algorithm>

#include "Graphics/Application/Renderer.h"
#include "Graphics/Application/TextureList.h"
#include "Graphics/Core/LiDARSimulation.h"
#include "Interface/Fonts/font_awesome.hpp"
#include "Interface/Fonts/lato.hpp"
#include "Interface/Fonts/IconsFontAwesome5.h"

/// Initialization of static attributes

const unsigned GUI::INTERPOLATION_RESOLUTION = 100;

/// [Protected methods]

GUI::GUI() :
	_customClasses(nullptr), _lidarClasses(nullptr), _showCameraSettings(false), _showLidarSettings(false), _showLidarASPRSClasses(false),_showLidarCustomClasses(false),
	_showRenderingSettings(false), _showSceneSettings(false), _showScreenshotSettings(false), _showAboutUs(false), _showControls(false), _showALSCanvas(false),
	_showTLSPointsFileDialog(false), _showTLSResolution(false), _showTLSCanvas(false)
{
	_LiDARParams		= LiDARSimulation::getLiDARParams();
	_pointCloudParams	= LiDARSimulation::getPointCloudParams();
	_renderer			= Renderer::getInstance();	
	_renderingParams	= Renderer::getInstance()->getRenderingParameters();

	_catmullRom			= new CatmullRom(_LiDARParams->_returnThreshold);
	this->buildLossPath();
}

void GUI::buildLossPath()
{
	_catmullRomPath_x.clear();
	_catmullRomPath_y.clear();

	std::vector<vec4> path;
	std::vector<float> timeKey;
	const float xAdvance = 0.005f;
	float x = .0f;

	while (x <= 1.0f && x < _LiDARParams->_zeroThreshold)
	{
		timeKey.push_back(x);
		path.emplace_back(x, .0f, .0f, 1.0f);
		
		x += xAdvance;
	}

	while (x <= 1.01f)
	{
		timeKey.push_back(x);
		path.emplace_back(x, _LiDARParams->_multCoefficient * std::pow(x + _LiDARParams->_addCoefficient, _LiDARParams->_lossPower), .0f, 1.0f);

		x += xAdvance;
	}

	_catmullRom->setWaypoints(path);
	_catmullRom->setTimeKey(timeKey);

	std::vector<vec4> interpolatedPath;
	_catmullRom->getPath(interpolatedPath, INTERPOLATION_RESOLUTION);

	for (vec4& point: interpolatedPath)
	{
		_catmullRomPath_x.push_back(point.x);
		_catmullRomPath_y.push_back(point.y);
	}
}

void GUI::createMainLayout()
{
}

void GUI::createMenu()
{
	ImGuiIO& io = ImGui::GetIO();

	if (_showRenderingSettings)		showRenderingSettings();
	if (_showCameraSettings)		showCameraSettings();
	if (_showLidarSettings)			showLiDARSettings();
	if (_showScreenshotSettings)	showScreenshotSettings();
	if (_showSceneSettings)			showSceneSettings();
	if (_showLidarASPRSClasses)		showLiDARASPRSClasses();
	if (_showLidarCustomClasses)	showLiDARCustomClasses();
	if (_showPointCloudSettings)	showPointCloudSettings();
	if (_showAboutUs)				showAboutUsWindow();
	if (_showControls)				showControls();
	if (_showLiDARExamples)			showLiDARExamples();
	if (_showALSCanvas)				showALSCanvas();
	if (_showTLSCanvas)				showTLSCanvas();
	if (_showTLSPointsFileDialog)	showTLSPositionsFileDialog();
	if (_showTLSResolution)			showTLSRanges();

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu(ICON_FA_COG "Settings"))
		{
			ImGui::MenuItem(ICON_FA_CUBE "Rendering", NULL, &_showRenderingSettings);
			ImGui::MenuItem(ICON_FA_CAMERA "Camera", NULL, &_showCameraSettings);
			ImGui::MenuItem(ICON_FA_IMAGE "Screenshot", NULL, &_showScreenshotSettings);
			ImGui::MenuItem(ICON_FA_SATELLITE_DISH "LiDAR", NULL, &_showLidarSettings);
			ImGui::MenuItem(ICON_FA_TREE "Scene", NULL, &_showSceneSettings);
			ImGui::MenuItem(ICON_FA_SAVE "Save Point Cloud", NULL, &_showPointCloudSettings);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(ICON_FA_QUESTION_CIRCLE "Help"))
		{
			ImGui::MenuItem(ICON_FA_INFO "About the project", NULL, &_showAboutUs);
			ImGui::MenuItem(ICON_FA_GAMEPAD "Controls", NULL, &_showControls);
			ImGui::MenuItem(ICON_FA_SATELLITE_DISH "LiDAR Models", NULL, &_showLiDARExamples);
			ImGui::EndMenu();
		}

		ImGui::SameLine();
		ImGui::SetCursorPosX(io.DisplaySize.x - 125);
		this->renderHelpMarker("Avoids some movements to also modify the camera parameters");
		
		ImGui::SameLine(0, 20);
		ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
		ImGui::EndMainMenuBar();
	}

	this->createMainLayout();
}

void GUI::leaveSpace(const unsigned numSlots)
{
	for (int i = 0; i < numSlots; ++i)
	{
		ImGui::Spacing();
	}
}

void GUI::renderHelpMarker(const char* message)
{
	ImGui::TextDisabled(ICON_FA_QUESTION);
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(message);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

void GUI::showAboutUsWindow()
{
	if (ImGui::Begin("About the project", &_showAboutUs))
	{
		ImGui::Text("This code belongs to a research project from University of Jaen (GGGJ group).");	
	}

	ImGui::End();
}

void GUI::showALSCanvas()
{
	if (ImGui::Begin("Aerial Path", &_showALSCanvas))
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		static bool addLine = false;

		// Controls of canvas
		{
			if (ImGui::Button("Clear"))
			{
				_LiDARParams->_alsManualPath.clear();
			}

			ImGui::PushItemWidth(120.0f);
			ImGui::SameLine(0, 20);
			ImGui::SliderFloat("Douglas Pecker Eps.", &_LiDARParams->_douglasPeckerEpsilon, .0f, 100.0f);
			ImGui::SameLine(0, 10);
			ImGui::Checkbox("Use Catmull-Rom", &_LiDARParams->_useCatmullRom);
			ImGui::PopItemWidth();

			this->leaveSpace(1);
			ImGui::Text("Left-click and drag to add lines");
			ImGui::Text("Right-click to delete last line");
			this->leaveSpace(2);
		}

		// Background
		ImVec2 canvasPos = ImGui::GetCursorScreenPos();            // ImDrawList API uses screen coordinates!
		ImVec2 canvasSize = ImGui::GetContentRegionAvail();        // Resize canvas to what's available
		Texture* imageTexture = _renderer->getCurrentScene()->getAerialView();

		canvasSize.x = std::max(canvasSize.x, 50.0f);
		canvasSize.y = std::max(canvasSize.y, 50.0f);
		drawList->AddImage(static_cast<ImTextureID>(static_cast<intptr_t>(imageTexture->getID())), canvasPos,
		                   ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y));
		drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(28, 28, 28, 10));
		drawList->AddRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(200, 200, 200, 255));
		drawList->PushClipRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), true);      // Clip lines within the canvas (if we resize it, etc.)
		
		ImGui::InvisibleButton("Canvas", canvasSize);
		ImVec2 mousePosCanvas = ImVec2(ImGui::GetIO().MousePos.x - canvasPos.x, ImGui::GetIO().MousePos.y - canvasPos.y);

		if (addLine)
		{
			_LiDARParams->_alsManualPath.emplace_back(mousePosCanvas.x, mousePosCanvas.y);

			if (!ImGui::IsMouseDown(0))
			{
				addLine = false;
			}
		}

		if (ImGui::IsItemHovered())
		{
			if (!addLine && ImGui::IsMouseClicked(0))
			{
				_LiDARParams->_alsManualPath.emplace_back(mousePosCanvas.x, mousePosCanvas.y);
				addLine = true;
			}

			if (ImGui::IsMouseClicked(1) && !_LiDARParams->_alsManualPath.empty())
			{
				addLine = false;
				_LiDARParams->_alsManualPath.pop_back();
			}
		}

		drawList->PushClipRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), true);      // Clip lines within the canvas (if we resize it, etc.)

		for (int i = 0; i < static_cast<int>(_LiDARParams->_alsManualPath.size() - 1); i += 1)
		{
			drawList->AddLine(
				ImVec2(canvasPos.x + _LiDARParams->_alsManualPath[i].x, canvasPos.y + _LiDARParams->_alsManualPath[i].y),
				ImVec2(canvasPos.x + _LiDARParams->_alsManualPath[i + 1].x, canvasPos.y + _LiDARParams->_alsManualPath[i + 1].y),
				IM_COL32(255, 255, 0, 255), 2.0f
			);
		}

		//static bool modifiedPath = false;

		//if (_LiDARParams->_alsManualPath.size() > 200 && !modifiedPath)
		//{
		//	modifiedPath = true;

		//	std::vector<vec2> result = douglasPecker(_LiDARParams->_alsManualPath, 20.0f);
		//	_LiDARParams->_alsManualPath = result;
		//	
		//	std::vector<vec4> waypoints;
		//	for (vec2& point: _LiDARParams->_alsManualPath)
		//	{
		//		waypoints.push_back(vec4(point, .0f, .0f));
		//	}
		//	
		//	_LiDARParams->_alsManualPath.clear();
		//	BezierCurve* bezier = new BezierCurve(waypoints);
		//	float t = .0f;
		//	bool finished;
		//	
		//	while (t < 1.0f)
		//	{
		//		_LiDARParams->_alsManualPath.push_back(bezier->getPosition(t, finished));
		//		t += .01f;
		//	}
		//}

		_LiDARParams->_alsManualPathCanvasSize = vec2(canvasSize.x, canvasSize.y);
		drawList->PopClipRect();
		
		ImGui::End();
	}
}

void GUI::showALSTabItem()
{
	LiDARScene* scene = Renderer::getInstance()->getCurrentScene();

	if (ImGui::BeginTabItem("Aerial LiDAR (ALS)"))
	{
		this->leaveSpace(2);

		{
			ImGui::PushID(0);
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(347.0f / 360.0f, 76.0f / 100.0f, 94.0f / 100.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(347.0f / 360.0f, 66.0f / 100.0f, 94.0f / 100.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(347.0f / 360.0f, 66.0f / 100.0f, 94.0f / 100.0f));

			if (ImGui::Button("Show canvas"))
			{
				_showALSCanvas = true;
			}

			ImGui::SameLine(0, 20);
			ImGui::Checkbox("Use user-defined path", &_LiDARParams->_alsUseManualPath);

			ImGui::PopStyleColor(3);
			ImGui::PopID();
		}
		
		this->leaveSpace(3);

		AABB sceneAABB = _renderer->getCurrentScene()->getAABB();

		ImGui::SliderFloat("LiDAR height", &_LiDARParams->_alsPosition.y, sceneAABB.max().y - sceneAABB.size().y / 2.0f, sceneAABB.max().y + sceneAABB.size().y * 10.0f);
		ImGui::SliderFloat("FOV Horizontal (Degrees)", &_LiDARParams->_alsFOVHorizontal, LiDARParameters::ALS_MIN_FOV, LiDARParameters::ALS_MAX_FOV);
		ImGui::SliderFloat("Ellipse Scale (Elliptical LiDAR)", &_LiDARParams->_alsFOVVertical, LiDARParameters::ALS_MIN_ELLIPTICAL_SCALE, LiDARParameters::ALS_MAX_ELLIPTICAL_SCALE);
		ImGui::SliderFloat("Speed (m/s)", &_LiDARParams->_alsSpeed, LiDARParameters::ALS_MIN_SPEED, LiDARParameters::ALS_MAX_SPEED);
		ImGui::SliderInt("Scan Frequency (Scans/s)", &_LiDARParams->_alsScanFrequency, LiDARParameters::ALS_MIN_SCAN_FREQUENCY, LiDARParameters::ALS_MAX_SCAN_FREQUENCY);
		ImGui::SliderInt("Pulse Frequency (Pulses/s)", &_LiDARParams->_alsPulseFrequency, LiDARParameters::ALS_MIN_PULSE_FREQUENCY, LiDARParameters::ALS_MAX_PULSE_FREQUENCY);

		this->leaveSpace(2);
		
		ImGui::SliderInt("Max. Scene Sweeps", &_LiDARParams->_alsMaxSceneSweeps, LiDARParameters::ALS_MIN_SCENE_SWEEPS, LiDARParameters::ALS_MAX_SCENE_SWEEPS);
		ImGui::SliderFloat("Overlapping", &_LiDARParams->_alsOverlapping, .0f, 1.0f);
		
		this->leaveSpace(2);

		ImGui::SliderFloat("Jittering of airbone height", &_LiDARParams->_alsHeightJittering, LiDARParameters::ALS_MIN_HEIGHT_JITTERING, LiDARParameters::ALS_MAX_HEIGHT_JITTERING, "%.8f", ImGuiSliderFlags_Logarithmic);
		ImGui::SliderFloat("Jittering of rays", &_LiDARParams->_alsRayJittering, LiDARParameters::ALS_MIN_RAY_JITTERING, LiDARParameters::ALS_MAX_RAY_JITTERING, "%.8f", ImGuiSliderFlags_Logarithmic);
		
		ImGui::EndTabItem();
	}
}

void GUI::showCameraSettings()
{
	if (ImGui::Begin("Scene controls", &_showCameraSettings))
	{
		Camera* camera = _renderer->getActiveCamera();

		if (ImGui::Button("Translate to LiDAR Position"))
		{
			if (_LiDARParams->_LiDARType == LiDARParameters::TERRESTRIAL_SPHERICAL)
			{
				camera->setPosition(_LiDARParams->_tlsPosition);
			}
			else
			{
				camera->setPosition(_LiDARParams->_alsPosition);
			}
		}

		this->leaveSpace(2);

		ImGui::PushItemWidth(150.0f);

		ImGui::SliderFloat("ZNear", &camera->_zNear, .0001f, camera->_zFar); ImGui::SameLine(0, 10);
		ImGui::SliderFloat("ZFar", &camera->_zFar, camera->_zNear, 2000.0f);

		ImGui::PopItemWidth();
	}

	ImGui::End();
}

void GUI::showControls()
{
	if (ImGui::Begin("Scene controls", &_showControls))
	{
		ImGui::Columns(2, "ControlColumns"); // 4-ways, with border
		ImGui::Separator();
		ImGui::Text("Movement"); ImGui::NextColumn();
		ImGui::Text("Control"); ImGui::NextColumn();
		ImGui::Separator();

		const int NUM_MOVEMENTS = 14;
		const char* movement[] = { "Orbit (XZ)", "Undo Orbit (XZ)", "Orbit (Y)", "Undo Orbit (Y)", "Dolly", "Truck", "Boom", "Crane", "Reset Camera", "Take Screenshot", "Continue Animation", "Zoom +/-", "Pan", "Tilt" };
		const char* controls[] = { "X", "Ctrl + X", "Y", "Ctrl + Y", "W, S", "D, A", "Up arrow", "Down arrow", "R", "K", "I", "Scroll wheel", "Move mouse horizontally(hold button)", "Move mouse vertically (hold button)" };

		for (int i = 0; i < NUM_MOVEMENTS; i++)
		{
			ImGui::Text(movement[i]); ImGui::NextColumn();
			ImGui::Text(controls[i]); ImGui::NextColumn();
		}

		ImGui::Columns(1);
		ImGui::Separator();

	}

	ImGui::End();
}

void GUI::showCurrentTLSPositions()
{
	LiDARScene* scene = Renderer::getInstance()->getCurrentScene();
	std::vector<vec3>* tlsPositions = scene->getLiDARSensor()->getTLSPositions();
	static unsigned pointSelected = 0;

	if (ImGui::BeginTabItem("TLS Positions"))
	{
		this->leaveSpace(2);

		{
			ImGui::PushID(0);
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1 / 7.0f, 0.6f, 0.6f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(1 / 7.0f, 0.7f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(1 / 7.0f, 0.8f, 0.8f));

			if (ImGui::Button("Load TLS Positions"))
			{
				_showTLSPointsFileDialog = true;
			}

			ImGui::PopStyleColor(3);
			ImGui::PopID();

			ImGui::PushID(0);
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(6.0f / 7.0f, 0.6f, 0.6f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(6.0f / 7.0f, 0.7f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(6.0f / 7.0f, 0.8f, 0.8f));

			ImGui::SameLine(0, 10);

			if (ImGui::Button("Clear TLS Positions"))
			{
				tlsPositions->clear();
				scene->getLiDARSensor()->setTLSPositions(*tlsPositions);
			}

			ImGui::SameLine(0, 10);

			if (ImGui::Button("Clear current position"))
			{
				if (pointSelected < tlsPositions->size())
				{
					tlsPositions->erase(tlsPositions->begin() + pointSelected);
				}
			}

			ImGui::PopStyleColor(3);
			ImGui::PopID();
		}

		this->leaveSpace(3);

		ImGui::BeginChild("TLS Positions", ImVec2(350, 0), true);

		for (int i = 0; i < tlsPositions->size(); ++i)
		{
			if (ImGui::Selectable((std::to_string(tlsPositions->at(i).x) + ", " + std::to_string(tlsPositions->at(i).y) + ", " + std::to_string(tlsPositions->at(i).z)).c_str(), pointSelected == i))
				pointSelected = i;
		}

		ImGui::EndChild();

		ImGui::EndTabItem();
	}
}

void GUI::showLiDARASPRSClasses()
{
	if (ImGui::Begin("ASPRS classes", &_showLidarASPRSClasses))
	{
		if (ImGui::BeginChild("ASPRSClassList"))
		{
			if (!_lidarClasses)
			{
				_lidarClasses = Model3D::getASPRSClasses();
			}

			int id = 0;
			for (auto pair : *_lidarClasses)
			{
				ImGui::Text(pair.first.c_str());

				ImGui::SameLine(250.0f);

				ImGui::PushID(id++);
				const char* label = "          ";
				ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(pair.second.x, pair.second.y, pair.second.z));
				ImGui::Button(label);
				ImGui::PopStyleColor(1);
				ImGui::PopID();
			}
		}

		ImGui::EndChild();
	}

	ImGui::End();
}

void GUI::showLiDARCustomClasses()
{
	if (ImGui::Begin("Custom classes", &_showLidarCustomClasses))
	{
		if (ImGui::BeginChild("CustomClassList"))
		{
			if (!_customClasses)
			{
				_customClasses = Model3D::getCustomClasses();
			}

			if (ImGui::Button("Save labels"))
			{
				Model3D::saveCustomClasses();
			}

			this->leaveSpace(2);

			int id = 0;
			for (auto pair : *_customClasses)
			{
				ImGui::Text(pair.first.c_str());

				ImGui::SameLine(150.0f);

				ImGui::PushID(id++);
				const char* label = "          ";
				ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(pair.second.x, pair.second.y, pair.second.z));
				ImGui::Button(label);
				ImGui::PopStyleColor(1);
				ImGui::PopID();
			}
		}

		ImGui::EndChild();
	}

	ImGui::End();
}

void GUI::showLiDARExamples()
{
	if (ImGui::Begin("LiDAR examples", &_showLiDARExamples))
	{
		const int NUM_MODELS = 7;
		const char* modelName[] = { "HDL-64E", "HDL-32E", "Puck", "Puck Lite", "Puck Hi-Res", "Ultra Puck", "Alpha Prime" };

		ImGui::Columns(1);

		ImGui::Text("Terrestrial LiDAR"); ImGui::NextColumn();
		
		ImGui::Columns(NUM_MODELS + 1, "LiDARModelColumns");
		ImGui::Separator();

		ImGui::Text("Attribute"); ImGui::NextColumn();

		for (int model = 0; model < NUM_MODELS; ++model)
		{
			ImGui::Text(modelName[model]); ImGui::NextColumn();
		}

		ImGui::Separator();

		const int NUM_ATTRIBUTES = 8;
		std::string attribute[] = { "Channels", "Range", "Vertical FOV (Deg.)", "Horizontal FOV (Deg.)", "Vertical resolution (Deg.)", "Horizontal resolution (Deg.)", "Power (watt)", "Diameter (mm)" };
		std::string channels[] = { "64", "32", "16", "16", "16", "32", "128" };
		std::string range[] = { "120", "100", "100", "100", "100", "200", "300" };
		std::string verticalFOV[] = { "26.9", "41.34", "30", "30", "20", "40", "40" };
		std::string horizontalFOV[] = { "360", "360", "360", "360", "360", "360", "360" };
		std::string verticalResolution[] = { "0.4", "1.33", "2", "2", "1.33", "0.33", "0.11" };
		std::string horizontalResolution[] = { "0.08", "0.1", "0.1", "0.1", "0.1", "0.1", "0.1" };
		std::string power[] = { "60", "12", "8", "8", "8", "10", "22" };
		std::string diameter[] = { "215", "85", "103", "103", "103", "103", "165.5" };
		std::vector<std::string*> tableRows{ channels, range, verticalFOV, horizontalFOV, verticalResolution, horizontalResolution, power, diameter };
		
		for (int attributeIdx = 0; attributeIdx < NUM_ATTRIBUTES; attributeIdx++)
		{
			ImGui::Text(attribute[attributeIdx].c_str()); ImGui::NextColumn();

			for (int modelIdx = 0; modelIdx < NUM_MODELS; ++modelIdx)
			{
				ImGui::Text(tableRows[attributeIdx][modelIdx].c_str()); ImGui::NextColumn();
			}
		}

		ImGui::Columns(1);
		ImGui::Separator();

	}

	ImGui::End();
}

void GUI::showLiDARSettings()
{
	if (ImGui::Begin("LiDAR Settings", &_showLidarSettings))
	{
		for (int i = 0; i < 2; i++) ImGui::Spacing();

		ImGui::PushID(0);
		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1 / 7.0f, 0.6f, 0.6f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(1 / 7.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(1 / 7.0f, 0.8f, 0.8f));

		if (ImGui::Button("Start simulation" ICON_FA_PLAY_CIRCLE))
		{
			_renderer->getCurrentScene()->launchSimulation();
		}

		ImGui::PopStyleColor(3);
		ImGui::PopID();

		ImGui::PushID(0);
		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(6.0f / 7.0f, 0.6f, 0.6f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(6.0f / 7.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(6.0f / 7.0f, 0.8f, 0.8f));

		ImGui::SameLine(0, 15);
		if (ImGui::Button("Reset simulation" ICON_FA_REMOVE_FORMAT))
		{
			_renderer->getCurrentScene()->clearSimulation();
		}

		ImGui::PopStyleColor(3);
		ImGui::PopID();

		ImVec2 windowSize = ImGui::GetContentRegionAvail();

		ImGui::SameLine(windowSize.x - 250);

		if (ImGui::Button("Show custom classes"))
		{
			_showLidarCustomClasses = true;
		}

		ImGui::SameLine();

		if (ImGui::Button("Show LiDAR classes"))
		{
			_showLidarASPRSClasses = true;
		}

		for (int i = 0; i < 4; i++) ImGui::Spacing();
		
		if (ImGui::BeginTabBar("LiDARTabBar"))
		{
			this->showLiDARTabItem();
			this->showALSTabItem();
			this->showTLSTabItem();
			this->showCurrentTLSPositions();

			ImGui::EndTabBar();
		}
	}

	ImGui::End();
}

void GUI::showLiDARTabItem()
{
	if (ImGui::BeginTabItem("General"))
	{
		this->leaveSpace(3);

		ImGui::Combo("LiDAR Type", &_LiDARParams->_LiDARType, _LiDARParams->RayBuilder_STR, IM_ARRAYSIZE(_LiDARParams->RayBuilder_STR));

		if (ImGui::Combo("LiDAR Specifications", &_LiDARParams->_LiDARSpecs, _LiDARParams->LiDARSpecs_STR, IM_ARRAYSIZE(_LiDARParams->LiDARSpecs_STR)))
		{
			_LiDARParams->buildSpecifications();
		}

		ImGui::Checkbox("GPU Instancing", &_LiDARParams->_gpuInstantiation);
		ImGui::SameLine(0, 20); 
		ImGui::PushItemWidth(120.0f); 
		ImGui::SliderInt("Num. Executions", &_LiDARParams->_numExecs, 1, 10); 
		ImGui::SameLine(0, 20);
		ImGui::Checkbox("Omit First Execution", &_LiDARParams->_discardFirstExecution);
		ImGui::SameLine(0, 20);
		ImGui::SliderInt("Ray Divisor", &_LiDARParams->_rayDivisor, 1, 20);
		ImGui::PopItemWidth();
		ImGui::Checkbox("Use Time", &_LiDARParams->_useSimulationTime);
		ImGui::SameLine(0, 20);
		ImGui::PushItemWidth(120.0f);
		ImGui::SliderFloat("Simulation Time (s)", &_LiDARParams->_simulationTime, .0f, 100.0f);
		ImGui::PopItemWidth();

		this->leaveSpace(5);

		if (ImGui::CollapsingHeader("Physical Properties"))
		{
			this->leaveSpace(2);

			ImGui::SliderInt2("Wavelength (nm)", &_LiDARParams->_wavelength[0], 400, 1000);
			ImGui::SliderScalar("Pulse Radius (m)", ImGuiDataType_Float, &_LiDARParams->_pulseRadius, &_LiDARParams->MIN_PULSE_RADIUS, &_LiDARParams->MAX_PULSE_RADIUS);
			ImGui::SliderInt("Rays per Pulse", &_LiDARParams->_raysPulse, _LiDARParams->MIN_RAYS_PULSE, _LiDARParams->MAX_RAYS_PULSE);
			ImGui::SliderScalar("Peak Power (watts)", ImGuiDataType_Float , &_LiDARParams->_peakPower, &_LiDARParams->MIN_PEAK_POWER, &_LiDARParams->MAX_PEAK_POWER);
			ImGui::SliderScalar("Sensor Diameter (m)", ImGuiDataType_Float , &_LiDARParams->_sensorDiameter, &_LiDARParams->MIN_SENSOR_DIAMETER, &_LiDARParams->MAX_SENSOR_DIAMETER);
			ImGui::SliderScalar("Maximum Number of Bounces", ImGuiDataType_U8, &_LiDARParams->_maxReturns, &_LiDARParams->MIN_NUMBER_OF_RETURNS, &_LiDARParams->MAX_NUMBER_OF_RETURNS);
			ImGui::Combo("Number of Channels", &_LiDARParams->_channels, _LiDARParams->Channels_STR, IM_ARRAYSIZE(_LiDARParams->Channels_STR));
			if (ImGui::SliderScalar("Maximum Range (m)", ImGuiDataType_Float, &_LiDARParams->_maxRange, &_LiDARParams->MIN_RANGE, &_LiDARParams->MAX_RANGE))
			{
				_LiDARParams->_maxRangeSoftBoundary = glm::clamp(_LiDARParams->_maxRangeSoftBoundary, vec2(-0.05f * _LiDARParams->_maxRange), vec2(0.05f * _LiDARParams->_maxRange));
			}
			ImGui::DragFloatRange2("Maximum Range Noisy Boundary", &_LiDARParams->_maxRangeSoftBoundary.x, &_LiDARParams->_maxRangeSoftBoundary.y, .001f,
								   -0.15f * _LiDARParams->_maxRange, 0.15f * _LiDARParams->_maxRange,
							       "Min. Boundary: %.2f (m)", "Max. Boundary: %.2f (m)");
			ImGui::SliderFloat("System attenuation", &_LiDARParams->_systemAttenuation, LiDARParameters::MIN_NORMALIZED_VALUE, LiDARParameters::MAX_NORMALIZED_VALUE);
			
			this->leaveSpace(2);
		}

		if (ImGui::CollapsingHeader("Induced Errors"))
		{
			this->leaveSpace(2);

			ImGui::Checkbox("Terrain Induced Error", &_LiDARParams->_includeTerrainInducedError);
			ImGui::Checkbox("Shiny Surface Induced Error", &_LiDARParams->_includeShinySurfaceError);	
			ImGui::Checkbox("Outlier Points", &_LiDARParams->_includeOutliers);

			{
				this->leaveSpace(1);

				ImGui::NewLine();
				ImGui::SameLine(30, 0);
				ImGui::SliderScalar("Outlier Threshold", ImGuiDataType_Float, &_LiDARParams->_outlierThreshold, &_LiDARParams->MIN_OUTLIER_THRESHOLD, &_LiDARParams->MAX_OUTLIER_THRESHOLD);

				ImGui::NewLine();
				ImGui::SameLine(30, 0);
				ImGui::DragFloatRange2("Outlier Atmospheric Range", &_LiDARParams->_outlierRange.x, &_LiDARParams->_outlierRange.y, .01f, 
									   LiDARParameters::MIN_NORMALIZED_VALUE, LiDARParameters::MAX_NORMALIZED_VALUE,  
									   "Min. Parametric: %.2f %%", "Max. Parametric: %.2f %%");
				
				this->leaveSpace(1);
			}

			this->leaveSpace(2);
		}

		if (ImGui::CollapsingHeader("Loss Behaviour"))
		{
			this->leaveSpace(2);

			if (ImGui::SliderFloat("Mult. Coefficient", &_LiDARParams->_multCoefficient, 1.0f, 200.0f))
			{
				this->buildLossPath();
			}

			if (ImGui::SliderFloat("Add Coefficient", &_LiDARParams->_addCoefficient, -1.0f, 1.0f))
			{
				this->buildLossPath();
			}

			if (ImGui::SliderFloat("Power", &_LiDARParams->_lossPower, 1.0f, 20.0f))
			{
				this->buildLossPath();
			}

			if (ImGui::SliderFloat("Loss Threshold", &_LiDARParams->_zeroThreshold, .0f, 1.0f))
			{
				this->buildLossPath();
			}

			this->leaveSpace(3);

			if (ImPlot::BeginPlot("Thresholds for Multiple Return LiDAR")) 
			{
				ImPlot::SetNextAxesLimits(-0.1f, 1.1f, -0.1f, 1.1f, ImGuiCond_Always);
				ImPlot::SetupAxes("Return", "Normalized Threshold");
				ImPlot::PlotLine("Threshold Function", _catmullRomPath_x.data(), _catmullRomPath_y.data(), _catmullRomPath_x.size());

				ImPlot::EndPlot();
			}

			this->leaveSpace(2);
		}
		
		if (ImGui::CollapsingHeader("Intensity"))
		{
			this->leaveSpace(2);

			ImGui::SliderFloat("Reflectance Weight", &_LiDARParams->_reflectanceWeight, _LiDARParams->MIN_REFLECTANCE_WEIGHT, _LiDARParams->MAX_REFLECTANCE_WEIGHT);

			this->leaveSpace(2);
		}

		ImGui::EndTabItem();
	}
}

void GUI::showPointCloudSettings()
{
	if (ImGui::Begin("Point Cloud File", &_showPointCloudSettings, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::InputText("Filename", _pointCloudParams->_filenameBuffer, IM_ARRAYSIZE(_pointCloudParams->_filenameBuffer));
		this->leaveSpace(1);
		ImGui::Checkbox("Asynchronous Write", &_pointCloudParams->_asynchronousWrite);
		ImGui::Checkbox("Save Point Cloud", &_pointCloudParams->_savePointCloud);
	}

	ImGui::End();
}

void GUI::showRenderingSettings()
{
	if (ImGui::Begin("Rendering Settings", &_showRenderingSettings))
	{
		ImGui::ColorEdit3("Background color", &_renderingParams->_backgroundColor[0]);

		this->leaveSpace(3);

		if (ImGui::BeginTabBar("LiDARTabBar"))
		{
			if (ImGui::BeginTabItem("General settings"))
			{
				this->leaveSpace(1);

				ImGui::Separator();
				ImGui::Text(ICON_FA_LIGHTBULB "Lighting");

				ImGui::SliderFloat("Scattering", &_renderingParams->_materialScattering, 0.0f, 10.0f);

				this->leaveSpace(2);

				ImGui::Separator();
				ImGui::Text(ICON_FA_TREE "Scenario");

				ImGui::Checkbox("Render scenario", &_renderingParams->_showTriangleMesh);

				{
					ImGui::Spacing();

					ImGui::NewLine();
					ImGui::SameLine(30, 0);
					ImGui::Checkbox("Screen Space Ambient Occlusion", &_renderingParams->_ambientOcclusion);

					ImGui::NewLine();
					ImGui::SameLine(30, 0);
					ImGui::Checkbox("Render Semantic Concepts", &_renderingParams->_renderSemanticConcept);

					{
						ImGui::SameLine(0, 30);
						ImGui::RadioButton("Custom class", &_renderingParams->_semanticRenderingConcept, RenderingParameters::CUSTOM_CONCEPT); ImGui::SameLine();
						ImGui::RadioButton("ASPRS class", &_renderingParams->_semanticRenderingConcept, RenderingParameters::ASPRS_CONCEPT);
					}

					const char* visualizationTitles[] = { "Points", "Lines", "Triangles", "All" };
					ImGui::NewLine();
					ImGui::SameLine(30, 0);
					ImGui::Combo("Visualization", &_renderingParams->_visualizationMode, visualizationTitles, IM_ARRAYSIZE(visualizationTitles));

					ImGui::Spacing();
				}

				ImGui::Checkbox("Render liDAR point cloud", &_renderingParams->_showLidarPointCloud);
				ImGui::NewLine(); ImGui::SameLine(30, 0); ImGui::RadioButton("Uniform", &_renderingParams->_pointCloudType, RenderingParameters::UNIFORM);
				ImGui::NewLine(); ImGui::SameLine(30, 0); ImGui::RadioButton("RGB", &_renderingParams->_pointCloudType, RenderingParameters::RGB);
				ImGui::NewLine(); ImGui::SameLine(30, 0); ImGui::RadioButton("Height", &_renderingParams->_pointCloudType, RenderingParameters::HEIGHT);

				{
					ImGui::SameLine(200, 0);
					ImGui::Checkbox("Grayscale", &_renderingParams->_grayscaleHeight);

					ImGui::SameLine(0, 20); ImGui::PushItemWidth(300.0f);
					ImGui::DragFloatRange2("Height Boundaries", &_renderingParams->_heightBoundaries.x, &_renderingParams->_heightBoundaries.y, .01f,
											-1.0f, 2.0f,
											"Min. Height: %.2f %%", "Max. Height: %.2f %%");
				}
				
				ImGui::NewLine(); ImGui::SameLine(30, 0); ImGui::RadioButton("Instance", &_renderingParams->_pointCloudType, RenderingParameters::INSTANCE);
				ImGui::NewLine(); ImGui::SameLine(30, 0); ImGui::RadioButton("Custom semantic concept", &_renderingParams->_pointCloudType, RenderingParameters::CUSTOM_SEMANTIC);
				ImGui::NewLine(); ImGui::SameLine(30, 0); ImGui::RadioButton("ASPRS semantic concept", &_renderingParams->_pointCloudType, RenderingParameters::ASPRS_SEMANTIC);
				ImGui::NewLine(); ImGui::SameLine(30, 0); ImGui::RadioButton("Return number", &_renderingParams->_pointCloudType, RenderingParameters::RETURN_NUMBER);

				{
					ImGui::SameLine(200, 0);
					ImGui::PushItemWidth(60.0f);
					ImGui::SliderScalar("First return rendered", ImGuiDataType_U8, &_renderingParams->_lastReturnRendered, &_LiDARParams->MIN_NUMBER_OF_RETURNS, &_LiDARParams->MAX_NUMBER_OF_RETURNS);

					ImGui::SameLine(0, 15);
					ImGui::PushItemWidth(100.0f);
					ImGui::SliderScalar("Return / Number of returns", ImGuiDataType_Float, &_renderingParams->_returnDivisionThreshold, &_LiDARParams->MIN_NORMALIZED_VALUE, &_LiDARParams->MAX_NORMALIZED_VALUE);
				}

				{
					ImGui::NewLine(); ImGui::SameLine(30, 0); ImGui::RadioButton("Intensity", &_renderingParams->_pointCloudType, RenderingParameters::INTENSITY);

					ImGui::SameLine(200, 0);
					ImGui::PushItemWidth(100.0f);
					ImGui::SliderFloat("Addition", &_renderingParams->_intensityAddition, -1.0f, 1.0f);

					ImGui::SameLine(0, 15.0f);
					ImGui::SliderFloat("Multiply", &_renderingParams->_intensityMultiplier, -3.0f, 3.0f);

					ImGui::NewLine();
					ImGui::SameLine(200, 0);
					ImGui::Checkbox("Tone Mapping", &_renderingParams->_intensityHDR);

					ImGui::SameLine(0, 15.0f);
					ImGui::Checkbox("Grayscale Intensity", &_renderingParams->_grayscaleIntensity);

					ImGui::SameLine(0, 15.0f);
					ImGui::SliderFloat("Gamma", &_renderingParams->_intensityGamma, .1f, 10.0f);

					ImGui::SameLine(0, 15.0f);
					ImGui::SliderFloat("Exposure", &_renderingParams->_intensityExposure, .1f, 20.0f);
					ImGui::PopItemWidth();
				}

				ImGui::NewLine(); ImGui::SameLine(30, 0); ImGui::RadioButton("Normal vector", &_renderingParams->_pointCloudType, RenderingParameters::NORMAL);
				ImGui::NewLine(); ImGui::SameLine(30, 0); ImGui::RadioButton("Scan angle rank", &_renderingParams->_pointCloudType, RenderingParameters::SCAN_ANGLE_RANK);
				ImGui::NewLine(); ImGui::SameLine(30, 0); ImGui::RadioButton("Scan direction", &_renderingParams->_pointCloudType, RenderingParameters::SCAN_DIRECTION);
				ImGui::NewLine(); ImGui::SameLine(30, 0); ImGui::RadioButton("GPS time", &_renderingParams->_pointCloudType, RenderingParameters::GPS_TIME);

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("LiDAR"))
			{
				this->leaveSpace(1);

				ImGui::Checkbox("Render LiDAR model", &_renderingParams->_showLidarModel);
				ImGui::SameLine(0, 15);
				ImGui::Text("(Basic scene rendering must be enabled)");
				
				ImGui::Checkbox("Render LiDAR rays", &_renderingParams->_showLidarRays);
				ImGui::NewLine(); ImGui::SameLine(0, 22); ImGui::PushItemWidth(150.0);
				ImGui::SliderFloat("Ray percentage", &_renderingParams->_raysPercentage, 0.0f, 1.0f);
				ImGui::PopItemWidth();
				
				ImGui::Checkbox("Render LiDAR aerial path", &_renderingParams->_showLiDARPath);
				ImGui::SameLine(0, 20);
				if (ImGui::Button("Update Path"))
				{
					_renderer->getCurrentScene()->updatePath();
				}

				ImGui::Checkbox("Render LiDAR beam", &_renderingParams->_showLidarBeam);
				ImGui::NewLine(); ImGui::SameLine(0, 22); ImGui::PushItemWidth(250.0f);
				ImGui::SliderFloat3("Normal vector", &_renderingParams->_lidarBeamNormal[0], -1.0f, 1.0f);
				ImGui::PopItemWidth(); ImGui::NewLine(); ImGui::SameLine(0, 22); ImGui::PushItemWidth(100.0f);
				ImGui::SliderInt("Number of subdivisions", &_renderingParams->_numLiDARRBeamSubdivisions, 10, 50);
				ImGui::PopItemWidth();

				ImGui::Checkbox("Render LiDAR maximum range", &_renderingParams->_showLidarMaxRange);
				ImGui::NewLine(); ImGui::SameLine(0, 22); ImGui::PushItemWidth(100.0f);
				ImGui::SliderInt("Number of subdivisions", &_renderingParams->_numLiDARRangeSubdivisions, 10, 100);
				ImGui::PopItemWidth();

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Data Structures"))
			{
				this->leaveSpace(1);

				ImGui::Checkbox("Render BVH", &_renderingParams->_showBVH);

				{
					this->leaveSpace(1);
					ImGui::NewLine(); ImGui::SameLine(0, 22);
					ImGui::ColorEdit3("BVH color", &_renderingParams->_bvhWireframeColor[0]);
					ImGui::NewLine(); ImGui::SameLine(0, 22);
					ImGui::SliderFloat("BVH nodes", &_renderingParams->_bvhNodesPercentage, 0.0f, 1.0f);
					this->leaveSpace(2);
				}

				ImGui::Checkbox("Render Terrain Regular Grid", &_renderingParams->_showTerrainRegularGrid);
				this->leaveSpace(1);

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Point Cloud"))
			{
				this->leaveSpace(1);

				ImGui::SliderFloat("Point Size", &_renderingParams->_lidarPointSize, 0.1f, 50.0f);
				ImGui::ColorEdit3("Point Cloud Color", &_renderingParams->_lidarPointCloudColor[0]);

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Wireframe"))
			{
				this->leaveSpace(1);

				ImGui::ColorEdit3("Wireframe Color", &_renderingParams->_wireframeColor[0]);
				ImGui::SliderFloat("Line Width", &_renderingParams->_lineWidth, 1.0f, 30.0f);

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Terrain"))
			{
				this->leaveSpace(1);

				ImGui::SliderFloat("Snow Height Factor", &_renderingParams->_snowHeightFactor, .0f, 100.0f);
				ImGui::SliderFloat("Snow Height Threshold", &_renderingParams->_snowHeightThreshold, .0f, 1.0f);
				ImGui::SliderFloat("Snow Slope Factor", &_renderingParams->_snowSlopeFactor, .0f, 50.0f);

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
	}

	ImGui::End();
}

void GUI::showSceneSettings()
{
	std::vector<Model3D::ModelComponent*>* modelComps = _renderer->getCurrentScene()->getModelComponents();

	ImGui::SetNextWindowSize(ImVec2(480, 440), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Scene Models", &_showSceneSettings, ImGuiWindowFlags_None))
	{
		this->leaveSpace(2);

		// Left
		static int modelCompSelected = 0;

		ImGui::BeginChild("Model Components", ImVec2(350, 0), true);

		for (int i = 0; i < modelComps->size(); ++i)
		{
			if (ImGui::Selectable(modelComps->at(i)->_name.c_str(), modelCompSelected == i))
				modelCompSelected = i;
		}

		ImGui::EndChild();

		ImGui::SameLine();

		// Right
		ImGui::BeginGroup();
		ImGui::BeginChild("Model Component View", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));		// Leave room for 1 line below us

		ImGui::Text(modelComps->at(modelCompSelected)->_name.c_str());
		ImGui::Separator();

		if (ImGui::BeginTabBar("Model Data", ImGuiTabBarFlags_None))
		{
			const vec2 nFloatBoundaries = vec2(.0f, 1.0f);

			if (ImGui::BeginTabItem("Settings"))
			{
				ImGui::Checkbox("Enabled", &modelComps->at(modelCompSelected)->_enabled);
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::EndChild();
		ImGui::EndGroup();
	}

	ImGui::End();
}

void GUI::showScreenshotSettings()
{
	if (ImGui::Begin("Screenshot Settings", &_showScreenshotSettings, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::SliderFloat("Size multiplier", &_renderingParams->_screenshotMultiplier, 1.0f, 10.0f);
		ImGui::InputText("Filename", _renderingParams->_screenshotFilenameBuffer, IM_ARRAYSIZE(_renderingParams->_screenshotFilenameBuffer));
		ImGui::SliderFloat("Alpha", &_renderingParams->_screenshotAlpha, 0.0f, 1.0f);

		this->leaveSpace(2);

		ImGui::PushID(0);
		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1 / 7.0f, 0.6f, 0.6f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(1 / 7.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(1 / 7.0f, 0.8f, 0.8f));

		if (ImGui::Button("Take screenshot"))
		{
			std::string filename = _renderingParams->_screenshotFilenameBuffer;

			if (filename.empty())
			{
				filename = "Screenshot.png";
			}
			else if (filename.find(".png") == std::string::npos)
			{
				filename += ".png";
			}

			Renderer::getInstance()->getScreenshot(filename);
		}

		ImGui::PopStyleColor(3);
		ImGui::PopID();
	}

	ImGui::End();
}

void GUI::showTLSCanvas()
{
	if (ImGui::Begin("Terrestrial Path", &_showTLSCanvas))
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		static bool addLineTLS = false;

		// Controls of canvas
		{
			if (ImGui::Button("Clear"))
			{
				_LiDARParams->_tlsManualPath.clear();
			}

			ImGui::PushItemWidth(120.0f);
			ImGui::SameLine(0, 20);
			ImGui::SliderFloat("Douglas Pecker Eps.", &_LiDARParams->_douglasPeckerEpsilon, .0f, 100.0f);
			ImGui::SameLine(0, 10);
			ImGui::Checkbox("Use Catmull-Rom", &_LiDARParams->_useCatmullRom);
			ImGui::PopItemWidth();

			this->leaveSpace(1);
			ImGui::Text("Left-click and drag to add lines");
			ImGui::Text("Right-click to delete last line");
			this->leaveSpace(2);
		}

		// Background
		ImVec2 canvasPos = ImGui::GetCursorScreenPos();            // ImDrawList API uses screen coordinates!
		ImVec2 canvasSize = ImGui::GetContentRegionAvail();        // Resize canvas to what's available
		Texture* imageTexture = _renderer->getCurrentScene()->getAerialView();

		canvasSize.x = std::max(canvasSize.x, 50.0f);
		canvasSize.y = std::max(canvasSize.y, 50.0f);
		drawList->AddImage(static_cast<ImTextureID>(static_cast<intptr_t>(imageTexture->getID())), canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y));
		drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(28, 28, 28, 10));
		drawList->AddRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(200, 200, 200, 255));
		drawList->PushClipRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), true);      // Clip lines within the canvas (if we resize it, etc.)

		ImGui::InvisibleButton("Canvas", canvasSize);
		ImVec2 mousePosCanvas = ImVec2(ImGui::GetIO().MousePos.x - canvasPos.x, ImGui::GetIO().MousePos.y - canvasPos.y);

		if (addLineTLS)
		{
			_LiDARParams->_tlsManualPath.emplace_back(mousePosCanvas.x, mousePosCanvas.y);

			if (!ImGui::IsMouseDown(0))
			{
				addLineTLS = false;
			}
		}

		if (ImGui::IsItemHovered())
		{
			if (!addLineTLS && ImGui::IsMouseClicked(0))
			{
				_LiDARParams->_tlsManualPath.emplace_back(mousePosCanvas.x, mousePosCanvas.y);
				addLineTLS = true;
			}

			if (ImGui::IsMouseClicked(1) && !_LiDARParams->_tlsManualPath.empty())
			{
				addLineTLS = false;
				_LiDARParams->_tlsManualPath.pop_back();
			}
		}

		drawList->PushClipRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), true);      // Clip lines within the canvas (if we resize it, etc.)

		for (int i = 0; i < int(_LiDARParams->_tlsManualPath.size() - 1); i += 1)
		{
			drawList->AddLine(
				ImVec2(canvasPos.x + _LiDARParams->_tlsManualPath[i].x, canvasPos.y + _LiDARParams->_tlsManualPath[i].y),
				ImVec2(canvasPos.x + _LiDARParams->_tlsManualPath[i + 1].x, canvasPos.y + _LiDARParams->_tlsManualPath[i + 1].y),
				IM_COL32(255, 255, 0, 255), 2.0f
			);
		}

		_LiDARParams->_tlsManualPathCanvasSize = vec2(canvasSize.x, canvasSize.y);
		drawList->PopClipRect();

		ImGui::End();
	}
}

void GUI::showTLSRanges()
{
	if (ImGui::Begin("TLS Resolution", &_showTLSResolution))
	{
		ImGui::PushID(0);
		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1 / 7.0f, 0.6f, 0.6f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(1 / 7.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(1 / 7.0f, 0.8f, 0.8f));

		if (ImGui::Button("Add Range" ICON_FA_PLUS))
		{
			vec2 availableRange(_LiDARParams->_tlsMiddleAngleVertical - _LiDARParams->_tlsFOVVertical / 2.0f, _LiDARParams->_tlsMiddleAngleVertical + _LiDARParams->_tlsFOVVertical / 2.0f);
			if (!_LiDARParams->_tlsRangeResolution.empty())
				availableRange.x = _LiDARParams->_tlsRangeResolution[_LiDARParams->_tlsRangeResolution.size() - 1]._rangeInterval.y;

			_LiDARParams->_tlsRangeResolution.emplace_back(availableRange);
		}

		ImGui::PopStyleColor(3);
		ImGui::PopID();

		ImGui::PushID(0);
		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(6.0f / 7.0f, 0.6f, 0.6f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(6.0f / 7.0f, 0.7f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(6.0f / 7.0f, 0.8f, 0.8f));

		ImGui::SameLine(0, 15);
		if (ImGui::Button("Remove Last Range" ICON_FA_REMOVE_FORMAT))
		{
			if (!_LiDARParams->_tlsRangeResolution.empty()) _LiDARParams->_tlsRangeResolution.erase(_LiDARParams->_tlsRangeResolution.begin() + _LiDARParams->_tlsRangeResolution.size() - 1);
		}

		ImGui::SameLine(0, 15);
		if (ImGui::Button("Clear Ranges" ICON_FA_ERASER))
		{
			_LiDARParams->_tlsRangeResolution.clear();
		}

		ImGui::PopStyleColor(3);
		ImGui::PopID();

		this->leaveSpace(2);

		// Left
		static int rangeSelected = -1;

		ImGui::BeginChild("Model Components", ImVec2(150, 0), true);

		for (int i = 0; i < _LiDARParams->_tlsRangeResolution.size(); ++i)
		{
			if (ImGui::Selectable(("Range " + std::to_string(i + 1)).c_str(), rangeSelected == i))
				rangeSelected = i;
		}

		ImGui::EndChild();
		ImGui::SameLine();

		// Right
		if (rangeSelected >= 0 && rangeSelected < _LiDARParams->_tlsRangeResolution.size())
		{
			ImGui::BeginGroup();
			ImGui::BeginChild("Range View", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));		// Leave room for 1 line below us
			ImGui::PushItemWidth(150.0f);

			ImGui::Text(("Range " + std::to_string(rangeSelected + 1)).c_str());
			ImGui::Separator();

			{
				LiDARParameters::RangeResolution& range = _LiDARParams->_tlsRangeResolution[rangeSelected];
				float min_min, min_max, max_min, max_max;

				if (rangeSelected == 0)
				{
					min_min = _LiDARParams->_tlsMiddleAngleVertical - _LiDARParams->_tlsFOVVertical / 2.0f;
					min_max = range._rangeInterval.y;
				}
				else
				{
					min_min = _LiDARParams->_tlsRangeResolution[rangeSelected - 1]._rangeInterval.y;
					min_max = range._rangeInterval.y;
				}

				if (rangeSelected == (_LiDARParams->_tlsRangeResolution.size() - 1)) max_max = _LiDARParams->_tlsMiddleAngleVertical + _LiDARParams->_tlsFOVVertical / 2.0f;
				else max_max = _LiDARParams->_tlsRangeResolution[rangeSelected + 1]._rangeInterval.x;
				max_min = range._rangeInterval.x;

				ImGui::SliderFloat("Min. Angle", &range._rangeInterval.x, min_min, min_max);
				ImGui::SameLine(0, 20);
				ImGui::SliderFloat("Max. Angle", &range._rangeInterval.y, max_min, max_max);
				ImGui::SameLine(0, 20);
				ImGui::SliderInt("Resolution", &range._resolution, 0, 100);
				this->leaveSpace(5);
			}

			ImGui::PopItemWidth();
			ImGui::EndChild();
			ImGui::EndGroup();
		}

		ImGui::End();
	}
}

void GUI::showTLSPositionsFileDialog()
{
	ImGuiFileDialog::Instance()->OpenDialog("Choose TLS Points", "Choose File", ".txt", "Assets/LiDAR/Paths/");

	// Display
	if (ImGuiFileDialog::Instance()->Display("Choose TLS Points"))
	{
		// Action if OK
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
			_tlsPositionsFilePath = filePathName;
			_showTLSPointsFileDialog = true;

			// Load points and save them into LiDAR instance
			LiDARScene* scene = Renderer::getInstance()->getCurrentScene();
			std::vector<vec3>* tlsPositions = scene->getLiDARSensor()->getTLSPositions();
			tlsPositions->clear();

			this->loadTLSPositions(_tlsPositionsFilePath, *tlsPositions);
			scene->getLiDARSensor()->setTLSPositions(*tlsPositions);
		}

		// Close
		ImGuiFileDialog::Instance()->Close();
		_showTLSPointsFileDialog = false;
	}
}

void GUI::showTLSTabItem()
{
	LiDARScene* scene = Renderer::getInstance()->getCurrentScene();
	AABB aabb = scene->getAABB();
	vec3 min = aabb.min() - aabb.size(), max = aabb.max() + aabb.size();

	if (ImGui::BeginTabItem("Terrestrial LiDAR (TLS)"))
	{
		this->leaveSpace(2);

		{
			ImGui::PushID(0);
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(347.0f / 360.0f, 76.0f / 100.0f, 94.0f / 100.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(347.0f / 360.0f, 66.0f / 100.0f, 94.0f / 100.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(347.0f / 360.0f, 66.0f / 100.0f, 94.0f / 100.0f));

			if (ImGui::Button("Show Canvas"))
			{
				_showTLSCanvas = true;
			}

			ImGui::SameLine(0, 10);

			if (ImGui::Button("Show Resolution Intervals"))
			{
				_showTLSResolution = true;
			}

			ImGui::PopStyleColor(3);
			ImGui::PopID();

			ImGui::SameLine(0, 20);
			ImGui::Checkbox("User-defined path", &_LiDARParams->_tlsUseManualPath);
		}

		this->leaveSpace(3);

		ImGui::PushItemWidth(400.0f);

		ImGui::SliderFloat3("Position", &_LiDARParams->_tlsPosition[0], glm::min(min.x, glm::min(min.y, min.z)) - 2.0f, glm::max(max.x, glm::max(max.y, max.z)) + 2.0f, "%.4f");
		ImGui::SliderFloat("Horizontal FOV (Degrees)", &_LiDARParams->_tlsFOVHorizontal, LiDARParameters::TLS_MIN_FOV_HORIZONTAL, LiDARParameters::TLS_MAX_FOV_HORIZONTAL);
		ImGui::SliderFloat("Vertical FOV (Degrees)", &_LiDARParams->_tlsFOVVertical, LiDARParameters::TLS_MIN_FOV_VERTICAL, LiDARParameters::TLS_MAX_FOV_VERTICAL);
		ImGui::SliderInt("Horizontal Resolution", &_LiDARParams->_tlsResolutionHorizontal, LiDARParameters::TLS_MIN_RESOLUTION_HORIZONTAL, LiDARParameters::TLS_MAX_RESOLUTION_HORIZONTAL);
		ImGui::SliderInt("Vertical Resolution", &_LiDARParams->_tlsResolutionVertical, LiDARParameters::TLS_MIN_RESOLUTION_VERTICAL, LiDARParameters::TLS_MAX_RESOLUTION_VERTICAL);
		ImGui::SameLine(0, 30); ImGui::Checkbox("Uniform Vertical Resolution", &_LiDARParams->_tlsUniformVerticalResolution);
		ImGui::SliderFloat("Middle Angle (Horizontal); Normal (Degrees)", &_LiDARParams->_tlsMiddleAngleHorizontal, LiDARParameters::TLS_MIN_ANGLE, LiDARParameters::TLS_MAX_ANGLE);
		ImGui::SliderFloat("Middle Angle (Vertical); Normal (Degrees)", &_LiDARParams->_tlsMiddleAngleVertical, -LiDARParameters::TLS_MAX_ANGLE / 4.0f, LiDARParameters::TLS_MAX_ANGLE / 4.0f);

		this->leaveSpace(2);

		ImGui::SliderFloat("Angle Jittering", &_LiDARParams->_tlsAngleJittering, LiDARParameters::TLS_MIN_ANGLE_JITTERING, LiDARParameters::TLS_MAX_ANGLE_JITTERING, "%.8f", ImGuiSliderFlags_Logarithmic);
		ImGui::SliderFloat("Axis Jittering", &_LiDARParams->_tlsAxisJittering, LiDARParameters::TLS_MIN_AXIS_JITTERING, LiDARParameters::TLS_MAX_AXIS_JITTERING, "%.8f", ImGuiSliderFlags_Logarithmic);
		ImGui::SliderFloat("Goodness of atmospheric conditions", &_LiDARParams->_tlsAtmosphericClearness, LiDARParameters::MIN_NORMALIZED_VALUE, LiDARParameters::MAX_NORMALIZED_VALUE, "%.2f", ImGuiSliderFlags_Logarithmic);

		ImGui::EndTabItem();

		ImGui::PopItemWidth();
	}
}

GUI::~GUI()
{
	delete _customClasses;
	delete _lidarClasses;

	ImPlot::DestroyContext();
	ImGui::DestroyContext();
}

/// [Public methods]

void GUI::initialize(GLFWwindow* window, const int openGLMinorVersion)
{
	const std::string openGLVersion = "#version 4" + std::to_string(openGLMinorVersion) + "0 core";

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();

	this->loadImGUIStyle();
	
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(openGLVersion.c_str());
}

void GUI::render()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	this->createMenu();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// ---------------- IMGUI ------------------

void GUI::loadImGUIStyle()
{
	ImGui::StyleColorsDark();

	loadFonts();
}

bool GUI::loadTLSPositions(const std::string& filename, std::vector<vec3>& tlsPositions)
{
	std::string currentLine;
	std::stringstream line;
	std::ifstream inputStream;
	std::vector<float> floatTokens;
	std::vector<std::string> strTokens;

	inputStream.open(filename.c_str());
	if (inputStream.fail()) return false;

	while (!(inputStream >> std::ws).eof())
	{
		std::getline(inputStream, currentLine);
		line.clear();
		line.str(currentLine);

		FileManagement::readTokens(currentLine, ',', strTokens, floatTokens);

		if (floatTokens.size() == 3)
		{
			tlsPositions.emplace_back(floatTokens[0], floatTokens[1], floatTokens[2]);
		}

		floatTokens.clear();
	}

	inputStream.close();

	return true;
}

void GUI::loadFonts()
{
	ImFontConfig cfg;
	ImGuiIO& io = ImGui::GetIO();
	
	std::copy_n("Lato", 5, cfg.Name);
	//io.Fonts->AddFontFromMemoryCompressedBase85TTF(lato_compressed_data_base85, 15.0f, &cfg);
	io.Fonts->AddFontFromFileTTF("Assets/Fonts/Ruda-Bold.ttf", 14.0f, &cfg);

	static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	cfg.MergeMode = true;
	cfg.PixelSnapH = true;
	cfg.GlyphMinAdvanceX = 20.0f;
	cfg.GlyphMaxAdvanceX = 20.0f;
	std::copy_n("FontAwesome", 12, cfg.Name);

	io.Fonts->AddFontFromFileTTF("Assets/Fonts/fa-regular-400.ttf", 13.0f, &cfg, icons_ranges);
	io.Fonts->AddFontFromFileTTF("Assets/Fonts/fa-solid-900.ttf", 13.0f, &cfg, icons_ranges);
}