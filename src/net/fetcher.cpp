// NEBULA HUB — Fetcher implementation
// Uses PS4 native libSceNet / libSceHttp (same approach as 4PT and FPKGi)

#include "fetcher.h"

#include <orbis/Net.h>
#include <orbis/Http.h>
#include <orbis/Ssl.h>
#include <orbis/libkernel.h>

#include <cstring>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#define HTTP_BUF   (256 * 1024)   // 256 KB read buffer
#define THUMB_DIR  "/data/NEBULAHUB/cache/thumbs/"

// ─────────────────────────────────────────────
//  Init / teardown
// ─────────────────────────────────────────────

Fetcher::Fetcher() {
    initNet();
    worker_ = std::thread([this]{ workerLoop(); });
}

Fetcher::~Fetcher() {
    running_ = false;
    if (worker_.joinable()) worker_.join();
    freeNet();
}

void Fetcher::initNet() {
    SceNetInitParam p{};
    p.memory = malloc(1 * 1024 * 1024);
    p.size   = 1 * 1024 * 1024;
    p.flags  = 0;
    sceNetInit(&p);
    sceNetCtlInit();

    sceSslInit(0x40000);
    sceHttpInit(0x40000);

    netHandle_ = sceNetCtlGetInfo(SCE_NET_CTL_INFO_IP_ADDRESS, nullptr);
    httpCtx_   = sceHttpCreateTemplate("NebulaHub/1.0", SCE_HTTP_VERSION_1_1, SCE_TRUE);
    sceHttpSetConnectTimeOut(httpCtx_, 10 * 1000 * 1000);   // 10 s
    sceHttpSetSendTimeOut(httpCtx_,    30 * 1000 * 1000);   // 30 s
    sceHttpSetRecvTimeOut(httpCtx_,    30 * 1000 * 1000);
}

void Fetcher::freeNet() {
    if (httpCtx_ >= 0) sceHttpDeleteTemplate(httpCtx_);
    sceHttpTerm();
    sceSslTerm();
    sceNetCtlTerm();
    sceNetTerm();
}

// ─────────────────────────────────────────────
//  HTTP helpers
// ─────────────────────────────────────────────

bool Fetcher::httpGet(const std::string& url, std::string& body, std::string* errOut) {
    int conn = sceHttpCreateConnectionWithURL(httpCtx_, url.c_str(), SCE_TRUE);
    if (conn < 0) { if (errOut) *errOut = "conn fail"; return false; }

    int req = sceHttpCreateRequestWithURL(conn, SCE_HTTP_METHOD_GET, url.c_str(), 0);
    if (req < 0) { sceHttpDeleteConnection(conn); return false; }

    int ret = sceHttpSendRequest(req, nullptr, 0);
    if (ret < 0) { sceHttpDeleteRequest(req); sceHttpDeleteConnection(conn); return false; }

    int statusCode = 0;
    sceHttpGetStatusCode(req, &statusCode);
    if (statusCode != 200) {
        sceHttpDeleteRequest(req); sceHttpDeleteConnection(conn);
        if (errOut) *errOut = "HTTP " + std::to_string(statusCode);
        return false;
    }

    char buf[HTTP_BUF];
    body.clear();
    int read;
    while ((read = sceHttpReadData(req, buf, sizeof(buf))) > 0)
        body.append(buf, read);

    sceHttpDeleteRequest(req);
    sceHttpDeleteConnection(conn);
    return read == 0;
}

bool Fetcher::httpDownload(DownloadJob& job) {
    int conn = sceHttpCreateConnectionWithURL(httpCtx_, job.url.c_str(), SCE_TRUE);
    if (conn < 0) { job.errorMsg = "connection failed"; return false; }

    int req = sceHttpCreateRequestWithURL(conn, SCE_HTTP_METHOD_GET, job.url.c_str(), 0);
    if (req < 0) { sceHttpDeleteConnection(conn); return false; }

    if (sceHttpSendRequest(req, nullptr, 0) < 0) {
        sceHttpDeleteRequest(req); sceHttpDeleteConnection(conn);
        job.errorMsg = "request failed"; return false;
    }

    int status = 0;
    sceHttpGetStatusCode(req, &status);
    if (status != 200) {
        sceHttpDeleteRequest(req); sceHttpDeleteConnection(conn);
        job.errorMsg = "HTTP " + std::to_string(status); return false;
    }

    uint64_t contentLen = 0;
    sceHttpGetContentLength(req, &contentLen);
    job.totalBytes = contentLen;

    std::ofstream out(job.destPath, std::ios::binary);
    if (!out) { job.errorMsg = "can't open dest"; return false; }

    char buf[HTTP_BUF];
    int  read;
    uint64_t prev  = 0;
    uint64_t t0    = sceKernelGetProcessTimeCounter();

    while (!job.paused && running_) {
        read = sceHttpReadData(req, buf, sizeof(buf));
        if (read < 0) { job.errorMsg = "read error"; break; }
        if (read == 0) break;

        out.write(buf, read);
        job.bytesDown += read;

        uint64_t now  = sceKernelGetProcessTimeCounter();
        double   dt   = (double)(now - t0) / 1000000.0;  // us → s
        if (dt > 0.5) {
            job.speedBps = (float)((job.bytesDown - prev) / dt);
            prev = job.bytesDown;
            t0   = now;
        }
    }

    sceHttpDeleteRequest(req);
    sceHttpDeleteConnection(conn);

    bool ok = (read == 0 && !job.paused);
    if (!ok && job.errorMsg.empty()) job.errorMsg = "incomplete";
    return ok;
}

// ─────────────────────────────────────────────
//  Worker loop
// ─────────────────────────────────────────────

void Fetcher::workerLoop() {
    // Ensure download dirs exist
    mkdir(DOWNLOAD_DIR, 0755);
    mkdir(THUMB_DIR,    0755);

    while (running_) {
        DownloadJob* active = nullptr;
        {
            std::lock_guard<std::mutex> lk(qMu_);
            for (auto& j : queue_) {
                if (j.state == InstallState::Queued && !j.paused) {
                    active = &j;
                    j.state = InstallState::Downloading;
                    break;
                }
            }
        }

        if (!active) { sceKernelUsleep(100000); continue; }

        bool ok = httpDownload(*active);

        std::lock_guard<std::mutex> lk(qMu_);
        active->state = ok ? InstallState::Done : InstallState::Error;
        if (!ok) active->speedBps = 0;
    }
}

// ─────────────────────────────────────────────
//  Public API
// ─────────────────────────────────────────────

void Fetcher::setManifestUrl(const std::string& url) { manifestUrl_ = url; }

void Fetcher::fetchManifestAsync(std::function<void(bool)> cb) {
    std::thread([this, cb]{
        std::string body;
        bool ok = httpGet(manifestUrl_, body);
        if (ok) manifestCache_ = body;
        cb(ok);
    }).detach();
}

std::string Fetcher::getManifest() const { return manifestCache_; }

void Fetcher::fetchThumbAsync(const std::string& url,
                              const std::string& cacheKey,
                              std::function<void(std::string)> onPathReady) {
    std::string cachePath = std::string(THUMB_DIR) + cacheKey + ".jpg";
    // Check disk cache first
    struct stat st{};
    if (stat(cachePath.c_str(), &st) == 0 && st.st_size > 0) {
        onPathReady(cachePath);
        return;
    }
    std::thread([this, url, cachePath, onPathReady]{
        std::string body;
        if (!httpGet(url, body)) return;
        std::ofstream f(cachePath, std::ios::binary);
        if (!f) return;
        f.write(body.data(), body.size());
        onPathReady(cachePath);
    }).detach();
}

void Fetcher::enqueue(DownloadJob job) {
    std::lock_guard<std::mutex> lk(qMu_);
    // Avoid duplicate
    for (auto& j : queue_)
        if (j.id == job.id && j.url == job.url) return;
    queue_.push_back(std::move(job));
}

void Fetcher::pause(const std::string& id) {
    std::lock_guard<std::mutex> lk(qMu_);
    for (auto& j : queue_) if (j.id == id) j.paused = true;
}

void Fetcher::resume(const std::string& id) {
    std::lock_guard<std::mutex> lk(qMu_);
    for (auto& j : queue_) {
        if (j.id == id) {
            j.paused = false;
            if (j.state == InstallState::Error) j.state = InstallState::Queued;
        }
    }
}

void Fetcher::cancel(const std::string& id) {
    std::lock_guard<std::mutex> lk(qMu_);
    queue_.erase(std::remove_if(queue_.begin(), queue_.end(),
        [&id](const DownloadJob& j){ return j.id == id; }), queue_.end());
}

void Fetcher::retry(const std::string& id) {
    std::lock_guard<std::mutex> lk(qMu_);
    for (auto& j : queue_) {
        if (j.id == id && j.state == InstallState::Error) {
            j.state = InstallState::Queued;
            j.bytesDown = 0;
            j.errorMsg.clear();
        }
    }
}

std::vector<DownloadJob> Fetcher::queueSnapshot() {
    std::lock_guard<std::mutex> lk(qMu_);
    return queue_;
}

bool Fetcher::isDownloading() const {
    std::lock_guard<std::mutex> lk(qMu_);
    for (auto& j : queue_)
        if (j.state == InstallState::Downloading) return true;
    return false;
}

int Fetcher::activeCount() const {
    std::lock_guard<std::mutex> lk(qMu_);
    int n = 0;
    for (auto& j : queue_)
        if (j.state == InstallState::Downloading || j.state == InstallState::Queued) ++n;
    return n;
}
