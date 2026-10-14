#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <string>
#include "json.hpp"

using json = nlohmann::json;
static const std::set<std::string> ROOT_KEYS = {
    "packages", "default_profile", "package_profiles", "profiles", "build", "global_system_server"
};
static const std::set<std::string> PROFILE_KEYS = {"build"};
static const std::set<std::string> BUILD_KEYS = {
    "BRAND", "DEVICE", "MANUFACTURER", "MODEL", "FINGERPRINT", "PRODUCT",
    "BOARD", "BOOTLOADER", "HARDWARE", "ID", "DISPLAY", "HOST", "TAGS",
    "TYPE", "USER", "ODM_SKU", "SKU", "RELEASE", "CODENAME", "INCREMENTAL",
    "SECURITY_PATCH", "SDK_INT"
};
static const std::regex PACKAGE_NAME("^[A-Za-z][A-Za-z0-9_]*(\\.[A-Za-z][A-Za-z0-9_]*)+$");
static const std::regex PROFILE_NAME("^[A-Za-z][A-Za-z0-9_-]{0,63}$");
static const std::regex SDK_VALUE("^[0-9]{1,2}$");

static bool reject_unknown(const json& object, const std::set<std::string>& allow, std::string& error) {
    if (!object.is_object()) { error = "expected object"; return false; }
    for (auto it = object.begin(); it != object.end(); ++it) {
        if (!allow.count(it.key())) { error = "unsupported field: " + it.key(); return false; }
    }
    return true;
}

static bool valid_build(const json& build, std::string& error) {
    if (!reject_unknown(build, BUILD_KEYS, error)) return false;
    for (auto it = build.begin(); it != build.end(); ++it) {
        if (!it.value().is_string()) { error = "build field must be string: " + it.key(); return false; }
        const auto value = it.value().get<std::string>();
        if (value.empty() || value.size() > 256) { error = "invalid build field length: " + it.key(); return false; }
    }
    if (build.contains("SDK_INT")) {
        const auto value = build["SDK_INT"].get<std::string>();
        if (!std::regex_match(value, SDK_VALUE)) { error = "SDK_INT must be decimal string"; return false; }
        int sdk = std::stoi(value);
        if (sdk < 21 || sdk > 36) { error = "SDK_INT must be in 21..36"; return false; }
    }
    return true;
}

static bool validate(const json& config, std::string& error) {
    if (!reject_unknown(config, ROOT_KEYS, error)) return false;
    if (config.contains("global_system_server") && !config["global_system_server"].is_boolean()) {
        error = "global_system_server must be boolean"; return false;
    }
    if (config.contains("global_system_server") && config["global_system_server"].get<bool>()) {
        error = "global_system_server is disabled in this build"; return false;
    }
    if (config.contains("packages")) {
        if (!config["packages"].is_array()) { error = "packages must be array"; return false; }
        std::set<std::string> seen;
        for (const auto& item : config["packages"]) {
            if (!item.is_string() || !std::regex_match(item.get<std::string>(), PACKAGE_NAME)) {
                error = "invalid package name"; return false;
            }
            if (!seen.insert(item.get<std::string>()).second) { error = "duplicate package"; return false; }
        }
    }
    if (config.contains("build") && !valid_build(config["build"], error)) return false;
    if (config.contains("profiles")) {
        if (!config["profiles"].is_object()) { error = "profiles must be object"; return false; }
        for (auto it = config["profiles"].begin(); it != config["profiles"].end(); ++it) {
            if (!std::regex_match(it.key(), PROFILE_NAME)) { error = "invalid profile name"; return false; }
            if (!reject_unknown(it.value(), PROFILE_KEYS, error)) return false;
            if (!it.value().contains("build") || !valid_build(it.value()["build"], error)) return false;
        }
    }
    if (config.contains("default_profile")) {
        if (!config["default_profile"].is_string()) { error = "default_profile must be string"; return false; }
        const auto name = config["default_profile"].get<std::string>();
        if (!config.contains("profiles") || !config["profiles"].contains(name)) { error = "default_profile missing"; return false; }
    }
    if (config.contains("package_profiles")) {
        if (!config["package_profiles"].is_object()) { error = "package_profiles must be object"; return false; }
        if (!config.contains("profiles")) { error = "package_profiles requires profiles"; return false; }
        for (auto it = config["package_profiles"].begin(); it != config["package_profiles"].end(); ++it) {
            if (!std::regex_match(it.key(), PACKAGE_NAME)) { error = "invalid mapping package"; return false; }
            if (!it.value().is_string() || !config["profiles"].contains(it.value().get<std::string>())) {
                error = "mapping references missing profile"; return false;
            }
        }
    }
    if (!config.contains("build") && !config.contains("profiles")) { error = "missing build or profiles"; return false; }
    return true;
}

int main(int argc, char** argv) {
    if (argc != 3 || std::string(argv[1]) != "validate") {
        std::cerr << "usage: skyspoofctl validate <config.json>\n";
        return 64;
    }
    std::ifstream input(argv[2]);
    if (!input) { std::cerr << "cannot read config\n"; return 2; }
    try {
        json config = json::parse(input);
        std::string error;
        if (!validate(config, error)) { std::cerr << "INVALID: " << error << "\n"; return 1; }
        std::cout << "VALID\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "INVALID: " << ex.what() << "\n";
        return 1;
    }
}
