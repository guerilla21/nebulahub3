#pragma once
#include "../nebula.h"
#include "../db/database.h"
#include <string>
#include <functional>

// ─────────────────────────────────────────────
//  Installer
//  Handles both avatar (.xavatar) and theme PKG installation
// ─────────────────────────────────────────────

class Installer {
public:
    explicit Installer(OrbisUserServiceUserId userId);

    // Avatar: copy .xavatar file to /data/AVATARS/ and notify user
    // If the entry is an FPKG pack, we display instructions instead
    bool installAvatar(const ContentEntry& entry,
                       std::function<void(bool,std::string)> cb);

    // Theme: install both PKG parts via PS4 PKG install API
    // Returns immediately — install happens in background via sceSystemService
    bool installTheme(const ContentEntry& entry,
                      std::function<void(bool,std::string)> cb);

    // Check if a PKG install is already pending/running
    bool isPkgInstallPending(const std::string& contentId);

    // Send PS4 notification (top-right popup)
    void notify(const std::string& title, const std::string& msg);

private:
    OrbisUserServiceUserId userId_;

    bool copyFile(const std::string& src, const std::string& dst);
    bool launchPkgInstall(const std::string& pkgPath);
    void ensureDir(const std::string& path);
};
