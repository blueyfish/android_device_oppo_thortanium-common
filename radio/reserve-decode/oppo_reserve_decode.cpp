/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "oppo_reserve_decode"

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/strings.h>

#include <fstream>

constexpr const char kPartitionPath[] = "/dev/block/bootdevice/by-name/reserve1";
constexpr std::streamoff kTargetOffset = 0x100000;
constexpr const char kTargetHeader[] = "start_";

#define IMEI_PROP "oppo.device.imeicache"
#define MEID_PROP "oppo.device.meidcache"

#pragma pack(push, 1)
struct find_my_device_info {
	char imei1[30];
	char imei2[30];
	char meid1[30];
	char meid2[30];
};
#pragma pack(pop)

std::string parse(const char* raw, size_t len) {
	std::string s(raw, len);
	size_t start = s.find("start_");
	size_t end = s.find("_end");
	if (start != std::string::npos && end != std::string::npos && end > start) {
		return s.substr(start + 6, end - (start + 6));
	}
	return "";
}

int main(void) {
	// Open partition file
	std::ifstream file(kPartitionPath, std::ios::binary);
	if (!file.is_open()) {
		LOG(ERROR) << "Unable to open partition: " << kPartitionPath;
		return 1;
	}

	// Seek to address
	struct find_my_device_info devid;
	file.seekg(kTargetOffset, std::ios::beg);
	if (!file.read(reinterpret_cast<char*>(&devid), sizeof(struct find_my_device_info))) {
		LOG(ERROR) << "Failed to read struct data at 0x" << std::hex << kTargetOffset;
		return 1;
	}

	// Check whether valid
	if (std::memcmp(devid.imei1, kTargetHeader, strlen(kTargetHeader)) != 0) {
        LOG(ERROR) << "Header mismatch at 0x" << std::hex << kTargetOffset
                   << ". Expected '" << kTargetHeader << "'";
        return 1;
    }

	// Extract device values
	std::string imei1 = parse(devid.imei1, sizeof(devid.imei1));
	std::string imei2 = parse(devid.imei2, sizeof(devid.imei2));
	std::string meid1 = parse(devid.meid1, sizeof(devid.meid1));
	std::string meid2 = parse(devid.meid2, sizeof(devid.meid2));

	if (!imei1.empty() || !imei2.empty()) {
		LOG(INFO) << "IMEI found!";
		android::base::SetProperty(IMEI_PROP, imei1 + "," + imei2);
	}

	if (!meid1.empty() || !meid2.empty()) {
		LOG(INFO) << "MEID found!";
		android::base::SetProperty(MEID_PROP, meid1 + "," + meid2);
	}

	return 0;
}
