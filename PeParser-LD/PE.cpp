#include "PE.h"

#include <Windows.h>
#include <winternl.h>
#include <TlHelp32.h>
#include <cmath>
#include <string>

#include <array>
#include <stdexcept>

#include <sstream>
#include <iomanip>



PE::PE(LPBYTE file, HANDLE fileHandle, HANDLE mapHandle, DWORD fileSize, DWORD StubSize)
    : lFile(file), hFile(fileHandle), hMap(mapHandle), dwFileSize(fileSize), StubS(StubSize) {
    // Use dwFileSize in constructor to avoid optimization
    if (fileSize == 0) {
        //std::cerr << "File size cannot be zero!" << std::endl;
		// return 0x20

    }
}

PE::~PE() {
    clean();
}

//--------------------------------------------------------------------------------------------

PIMAGE_DOS_HEADER PE::GetDosHeader() {
    return (PIMAGE_DOS_HEADER)lFile;
}

PIMAGE_NT_HEADERS PE::GetNtHeaders() {
	return (PIMAGE_NT_HEADERS)(lFile + GetDosHeader()->e_lfanew);
}

bool PE::VerifyDos() {
	return GetDosHeader()->e_magic == IMAGE_DOS_SIGNATURE ? true : false;
}

bool PE::VerifyPe() {
	return GetNtHeaders()->Signature == IMAGE_NT_SIGNATURE ? true : false;
}

PIMAGE_SECTION_HEADER PE::GetFirstSectionHeader() {
	//return (PIMAGE_SECTION_HEADER)((LPBYTE)GetNtHeaders() + sizeof(IMAGE_NT_HEADERS));
	return (PIMAGE_SECTION_HEADER)IMAGE_FIRST_SECTION(GetNtHeaders());
}

PIMAGE_SECTION_HEADER PE::GetLastSectionHeader() {
	return (PIMAGE_SECTION_HEADER)(GetFirstSectionHeader() + (GetNtHeaders()->FileHeader.NumberOfSections - 1));
}

DWORD PE::OEP() {
	return GetNtHeaders()->OptionalHeader.AddressOfEntryPoint;
}


//--------------------------------------------------------------------------------------------

PIMAGE_SECTION_HEADER PE::retSectionPtr(char* sec) {
	PIMAGE_SECTION_HEADER pSec = IMAGE_FIRST_SECTION(GetNtHeaders());
	WORD wCount = GetNtHeaders()->FileHeader.NumberOfSections;
	for (int i = 0; i < wCount; i++) {
		if (strcmp((char*)pSec[i].Name, sec) == 0) {
			return &pSec[i];
		}
	}
	return nullptr;
}


std::vector<std::string> PE::GetSectionName() {
	WORD wCount = GetNtHeaders()->FileHeader.NumberOfSections;
	
	PIMAGE_SECTION_HEADER pSec = IMAGE_FIRST_SECTION(GetNtHeaders());

    std::vector<std::string> sectionNames;
    
    for (int i = 0; i < wCount; i++) {
        char secName[9] = { 0 }; // Buffer for section name (8 chars + null terminator)
        memcpy(secName, pSec[i].Name, 8); // Copy section name safely
        sectionNames.push_back(std::string(secName)); // Store in vector
    }

    return sectionNames; // Return the vector of section names
}

//--------------------------------------------------------------------------------------------

void PE::clean() {
    if (lFile) {
        FlushViewOfFile(lFile, 0);
        UnmapViewOfFile(lFile);
        lFile = nullptr;  
    }

    if (hMap && hMap != INVALID_HANDLE_VALUE) {
        CloseHandle(hMap);
        hMap = NULL; 
    }

    if (hFile && hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
        hFile = NULL;  
    }
}

//--------------------------------------------------------------------------------------------


std::string PE::PEHash(const std::string& path) {
    // Convert std::string to std::wstring
    std::wstring wpath(path.begin(), path.end());

    // Define PowerShell command (SHA-256)
    std::wstring command = L"powershell -Command \"Get-FileHash '" + wpath + L"' -Algorithm SHA256 | Select-Object -ExpandProperty Hash\"";

    // Create a pipe to capture output
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        throw std::runtime_error("Failed to create pipe.");
    }

    // Set up process startup info
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;

    PROCESS_INFORMATION pi = {};

    // Execute PowerShell command
    if (!CreateProcessW(nullptr, &command[0], nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        throw std::runtime_error("Failed to execute PowerShell.");
    }

    // Close write handle to avoid hanging
    CloseHandle(hWritePipe);

    // Read output from the pipe
    std::array<char, 4096> buffer;
    DWORD bytesRead;
    std::string result;
    while (ReadFile(hReadPipe, buffer.data(), buffer.size() - 1, &bytesRead, nullptr) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        result.append(buffer.data(), bytesRead);
    }

    // Cleanup
    CloseHandle(hReadPipe);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // Trim result (remove newline)
    result.erase(result.find_last_not_of(" \n\r\t") + 1);

    return result;
}

std::future<std::string> PE::PEHashAsync(const std::string& path) {
    return std::async(std::launch::async, [path]() {
        return PE::PEHash(path);
        });
}


//--------------------------------------------------------------------------------------------

VOID PE::ReadBytes(DWORD offset, LPVOID buffer, DWORD size) {
    memcpy(buffer, this->lFile + offset, size); 
}

IMAGE_SECTION_HEADER* PE::GetSectionFromRVA(DWORD rva){
	IMAGE_SECTION_HEADER* section = GetFirstSectionHeader();
    for(int i=0; i< GetNtHeaders()->FileHeader.NumberOfSections; i++, section++){
            if(rva >= section->VirtualAddress && rva < section->VirtualAddress + section->Misc.VirtualSize){
                return section;
            }
    }
}

void* PE::RVAtoPointer(DWORD rva){
	IMAGE_SECTION_HEADER* section = GetSectionFromRVA(rva);
	if (section == nullptr) {
		return nullptr;
	}
	return lFile + section->PointerToRawData + rva - section->VirtualAddress;
}

PIMAGE_SECTION_HEADER PE::ITSection(DWORD ID_RVA) {
    PIMAGE_SECTION_HEADER SectionHeader = GetFirstSectionHeader(); 
    for (int i = 0; i < GetNtHeaders()->FileHeader.NumberOfSections; i++) {
        if (ID_RVA >= SectionHeader->VirtualAddress && ID_RVA < SectionHeader->VirtualAddress + SectionHeader->Misc.VirtualSize) {
            return SectionHeader;  
        }

        SectionHeader++;  
    }
    return nullptr;  
}

int PE::PETYPE(){
	//if 32 return 32 otherwise if it's 64 return 64
	return GetNtHeaders()->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64 ? 64 : 32;
}