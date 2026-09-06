#pragma once

#include "../../storage/include/StorageDevice.h"
#include  "ClassificationResult.h"

class DeviceClassifier
{
public:
    ClassificationResult classify(const StorageDevice &device);
};