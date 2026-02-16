# `nxpm` - NexaPackageManager

**nxpm** is a minimalist, source-based package manager written in C11, specifically engineered for **Linux From Scratch (LFS)** environments. It bridges the gap between manual `make install` commands and complex binary managers by automating the fetching, dependency resolution, and compilation of software directly from source manifests.

---

## 🏗️ Architecture & Logic

`nxpm` operates on a **Staging-First** installation logic. To maintain system integrity in an LFS environment, it avoids direct installation to `/`.

1. **Dependency Resolution**: Uses a recursive algorithm to ensure that if Package A requires Package B, B is built and linked before A starts.
2. **DESTDIR Shadowing**: Intercepts the `make install` process by redirecting it to a temporary `BUILD_ROOT`.
3. **File Indexing**: Scans the `BUILD_ROOT` to create a manifest of every file created by the package.
4. **Atomic Deployment**: Moves files from the staging area to the system root and updates the local JSON database.

---

## 🛠️ Requirements

* **Compiler:** GCC 10+ or Clang  
* **Libraries:** 
  * `libcurl` (Network operations)  
  * `cJson` (JSON parsing)  
  * `vi` (editor)

### Installation

#### Debian / Ubuntu
```bash
sudo apt update
sudo apt install libcurl4-openssl-dev cjson vi
```

#### ArchLinux

```bash
sudo pacman -Syu
sudo pacman -S vi cjson curl libcurl
```

#### Fedora / RHEL

```bash
sudo dnf install libcurl-devel cjson vi
```

#### Windows (WSL)

* WSL Ubuntu / Debian:

```bash
sudo apt update
sudo apt install libcurl4-openssl-dev cjson vi
```


* **Build System:** GNU Make

## 📦 Installation (Bootstrap)

In a fresh LFS environment, you must bootstrap `nxpm` manually:

```bash
git clone https://github.com/your-repo/nxpm.git
cd nxpm
make
# Note: Initial install usually requires manual placement
sudo cp nxpm /usr/local/bin/

```

## ⚙️ Configuration

The package manager looks for a remote mirror list defined in its configuration. Ensure your `MIRROR_LIST_URL` points to a valid JSON file.

| Feature | Path |
| --- | --- |
| **Mirror List** | `https://WEB.com/nxpm-packages.json` |
| **Database** | `/var/lib/nxpm/installed.json` |
| **Source Cache** | `/var/cache/nxpm/sources` |
| **Build Temp** | `/tmp/nxpm-build/` |

---

## 📖 Usage Guide

### Sync with Repository

Update the local cache of available packages from the remote JSON mirror.

```bash
nxpm update

```

### Install a Package

This will resolve dependencies, download the source tarball, compile it, and install it.

```bash
nxpm install <package_name>

```

### Remove a Package

`nxpm` uses the tracked file list in the local database to safely remove all binaries, headers, and manpages associated with the software.

```bash
nxpm remove <package_name>

```

### Check Installed Software

```bash
nxpm list

```

---

## 📝 Remote Manifest Format (`nxpm-packages.json`)

Your mirror must host a JSON file following this schema:

```json
[
  {
    "name": "example-lib",
    "version": "1.0.2",
    "source_url": "https://mirrors.kernel.org/example-lib-1.0.2.tar.xz",
    "dependencies": ["zlib", "make"],
    "build_commands": [
      "./configure --prefix=/usr",
      "make -j$(nproc)",
      "make install DESTDIR=$BUILD_ROOT"
    ]
  }
]

```

> [!IMPORTANT]
> Always include `DESTDIR=$BUILD_ROOT` in the `make install` command. This variable is exported by `nxpm` during the build process to track which files belong to which package.

---

## ⚖️ License & Safety

**nxpm** is provided "as is". Since it executes build scripts as root, always ensure the `MIRROR_LIST_URL` is a trusted source. In LFS, a broken build script can overwrite critical system symlinks.

### REMEMBER! THIS PROJECT IS CURRENTLY IN PROGRESS. I AM NOT RESPONSIBLE FOR ANY CURRENT BUGS
