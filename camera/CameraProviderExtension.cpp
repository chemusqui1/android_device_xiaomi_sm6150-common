/*
 * Copyright (C) 2024 LibreMobileOS Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "CameraProviderExtension.h"

#include <fstream>

#define TORCH_BRIGHTNESS "brightness"
#define TOGGLE_SWITCH "/sys/devices/platform/soc/c440000.qcom,spmi/spmi-0/spmi0-05/c440000.qcom,spmi:qcom,pm6150l@5:qcom,leds@d300/leds/led:switch_2/brightness"

static std::string kTorchLedPaths[] = {
        "/sys/devices/platform/soc/c440000.qcom,spmi/spmi-0/spmi0-05/c440000.qcom,spmi:qcom,pm6150l@5:qcom,leds@d300/leds/led:torch_0",
        "/sys/devices/platform/soc/c440000.qcom,spmi/spmi-0/spmi0-05/c440000.qcom,spmi:qcom,pm6150l@5:qcom,leds@d300/leds/led:torch_1",
};

// Soft light paths (toco and tucana only)
static std::string kSoftLightPaths[] = {
        "/sys/devices/platform/soc/890000.i2c/i2c-3/3-0063/leds/softlight_torch_0",
        "/sys/devices/platform/soc/890000.i2c/i2c-3/3-0063/leds/softlight_torch_1",
};

/**
 * Write value to path and close file.
 */
template <typename T>
static void set(const std::string& path, const T& value) {
    std::ofstream file(path);
    // Avoid messing up all other sm6150 family
    if (file.is_open()) {
        file << value << std::endl; 
    }
}

/**
 * Read value from the path and close file.
 */
template <typename T>
static T get(const std::string& path, const T& def) {
    std::ifstream file(path);
    T result;

    file >> result;
    return file.fail() ? def : result;
}

bool supportsTorchStrengthControlExt() {
    return true;
}

bool supportsSetTorchModeExt() {
    return false;
}

int32_t getTorchDefaultStrengthLevelExt() {
    // Our default value is 75. This corresponds to 15%.
    // As we have changed the maximum value, 59% now corresponds to 75.
    return 59;
}

int32_t getTorchMaxStrengthLevelExt() {
    // 255 out of 500 is a sane brightness.
    // Let's cap it to 255 as max, we can go much higher, but I don't want to test this.
    return 255;
}

int32_t getTorchStrengthLevelExt() {
    // We write same value in the both LEDs,
    // so get from one.
    auto node = kTorchLedPaths[0] + "/" + TORCH_BRIGHTNESS;
    return get(node, 0);
}

void setTorchStrengthLevelExt(int32_t torchStrength, bool enabled) {
    set(TOGGLE_SWITCH, 0);
    for (auto& path : kTorchLedPaths) {
        auto node = path + "/" + TORCH_BRIGHTNESS;
        set(node, torchStrength);
    }
    
    // Tucana and toco soft lights have a max_brightness value of 127,
    // so set values to half for a balanced experience
    int32_t softStrength = torchStrength / 2;
    for (auto& path : kSoftLightPaths) {
        auto node = path + "/" + TORCH_BRIGHTNESS;
        set(node, softStrength);
    }
    if (enabled) {
        set(TOGGLE_SWITCH, 255);
    // Make soft lights kick in too
    } else {
        for (auto& path : kSoftLightPaths) {
            auto node = path + "/" + TORCH_BRIGHTNESS;
            set(node, 0);
        }
    }
}

void setTorchModeExt(bool enabled) {
    int32_t strength = getTorchDefaultStrengthLevelExt();
    setTorchStrengthLevelExt(enabled ? strength : 0, enabled);
}
