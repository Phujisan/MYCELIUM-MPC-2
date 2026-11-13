# MYCELIUM 0.9.8 — official inMusic MPC Plugin SDK port

This is the **official-SDK source** for the MPC Live 3, MPC OS 3.10.1+. The original granular/tape/time/reverb C DSP is embedded in a new SDK-native C++ VST2 effect wrapper. No Open MPC community framework files are used.

## Build

At the SDK root (after copying `MYCELIUM` into its `plugins/` folder):

```sh
python3 tools/build.py plugins/MYCELIUM
python3 tools/validate.py plugins/MYCELIUM
```

The GitHub workflow in the outer ZIP performs this on a cloud Linux builder and provides an installable ZIP artifact.

## Controls and performance

- 12 native touch pages; 4x4 parameter arrangement, supported by the SDK's UI generator and Q-Link mapping generator. The `ui.json` uses `knob`, `fader`, `switch` and menu widgets. Lit switches are represented by the SDK's switch widgets.
- 98 parameter IDs preserved from the Windows engine, with legacy chorus/flanger/drive parameters reserved but **no associated DSP** and not shown on touch pages.
- 32 simultaneous grains, 4 microloops, 60-second capture buffer, tape, reverb, musical pitch pool, time bending, Smart Memory and Freeze. The source uses native `float` audio, and a fixed 256-frame scratch buffer.
- The **Windows VST3 GUI cannot be transferred literally** to the MPC. Official UI theme: violet knob kit with warm rose accent, dark-plum background and cream text. Decorative pink-cloud image skin can come later, after the native tabs and knob banks prove reliable.
- Engine has no allocation in its `process()` path; old buffers get allocated in `create()`/`prepare()`. Project state parameters are stored and restored by SDK automatically. The Freeze **recorded audio itself is not serialized** into project state.
- No MPCE/XY controls or MIDI input features.

## Live 3 installation

Once GitHub Actions succeeds: download its Artifact, extract the *outer* artifact ZIP and then extract `MYCELIUM-v0.9.8-Akai-SDK-Live3.zip` directly into `VST2 Plugins` at the root of your SD card/USB stick. Confirm it creates `VST2 Plugins/MYCELIUM - VST - MYCELIUM/` containing `plugin.json`, `version.xml`, `VST/` and `Plugin Skins/`. MPC OS 3.10.1+: Preferences → Enhancements → Enable Enhancements + External Plugin Support. Add an **insert effect**, External Plugins → MYCELIUM. Restart device if not showing, or remove and re-add the effect after updating.

Back up projects first. Start with an empty MPC project and a quiet audio track. Test dry-only signal, freeze, grains, loops, time pages, Q-Link banks, save/reopen and CPU meter. This native build has not been tested on actual hardware.

Framework and license: https://github.com/inmusic-software/mpc-plugin-builder-sdk (MIT); MYCELIUM DSP original, MIT.
