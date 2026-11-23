# ✦ NEBULA HUB

**The ultimate PS4 avatar and theme library**
1,000+ avatars · 1,500+ themes · animated dynamic themes · DualShock 4 navigation

---

## What it is

NEBULA HUB is a native PS4 homebrew application (`.pkg`) you install on a jailbroken PS4.
It connects to a remote manifest, lets you browse thousands of avatars and themes by category,
search by name or series (Naruto, God of War, etc.), download them, and install them directly
from the app.

---

## Content sources

| Source | Content | Count |
|---|---|---|
| `archive.org/ps4-jb-avatars` | Avatar FPKG packs | 1,100+ individual avatars |
| `archive.org/ps4-permanent-themes-repository` | Official PSN theme PKGs | 1,500+ themes |

---

## BUILD ON MOBILE — Step by Step (GitHub Actions, no PC needed)

This is the **recommended path** if you're on Android.

### Step 1 — Fork / create the repo

1. Open [github.com](https://github.com) in your phone browser
2. Sign in or create a free account
3. Tap **+** (top right) → **New repository**
4. Name it `nebulahub`
5. Set it to **Public**
6. Tap **Create repository**

### Step 2 — Upload the project files

1. On GitHub, tap **Add file** → **Upload files**
2. Upload this entire zip contents (you can upload the zip itself or extract first)
   - On Android: use **Files by Google** or **ZArchiver** to extract, then upload the folder contents
3. Tap **Commit changes**

   Alternatively use **GitHub Mobile** app (free on Play Store):
   - Open the repo → tap Files tab → upload

### Step 3 — Trigger the build

1. In your repo, tap the **Actions** tab
2. If you see "I understand my workflows…" — tap **Enable**
3. Tap **Build NEBULA HUB PKG** in the left sidebar
4. Tap **Run workflow** → **Run workflow** (green button)
5. Wait 3–8 minutes

### Step 4 — Download your PKG

1. Once the build goes green (✅), tap it
2. Scroll to **Artifacts** at the bottom
3. Tap **nebulahub-pkg**
4. It downloads as a `.zip` — extract it to get `nebulahub.pkg`

### Step 5 — Install on PS4

**Option A — USB**
1. Format a USB drive as exFAT on your phone/PC
2. Create folder: `PS4/UPDATE/` (or just root)
3. Copy `nebulahub.pkg` to the USB root
4. Plug into PS4
5. GoldHEN → **Package Installer** → select the PKG → install

**Option B — Network (Remote PKG Install)**
1. On PS4, go to **Settings** → **Remote Play Connection Settings** (or use GoldHEN's built-in HTTP)
2. Use **Direct Package Installer** on your phone to send the PKG over WiFi
   - App: [github.com/devcrono/DirectPackageInstaller](https://github.com/devcrono/DirectPackageInstaller) (Android build available)

---

## CUSTOMIZE THE MANIFEST URL

Before uploading, open `src/nebula.h` and replace:

```cpp
#define MANIFEST_URL "https://your-username.github.io/nebulahub-cdn/manifest.json"
#define CDN_BASE     "https://your-username.github.io/nebulahub-cdn"
```

with your actual GitHub Pages URL (or any HTTPS server you control).

**To host the manifest on GitHub Pages for free:**

1. Create a new repo called `nebulahub-cdn`
2. Go to **Settings** → **Pages** → Source: main branch / root
3. Upload `manifest/manifest.json` to that repo
4. Your manifest URL will be: `https://YOUR_USERNAME.github.io/nebulahub-cdn/manifest.json`
5. Regenerate the manifest with more entries:
   ```bash
   python3 scripts/gen_manifest.py --output manifest/manifest.json
   ```
   Then upload the new `manifest.json` to `nebulahub-cdn`

---

## USING THE APP

| Button | Action |
|---|---|
| D-Pad | Navigate |
| ✕ | Select / confirm |
| ○ | Back |
| △ | Favorite / unfavorite |
| □ | Jump to search |
| L1 / R1 | Filter tabs (in search) |
| OPTIONS | Back / quit |

**To install an avatar:**
1. Avatars → pick category → select item → ✕
2. Preview screen → DOWNLOAD & INSTALL → ✕
3. Wait for download (see Downloads screen)
4. Launch **Avatar Changer (LAPY)** on your PS4 — the avatar will be there

**To install a theme:**
1. Themes → pick category → select item → ✕
2. Preview screen → DOWNLOAD & INSTALL → ✕
3. Both PKG parts download automatically
4. Theme installs via GoldHEN's PKG installer
5. Go to **Settings → Themes → My Themes** to apply

---

## FILE STRUCTURE

```
nebulahub/
├── src/
│   ├── main.cpp                 ← entry point
│   ├── nebula.h                 ← constants / palette / defines
│   ├── db/
│   │   ├── database.h/cpp       ← content database + JSON parser
│   ├── net/
│   │   ├── fetcher.h/cpp        ← HTTP client + download queue
│   ├── input/
│   │   ├── controller.h/cpp     ← DualShock 4 input
│   ├── install/
│   │   ├── installer.h/cpp      ← avatar + theme PKG install
│   └── ui/
│       ├── renderer.h/cpp       ← SDL2 render wrapper
│       └── screens.h/cpp        ← all 8 screens
├── sce_sys/
│   └── param.sfo.txt            ← app metadata descriptor
├── assets/
│   └── fonts/                   ← NotoSans (downloaded by build)
├── manifest/
│   └── manifest.json            ← seed content database (25 entries)
├── scripts/
│   └── gen_manifest.py          ← generate full manifest from archive.org
├── .github/workflows/
│   └── build.yml                ← GitHub Actions auto-build
├── Makefile                     ← local build (OpenOrbis)
└── README.md                    ← this file
```

---

## TROUBLESHOOTING

| Problem | Fix |
|---|---|
| Build fails: `clang not found` | Actions runner installs it — wait for step 2 to complete |
| Build fails: `PkgTool.Core not found` | AppVeyor link stale — download manually from LibOrbisPkg releases |
| PKG installs but app crashes on launch | Missing SDL2 stubs in toolchain — ensure OpenOrbis release has SDL2_stub |
| App opens but shows "No content" | Manifest URL in nebula.h is placeholder — set your CDN URL |
| Download fails on PS4 | PS4 needs internet — test with GoldHEN's browser first |
| Avatar doesn't appear in Avatar Changer | Avatar Changer (LAPY20015) must be installed separately |
| Theme doesn't appear in Settings | PKG install may still be running — wait 60s and check |

---

## ADDING MORE CONTENT

Run the manifest generator to pull the full 1,500+ theme list from archive.org:

```bash
python3 scripts/gen_manifest.py --output manifest/manifest.json
```

Upload the new manifest to your `nebulahub-cdn` repo — no PKG rebuild needed.
The app fetches the manifest on launch and updates its local database automatically.
