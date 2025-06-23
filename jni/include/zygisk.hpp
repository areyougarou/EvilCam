#pragma once

#include <jni.h>

namespace zygisk {

class Api;
struct AppSpecializeArgs;
struct ServerSpecializeArgs;

class ModuleBase {
public:
    virtual void onLoad(Api *api, JNIEnv *env) {}
    virtual void preAppSpecialize(AppSpecializeArgs *args) {}
    virtual void postAppSpecialize(const AppSpecializeArgs *args) {}
    virtual void preServerSpecialize(ServerSpecializeArgs *args) {}
    virtual void postServerSpecialize(const ServerSpecializeArgs *args) {}

    virtual ~ModuleBase() = default;

    static constexpr int PROCESS_GRANTED_ROOT = (1 << 30);
    static constexpr int PROCESS_ON_DENYLIST = (1 << 31);
};

struct AppSpecializeArgs {
    jint &uid;
    jint &gid;
    jintArray &gids;
    jint &runtime_flags;
    jint &mount_external;
    jstring &se_info;
    jstring &nice_name;
    jstring &instruction_set;
    jstring &app_data_dir;
    jintArray &fds_to_ignore;
    jboolean &is_child_zygote;
    jboolean &is_top_app;
    jstring &pkg_data_info_map;
    jstring &whitelisted_data_info_map;
    jboolean &mount_data_dirs;
    jboolean &mount_storage_dirs;
};

struct ServerSpecializeArgs {
    jint &uid;
    jint &gid;
    jintArray &gids;
    jint &runtime_flags;
    jlong &permitted_capabilities;
    jlong &effective_capabilities;
};

class Api {
public:
    virtual JNIEnv *getJNIEnv() = 0;
    virtual void setOption(int opt) = 0;
    virtual int connectCompanion(size_t *size = nullptr) = 0;
    virtual ssize_t readCompanion(void *buf, size_t bufsz) = 0;
    virtual bool writeCompanion(const void *buf, size_t bufsz) = 0;
    virtual void closeCompanion() = 0;
    virtual const char *getModuleDir() = 0;

protected:
    virtual ~Api() = default;
};

} // namespace zygisk

#define REGISTER_ZYGISK_MODULE(clazz) \
    extern "C" [[gnu::visibility("default")]] [[gnu::used]] \
    void zygisk_module_entry(zygisk::Api *api, JNIEnv *env) { \
        static clazz instance; \
        instance.onLoad(api, env); \
    } \
    extern "C" [[gnu::visibility("default")]] [[gnu::used]] \
    zygisk::ModuleBase *zygisk_module_instance() { \
        static clazz instance; \
        return &instance; \
    }