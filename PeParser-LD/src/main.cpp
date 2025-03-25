#include <algorithm>
#define NOMINMAX
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <shobjidl.h> 
#include <stddef.h> 
#include <atomic>  
#include <thread>
#include <chrono>

#include "../PE.h"
#include"../machine.h"



//global variables
bool isLoading = false;
std::chrono::steady_clock::time_point loadingStartTime;

char filter[] = "Executable Files\0*.exe\0Dynamic Link Library\0*.dll\0All Files\0*.*\0";
//used to extratc the path of the opened file 
std::wstring file_path = L""; 
std::string file_name_str_arg;
//used to show the about window
bool showAboutWindow = false;
PE* pe;

//Hel window vars
bool showHelp = false; 
int currentPage = 0;   
const int totalPages = 3; 

static int helpPage = 0;


ImFont* defaultFont = nullptr;
ImFont* boldFont = nullptr;

//size of main window 
int x = 1600;
int y = 600;


//IAT vars
DWORD importSection;



//global functions
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

void LoadFonts() {
	ImGuiIO& io = ImGui::GetIO();
	if (!defaultFont) defaultFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 18.0f);
	if (!boldFont) boldFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arialbd.ttf", 18.0f);
}

void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
}

//Prototypes

void GetPeVals(const std::wstring& file_path, PE*& pe);

void BarMenu(GLFWwindow* window);

void PanelView(GLFWwindow* window, int i, PE* pe);

void TreeViewGui(GLFWwindow* window, std::wstring file_path, PE* pe);

void StartLoadingThread(const std::wstring& selectedFile);

void LoadingScreen(GLFWwindow* window);

void Help_Dos_Header(GLFWwindow* window);

void CreateHelpWindow();

void RenderHelpWindow();

void GetImportLibrary();

std::string ConvertSize(size_t size) {
	const char* units[] = { "B", "KB", "MB", "GB", "TB" };
	int i = 0;
	double convertedSize = static_cast<double>(size);

	while (convertedSize >= 1024.0 && i < 4) {
		convertedSize /= 1024.0;
		++i;
	}

	return std::to_string(convertedSize) + " " + units[i];
}



int main() {
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW\n";
		return -1;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	GLFWwindow* window = glfwCreateWindow(x, y, "PE Parser", nullptr, nullptr);
	if (!window) {
		std::cerr << "Failed to create GLFW window\n";
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD\n";
		return -1;
	}

	// Initialize ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;  // Prevent unused warning

	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	// Backgrounds
	colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);  // Dark gray
	colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);

	// Borders
	colors[ImGuiCol_Border] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.0f);

	// Headers
	colors[ImGuiCol_Header] = ImVec4(0.50f, 0.15f, 0.15f, 1.0f); // Dark Red Accent
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.70f, 0.20f, 0.20f, 1.0f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.90f, 0.25f, 0.25f, 1.0f);

	// Buttons
	colors[ImGuiCol_Button] = ImVec4(0.40f, 0.10f, 0.10f, 1.0f); // Dark red button
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.60f, 0.15f, 0.15f, 1.0f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.80f, 0.20f, 0.20f, 1.0f);

	// Frames (input boxes, checkboxes, sliders)
	colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);

	// Text
	colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.0f); // Almost white text
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.0f); // Gray disabled text

	// Scrollbars
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.0f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);

	// Sliders
	colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.20f, 0.20f, 1.0f); // Dark red accent
	colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 0.25f, 0.25f, 1.0f);

	// Tabs
	colors[ImGuiCol_Tab] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
	colors[ImGuiCol_TabActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.0f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);

	// Resize Grip
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.0f);

	// Title Bar
	colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);

	// Change rounding for a modern look
	style.WindowRounding = 6.0f;
	style.FrameRounding = 4.0f;
	style.GrabRounding = 3.0f;
	style.PopupRounding = 4.0f;
	style.ScrollbarRounding = 3.0f;
	style.TabRounding = 4.0f;


	LoadFonts();



	// Set background color once
	glClearColor(0.10f, 0.10f, 0.10f, 1.0f); 




	while (!glfwWindowShouldClose(window)) {
		processInput(window);

		// Clear screen
		glClear(GL_COLOR_BUFFER_BIT);

		// Start ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Draw UI
		BarMenu(window);

		if (showAboutWindow) {
			ImGui::SetNextWindowPos(ImVec2(200, 100), ImGuiCond_FirstUseEver);
			ImGui::Begin("About", &showAboutWindow,
				ImGuiWindowFlags_AlwaysAutoResize |
				ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoSavedSettings);
			ImGui::Text("PE Parser");
			ImGui::Text("Version 1.0");
			ImGui::Separator();
			ImGui::Text("Developed by: ");
			ImGui::Text("Ldmd");
			ImGui::End();
		}

		if (showHelp) {
			Help_Dos_Header(window);
		}

		if (!file_path.empty()) {
			if (isLoading) {
				LoadingScreen(window);
			}
			else {
				TreeViewGui(window, file_path, pe);
			}
		}

		// Render UI
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Swap buffers and poll events
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	if (pe) {
		pe->clean();
		delete pe;
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;

}



void BarMenu(GLFWwindow* window) {

	if (ImGui::BeginMainMenuBar()) {

		if (ImGui::BeginMenu("File")) {
			//set the windowfocus on this window
	

			if (ImGui::MenuItem("Open")) {
				
				std::cout << "Opened" << std::endl;

				OPENFILENAME ofn;
				wchar_t filepath[MAX_PATH] = L"";
				ZeroMemory(&ofn, sizeof(ofn));

				// Initialize OPENFILENAME
				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = NULL;
				ofn.lpstrFilter = L"Executable Files\0*.exe\0Dynamic Link Library\0*.dll\0All Files\0*.*\0\0";
				ofn.lpstrFile = filepath;
				ofn.nMaxFile = MAX_PATH;
				ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
				ofn.lpstrDefExt = L"exe";

				// Display the Open dialog box
				if (GetOpenFileName(&ofn) == TRUE) {
					std::wcout << L"Selected file: " << filepath << std::endl;
					file_path = filepath;

					StartLoadingThread(file_path);
				}


				file_path = filepath;
			}
			if (ImGui::MenuItem("Exit")) {
				glfwSetWindowShouldClose(window, true);
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help")) {
		
			if (ImGui::MenuItem("About")) {
				std::cout << "About" << std::endl;
				showAboutWindow = true;
			}
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
}
void GetPeVals(const std::wstring& file_path, PE*& pe) {
	// Get HANDLE to the file
	HANDLE hFile = CreateFile(file_path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	
	if (hFile == INVALID_HANDLE_VALUE) {
		std::cerr << "Error: Could not open file!" << std::endl;
		return;
	}
	// Get HANDLE to the file mapping
	HANDLE hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (!hMap) {
		std::cerr << "Error: Could not create file mapping!" << std::endl;
		CloseHandle(hFile);
		return;
	}

	// Get pointer to the file mapping
	LPBYTE lFile = (LPBYTE)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
	if (!lFile) {
		std::cerr << "Error: Could not map view of file!" << std::endl;
		CloseHandle(hMap);
		CloseHandle(hFile);
		return;
	}

	// Get the size of the file
	DWORD dwFileSize = GetFileSize(hFile, NULL);
	if (dwFileSize == INVALID_FILE_SIZE) {
		std::cerr << "Error: Could not get file size!" << std::endl;
		UnmapViewOfFile(lFile);
		CloseHandle(hMap);
		CloseHandle(hFile);
		return;
	}

	pe = new PE(lFile, hFile, hMap, dwFileSize, 0);
}



void TreeViewGui(GLFWwindow* window, std::wstring file_path, PE* pe) {
	if (!pe) {
		if (ImGui::Begin("PE Structure")) {  // Ensure proper wrapping
			ImGui::Text("Invalid PE structure.");
			ImGui::End();  // Properly close the window
		}
		return;
	}

	std::wstring file_name = file_path.substr(file_path.find_last_of(L"\\") + 1);
	std::string file_name_str(file_name.begin(), file_name.end());

	// Window to the left
	ImGui::SetNextWindowPos(ImVec2(0, 22));
	ImGui::SetNextWindowSize(ImVec2(250, y), ImGuiCond_Always);

	if (ImGui::Begin("PE Structure", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
		static int selectedNode = -1;

		if (ImGui::TreeNodeEx(file_name_str.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth)) {
			if (ImGui::TreeNodeEx("General", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth)) {
				if (ImGui::IsItemClicked()) selectedNode = 0;
				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("DOS Header", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth)) {
				if (ImGui::IsItemClicked()) selectedNode = 1;
				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("DOS Stub", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth)) {
				if (ImGui::IsItemClicked()) selectedNode = 2;
				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("NT Headers", ImGuiTreeNodeFlags_SpanAvailWidth)) {
				if (ImGui::TreeNodeEx("Signature", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth)) {
					if (ImGui::IsItemClicked()) selectedNode = 3;
					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("File Header", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth)) {
					if (ImGui::IsItemClicked()) selectedNode = 4;
					ImGui::TreePop();
				}

				if (ImGui::TreeNodeEx("Optional Header", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth)) {
					if (ImGui::IsItemClicked()) selectedNode = 5;
					ImGui::TreePop();
				}

				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Sections", ImGuiTreeNodeFlags_SpanAvailWidth)) {
				std::vector<std::string> sectionNames = pe->GetSectionName();
				for (const std::string& secName : sectionNames) {
					if (!secName.empty()) {
						if (ImGui::TreeNodeEx(secName.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth)) {
							if (ImGui::IsItemClicked()) selectedNode = 6;
							ImGui::TreePop();
						}
					}
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Import Library", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth)) {
				if (ImGui::IsItemClicked()) selectedNode = 7;
				ImGui::TreePop();
			}

			ImGui::TreePop();
		}

		if (selectedNode != -1) {
			PanelView(window, selectedNode, pe);
		}
	}

	ImGui::End();  // Properly close the main window
}


void PanelView(GLFWwindow* window, int i, PE* pe) {
	ImGui::SetNextWindowPos(ImVec2(250, 22));
	ImGui::SetNextWindowSize(ImVec2(x - 250, y), ImGuiCond_Always);

	if (i == 0) {
		//ImGui::SetNextWindowFocus();

		ImGui::Begin("General", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

		ImGui::Columns(2, "Columns", true);
		ImGui::SetColumnWidth(0, 200);
		ImGui::SetColumnWidth(1, 600);

		ImGui::Separator();

		//Path of the file
		ImGui::Text("Path: "); ImGui::NextColumn();

		std::string file_path_str(file_path.begin(), file_path.end());

		ImGui::Text("%s", file_path_str.c_str()); ImGui::NextColumn();

		ImGui::Separator();

		ImGui::Text("Size on Disk: "); ImGui::NextColumn();

		//Size of the executab;e

		size_t DiskSize = pe->GetNtHeaders()->OptionalHeader.SizeOfImage;

		std::string formattedSize = ConvertSize(DiskSize);

		ImGui::Text("%s", formattedSize.c_str()); ImGui::NextColumn();

		ImGui::Separator();

	}
	//Dos Header

	if (i == 1) {
		//ImGui::SetNextWindowFocus();
		struct DOSField {
			const char* descriptor;
			size_t offset;
			uint32_t value;
		};

		IMAGE_DOS_HEADER* dosHeader = pe->GetDosHeader();

		DOSField fields[] = {
		   {"e_magic", offsetof(IMAGE_DOS_HEADER, e_magic), dosHeader->e_magic},
		   {"Bytes on last page of file", offsetof(IMAGE_DOS_HEADER, e_cblp), dosHeader->e_cblp},
		   {"Pages in file", offsetof(IMAGE_DOS_HEADER, e_cp), dosHeader->e_cp},
		   {"Relocations", offsetof(IMAGE_DOS_HEADER, e_crlc), dosHeader->e_crlc},
		   {"Size of header in paragraphs", offsetof(IMAGE_DOS_HEADER, e_cparhdr), dosHeader->e_cparhdr},
		   {"Minimum paragraphs allocated", offsetof(IMAGE_DOS_HEADER, e_minalloc), dosHeader->e_minalloc},
		   {"Maximum paragraphs allocated", offsetof(IMAGE_DOS_HEADER, e_maxalloc), dosHeader->e_maxalloc},
		   {"Initial stack segment", offsetof(IMAGE_DOS_HEADER, e_ss), dosHeader->e_ss},
		   {"Checksum", offsetof(IMAGE_DOS_HEADER, e_csum), dosHeader->e_csum},
		   {"Initial IP Value", offsetof(IMAGE_DOS_HEADER, e_ip), dosHeader->e_ip},
		   {"Initial [relative] IP value", offsetof(IMAGE_DOS_HEADER, e_cs), dosHeader->e_cs},
		   {"Relocation table offset", offsetof(IMAGE_DOS_HEADER, e_lfarlc), dosHeader->e_lfarlc},
		   {"Overlay Number", offsetof(IMAGE_DOS_HEADER, e_ovno), dosHeader->e_ovno},

		   // Array ellemtns 
		   {"Reserved Words [4]", offsetof(IMAGE_DOS_HEADER, e_res[0]), dosHeader->e_res[0]},
		   {"OEM Identifier", offsetof(IMAGE_DOS_HEADER, e_oemid), dosHeader->e_oemid},
		   {"OEM Information", offsetof(IMAGE_DOS_HEADER, e_oeminfo), dosHeader->e_oeminfo},

		   // Res_2 fix
		   {"Reserved Words [10]", offsetof(IMAGE_DOS_HEADER, e_res2[0]), dosHeader->e_res2[0]},
		   {"PE Header Offset", offsetof(IMAGE_DOS_HEADER, e_lfanew), dosHeader->e_lfanew}
		};





		ImGui::Begin("DOS Header", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

		ImGui::Columns(3, "Columns", true);
		ImGui::SetColumnWidth(0, 100);
		ImGui::SetColumnWidth(1, 600);
		ImGui::SetColumnWidth(2, 100);

		ImGui::Separator();
		ImGui::Text("Offset"); ImGui::NextColumn();
		ImGui::Text("Descriptor"); ImGui::NextColumn();
		ImGui::Text("Value"); ImGui::NextColumn();
		ImGui::Separator();

		for (const auto& field : fields) {
			ImGui::Text("0x%02X", field.offset);
			ImGui::NextColumn();
			ImGui::Text("%s", field.descriptor);
			ImGui::NextColumn();
			ImGui::Text("0x%X", field.value);  // No need to check 16-bit/32-bit, as we fixed struct
			ImGui::NextColumn();
		}

		ImGui::Separator();

		ImGui::Columns(1);

		// Centered Help Button
		ImVec2 buttonSize(120, 30);
		ImVec2 windowSize = ImGui::GetWindowSize();
		ImGui::SetCursorPosX((windowSize.x - buttonSize.x) * 0.5f);
		float centerY = (windowSize.y - (buttonSize.y - 20)) / 1.101f;
		ImGui::SetCursorPosY(centerY);
		if (ImGui::Button("Help", buttonSize)) {
			showHelp = true;
		}

	}

	//Dos Stub
	if (i == 2) {
		ImGui::Begin("Dos Stub", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

		ImGui::Columns(3, "DosStubColumns");

		// Set column sizes
		ImGui::SetColumnWidth(0, 100);
		ImGui::SetColumnWidth(1, 600);
		ImGui::SetColumnWidth(2, 100);

		ImGui::Separator();

		// Column Headers
		ImGui::Text("Offset"); ImGui::NextColumn();
		ImGui::Text("Descriptor"); ImGui::NextColumn();
		ImGui::Text("Value"); ImGui::NextColumn();

		ImGui::Separator();



		ImGui::Columns(1);
		//ImGui::End();  
	}


	//NT Headers

	//Signature
	if (i == 3) {
		ImGui::Begin("Signature", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

		ImGui::Columns(3, "SignatureColumns", true);
		ImGui::SetColumnWidth(0, 200);
		ImGui::SetColumnWidth(1, 600);
		ImGui::SetColumnWidth(2, 200);

		ImGui::Separator();

		// Column Headers
		ImGui::Text("Offset"); ImGui::NextColumn();
		ImGui::Text("Descriptor"); ImGui::NextColumn();
		ImGui::Text("Value"); ImGui::NextColumn();
		ImGui::Separator();

		// Read Signature
		BYTE Signature[4] = { 0 };
		DWORD offsetl = (DWORD)((LPBYTE)pe->GetNtHeaders() - pe->GetFile());
		pe->ReadBytes(offsetl, Signature, 4);

		DWORD offset;
		memcpy(&offset, Signature, sizeof(DWORD));


		printf("Signature Bytes: %02X %02X %02X %02X\n", Signature[0], Signature[1], Signature[2], Signature[3]);

		std::string byteString;
		for (int i = 0; i < 4; i++) {
			char hexByte[10];
			snprintf(hexByte, sizeof(hexByte), "\\x%02X", Signature[i]);
			byteString += hexByte;
		}

		// Display data
		ImGui::Text("0x%X", offsetl);
		ImGui::NextColumn();
		ImGui::Text("Signature:");
		ImGui::NextColumn();
		ImGui::Text("%s", byteString.c_str());
		ImGui::NextColumn();

		ImGui::Columns(1);
		//ImGui::End();  
	}

	//File Header
	if (i == 4) {
		ImGui::Begin("File Header", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

		ImGui::Columns(4, "Columns", true);
		ImGui::SetColumnWidth(0, 200);
		ImGui::SetColumnWidth(1, 300);
		ImGui::SetColumnWidth(2, 200);
		ImGui::SetColumnWidth(2, 200);

		ImGui::Separator();

		struct FileHeaderField {
			const char* descriptor;
			size_t offset;
			uint32_t value;
		};

		IMAGE_FILE_HEADER* fileHeader = &pe->GetNtHeaders()->FileHeader;

		FileHeaderField fHeader[] = {
			{"Machine", offsetof(IMAGE_FILE_HEADER, Machine), fileHeader->Machine},
			{"Number of Sections",offsetof(IMAGE_FILE_HEADER, NumberOfSections), fileHeader->NumberOfSections},
			{"Time Date Stamp", offsetof(IMAGE_FILE_HEADER, TimeDateStamp), fileHeader->TimeDateStamp},
			{"Pointer to Symbol Table", offsetof(IMAGE_FILE_HEADER, PointerToSymbolTable), fileHeader->PointerToSymbolTable},
			{"Number of Symbols", offsetof(IMAGE_FILE_HEADER, NumberOfSymbols), fileHeader->NumberOfSymbols},
			{"Size of Optional Header", offsetof(IMAGE_FILE_HEADER, SizeOfOptionalHeader), fileHeader->SizeOfOptionalHeader},
			{"Characteristics", offsetof(IMAGE_FILE_HEADER, Characteristics), fileHeader->Characteristics}
		};

		for (const auto& field : fHeader) {
			ImGui::Text("0x%08X", field.offset);
			ImGui::NextColumn();
			ImGui::Text("%s", field.descriptor);
			ImGui::NextColumn();



			if (field.value <= 0xFFFF) {
				ImGui::Text("0x%04X", field.value);
			}
			else {
				ImGui::Text("0x%08X", field.value);
			}

			ImGui::NextColumn();
			if (field.descriptor == "Machine") {
				ImGui::Text("%s", GetMachineNameById(fileHeader->Machine));
			}
			ImGui::NextColumn();
			ImGui::Separator();
		}



	}

	//Optional Header
	if (i == 5) {
		// Template for the Optional Header
		ImGui::Begin("Optional Header", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

		// Define columns
		ImGui::Columns(3, "OptionalHeaderColumns", true); // 3 columns, with borders
		ImGui::SetColumnWidth(0, 200.0f); // Offset column width
		ImGui::SetColumnWidth(1, 600.0f); // Descriptor column width
		ImGui::SetColumnWidth(2, 200.0f); // Value column width

		// Display column headers
		ImGui::Text("Offset");
		ImGui::NextColumn();
		ImGui::Text("Descriptor");
		ImGui::NextColumn();
		ImGui::Text("Value");
		ImGui::NextColumn();
		ImGui::Separator();

		// Define the OptionalHeaderField struct
		struct OptionalHeaderField {
			const char* descriptor; // Human-readable name of the field
			size_t offset;          // Byte offset of the field in the Optional Header
			uint32_t value;         // Value of the field (extracted from the Optional Header)
		};

		OptionalHeaderField* oHeader = nullptr; // Pointer to hold the correct array

		// Get Magic value from Optional Header
		DWORD magic = pe->GetNtHeaders()->OptionalHeader.Magic;

		// If PE32 (0x10B), use IMAGE_OPTIONAL_HEADER32
		if (magic == 0x10B) {
			IMAGE_OPTIONAL_HEADER32* optionalHeader32 = reinterpret_cast<IMAGE_OPTIONAL_HEADER32*>(&pe->GetNtHeaders()->OptionalHeader);
			OptionalHeaderField oHeader32[] = {
				{"Magic: ", offsetof(IMAGE_OPTIONAL_HEADER32, Magic), optionalHeader32->Magic},
				{"MajorLinkerVersion: ", offsetof(IMAGE_OPTIONAL_HEADER32, MajorLinkerVersion), optionalHeader32->MajorLinkerVersion},
				{"MinorLinkerVersion: ", offsetof(IMAGE_OPTIONAL_HEADER32, MinorLinkerVersion), optionalHeader32->MinorLinkerVersion},
				{"SizeOfCode: ", offsetof(IMAGE_OPTIONAL_HEADER32, SizeOfCode), optionalHeader32->SizeOfCode},
				{"SizeOfInitializedData: ", offsetof(IMAGE_OPTIONAL_HEADER32, SizeOfInitializedData), optionalHeader32->SizeOfInitializedData},
				{"SizeOfUninitializedData: ", offsetof(IMAGE_OPTIONAL_HEADER32, SizeOfUninitializedData), optionalHeader32->SizeOfUninitializedData},
				{"AddressOfEntryPoint: ", offsetof(IMAGE_OPTIONAL_HEADER32, AddressOfEntryPoint), optionalHeader32->AddressOfEntryPoint},
				{"BaseOfCode: ", offsetof(IMAGE_OPTIONAL_HEADER32, BaseOfCode), optionalHeader32->BaseOfCode},
				{"BaseOfData: ", offsetof(IMAGE_OPTIONAL_HEADER32, BaseOfData), optionalHeader32->BaseOfData}, // Specific to 32-bit
				{"ImageBase: ", offsetof(IMAGE_OPTIONAL_HEADER32, ImageBase), optionalHeader32->ImageBase},
				{"SectionAlignment: ", offsetof(IMAGE_OPTIONAL_HEADER32, SectionAlignment), optionalHeader32->SectionAlignment},
				{"FileAlignment: ", offsetof(IMAGE_OPTIONAL_HEADER32, FileAlignment), optionalHeader32->FileAlignment},
				{"MajorOperatingSystemVersion: ", offsetof(IMAGE_OPTIONAL_HEADER32, MajorOperatingSystemVersion), optionalHeader32->MajorOperatingSystemVersion},
				{"MinorOperatingSystemVersion: ", offsetof(IMAGE_OPTIONAL_HEADER32, MinorOperatingSystemVersion), optionalHeader32->MinorOperatingSystemVersion},
				{"MajorImageVersion: ", offsetof(IMAGE_OPTIONAL_HEADER32, MajorImageVersion), optionalHeader32->MajorImageVersion},
				{"MinorImageVersion: ", offsetof(IMAGE_OPTIONAL_HEADER32, MinorImageVersion), optionalHeader32->MinorImageVersion},
				{"MajorSubsystemVersion: ", offsetof(IMAGE_OPTIONAL_HEADER32, MajorSubsystemVersion), optionalHeader32->MajorSubsystemVersion},
				{"MinorSubsystemVersion: ", offsetof(IMAGE_OPTIONAL_HEADER32, MinorSubsystemVersion), optionalHeader32->MinorSubsystemVersion},
				{"Win32VersionValue: ", offsetof(IMAGE_OPTIONAL_HEADER32, Win32VersionValue), optionalHeader32->Win32VersionValue},
				{"SizeOfImage: ", offsetof(IMAGE_OPTIONAL_HEADER32, SizeOfImage), optionalHeader32->SizeOfImage},
				{"SizeOfHeaders: ", offsetof(IMAGE_OPTIONAL_HEADER32, SizeOfHeaders), optionalHeader32->SizeOfHeaders},
				{"CheckSum: ", offsetof(IMAGE_OPTIONAL_HEADER32, CheckSum), optionalHeader32->CheckSum},
				{"Subsystem: ", offsetof(IMAGE_OPTIONAL_HEADER32, Subsystem), optionalHeader32->Subsystem},
				{"DllCharacteristics: ", offsetof(IMAGE_OPTIONAL_HEADER32, DllCharacteristics), optionalHeader32->DllCharacteristics},
				{"SizeOfStackReserve: ", offsetof(IMAGE_OPTIONAL_HEADER32, SizeOfStackReserve), optionalHeader32->SizeOfStackReserve},
				{"SizeOfStackCommit: ", offsetof(IMAGE_OPTIONAL_HEADER32, SizeOfStackCommit), optionalHeader32->SizeOfStackCommit},
				{"SizeOfHeapReserve: ", offsetof(IMAGE_OPTIONAL_HEADER32, SizeOfHeapReserve), optionalHeader32->SizeOfHeapReserve},
				{"SizeOfHeapCommit: ", offsetof(IMAGE_OPTIONAL_HEADER32, SizeOfHeapCommit), optionalHeader32->SizeOfHeapCommit},
				{"LoaderFlags: ", offsetof(IMAGE_OPTIONAL_HEADER32, LoaderFlags), optionalHeader32->LoaderFlags},
				{"NumberOfRvaAndSizes: ", offsetof(IMAGE_OPTIONAL_HEADER32, NumberOfRvaAndSizes), optionalHeader32->NumberOfRvaAndSizes},
				{nullptr, 0, 0}
			};


			oHeader = oHeader32;
		}
		// If PE64 (0x20B), use IMAGE_OPTIONAL_HEADER64
		else if (magic == 0x20B) {
			IMAGE_OPTIONAL_HEADER64* optionalHeader64 = reinterpret_cast<IMAGE_OPTIONAL_HEADER64*>(&pe->GetNtHeaders()->OptionalHeader);
			static OptionalHeaderField oHeader64[] = {
					{"Magic: ", offsetof(IMAGE_OPTIONAL_HEADER64, Magic), optionalHeader64->Magic},
					{"MajorLinkerVersion: ", offsetof(IMAGE_OPTIONAL_HEADER64, MajorLinkerVersion), optionalHeader64->MajorLinkerVersion},
					{"MinorLinkerVersion: ", offsetof(IMAGE_OPTIONAL_HEADER64, MinorLinkerVersion), optionalHeader64->MinorLinkerVersion},
					{"SizeOfCode: ", offsetof(IMAGE_OPTIONAL_HEADER64, SizeOfCode), optionalHeader64->SizeOfCode},
					{"SizeOfInitializedData: ", offsetof(IMAGE_OPTIONAL_HEADER64, SizeOfInitializedData), optionalHeader64->SizeOfInitializedData},
					{"SizeOfUninitializedData: ", offsetof(IMAGE_OPTIONAL_HEADER64, SizeOfUninitializedData), optionalHeader64->SizeOfUninitializedData},
					{"AddressOfEntryPoint: ", offsetof(IMAGE_OPTIONAL_HEADER64, AddressOfEntryPoint), optionalHeader64->AddressOfEntryPoint},
					{"BaseOfCode: ", offsetof(IMAGE_OPTIONAL_HEADER64, BaseOfCode), optionalHeader64->BaseOfCode},
					{"ImageBase: ", offsetof(IMAGE_OPTIONAL_HEADER64, ImageBase), optionalHeader64->ImageBase},
					{"SectionAlignment: ", offsetof(IMAGE_OPTIONAL_HEADER64, SectionAlignment), optionalHeader64->SectionAlignment},
					{"FileAlignment: ", offsetof(IMAGE_OPTIONAL_HEADER64, FileAlignment), optionalHeader64->FileAlignment},
					{"MajorOperatingSystemVersion: ", offsetof(IMAGE_OPTIONAL_HEADER64, MajorOperatingSystemVersion), optionalHeader64->MajorOperatingSystemVersion},
					{"MinorOperatingSystemVersion: ", offsetof(IMAGE_OPTIONAL_HEADER64, MinorOperatingSystemVersion), optionalHeader64->MinorOperatingSystemVersion},
					{"MajorImageVersion: ", offsetof(IMAGE_OPTIONAL_HEADER64, MajorImageVersion), optionalHeader64->MajorImageVersion},
					{"MinorImageVersion: ", offsetof(IMAGE_OPTIONAL_HEADER64, MinorImageVersion), optionalHeader64->MinorImageVersion},
					{"MajorSubsystemVersion: ", offsetof(IMAGE_OPTIONAL_HEADER64, MajorSubsystemVersion), optionalHeader64->MajorSubsystemVersion},
					{"MinorSubsystemVersion: ", offsetof(IMAGE_OPTIONAL_HEADER64, MinorSubsystemVersion), optionalHeader64->MinorSubsystemVersion},
					{"Win32VersionValue: ", offsetof(IMAGE_OPTIONAL_HEADER64, Win32VersionValue), optionalHeader64->Win32VersionValue},
					{"SizeOfImage: ", offsetof(IMAGE_OPTIONAL_HEADER64, SizeOfImage), optionalHeader64->SizeOfImage},
					{"SizeOfHeaders: ", offsetof(IMAGE_OPTIONAL_HEADER64, SizeOfHeaders), optionalHeader64->SizeOfHeaders},
					{"CheckSum: ", offsetof(IMAGE_OPTIONAL_HEADER64, CheckSum), optionalHeader64->CheckSum},
					{"Subsystem: ", offsetof(IMAGE_OPTIONAL_HEADER64, Subsystem), optionalHeader64->Subsystem},
					{"DllCharacteristics: ", offsetof(IMAGE_OPTIONAL_HEADER64, DllCharacteristics), optionalHeader64->DllCharacteristics},
					{"SizeOfStackReserve: ", offsetof(IMAGE_OPTIONAL_HEADER64, SizeOfStackReserve), optionalHeader64->SizeOfStackReserve},
					{"SizeOfStackCommit: ", offsetof(IMAGE_OPTIONAL_HEADER64, SizeOfStackCommit), optionalHeader64->SizeOfStackCommit},
					{"SizeOfHeapReserve: ", offsetof(IMAGE_OPTIONAL_HEADER64, SizeOfHeapReserve), optionalHeader64->SizeOfHeapReserve},
					{"SizeOfHeapCommit: ", offsetof(IMAGE_OPTIONAL_HEADER64, SizeOfHeapCommit), optionalHeader64->SizeOfHeapCommit},
					{"LoaderFlags: ", offsetof(IMAGE_OPTIONAL_HEADER64, LoaderFlags), optionalHeader64->LoaderFlags},
					{"NumberOfRvaAndSizes: ", offsetof(IMAGE_OPTIONAL_HEADER64, NumberOfRvaAndSizes), optionalHeader64->NumberOfRvaAndSizes},
								{nullptr, 0, 0} // Sentinel value to mark the end of the array
			};

			oHeader = oHeader64;
		}






		if (oHeader != nullptr) {
			for (const auto* field = oHeader; field->descriptor != nullptr; ++field) {
				// Display the offset, descriptor, and value
				ImGui::Text("%d", field->offset);
				ImGui::NextColumn();
				ImGui::Text("%s", field->descriptor);
				ImGui::NextColumn();
				ImGui::Text("%04X", field->value);
				ImGui::NextColumn();
			}
		}

		// End the columns
		ImGui::Columns(1);


	}

	//Sections
	if (i == 6) {
		ImGui::Begin("Sections", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

		ImGui::Columns(6, "SectionColumns");
		ImGui::Separator();

		// Column Headers
		ImGui::Text("Name"); ImGui::NextColumn();
		ImGui::Text("Raw Addr."); ImGui::NextColumn();
		ImGui::Text("Raw Size"); ImGui::NextColumn();
		ImGui::Text("Virtual Addr."); ImGui::NextColumn();
		ImGui::Text("Virtual Size"); ImGui::NextColumn();
		ImGui::Text("Characteristics"); ImGui::NextColumn();

		ImGui::Separator();

		std::vector<std::string> sectionNames = pe->GetSectionName();
		bool firstRow = true;

		for (const std::string& secName : sectionNames) {
			PIMAGE_SECTION_HEADER section = pe->retSectionPtr((char*)secName.c_str());

			if (!section) {
				printf("Warning: Section '%s' not found!\n", secName.c_str());
				continue;
			}

			if (!firstRow) ImGui::Separator();  // Avoid extra separator at start
			firstRow = false;

			ImGui::Text("%s", secName.c_str()); ImGui::NextColumn();
			ImGui::Text("0x%X", section->PointerToRawData); ImGui::NextColumn();
			ImGui::Text("0x%X", section->SizeOfRawData); ImGui::NextColumn();
			ImGui::Text("0x%X", section->VirtualAddress); ImGui::NextColumn();
			ImGui::Text("0x%X", section->Misc.VirtualSize); ImGui::NextColumn();
			ImGui::Text("0x%X", section->Characteristics); ImGui::NextColumn();
		}

		ImGui::Columns(1);

		ImVec2 buttonSize = ImVec2(120, 30);
		ImVec2 availableSpace = ImGui::GetContentRegionAvail();

		ImGui::SetCursorPosX((availableSpace.x - buttonSize.x) * 0.5f);
		ImGui::SetCursorPosY((availableSpace.y - buttonSize.y) * 0.5f);

		if (ImGui::Button("Help", buttonSize)) {
			std::cout << "Help" << std::endl;
		}


	}

	// IAT
	if (i == 7) {
		ImGui::Begin("Import Library", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

		ImGui::Columns(7, "IATColumns");
		ImGui::Separator();

		// Column Headers
		ImGui::Text("Offset"); ImGui::NextColumn();
		ImGui::Text("Name"); ImGui::NextColumn();
		ImGui::Text("Func. Count"); ImGui::NextColumn();
		ImGui::Text("Bound?"); ImGui::NextColumn();
		ImGui::Text("OriginalFirstThunk"); ImGui::NextColumn();
		ImGui::Text("NameRVA"); ImGui::NextColumn();
		ImGui::Text("FirstThunk"); ImGui::NextColumn();

		ImGui::Separator();

		// Validate PE file
		if (!pe || !pe->VerifyDos() || !pe->VerifyPe()) {
			ImGui::Text("Invalid PE File");
			ImGui::End();
			return;
		}
		DWORD ID_RVA;
		if (pe) {
			ID_RVA = pe->GetNtHeaders()->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
			std::cout << "Import Directory RVA: " <<std::hex<< ID_RVA << std::endl;
		}


		PIMAGE_SECTION_HEADER sec = pe->ITSection(ID_RVA);


		if (sec != nullptr) {
			//File Offset -> Import Table
			DWORD FilerawOffset = (DWORD)pe->GetFile() + sec->PointerToRawData;

			DWORD importDescriptorOffset = ID_RVA - sec->VirtualAddress + sec->PointerToRawData;
			//Pointer to Import Descriptor File offset
			//Formula: imageBaseAddress + pointerToRawDataOfTheSectionContainingRVAofInterest + (RVAofInterest - SectionContainingRVAofInterest.VirtualAddress)
			PIMAGE_IMPORT_DESCRIPTOR importDescriptor;

			importDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(pe->GetFile() + importDescriptorOffset);

			//Show the DLL improts

			if (importDescriptor) {

				for (; importDescriptor->Name != 0; importDescriptor++) {
					char dllName[256] = { 0 };
					DWORD nameRVA = importDescriptor->Name;
					PIMAGE_SECTION_HEADER nameSection = pe->ITSection(nameRVA);
					DWORD nameOffset = nameRVA - nameSection->VirtualAddress + nameSection->PointerToRawData;
					strcpy_s(dllName, sizeof(dllName), (char*)(pe->GetFile() + nameOffset));

					DWORD originalFirstThunk = importDescriptor->OriginalFirstThunk;
					DWORD firstThunk = importDescriptor->FirstThunk;



					//Offset of the DLL
					ImGui::Text("%02x", importDescriptor); ImGui::NextColumn();
					//Name
					ImGui::Text("%s", dllName); ImGui::NextColumn();
					//FunCount
					int funcCount = 0;
					for (DWORD iat = originalFirstThunk; iat != 0; iat += sizeof(DWORD)) {
						DWORD iatValue = *(DWORD*)(pe->GetFile() + iat);

						// Check if the IAT entry is valid 
						if (iatValue == 0) {
							break; // Null ENtry
						}
						funcCount++;
					}
					ImGui::Text("%d", funcCount);
					ImGui::NextColumn();
					//Bound 
					BOOL  bound = FALSE;
					if (importDescriptor->Characteristics == 0) {
						bound = TRUE;
					}
					ImGui::Text("%d", bound);
					ImGui::NextColumn();
					//OriginalFIrstThunk
					ImGui::Text("%02x", importDescriptor->OriginalFirstThunk); ImGui::NextColumn();
					//nameRVA
					ImGui::Text("%02x", importDescriptor->Name); ImGui::NextColumn();
					//FirstTHunk
					ImGui::Text("%02x", importDescriptor->FirstThunk); ImGui::NextColumn();


				}
			}
			else {
				ImGui::Text("We dont like ImportDescriptor");
				std::cout << "ImportDescritpor: " <<std::hex<< importDescriptor << std::endl;

			}




		}

		ImGui::Columns(1);
	}




	ImGui::End();
		
}

void StartLoadingThread(const std::wstring& selectedFile) {
	isLoading = true;
	loadingStartTime = std::chrono::steady_clock::now();

	// Start a separate thread for loading
	std::thread loadingThread([selectedFile]() {
		std::this_thread::sleep_for(std::chrono::seconds(3)); // Simulate loading time
		isLoading = false;

		// Parse the file after loading
		delete pe;
		GetPeVals(selectedFile, pe);
		});

	loadingThread.detach(); 
}

void LoadingScreen(GLFWwindow* window) {
	int mainWindowWidth = x;
	int mainWindowHeight = y;

	float centerX = (mainWindowWidth - 300) / 2.0f; 
	float centerY = (mainWindowHeight - 100) / 2.0f; 

	ImGui::SetNextWindowPos(ImVec2(centerX, centerY), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_Always);

	ImGui::Begin("Loading...", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

	ImGui::Text("Loading file, please wait...");

	// Progress bar 
	float progress = std::chrono::duration<float>(std::chrono::steady_clock::now() - loadingStartTime).count() / 10.0f;
	if (progress > 1.0f) progress = 1.0f; // Cap 

	ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));

	ImGui::ProgressBar(progress, ImVec2(250, 20));

	ImGui::PopStyleColor();

	ImGui::End();
}

void Help_Dos_Header(GLFWwindow* window) {
	if (showHelp) {


		LoadFonts();

		// Allow the window to be moved and resized, but prevent it from stealing focus
		ImGui::Begin("Help", &showHelp);

		ImGui::PushFont(boldFont);
		ImGui::Text("DOS Header");
		ImGui::PopFont();

		ImGui::Separator();

		// Page navigation
		if (helpPage == 0) {
			ImGui::Text("Welcome to the Dos Header Help Page");
			ImGui::Text("This function was meant to help in your journey");
			ImGui::Text("It works pretty basic you have next and previous buttons");

		
		}
		else if (helpPage == 1) {
			
			ImGui::PushFont(boldFont);
			ImGui::Text("Page 2 - Structure");
			ImGui::PopFont();

			//Image_DOS_Header structure 
			ImGui::Text("typedef struct _IMAGE_DOS_HEADER {");
			ImGui::Text("WORD e_magic; // Magic number");
			ImGui::Text("WORD e_cblp; // Bytes on last page of file");
			ImGui::Text("WORD e_cp; // Pages in file");
			ImGui::Text("WORD e_crlc; // Relocations");
			ImGui::Text("WORD e_cparhdr; // Size of header in paragraphs");
			ImGui::Text("WORD e_minalloc; // Minimum extra paragraphs needed");
			ImGui::Text("WORD e_maxalloc; // Maximum extra paragraphs needed");
			ImGui::Text("WORD e_ss; // Initial (relative) SS value");
			ImGui::Text("WORD e_sp; // Initial SP value");
			ImGui::Text("WORD e_csum; // Checksum");
			ImGui::Text("WORD e_ip; // Initial IP value");
			ImGui::Text("WORD e_cs; // Initial (relative) CS value");
			ImGui::Text("WORD e_lfarlc; // File address of relocation table");
			ImGui::Text("WORD e_ovno; // Overlay number");
			ImGui::Text("WORD e_res[4]; // Reserved words");
			ImGui::Text("WORD e_oemid; // OEM identifier (for e_oeminfo)");
			ImGui::Text("WORD e_oeminfo; // OEM information; e_oemid specific");
			ImGui::Text("WORD e_res2[10]; // Reserved words");
			ImGui::Text("LONG e_lfanew; // File address of new exe header");
			ImGui::Text("} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;");



		}
		else if (helpPage == 2) {

			ImGui::PushFont(boldFont);
			ImGui::Text("Page 3 - Information");
			ImGui::PopFont();

			//------------------------------------------------------------
			ImGui::Separator();

			ImGui::PushFont(boldFont);
			ImGui::Text("Where are they used?");
			ImGui::PopFont();
			
			ImGui::Separator();

			ImGui::TextWrapped("The IMAGE_DOS_HEADER structure represents the DOS header of a PE file. "
				"This header is the first structure in a PE file and contains critical information, including:");
			ImGui::BulletText("MS-DOS compatibility details");
			ImGui::BulletText("Location of the PE Header");
			ImGui::BulletText("Additional metadata");

			//------------------------------------------------------------
			ImGui::Separator();

			ImGui::PushFont(boldFont);
	        ImGui::Text("What is a PE Header?");
			ImGui::PopFont();
		
			ImGui::Separator();

			ImGui::TextWrapped("The Portable Executable (PE) Header is a crucial structure in Windows executables. "
				"It provides the operating system with essential information to load and manage executable code. "
				"Located at the offset specified by the e_lfanew field in the DOS Header, it includes:");
			ImGui::BulletText("PE signature");
			ImGui::BulletText("File header");
			ImGui::BulletText("Optional header");
			ImGui::TextWrapped("Collectively, these components define the file's format, required libraries, and memory layout.");

		}


		if (ImGui::Button("Next")) {
			helpPage++;
		}

		if (ImGui::Button("Previous")) {
			helpPage--;
		}
		ImGui::End();
	}
}

