#pragma once
#include <Windows.h>
#include <winternl.h>
#include <TlHelp32.h>
#include <cmath>
#include <string>
#include <vector>

#include <future>

class PE
{
private:
	LPBYTE lFile;
	HANDLE hFile;
	HANDLE hProc;
	HANDLE hMap;
	DWORD StubS;
	DWORD dwFileSize;
	BYTE* Stub;

	//sizes
	DWORD sectionSize;

public:
	PE(LPBYTE file = nullptr, HANDLE filehandle = INVALID_HANDLE_VALUE, HANDLE mapHandle = nullptr, DWORD filesize =0, DWORD stubsize =0);
	~PE();

	bool VerifyDos();
	bool VerifyPe();

	PIMAGE_DOS_HEADER GetDosHeader();
	PIMAGE_NT_HEADERS GetNtHeaders();

	PIMAGE_SECTION_HEADER GetFirstSectionHeader();
	PIMAGE_SECTION_HEADER GetLastSectionHeader();
	PIMAGE_SECTION_HEADER retSectionPtr(char* sec);


	IMAGE_SECTION_HEADER* GetSectionFromRVA(DWORD rva);

	void* RVAtoPointer(DWORD rva);

	PIMAGE_SECTION_HEADER ITSection(DWORD ID_RVA);

	std::vector<std::string> GetSectionName();

	DWORD OEP();

	void clean();

	//SHA256 Hash function 

	static std::future<std::string> PEHashAsync(const std::string& path);
	static std::string PEHash(const std::string& path);






	//Read Bytes from the PE file x address 
	VOID ReadBytes( DWORD offset, LPVOID buffer, DWORD size);


	//Getters 
	LPBYTE GetFile() const { return lFile; }
	
	int PETYPE();


};

