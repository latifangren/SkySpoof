#include <jni.h>
#include <android/log.h>
#include <fstream>
#include <string>
#include <vector>
#include "zygisk.hpp"
#include "json.hpp"

using json = nlohmann::json;
static constexpr const char* TAG = "SkySpoof";
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static const char* CONFIG = "/data/adb/SkySpoof.json";
static const char* MODULE_CONFIG = "/data/adb/modules/SkySpoof/config.json";

static std::string read_file(const char* path) {
    std::ifstream f(path);
    if (!f) return {};
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

static json load_config() {
    for (const char* path : {CONFIG, MODULE_CONFIG}) {
        auto text = read_file(path);
        if (text.empty()) continue;
        try { return json::parse(text); }
        catch (...) { LOGE("invalid JSON: %s", path); }
    }
    return json::object();
}

static std::string string_value(const json& object, const char* key) {
    if (!object.is_object() || !object.contains(key) || !object[key].is_string()) return {};
    return object[key].get<std::string>();
}

static bool bool_value(const json& object, const char* key, bool fallback) {
    if (!object.is_object() || !object.contains(key) || !object[key].is_boolean()) return fallback;
    return object[key].get<bool>();
}

static bool package_in_array(const json& config, const std::string& package_name) {
    if (!config.contains("packages") || !config["packages"].is_array()) return false;
    for (const auto& item : config["packages"]) {
        if (item.is_string() && item.get<std::string>() == package_name) return true;
    }
    return false;
}

static std::string base_package(std::string process_name) {
    const auto colon = process_name.find(':');
    if (colon != std::string::npos) process_name.resize(colon);
    return process_name;
}

static json select_profile(const json& config, const std::string& process_name) {
    if (!config.contains("profiles") || !config["profiles"].is_object()) {
        return config.value("build", json::object());
    }
    const auto package = base_package(process_name);
    const auto mapping = config.value("package_profiles", json::object());
    std::string profile_name;
    if (mapping.is_object() && mapping.contains(package) && mapping[package].is_string()) {
        profile_name = mapping[package].get<std::string>();
    } else if (mapping.is_object() && mapping.contains(process_name) && mapping[process_name].is_string()) {
        profile_name = mapping[process_name].get<std::string>();
    } else if (config.contains("default_profile") && config["default_profile"].is_string()) {
        profile_name = config["default_profile"].get<std::string>();
    }
    if (profile_name.empty() || !config["profiles"].contains(profile_name)) return json::object();
    const auto& profile = config["profiles"][profile_name];
    return profile.is_object() ? profile.value("build", json::object()) : json::object();
}

static bool package_selected(const json& config, const std::string& process_name) {
    if (config.contains("package_profiles") && config["package_profiles"].is_object() &&
        !config["package_profiles"].empty()) {
        const auto package = base_package(process_name);
        return config["package_profiles"].contains(package) || config["package_profiles"].contains(process_name);
    }
    if (!config.contains("packages") || !config["packages"].is_array() || config["packages"].empty()) return true;
    return package_in_array(config, process_name) || package_in_array(config, base_package(process_name));
}

class SkySpoofModule final : public zygisk::ModuleBase {
    zygisk::Api* api_ = nullptr;
    JNIEnv* env_ = nullptr;
    json config_;
    std::string process_name_;
    bool enabled_ = false;

    void set_string(jclass klass, const char* field, const std::string& value) {
        if (!klass || value.empty()) return;
        jfieldID id = env_->GetStaticFieldID(klass, field, "Ljava/lang/String;");
        if (!id || env_->ExceptionCheck()) { env_->ExceptionClear(); return; }
        jstring replacement = env_->NewStringUTF(value.c_str());
        env_->SetStaticObjectField(klass, id, replacement);
        env_->DeleteLocalRef(replacement);
    }

    void set_int(jclass klass, const char* field, int value) {
        if (!klass) return;
        jfieldID id = env_->GetStaticFieldID(klass, field, "I");
        if (!id || env_->ExceptionCheck()) { env_->ExceptionClear(); return; }
        env_->SetStaticIntField(klass, id, value);
    }

    void apply_build_profile() {
        const json profile = select_profile(config_, process_name_);
        if (!profile.is_object() || profile.empty()) {
            LOGE("no profile selected for %s", process_name_.c_str());
            return;
        }
        jclass build = env_->FindClass("android/os/Build");
        if (env_->ExceptionCheck()) { env_->ExceptionClear(); return; }
        for (const char* field : {"BRAND", "DEVICE", "MANUFACTURER", "MODEL", "FINGERPRINT",
                                  "PRODUCT", "BOARD", "BOOTLOADER", "HARDWARE", "ID", "DISPLAY",
                                  "HOST", "TAGS", "TYPE", "USER", "ODM_SKU", "SKU"}) {
            set_string(build, field, string_value(profile, field));
        }
        jclass version = env_->FindClass("android/os/Build$VERSION");
        if (version) {
            for (const char* field : {"RELEASE", "CODENAME", "INCREMENTAL", "SECURITY_PATCH"}) {
                set_string(version, field, string_value(profile, field));
            }
            const auto sdk = string_value(profile, "SDK_INT");
            if (!sdk.empty()) {
                try { set_int(version, "SDK_INT", std::stoi(sdk)); }
                catch (...) { LOGE("invalid SDK_INT"); }
            }
        }
        LOGI("profile applied to %s", process_name_.c_str());
    }

public:
    void onLoad(zygisk::Api* api, JNIEnv* env) override {
        api_ = api;
        env_ = env;
        config_ = load_config();
        api_->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
    }

    void preAppSpecialize(zygisk::AppSpecializeArgs* args) override {
        if (!args || !args->nice_name) return;
        const char* raw = env_->GetStringUTFChars(args->nice_name, nullptr);
        process_name_ = raw ? raw : "";
        if (raw) env_->ReleaseStringUTFChars(args->nice_name, raw);
        enabled_ = package_selected(config_, process_name_);
    }

    void preServerSpecialize(zygisk::ServerSpecializeArgs*) override {
        // System-server remains untouched. This module is app-scoped by design.
        enabled_ = false;
        process_name_ = "system_server";
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs*) override {
        if (enabled_) apply_build_profile();
    }

    void postServerSpecialize(const zygisk::ServerSpecializeArgs*) override {}
};

REGISTER_ZYGISK_MODULE(SkySpoofModule)
