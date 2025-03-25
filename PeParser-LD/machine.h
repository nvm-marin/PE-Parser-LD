#pragma once
#include <Windows.h>
#include <winternl.h>
#include <TlHelp32.h>
#include <string>
#include <cstdint> // For uint16_t

// Struct to represent a machine type
struct Machine {
    uint16_t id;
    const char* name;
};


 const Machine machines[] = {
    {0x0, "Unknown Machine"},
    {0x184, "Alpha AXP, 32-bit"},
    {0x284, "Alpha 64"},
    {0x1d3, "Matsushita AM33"},
    {0x8664, "x64"},
    {0x1c0, "ARM little endian"},
    {0xaa64, "ARM64 little endian"},
    {0x1c4, "ARM Thumb-2 little endian"},
    {0x284, "AXP 64 (Same as Alpha 64)"},
    {0xebc, "EFI byte code"},
    {0x14c, "Intel 386"},
    {0x200, "Intel Itanium"},
    {0x6232, "LoongArch 32-bit"},
    {0x6264, "LoongArch 64-bit"},
    {0x9041, "Mitsubishi M32R little endian"},
    {0x266, "MIPS16"},
    {0x366, "MIPS with FPU"},
    {0x466, "MIPS16 with FPU"},
    {0x1f0, "Power PC little endian"},
    {0x1f1, "Power PC with FPU"},
    {0x166, "MIPS little endian"},
    {0x5032, "RISC-V 32-bit"},
    {0x5064, "RISC-V 64-bit"},
    {0x5128, "RISC-V 128-bit"},
    {0x1a2, "Hitachi SH3"},
    {0x1a3, "Hitachi SH3 DSP"},
    {0x1a6, "Hitachi SH4"},
    {0x1a8, "Hitachi SH5"},
    {0x1c2, "Thumb"},
    {0x169, "MIPS little-endian WCE v2"},
    {0, nullptr}
};


 const char* GetMachineNameById(uint32_t id) {
    for (const Machine* machine = machines; machine->name != nullptr; ++machine) {
        if (machine->id == id) {
            return machine->name;
        }
    }
    return "Unknown Machine";
}