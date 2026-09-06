#pragma once

#include <Windows.h>
#include "SanitizationCapability.h"

bool detectNvmeCapability(HANDLE deviceHandle,SanitizationCapability& capability);