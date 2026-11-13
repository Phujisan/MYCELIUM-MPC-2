# 🍄 MYCELIUM — MPC Live 3 official SDK builder

This replaces the older community Open MPC port with the new official inMusic plugin SDK for MPC OS **3.10.1+**. The source is MYCELIUM v0.9.8. The GitHub workflow builds **real ARM64** shared-library code, generates tabs and Q-Links, runs native tests and packages the output for `VST2 Plugins` on SD/USB.

## 1. Create a new GitHub repository

Use `MYCELIUM-MPC-Official` (private if preferred). Extract this ZIP and upload its **contents**, keeping `MYCELIUM/` and `.github/workflows/build-official-sdk.yml` in their folders.

## 2. Build it on GitHub

Commit files to `main`, open **Actions → Build MYCELIUM with Akai official SDK (Live 3)** and click **Run workflow**. A push to `main` also starts it automatically. After a successful run, download the **MYCELIUM-Official-SDK-Live3** artifact.

If the workflow is red, open its failed step and share the text of the error. Do not install a partial build.

## 3. Install it on MPC Live 3

The downloaded artifact ZIP contains `MYCELIUM-v0.9.8-Akai-SDK-Live3.zip`. Unzip **that** inside the `VST2 Plugins` folder on SD/USB so the actual plugin lives at:

`VST2 Plugins/MYCELIUM - VST - MYCELIUM/`

Keep the files and folders inside it intact. On the MPC, enable Enhancements + External Plugin Support and load MYCELIUM as an **External Plugin insert effect**. Never put the old development source ZIP on the MPC. Remove/re-add the effect or restart after an update.

## 4. Test before real projects

In a blank project, start quiet and verify dry output, grains, loops, freeze, Q-Links across pages, and CPU load. Compare with 0.9.8 Windows for sound. Host-level state saving stores parameters but **not the live recorded buffer**. Third-party plugins run in MPC's process, so back up any important work.

## Build status

The original C DSP is locally regression-tested, and the new source is checked for basic C++ integration and parameter/page mapping. The GitHub workflow **still needs to run** before a real `.so` can be claimed. No hardware benchmark or touchscreen test is possible here.
