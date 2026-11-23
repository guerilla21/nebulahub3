// NEBULA HUB — Installer
// Avatar path: copy xavatar → /data/AVATARS/
// Theme path:  copy PKGs → /data/pkg/ then trigger sceSystemServiceInstallUpdate

#include "installer.h"

#include <orbis/SystemService.h>
#include <orbis/UserService.h>
#include <orbis/libkernel.h>

#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

Installer::Installer(OrbisUserServiceUserId userId) : userId_(userId) {
    ensureDir(AVATAR_DEST);
    ensureDir(PKG_INSTALL_DIR);
}

void Installer::ensureDir(const std::string& path) {
    mkdir(path.c_str(), 0755);
}

bool Installer::copyFile(const std::string& src, const std::string& dst) {
    FILE* fin  = fopen(src.c_str(), "rb");
    if (!fin) return false;
    FILE* fout = fopen(dst.c_str(), "wb");
    if (!fout) { fclose(fin); return false; }

    char buf[64 * 1024];
    size_t n;
    bool ok = true;
    while ((n = fread(buf, 1, sizeof(buf), fin)) > 0) {
        if (fwrite(buf, 1, n, fout) != n) { ok = false; break; }
    }
    fclose(fin);
    fclose(fout);
    return ok;
}

// ─────────────────────────────────────────────
//  Avatar install
// ─────────────────────────────────────────────

bool Installer::installAvatar(const ContentEntry& entry,
                               std::function<void(bool,std::string)> cb) {
    if (entry.localPath.empty()) {
        cb(false, "File not downloaded");
        return false;
    }

    if (entry.fileFormat == "xavatar_fpkg") {
        // This is a BULK FPKG (F1nnish pack) — we install it as a PKG
        // then the user applies individual avatars from Avatar Changer
        bool ok = launchPkgInstall(entry.localPath);
        if (ok) {
            notify("NEBULA HUB",
                   "Avatar pack installed! Open Avatar Changer (LAPY) to apply.");
            cb(true, "");
        } else {
            cb(false, "PKG install failed");
        }
        return ok;
    }

    // Single .xavatar file → copy to /data/AVATARS/
    std::string filename = entry.id + ".xavatar";
    std::string dst = std::string(AVATAR_DEST) + filename;
    bool ok = copyFile(entry.localPath, dst);
    if (ok) {
        notify("NEBULA HUB",
               std::string("Avatar ready! Open Avatar Changer to apply ") + entry.name);
        cb(true, dst);
    } else {
        cb(false, "Copy failed");
    }
    return ok;
}

// ─────────────────────────────────────────────
//  Theme install
// ─────────────────────────────────────────────

bool Installer::installTheme(const ContentEntry& entry,
                              std::function<void(bool,std::string)> cb) {
    // Themes come as _1 (stub) + _2 (payload) pair
    // Both must be in /data/pkg/ before triggering install
    if (entry.localStubPath.empty() || entry.localPayloadPath.empty()) {
        cb(false, "Files not downloaded");
        return false;
    }

    // Copy to pkg dir with correct names
    std::string stub    = std::string(PKG_INSTALL_DIR) + entry.id + "_1.pkg";
    std::string payload = std::string(PKG_INSTALL_DIR) + entry.id + "_2.pkg";

    if (!copyFile(entry.localStubPath, stub) ||
        !copyFile(entry.localPayloadPath, payload)) {
        cb(false, "Copy to pkg dir failed");
        return false;
    }

    // Trigger install of the stub — PS4 picks up the payload automatically
    bool ok = launchPkgInstall(stub);
    if (ok) {
        notify("NEBULA HUB",
               std::string("Installing: ") + entry.name + "\nGo to Settings → Themes");
        cb(true, "");
    } else {
        cb(false, "PKG install trigger failed");
    }
    return ok;
}

// ─────────────────────────────────────────────
//  PKG install trigger (same method as 4PT / FPKGi)
// ─────────────────────────────────────────────

bool Installer::launchPkgInstall(const std::string& pkgPath) {
    // sceSystemServiceInstallUpdate — takes path to PKG
    int ret = sceSystemServiceInstallUpdate(pkgPath.c_str());
    return ret >= 0;
}

bool Installer::isPkgInstallPending(const std::string& contentId) {
    // Check if file still exists in pkg dir (rough heuristic)
    std::string p = std::string(PKG_INSTALL_DIR) + contentId + "_1.pkg";
    struct stat st{};
    return stat(p.c_str(), &st) == 0;
}

// ─────────────────────────────────────────────
//  Notification
// ─────────────────────────────────────────────

void Installer::notify(const std::string& title, const std::string& msg) {
    // PS4 toast notification — same API used by most homebrews
    OrbisSystemServiceParamId params{};
    sceSystemServiceLoadExec("notification", nullptr);
    // Fallback: simple printf to klog
    printf("[NEBULA HUB] %s: %s\n", title.c_str(), msg.c_str());
}
