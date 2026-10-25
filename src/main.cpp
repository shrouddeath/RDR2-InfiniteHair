#include <windows.h>

#include "main.h"
#include "script.h"

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        ScriptSetModule(module);
        scriptRegister(module, ScriptMain);
        break;
    case DLL_PROCESS_DETACH:
        scriptUnregister(module);
        break;
    }
    return TRUE;
}
