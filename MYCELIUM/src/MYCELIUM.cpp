// MYCELIUM for the official inMusic MPC Plugin SDK (MPC OS 3.10.1+).
// Original DSP port, 32 granular voices / 4 microloops. MIT.
#include "mpc_plugin.h"
#include <array>
#include <cmath>
#include <limits>
// The original C DSP can compile as C++ and is embedded in this translation unit.
// No source-time dependence on the previous unofficial Open MPC plugin wrapper.
#include "../dsp/mycelium.c"

class MyceliumOfficial final : public mpc::Plugin {
public:
    static mpc::PluginInfo makeInfo() {
        mpc::PluginInfo i;
        i.name = "MYCELIUM";
        i.vendor = "MYCELIUM";
        i.uniqueId = MPC_FOURCC('M', 'y', '9', '8');
        i.version = 980;
        i.isInstrument = false;
        i.numInputs = 2;
        i.numOutputs = 2;
        i.tailSamples = 0; // infinite or dynamic textures, unknown tail duration
        return i;
    }

    MyceliumOfficial() : mpc::Plugin(makeInfo()),
        engine_(mpc_engine()), dsp_(engine_->create("MYCELIUM")) {
        // The exact ordering and IDs are essential for DSP mapping and projects.
        addParam(mpc::floatParam("buffer", "Memory Time", 0.5f, 60.0f, 12.0f, "s", 2)); // 0: buffer
        addParam(mpc::floatParam("duration", "Keep Playing", 0.0f, 180.0f, 0.0f, "s", 2)); // 1: duration
        addParam(mpc::boolParam("freeze", "Freeze", false)); // 2: freeze
        addParam(mpc::floatParam("threshold", "Sensitivity", 0.0001f, 0.05f, 0.004f, "", 3)); // 3: threshold
        addParam(mpc::floatParam("decay", "Memory Fade", 0.0f, 1.0f, 0.0f, "", 2)); // 4: decay
        addParam(mpc::floatParam("regen", "Regenerate", 0.0f, 0.85f, 0.0f, "", 2)); // 5: regen
        addParam(mpc::floatParam("grain", "Grain Size", 80.0f, 5000.0f, 1800.0f, "ms", 0)); // 6: grain
        addParam(mpc::floatParam("density", "Grain Density", 0.5f, 32.0f, 6.0f, "", 2)); // 7: density
        addParam(mpc::floatParam("spray", "Grain Spray", 0.0f, 1.0f, 0.35f, "", 2)); // 8: spray
        addParam(mpc::floatParam("reverse", "Reverse", 0.0f, 1.0f, 0.14f, "", 2)); // 9: reverse
        addParam(mpc::floatParam("width", "Stereo Width", 0.0f, 1.0f, 0.9f, "", 2)); // 10: width
        addParam(mpc::choiceParam("grain_note", "Grain Note", {"Down 2 octaves", "Down octave + fifth", "Down 1 octave", "Down a fifth", "Down a fourth", "Original pitch", "Up a fourth", "Up a fifth", "Up 1 octave", "Up octave + fifth", "Up 2 octaves"}, 5)); // 11: grain_note
        addParam(mpc::choiceParam("grain_var", "Grain Flavor", {"Steady note", "Add occasional fifths", "Add occasional octaves", "Mix fifths + octaves"}, 1)); // 12: grain_var
        addParam(mpc::boolParam("grain_quant", "Grain Scale", true)); // 13: grain_quant
        addParam(mpc::floatParam("loop_level", "Loop Amount", 0.0f, 1.0f, 0.52f, "", 2)); // 14: loop_level
        addParam(mpc::choiceParam("loop_rate", "Loop Rate", {"1 bar", "1/2 bar", "1 beat", "1/2 beat", "1/4 beat", "1/8 beat", "1/16 beat", "Dotted 1/8", "1/8 triplet"}, 3)); // 15: loop_rate
        addParam(mpc::floatParam("loop_length", "Loop Size", 100.0f, 8000.0f, 1600.0f, "ms", 0)); // 16: loop_length
        addParam(mpc::floatParam("loop_feedback", "Loop Repeats", 0.0f, 0.98f, 0.75f, "", 2)); // 17: loop_feedback
        addParam(mpc::floatParam("loop_reverse", "Loop Reverse", 0.0f, 1.0f, 0.1f, "", 2)); // 18: loop_reverse
        addParam(mpc::floatParam("loop_jitter", "Loop Drift", 0.0f, 1.0f, 0.12f, "", 2)); // 19: loop_jitter
        addParam(mpc::choiceParam("loop_note", "Loop Note", {"Down 2 octaves", "Down octave + fifth", "Down 1 octave", "Down a fifth", "Down a fourth", "Original pitch", "Up a fourth", "Up a fifth", "Up 1 octave", "Up octave + fifth", "Up 2 octaves"}, 5)); // 20: loop_note
        addParam(mpc::choiceParam("loop_var", "Loop Flavor", {"Steady note", "Add occasional fifths", "Add occasional octaves", "Mix fifths + octaves"}, 1)); // 21: loop_var
        addParam(mpc::boolParam("loop_quant", "Loop Scale", true)); // 22: loop_quant
        addParam(mpc::choiceParam("pitch_mode", "Scale", {"Off (fixed intervals)", "Chromatic", "Major", "Natural minor", "Major pentatonic", "Dorian", "Whole tone"}, 2)); // 23: pitch_mode
        addParam(mpc::choiceParam("root", "Root Note", {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"}, 0)); // 24: root
        addParam(mpc::logParam("source_hp", "Memory HP", 20.0f, 1200.0f, 20.0f, "Hz", 0)); // 25: source_hp
        addParam(mpc::logParam("source_lp", "Memory LP", 500.0f, 20000.0f, 20000.0f, "Hz", 0)); // 26: source_lp
        addParam(mpc::logParam("grain_hp", "Grains HP", 20.0f, 2000.0f, 40.0f, "Hz", 0)); // 27: grain_hp
        addParam(mpc::logParam("grain_lp", "Grains LP", 500.0f, 20000.0f, 14000.0f, "Hz", 0)); // 28: grain_lp
        addParam(mpc::logParam("loop_hp", "Loops HP", 20.0f, 2000.0f, 30.0f, "Hz", 0)); // 29: loop_hp
        addParam(mpc::logParam("loop_lp", "Loops LP", 500.0f, 20000.0f, 14000.0f, "Hz", 0)); // 30: loop_lp
        addParam(mpc::logParam("rev_hp", "Reverb HP", 20.0f, 1200.0f, 120.0f, "Hz", 0)); // 31: rev_hp
        addParam(mpc::logParam("rev_lp", "Reverb LP", 500.0f, 20000.0f, 11500.0f, "Hz", 0)); // 32: rev_lp
        addParam(mpc::floatParam("rev_mix", "Rev Wet", 0.0f, 1.0f, 0.6f, "", 2)); // 33: rev_mix
        addParam(mpc::floatParam("rev_time", "Rev Tail", 1.0f, 60.0f, 22.0f, "s", 2)); // 34: rev_time
        addParam(mpc::floatParam("rev_size", "Rev Size", 0.5f, 2.3f, 1.4f, "", 2)); // 35: rev_size
        addParam(mpc::floatParam("rev_predelay", "Rev PreDelay", 0.0f, 500.0f, 90.0f, "ms", 0)); // 36: rev_predelay
        addParam(mpc::floatParam("rev_damp", "Rev Darkness", 0.0f, 1.0f, 0.55f, "", 2)); // 37: rev_damp
        addParam(mpc::floatParam("rev_width", "Rev Width", 0.0f, 1.0f, 1.0f, "", 2)); // 38: rev_width
        addParam(mpc::floatParam("mix", "Dry / Texture", 0.0f, 1.0f, 0.78f, "", 2)); // 39: mix
        addParam(mpc::floatParam("wet_gain", "FX Gain", 0.0f, 2.0f, 0.85f, "", 2)); // 40: wet_gain
        addParam(mpc::floatParam("tempo", "Tempo Fallback", 40.0f, 240.0f, 110.0f, "BPM", 0)); // 41: tempo
        addParam(mpc::boolParam("soft_clip", "Soft Limit", true)); // 42: soft_clip
        addParam(mpc::boolParam("pool_original", "Original", true)); // 43: pool_original
        addParam(mpc::boolParam("pool_oct_up", "Oct +12", true)); // 44: pool_oct_up
        addParam(mpc::boolParam("pool_oct_down", "Oct -12", true)); // 45: pool_oct_down
        addParam(mpc::boolParam("pool_fifth_up", "Fifth +7", true)); // 46: pool_fifth_up
        addParam(mpc::boolParam("pool_fifth_down", "Fifth -7", true)); // 47: pool_fifth_down
        addParam(mpc::boolParam("grain_pool", "Grain Pool", true)); // 48: grain_pool
        addParam(mpc::choiceParam("grain_algorithm", "Grain Picker", {"Random", "Round robin", "Back and forth", "Shuffle (all once)", "Favor original", "Neighbor wander"}, 0)); // 49: grain_algorithm
        addParam(mpc::boolParam("loop_pool", "Loop Pool", true)); // 50: loop_pool
        addParam(mpc::choiceParam("loop_algorithm", "Loop Picker", {"Random", "Round robin", "Back and forth", "Shuffle (all once)", "Favor original", "Neighbor wander"}, 1)); // 51: loop_algorithm
        addParam(mpc::floatParam("clean_send", "Clean Send", 0.0f, 1.0f, 0.35f, "", 2)); // 52: clean_send
        addParam(mpc::floatParam("texture_send", "Texture Send", 0.0f, 1.0f, 0.65f, "", 2)); // 53: texture_send
        addParam(mpc::floatParam("grain_adventure", "Grain Explore", 0.0f, 1.0f, 0.35f, "", 2)); // 54: grain_adventure
        addParam(mpc::floatParam("loop_adventure", "Loop Explore", 0.0f, 1.0f, 0.35f, "", 2)); // 55: loop_adventure
        addParam(mpc::choiceParam("phrase_style", "Phrase Style", {"Whole phrase", "Little pieces", "Call & response", "Stepping stones", "Rolling waves", "Happy accidents"}, 0)); // 56: phrase_style
        addParam(mpc::floatParam("phrase_density", "Phrase Full", 0.0f, 1.0f, 0.75f, "", 2)); // 57: phrase_density
        addParam(mpc::floatParam("phrase_variation", "Phrase Change", 0.0f, 1.0f, 0.55f, "", 2)); // 58: phrase_variation
        addParam(mpc::boolParam("chorus_on", "Legacy chorus on", false)); // 59: chorus_on
        addParam(mpc::logParam("chorus_rate", "Legacy chorus rate", 0.05f, 2.0f, 0.22f, "Hz", 0)); // 60: chorus_rate
        addParam(mpc::floatParam("chorus_depth", "Legacy chorus depth", 0.0f, 1.0f, 0.55f, "", 2)); // 61: chorus_depth
        addParam(mpc::floatParam("chorus_mix", "Legacy chorus mix", 0.0f, 1.0f, 0.4f, "", 2)); // 62: chorus_mix
        addParam(mpc::boolParam("flanger_on", "Legacy flanger on", false)); // 63: flanger_on
        addParam(mpc::logParam("flanger_rate", "Legacy flanger rate", 0.02f, 3.0f, 0.12f, "Hz", 0)); // 64: flanger_rate
        addParam(mpc::floatParam("flanger_depth", "Legacy flanger depth", 0.0f, 1.0f, 0.65f, "", 2)); // 65: flanger_depth
        addParam(mpc::floatParam("flanger_feedback", "Legacy flanger feedbac", 0.0f, 0.85f, 0.32f, "", 2)); // 66: flanger_feedback
        addParam(mpc::floatParam("flanger_mix", "Legacy flanger mix", 0.0f, 1.0f, 0.35f, "", 2)); // 67: flanger_mix
        addParam(mpc::boolParam("drive_on", "Legacy drive on", false)); // 68: drive_on
        addParam(mpc::floatParam("drive_amount", "Legacy drive amount", 0.0f, 1.0f, 0.32f, "", 2)); // 69: drive_amount
        addParam(mpc::floatParam("drive_mix", "Legacy drive mix", 0.0f, 1.0f, 0.55f, "", 2)); // 70: drive_mix
        addParam(mpc::boolParam("smart_memory", "Smart Memory", true)); // 71: smart_memory
        addParam(mpc::floatParam("new_phrase_gap", "Fresh After", 1.0f, 12.0f, 2.5f, "s", 2)); // 72: new_phrase_gap
        addParam(mpc::floatParam("dry_level", "Clean Level", 0.0f, 1.5f, 1.0f, "\u00d7", 2)); // 73: dry_level
        addParam(mpc::floatParam("grain_level", "Grains Level", 0.0f, 1.5f, 0.8f, "\u00d7", 2)); // 74: grain_level
        addParam(mpc::floatParam("loop_mix_level", "Loops Level", 0.0f, 1.5f, 0.8f, "\u00d7", 2)); // 75: loop_mix_level
        addParam(mpc::floatParam("reverb_level", "Reverb Level", 0.0f, 1.5f, 1.0f, "\u00d7", 2)); // 76: reverb_level
        addParam(mpc::floatParam("texture_sustain", "Texture Linger", 0.0f, 1.0f, 0.75f, "", 2)); // 77: texture_sustain
        addParam(mpc::choiceParam("reverb_character", "Reverb Mood", {"Bright room", "Dark chamber", "Large hall", "Endless ambient"}, 2)); // 78: reverb_character
        addParam(mpc::floatParam("input_tape_mix", "Clean Tape", 0.0f, 1.0f, 0.0f, "", 2)); // 79: input_tape_mix
        addParam(mpc::floatParam("texture_tape_mix", "Texture Tape", 0.0f, 1.0f, 0.0f, "", 2)); // 80: texture_tape_mix
        addParam(mpc::floatParam("tape_wow", "Wow", 0.0f, 1.0f, 0.18f, "", 2)); // 81: tape_wow
        addParam(mpc::floatParam("tape_flutter", "Flutter", 0.0f, 1.0f, 0.15f, "", 2)); // 82: tape_flutter
        addParam(mpc::floatParam("tape_age", "Wear", 0.0f, 1.0f, 0.12f, "", 2)); // 83: tape_age
        addParam(mpc::floatParam("tape_saturation", "Warmth", 0.0f, 1.0f, 0.2f, "", 2)); // 84: tape_saturation
        addParam(mpc::floatParam("tape_tone", "Brightness", 0.0f, 1.0f, 0.75f, "", 2)); // 85: tape_tone
        addParam(mpc::floatParam("tape_stereo", "Stereo Drift", 0.0f, 1.0f, 0.65f, "", 2)); // 86: tape_stereo
        addParam(mpc::floatParam("tape_output", "Tape Output", 0.0f, 2.0f, 1.0f, "\u00d7", 2)); // 87: tape_output
        addParam(mpc::floatParam("phrase_bias", "Familiarity", 0.0f, 1.0f, 0.35f, "", 2)); // 88: phrase_bias
        addParam(mpc::floatParam("master_level", "Master Level", 0.0f, 1.5f, 1.0f, "\u00d7", 2)); // 89: master_level
        addParam(mpc::boolParam("tape_enabled", "Tape On", true)); // 90: tape_enabled
        addParam(mpc::floatParam("time_bend", "Time Speed", 0.125f, 2.0f, 1.0f, "\u00d7", 2)); // 91: time_bend
        addParam(mpc::choiceParam("time_mode", "Time Mode", {"Tape pitch", "Pitch-preserving stretch", "Smooth bend", "Rhythmic ripple", "Dreamy drift", "Time hold"}, 0)); // 92: time_mode
        addParam(mpc::floatParam("time_motion", "Motion Amount", 0.0f, 1.0f, 0.25f, "", 2)); // 93: time_motion
        addParam(mpc::logParam("time_rate", "Motion Rate", 0.01f, 4.0f, 0.15f, "Hz", 0)); // 94: time_rate
        addParam(mpc::choiceParam("new_notes_mode", "Fresh Notes", {"Fresh start", "Join the flow", "Slowly melt", "Parallel worlds", "Restart time", "Alternating time"}, 0)); // 95: new_notes_mode
        addParam(mpc::floatParam("transition_time", "Melt Time", 0.0f, 30.0f, 6.0f, "s", 2)); // 96: transition_time
        addParam(mpc::floatParam("time_depth", "Time Depth", 0.0f, 1.0f, 1.0f, "", 2)); // 97: time_depth
        if (numParams()!=kParamCount) mpc::log("MYCELIUM parameter count mismatch");
    }

    ~MyceliumOfficial() override { if(dsp_) engine_->destroy(dsp_); }

    void prepare(double sampleRate, int maxBlockSize) override {
        (void)maxBlockSize;
        if(!dsp_) return;
        // Rebuild audio rings ONLY on an actual sample rate change. Never in process().
        if(std::fabs(preparedRate_ - sampleRate) > 0.5) {
            if(engine_->set_sample_rate(dsp_, sampleRate))
                preparedRate_ = sampleRate;
            else {
                ready_ = false;
                return;
            }
        }
        ready_ = (preparedRate_ > 0);
        dirty_ = true;
    }

    void reset() override {
        if(dsp_) mycelium_reset_realtime(dsp_);
        dirty_ = true; // reapply saved parameters after state load
    }

    void process(mpc::ProcessContext& ctx) override {
        if(!dsp_ || !ready_ || ctx.numFrames <= 0 || ctx.numOutputs <= 0) {
            for(int c=0;c<ctx.numOutputs;c++)
                if(ctx.outputs && ctx.outputs[c])
                    for(int f=0;f<ctx.numFrames;f++) ctx.outputs[c][f]=0;
            return;
        }

        // Parameter atomics come from the official SDK. DSP gets changes only
        // at block boundaries, avoiding races with MIDI/UI/project save threads.
        for(int i=0;i<kParamCount;i++) {
            float n = getNormalised(i);
            if(dirty_ || n!=lastNorm_[i]) {
                lastNorm_[i]=n;
                mycelium_set_by_index(dsp_, i, get(i));
            }
        }
        dirty_=false;
        engine_->song_pos(dsp_, ctx.transport.ppqValid?ctx.transport.ppqPos:0.0,
                          (ctx.transport.tempoValid && ctx.transport.bpm>20.0)
                          ?ctx.transport.bpm:(double)get(41),
                          ctx.transport.valid && ctx.transport.playing?1:0,
                          ctx.numFrames);

        // Fixed-size audio scratch. Works even when the MPC's VST host passes
        // identical input/output pointers. No allocation and no audio conversion.
        // Chunking handles even unusually large host blocks without resizing.
        for(int pos=0;pos<ctx.numFrames;pos+=kScratchFrames) {
            int count = ctx.numFrames-pos;
            if(count>kScratchFrames) count=kScratchFrames;
            for(int f=0;f<count;f++) {
                float left=0, right=0;
                if(ctx.numInputs>0 && ctx.inputs && ctx.inputs[0]) left=ctx.inputs[0][pos+f];
                if(ctx.numInputs>1 && ctx.inputs && ctx.inputs[1]) right=ctx.inputs[1][pos+f];
                else right=left;
                input_[2*f] = left;
                input_[2*f+1] = right;
            }
            engine_->process_float(dsp_, input_.data(), output_.data(), count);
            if(ctx.outputs && ctx.outputs[0])
                for(int f=0;f<count;f++) ctx.outputs[0][pos+f]=output_[2*f];
            if(ctx.numOutputs>1 && ctx.outputs && ctx.outputs[1])
                for(int f=0;f<count;f++) ctx.outputs[1][pos+f]=output_[2*f+1];
        }
    }

private:
    static constexpr int kParamCount = 98;
    static constexpr int kScratchFrames = 256;
    const mpc_engine_t* engine_;
    void* dsp_ = nullptr;
    bool dirty_ = true;
    bool ready_ = true;
    double preparedRate_ = 44100.0; // C engine creates its initial buffers at 44.1kHz
    std::array<float,kParamCount> lastNorm_{};
    std::array<float,2*kScratchFrames> input_{};
    std::array<float,2*kScratchFrames> output_{};
};

MPC_PLUGIN_ENTRY(MyceliumOfficial)
