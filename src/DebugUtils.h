
#pragma once

#include "DebugTypes.h"

TBuffer ReadEntireFile(const char* Filename);
TBuffer ReadEntireProcFile(const char* Filename);

bool IsFileElf64(TBuffer Buffer);