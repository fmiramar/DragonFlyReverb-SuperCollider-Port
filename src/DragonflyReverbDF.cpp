/*
 * DragonflyReverbDF
 *
 * SuperCollider server-plugin port of Dragonfly Reverb.
 * Derived from Dragonfly Reverb / Freeverb3 DSP code.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "SC_PlugIn.h"

#include "freeverb/earlyref.hpp"
#include "freeverb/nrev.hpp"
#include "freeverb/nrevb.hpp"
#include "freeverb/progenitor2.hpp"
#include "freeverb/strev.hpp"
#include "freeverb/zrev2.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <new>

static InterfaceTable* ft;

namespace
{
constexpr int earlyProgramCount = 8;
constexpr int earlyBufferSize = 256;
constexpr int hallBufferSize = 256;
constexpr int plateBufferSize = 256;
constexpr int roomBufferSize = 256;
constexpr float roomLateGain = 2.5f;

enum EarlyInputIndex
{
    EarlyInL = 0,
    EarlyInR,
    EarlyDry,
    EarlyWet,
    EarlyProgram,
    EarlySize,
    EarlyWidth,
    EarlyLowCut,
    EarlyHighCut,
    EarlyParamCount
};

enum HallInputIndex
{
    HallInL = 0,
    HallInR,
    HallDry,
    HallEarly,
    HallLate,
    HallSize,
    HallWidth,
    HallPredelay,
    HallDiffuse,
    HallLowCut,
    HallLowXover,
    HallLowMult,
    HallHighCut,
    HallHighXover,
    HallHighMult,
    HallSpin,
    HallWander,
    HallDecay,
    HallEarlySend,
    HallModulation,
    HallParamCount
};

enum PlateInputIndex
{
    PlateInL = 0,
    PlateInR,
    PlateDry,
    PlateWet,
    PlateAlgorithm,
    PlateWidth,
    PlatePredelay,
    PlateDecay,
    PlateLowCut,
    PlateHighCut,
    PlateDamp,
    PlateParamCount
};

enum RoomInputIndex
{
    RoomInL = 0,
    RoomInR,
    RoomDry,
    RoomEarly,
    RoomEarlySend,
    RoomLate,
    RoomSize,
    RoomWidth,
    RoomPredelay,
    RoomDecay,
    RoomDiffuse,
    RoomSpin,
    RoomWander,
    RoomInHighCut,
    RoomEarlyDamp,
    RoomLateDamp,
    RoomBoost,
    RoomBoostLPF,
    RoomInLowCut,
    RoomParamCount
};

enum PlateAlgorithm
{
    PlateAlgorithmNRev = 0,
    PlateAlgorithmNRevB,
    PlateAlgorithmSTRev
};

constexpr int earlyProgramMap[earlyProgramCount] = {
    2,  // Abrupt Echo
    18, // Backstage Pass
    0,  // Concert Venue
    19, // Damaged Goods
    1,  // Elevator Pitch
    13, // Floor Thirteen
    14, // Garage Band
    21  // Home Studio
};

class PlateNRev : public fv3::nrev_f {
public:
    PlateNRev() : fv3::nrev_f() {}

    void setDampLpf(float value)
    {
        dampLpf = limFs2(value);
        dampLpfL.setLPF_BW(dampLpf, getTotalSampleRate());
        dampLpfR.setLPF_BW(dampLpf, getTotalSampleRate());
    }

    void mute() override
    {
        fv3::nrev_f::mute();
        dampLpfL.mute();
        dampLpfR.mute();
    }

    void setFsFactors() override
    {
        fv3::nrev_f::setFsFactors();
        setDampLpf(dampLpf);
    }

    void processloop2(long count, float* inputL, float* inputR, float* outputL, float* outputR)
    {
        float outL = 0.0f;
        float outR = 0.0f;

        while (count-- > 0) {
            outL = outR = 0.0f;
            hpf = damp3_1 * inDCC(*inputL + *inputR) - damp3 * hpf;
            UNDENORMAL(hpf);

            hpf *= FV3_NREV_SCALE_WET;

            for (long i = 0; i < FV3_NREV_NUM_COMB; ++i)
                outL += combL[i]._process(hpf);
            for (long i = 0; i < 3; ++i)
                outL = allpassL[i]._process_ov(outL);
            lpfL = dampLpfL(damp2 * lpfL + damp2_1 * outL);
            UNDENORMAL(lpfL);
            outL = allpassL[3]._process_ov(lpfL);
            outL = allpassL[5]._process_ov(outL);
            outL = delayWL(lLDCC(outL));

            for (long i = 0; i < FV3_NREV_NUM_COMB; ++i)
                outR += combR[i]._process(hpf);
            for (long i = 0; i < 3; ++i)
                outR = allpassR[i]._process_ov(outR);
            lpfR = dampLpfR(damp2 * lpfR + damp2_1 * outR);
            UNDENORMAL(lpfR);
            outR = allpassR[3]._process_ov(lpfR);
            outR = allpassR[6]._process_ov(outR);
            outR = delayWR(lRDCC(outR));

            *outputL = outL * wet1 + outR * wet2 + delayL(*inputL) * dry;
            *outputR = outR * wet1 + outL * wet2 + delayR(*inputR) * dry;
            ++inputL;
            ++inputR;
            ++outputL;
            ++outputR;
        }
    }

private:
    float dampLpf = 20000.0f;
    fv3::iir_1st_f dampLpfL;
    fv3::iir_1st_f dampLpfR;
};

class PlateNRevB : public fv3::nrevb_f {
public:
    PlateNRevB() : fv3::nrevb_f() {}

    void setDampLpf(float value)
    {
        dampLpf = limFs2(value);
        dampLpfL.setLPF_BW(dampLpf, getTotalSampleRate());
        dampLpfR.setLPF_BW(dampLpf, getTotalSampleRate());
    }

    void mute() override
    {
        fv3::nrevb_f::mute();
        dampLpfL.mute();
        dampLpfR.mute();
    }

    void setFsFactors() override
    {
        fv3::nrevb_f::setFsFactors();
        setDampLpf(dampLpf);
    }

    void processloop2(long count, float* inputL, float* inputR, float* outputL, float* outputR) override
    {
        float outL = 0.0f;
        float outR = 0.0f;
        float tmpL = 0.0f;
        float tmpR = 0.0f;

        while (count-- > 0) {
            hpf = damp3_1 * inDCC.process(*inputL + *inputR) - damp3 * hpf;
            UNDENORMAL(hpf);
            outL = outR = tmpL = tmpR = hpf;

            outL += apfeedback * lastL;
            lastL += -1.0f * apfeedback * outL;

            for (long i = 0; i < FV3_NREV_NUM_COMB; ++i)
                outL += combL[i]._process(tmpL);
            for (long i = 0; i < FV3_NREVB_NUM_COMB_2; ++i)
                outL += comb2L[i]._process(tmpL);
            for (long i = 0; i < 3; ++i)
                outL = allpassL[i]._process(outL);
            for (long i = 0; i < FV3_NREVB_NUM_ALLPASS_2; ++i)
                outL = allpass2L[i]._process(outL);
            lpfL = dampLpfL(damp2 * lpfL + damp2_1 * outL);
            UNDENORMAL(lpfL);
            outL = allpassL[3]._process(lpfL);
            outL = allpassL[5]._process(outL);
            outL = lLDCC(outL);

            outR += apfeedback * lastR;
            lastR += -1.0f * apfeedback * outR;
            for (long i = 0; i < FV3_NREV_NUM_COMB; ++i)
                outR += combR[i]._process(tmpR);
            for (long i = 0; i < FV3_NREVB_NUM_COMB_2; ++i)
                outR += comb2R[i]._process(tmpR);
            for (long i = 0; i < 3; ++i)
                outR = allpassR[i]._process(outR);
            for (long i = 0; i < FV3_NREVB_NUM_ALLPASS_2; ++i)
                outR = allpass2R[i]._process(outR);
            lpfR = dampLpfR(damp2 * lpfR + damp2_1 * outR);
            UNDENORMAL(lpfR);
            outR = allpassR[3]._process(lpfR);
            outR = allpassL[6]._process(outR);
            outR = lRDCC(outR);

            lastL = FV3_NREVB_SCALE_WET * delayWL(lastL);
            lastR = FV3_NREVB_SCALE_WET * delayWR(lastR);
            *outputL = lastL * wet1 + lastR * wet2 + delayL(*inputL) * dry;
            *outputR = lastR * wet1 + lastL * wet2 + delayR(*inputR) * dry;
            lastL = outL;
            lastR = outR;
            ++inputL;
            ++inputR;
            ++outputL;
            ++outputR;
        }
    }

private:
    float dampLpf = 20000.0f;
    fv3::iir_1st_f dampLpfL;
    fv3::iir_1st_f dampLpfR;
};

inline float inputAt(Unit* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline float clampRange(float value, float minValue, float maxValue) noexcept
{
    return sc_clip(value, minValue, maxValue);
}

inline int clampProgram(float value) noexcept
{
    return static_cast<int>(clampRange(std::round(value), 0.0f, static_cast<float>(earlyProgramCount - 1)));
}

template <typename UnitType>
void passthroughNext(UnitType* unit, int inNumSamples)
{
    const float* inL = IN(0);
    const float* inR = IN(1);
    float* outL = OUT(0);
    float* outR = OUT(1);

    for (int i = 0; i < inNumSamples; ++i) {
        outL[i] = inL[i];
        outR[i] = inR[i];
    }
}

struct EarlyReflectionsDF : public Unit {
    fv3::earlyref_f model;
    std::array<float, EarlyParamCount> previousParams {};
    std::array<float, earlyBufferSize> inputL {};
    std::array<float, earlyBufferSize> inputR {};
    std::array<float, earlyBufferSize> outputL {};
    std::array<float, earlyBufferSize> outputR {};
    float dryLevel = 0.8f;
    float wetLevel = 0.2f;
};

struct HallDF : public Unit {
    fv3::earlyref_f early;
    fv3::zrev2_f late;
    std::array<float, HallParamCount> previousParams {};
    std::array<float, hallBufferSize> earlyOutL {};
    std::array<float, hallBufferSize> earlyOutR {};
    std::array<float, hallBufferSize> lateInL {};
    std::array<float, hallBufferSize> lateInR {};
    std::array<float, hallBufferSize> lateOutL {};
    std::array<float, hallBufferSize> lateOutR {};
    float dryLevel = 0.8f;
    float earlyLevel = 0.1f;
    float lateLevel = 0.2f;
    float earlySend = 0.2f;
};

struct PlateDF : public Unit {
    fv3::iir_1st_f inputLpfL;
    fv3::iir_1st_f inputLpfR;
    fv3::iir_1st_f inputHpfL;
    fv3::iir_1st_f inputHpfR;
    PlateNRev nrev;
    PlateNRevB nrevb;
    fv3::strev_f strev;
    fv3::revbase_f* model = nullptr;
    std::array<float, PlateParamCount> previousParams {};
    std::array<float, plateBufferSize> filteredInputL {};
    std::array<float, plateBufferSize> filteredInputR {};
    std::array<float, plateBufferSize> outputL {};
    std::array<float, plateBufferSize> outputR {};
    float sampleRate = 48000.0f;
    float dryLevel = 0.8f;
    float wetLevel = 0.2f;
};

struct RoomDF : public Unit {
    fv3::iir_1st_f inputLpfL;
    fv3::iir_1st_f inputLpfR;
    fv3::iir_1st_f inputHpfL;
    fv3::iir_1st_f inputHpfR;
    fv3::earlyref_f early;
    fv3::progenitor2_f late;
    std::array<float, RoomParamCount> previousParams {};
    std::array<float, roomBufferSize> filteredInputL {};
    std::array<float, roomBufferSize> filteredInputR {};
    std::array<float, roomBufferSize> earlyOutL {};
    std::array<float, roomBufferSize> earlyOutR {};
    std::array<float, roomBufferSize> lateInL {};
    std::array<float, roomBufferSize> lateInR {};
    std::array<float, roomBufferSize> lateOutL {};
    std::array<float, roomBufferSize> lateOutR {};
    float sampleRate = 48000.0f;
    float dryLevel = 0.8f;
    float earlyLevel = 0.1f;
    float earlySend = 0.2f;
    float lateLevel = 0.2f;
};

void applyEarlyParameter(EarlyReflectionsDF* unit, int index, float value)
{
    switch (index) {
    case EarlyDry:
        unit->dryLevel = clampRange(value, 0.0f, 100.0f) * 0.01f;
        break;
    case EarlyWet:
        unit->wetLevel = clampRange(value, 0.0f, 100.0f) * 0.01f;
        break;
    case EarlyProgram:
        unit->model.loadPresetReflection(earlyProgramMap[clampProgram(value)]);
        break;
    case EarlySize:
        unit->model.setRSFactor(clampRange(value, 10.0f, 60.0f) * 0.1f);
        break;
    case EarlyWidth:
        unit->model.setwidth(clampRange(value, 50.0f, 150.0f) * 0.01f);
        break;
    case EarlyLowCut:
        unit->model.setoutputhpf(clampRange(value, 0.0f, 200.0f));
        break;
    case EarlyHighCut:
        unit->model.setoutputlpf(clampRange(value, 1000.0f, 16000.0f));
        break;
    default:
        break;
    }
}

void initializeEarlyModel(EarlyReflectionsDF* unit)
{
    unit->model.setMuteOnChange(false);
    unit->model.setdryr(0.0f);
    unit->model.setwet(0.0f);
    unit->model.setwidth(0.8f);
    unit->model.setLRDelay(0.3f);
    unit->model.setLRCrossApFreq(750.0f, 4.0f);
    unit->model.setDiffusionApFreq(150.0f, 4.0f);
    unit->model.setSampleRate(static_cast<float>(SAMPLERATE));

    unit->previousParams.fill(std::numeric_limits<float>::quiet_NaN());
    for (int i = EarlyDry; i < EarlyParamCount; ++i) {
        const float value = IN0(i);
        unit->previousParams[i] = value;
        applyEarlyParameter(unit, i, value);
    }

    unit->model.mute();
}

void applyHallParameter(HallDF* unit, int index, float value)
{
    switch (index) {
    case HallDry:
        unit->dryLevel = clampRange(value, 0.0f, 100.0f) * 0.01f;
        break;
    case HallEarly:
        unit->earlyLevel = clampRange(value, 0.0f, 100.0f) * 0.01f;
        break;
    case HallLate:
        unit->lateLevel = clampRange(value, 0.0f, 100.0f) * 0.01f;
        break;
    case HallSize: {
        const float size = clampRange(value, 10.0f, 60.0f);
        unit->early.setRSFactor(size * 0.1f);
        unit->late.setRSFactor(size / 80.0f);
        break;
    }
    case HallWidth: {
        const float width = clampRange(value, 50.0f, 150.0f) * 0.01f;
        unit->early.setwidth(width);
        unit->late.setwidth(width);
        break;
    }
    case HallPredelay:
        unit->late.setPreDelay(std::max(0.1f, clampRange(value, 0.0f, 100.0f)));
        break;
    case HallDiffuse: {
        const float diffuse = clampRange(value, 0.0f, 100.0f) / 140.0f;
        unit->late.setidiffusion1(diffuse);
        unit->late.setapfeedback(diffuse);
        break;
    }
    case HallLowCut: {
        const float lowCut = clampRange(value, 0.0f, 200.0f);
        unit->early.setoutputhpf(lowCut);
        unit->late.setoutputhpf(lowCut);
        break;
    }
    case HallLowXover:
        unit->late.setxover_low(clampRange(value, 200.0f, 1200.0f));
        break;
    case HallLowMult:
        unit->late.setrt60_factor_low(clampRange(value, 0.5f, 2.5f));
        break;
    case HallHighCut: {
        const float highCut = clampRange(value, 1000.0f, 16000.0f);
        unit->early.setoutputlpf(highCut);
        unit->late.setoutputlpf(highCut);
        break;
    }
    case HallHighXover:
        unit->late.setxover_high(clampRange(value, 1000.0f, 16000.0f));
        break;
    case HallHighMult:
        unit->late.setrt60_factor_high(clampRange(value, 0.2f, 1.2f));
        break;
    case HallSpin:
        unit->late.setspin(clampRange(value, 0.0f, 10.0f));
        break;
    case HallWander:
        unit->late.setwander(clampRange(value, 0.0f, 40.0f));
        break;
    case HallDecay:
        unit->late.setrt60(clampRange(value, 0.1f, 10.0f));
        break;
    case HallEarlySend:
        unit->earlySend = clampRange(value, 0.0f, 100.0f) * 0.01f;
        break;
    case HallModulation: {
        const float modulation = value == 0.0f ? 0.001f : clampRange(value, 0.0f, 100.0f) * 0.01f;
        unit->late.setspinfactor(modulation);
        unit->late.setlfofactor(modulation);
        break;
    }
    default:
        break;
    }
}

void initializeHallModel(HallDF* unit)
{
    unit->early.loadPresetReflection(FV3_EARLYREF_PRESET_1);
    unit->early.setMuteOnChange(false);
    unit->early.setdryr(0.0f);
    unit->early.setwet(0.0f);
    unit->early.setwidth(0.8f);
    unit->early.setLRDelay(0.3f);
    unit->early.setLRCrossApFreq(750.0f, 4.0f);
    unit->early.setDiffusionApFreq(150.0f, 4.0f);
    unit->early.setSampleRate(static_cast<float>(SAMPLERATE));

    unit->late.setMuteOnChange(false);
    unit->late.setwet(0.0f);
    unit->late.setdryr(0.0f);
    unit->late.setwidth(1.0f);
    unit->late.setSampleRate(static_cast<float>(SAMPLERATE));

    unit->previousParams.fill(std::numeric_limits<float>::quiet_NaN());
    for (int i = HallDry; i < HallParamCount; ++i) {
        const float value = IN0(i);
        unit->previousParams[i] = value;
        applyHallParameter(unit, i, value);
    }

}

void setPlateInputLPF(PlateDF* unit, float frequency)
{
    const float clamped = clampRange(frequency, 0.0f, unit->sampleRate * 0.5f);
    unit->inputLpfL.setLPF_BW(clamped, unit->sampleRate);
    unit->inputLpfR.setLPF_BW(clamped, unit->sampleRate);
}

void setPlateInputHPF(PlateDF* unit, float frequency)
{
    const float clamped = clampRange(frequency, 0.0f, unit->sampleRate * 0.5f);
    unit->inputHpfL.setHPF_BW(clamped, unit->sampleRate);
    unit->inputHpfR.setHPF_BW(clamped, unit->sampleRate);
}

void applyPlateParameter(PlateDF* unit, int index, float value)
{
    switch (index) {
    case PlateDry:
        unit->dryLevel = clampRange(value, 0.0f, 100.0f) * 0.01f;
        break;
    case PlateWet:
        unit->wetLevel = clampRange(value, 0.0f, 100.0f) * 0.01f;
        break;
    case PlateWidth: {
        const float width = clampRange(value, 50.0f, 150.0f) / 120.0f;
        unit->strev.setwidth(width);
        unit->nrev.setwidth(width);
        unit->nrevb.setwidth(width);
        break;
    }
    case PlatePredelay: {
        const float predelay = std::max(0.1f, clampRange(value, 0.0f, 100.0f));
        unit->strev.setPreDelay(predelay);
        unit->nrev.setPreDelay(predelay);
        unit->nrevb.setPreDelay(predelay);
        break;
    }
    case PlateDecay: {
        const float decay = clampRange(value, 0.1f, 10.0f);
        unit->strev.setrt60(decay);
        unit->nrev.setrt60(decay);
        unit->nrevb.setrt60(decay);
        break;
    }
    case PlateLowCut:
        setPlateInputHPF(unit, clampRange(value, 0.0f, 200.0f));
        break;
    case PlateHighCut:
        setPlateInputLPF(unit, clampRange(value, 1000.0f, 16000.0f));
        break;
    case PlateDamp: {
        const float damp = clampRange(value, 1000.0f, 16000.0f);
        unit->nrev.setDampLpf(damp);
        unit->nrevb.setDampLpf(damp);
        unit->strev.setdamp(damp);
        unit->strev.setoutputdamp(std::max(damp * 2.0f, 16000.0f));
        break;
    }
    case PlateAlgorithm: {
        auto* previous = unit->model;
        const int algorithm = static_cast<int>(clampRange(std::round(value), 0.0f, 2.0f));
        if (algorithm == PlateAlgorithmNRev)
            unit->model = &unit->nrev;
        else if (algorithm == PlateAlgorithmNRevB)
            unit->model = &unit->nrevb;
        else
            unit->model = &unit->strev;

        if (previous != nullptr && unit->model != previous)
            previous->mute();
        break;
    }
    default:
        break;
    }
}

void initializePlateModel(PlateDF* unit)
{
    unit->sampleRate = static_cast<float>(SAMPLERATE);

    unit->inputLpfL.mute();
    unit->inputLpfR.mute();
    unit->inputHpfL.mute();
    unit->inputHpfR.mute();

    unit->nrev.setdryr(0.0f);
    unit->nrev.setwetr(1.0f);
    unit->nrev.setMuteOnChange(false);
    unit->nrev.setSampleRate(unit->sampleRate);

    unit->nrevb.setdryr(0.0f);
    unit->nrevb.setwetr(1.0f);
    unit->nrevb.setMuteOnChange(false);
    unit->nrevb.setSampleRate(unit->sampleRate);

    unit->strev.setdryr(0.0f);
    unit->strev.setwetr(1.0f);
    unit->strev.setMuteOnChange(false);
    unit->strev.setdccutfreq(6.0f);
    unit->strev.setspinlimit(12.0f);
    unit->strev.setspindiff(0.15f);
    unit->strev.setSampleRate(unit->sampleRate);

    unit->model = &unit->nrevb;
    unit->previousParams.fill(std::numeric_limits<float>::quiet_NaN());
    for (int i = PlateDry; i < PlateParamCount; ++i) {
        const float value = IN0(i);
        unit->previousParams[i] = value;
        applyPlateParameter(unit, i, value);
    }

    unit->nrev.mute();
    unit->nrevb.mute();
    unit->strev.mute();
}

float roomBassBoost(RoomDF* unit)
{
    const float boost = clampRange(unit->previousParams[RoomBoost], 0.0f, 100.0f);
    const float decay = std::max(clampRange(unit->previousParams[RoomDecay], 0.1f, 10.0f), 0.1f);
    const float size = clampRange(unit->previousParams[RoomSize], 8.0f, 32.0f);
    return boost / 20.0f / std::pow(decay, 1.5f) * (size / 10.0f);
}

void updateRoomBassBoost(RoomDF* unit)
{
    unit->late.setbassboost(roomBassBoost(unit));
}

void setRoomInputLPF(RoomDF* unit, float frequency)
{
    const float clamped = clampRange(frequency, 0.0f, unit->sampleRate * 0.5f);
    unit->inputLpfL.setLPF_BW(clamped, unit->sampleRate);
    unit->inputLpfR.setLPF_BW(clamped, unit->sampleRate);
}

void setRoomInputHPF(RoomDF* unit, float frequency)
{
    const float clamped = clampRange(frequency, 0.0f, unit->sampleRate * 0.5f);
    unit->inputHpfL.setHPF_BW(clamped, unit->sampleRate);
    unit->inputHpfR.setHPF_BW(clamped, unit->sampleRate);
}

void applyRoomParameter(RoomDF* unit, int index, float value)
{
    switch (index) {
    case RoomDry:
        unit->dryLevel = clampRange(value, 0.0f, 100.0f) * 0.01f;
        break;
    case RoomEarly:
        unit->earlyLevel = clampRange(value, 0.0f, 100.0f) * 0.01f;
        break;
    case RoomEarlySend:
        unit->earlySend = clampRange(value, 0.0f, 100.0f) * 0.01f;
        break;
    case RoomLate:
        unit->lateLevel = clampRange(value, 0.0f, 100.0f) * 0.01f;
        break;
    case RoomSize: {
        const float size = clampRange(value, 8.0f, 32.0f);
        unit->early.setRSFactor(size / 10.0f);
        unit->late.setRSFactor(size / 10.0f);
        updateRoomBassBoost(unit);
        break;
    }
    case RoomWidth:
        unit->early.setwidth(clampRange(value, 50.0f, 150.0f) / 120.0f);
        unit->late.setwidth(clampRange(value, 50.0f, 150.0f) * 0.01f);
        break;
    case RoomPredelay:
        unit->late.setPreDelay(std::max(0.1f, clampRange(value, 0.0f, 100.0f)));
        break;
    case RoomDecay:
        unit->late.setrt60(clampRange(value, 0.1f, 10.0f));
        updateRoomBassBoost(unit);
        break;
    case RoomDiffuse: {
        const float diffusion = clampRange(value, 0.0f, 100.0f) / 120.0f;
        unit->late.setidiffusion1(diffusion);
        unit->late.setodiffusion1(diffusion);
        break;
    }
    case RoomSpin: {
        const float spin = clampRange(value, 0.0f, 5.0f);
        unit->late.setspin(spin);
        unit->late.setspin2(std::sqrt(std::max(100.0f - (10.0f - spin) * (10.0f - spin), 0.0f)) * 0.5f);
        break;
    }
    case RoomWander: {
        const float wander = clampRange(value, 0.0f, 100.0f) / 200.0f + 0.1f;
        unit->late.setwander(wander);
        unit->late.setwander2(wander);
        break;
    }
    case RoomInHighCut:
        setRoomInputLPF(unit, clampRange(value, 1000.0f, 16000.0f));
        break;
    case RoomEarlyDamp:
        unit->early.setoutputlpf(clampRange(value, 1000.0f, 16000.0f));
        break;
    case RoomLateDamp: {
        const float damp = clampRange(value, 1000.0f, 16000.0f);
        unit->late.setdamp(damp);
        unit->late.setoutputdamp(damp);
        break;
    }
    case RoomBoost:
        updateRoomBassBoost(unit);
        break;
    case RoomBoostLPF:
        unit->late.setdamp2(clampRange(value, 50.0f, 1050.0f));
        break;
    case RoomInLowCut:
        setRoomInputHPF(unit, clampRange(value, 0.0f, 200.0f));
        break;
    default:
        break;
    }
}

void initializeRoomModel(RoomDF* unit)
{
    unit->sampleRate = static_cast<float>(SAMPLERATE);

    unit->inputLpfL.mute();
    unit->inputLpfR.mute();
    unit->inputHpfL.mute();
    unit->inputHpfR.mute();

    unit->early.loadPresetReflection(FV3_EARLYREF_PRESET_1);
    unit->early.setMuteOnChange(false);
    unit->early.setdryr(0.0f);
    unit->early.setwet(0.0f);
    unit->early.setwidth(0.8f);
    unit->early.setLRDelay(0.3f);
    unit->early.setLRCrossApFreq(750.0f, 4.0f);
    unit->early.setDiffusionApFreq(150.0f, 4.0f);
    unit->early.setSampleRate(unit->sampleRate);

    unit->late.setMuteOnChange(false);
    unit->late.setwet(0.0f);
    unit->late.setdryr(0.0f);
    unit->late.setwidth(1.0f);
    unit->late.setSampleRate(unit->sampleRate);

    unit->previousParams.fill(std::numeric_limits<float>::quiet_NaN());
    for (int i = RoomDry; i < RoomParamCount; ++i) {
        const float value = IN0(i);
        unit->previousParams[i] = value;
    }
    for (int i = RoomDry; i < RoomParamCount; ++i)
        applyRoomParameter(unit, i, unit->previousParams[i]);

}

void EarlyReflectionsDF_next(EarlyReflectionsDF* unit, int inNumSamples)
{
    const float* inL = IN(EarlyInL);
    const float* inR = IN(EarlyInR);
    float* outL = OUT(0);
    float* outR = OUT(1);

    for (int i = EarlyDry; i < EarlyParamCount; ++i) {
        const float value = inputAt(unit, i, 0);
        if (value != unit->previousParams[i]) {
            unit->previousParams[i] = value;
            applyEarlyParameter(unit, i, value);
        }
    }

    for (int offset = 0; offset < inNumSamples; offset += earlyBufferSize) {
        const int blockSize = std::min(earlyBufferSize, inNumSamples - offset);

        for (int i = 0; i < blockSize; ++i) {
            unit->inputL[i] = inL[offset + i];
            unit->inputR[i] = inR[offset + i];
        }

        unit->model.processreplace(
            unit->inputL.data(),
            unit->inputR.data(),
            unit->outputL.data(),
            unit->outputR.data(),
            blockSize
        );

        for (int i = 0; i < blockSize; ++i) {
            const int sampleIndex = offset + i;
            outL[sampleIndex] = zapgremlins(unit->dryLevel * inL[sampleIndex] + unit->wetLevel * unit->outputL[i]);
            outR[sampleIndex] = zapgremlins(unit->dryLevel * inR[sampleIndex] + unit->wetLevel * unit->outputR[i]);
        }
    }
}

void HallDF_next(HallDF* unit, int inNumSamples)
{
    const float* inL = IN(HallInL);
    const float* inR = IN(HallInR);
    float* outL = OUT(0);
    float* outR = OUT(1);

    for (int i = HallDry; i < HallParamCount; ++i) {
        const float value = inputAt(unit, i, 0);
        if (value != unit->previousParams[i]) {
            unit->previousParams[i] = value;
            applyHallParameter(unit, i, value);
        }
    }

    for (int offset = 0; offset < inNumSamples; offset += hallBufferSize) {
        const int blockSize = std::min(hallBufferSize, inNumSamples - offset);

        unit->early.processreplace(
            const_cast<float*>(inL + offset),
            const_cast<float*>(inR + offset),
            unit->earlyOutL.data(),
            unit->earlyOutR.data(),
            blockSize
        );

        for (int i = 0; i < blockSize; ++i) {
            unit->lateInL[i] = unit->earlySend * unit->earlyOutL[i] + inL[offset + i];
            unit->lateInR[i] = unit->earlySend * unit->earlyOutR[i] + inR[offset + i];
        }

        unit->late.processreplace(
            unit->lateInL.data(),
            unit->lateInR.data(),
            unit->lateOutL.data(),
            unit->lateOutR.data(),
            blockSize
        );

        for (int i = 0; i < blockSize; ++i) {
            const int sampleIndex = offset + i;
            float left = unit->dryLevel * inL[sampleIndex];
            float right = unit->dryLevel * inR[sampleIndex];

            if (unit->earlyLevel > 0.0f) {
                left += unit->earlyLevel * unit->earlyOutL[i];
                right += unit->earlyLevel * unit->earlyOutR[i];
            }

            if (unit->lateLevel > 0.0f) {
                left += unit->lateLevel * unit->lateOutL[i];
                right += unit->lateLevel * unit->lateOutR[i];
            }

            outL[sampleIndex] = zapgremlins(left);
            outR[sampleIndex] = zapgremlins(right);
        }
    }
}

void PlateDF_next(PlateDF* unit, int inNumSamples)
{
    const float* inL = IN(PlateInL);
    const float* inR = IN(PlateInR);
    float* outL = OUT(0);
    float* outR = OUT(1);

    for (int i = PlateDry; i < PlateParamCount; ++i) {
        const float value = inputAt(unit, i, 0);
        if (value != unit->previousParams[i]) {
            unit->previousParams[i] = value;
            applyPlateParameter(unit, i, value);
        }
    }

    for (int offset = 0; offset < inNumSamples; offset += plateBufferSize) {
        const int blockSize = std::min(plateBufferSize, inNumSamples - offset);

        for (int i = 0; i < blockSize; ++i) {
            unit->filteredInputL[i] = unit->inputLpfL.process(unit->inputHpfL.process(inL[offset + i]));
            unit->filteredInputR[i] = unit->inputLpfR.process(unit->inputHpfR.process(inR[offset + i]));
        }

        unit->model->processreplace(
            unit->filteredInputL.data(),
            unit->filteredInputR.data(),
            unit->outputL.data(),
            unit->outputR.data(),
            blockSize
        );

        for (int i = 0; i < blockSize; ++i) {
            const int sampleIndex = offset + i;
            outL[sampleIndex] = zapgremlins(unit->dryLevel * inL[sampleIndex] + unit->wetLevel * unit->outputL[i]);
            outR[sampleIndex] = zapgremlins(unit->dryLevel * inR[sampleIndex] + unit->wetLevel * unit->outputR[i]);
        }
    }
}

void RoomDF_next(RoomDF* unit, int inNumSamples)
{
    const float* inL = IN(RoomInL);
    const float* inR = IN(RoomInR);
    float* outL = OUT(0);
    float* outR = OUT(1);

    for (int i = RoomDry; i < RoomParamCount; ++i) {
        const float value = inputAt(unit, i, 0);
        if (value != unit->previousParams[i]) {
            unit->previousParams[i] = value;
            applyRoomParameter(unit, i, value);
        }
    }

    for (int offset = 0; offset < inNumSamples; offset += roomBufferSize) {
        const int blockSize = std::min(roomBufferSize, inNumSamples - offset);

        for (int i = 0; i < blockSize; ++i) {
            unit->filteredInputL[i] = unit->inputLpfL.process(unit->inputHpfL.process(inL[offset + i]));
            unit->filteredInputR[i] = unit->inputLpfR.process(unit->inputHpfR.process(inR[offset + i]));
        }

        unit->early.processreplace(
            unit->filteredInputL.data(),
            unit->filteredInputR.data(),
            unit->earlyOutL.data(),
            unit->earlyOutR.data(),
            blockSize
        );

        for (int i = 0; i < blockSize; ++i) {
            unit->lateInL[i] = unit->earlySend * unit->earlyOutL[i] + unit->filteredInputL[i];
            unit->lateInR[i] = unit->earlySend * unit->earlyOutR[i] + unit->filteredInputR[i];
        }

        unit->late.processreplace(
            unit->lateInL.data(),
            unit->lateInR.data(),
            unit->lateOutL.data(),
            unit->lateOutR.data(),
            blockSize
        );

        for (int i = 0; i < blockSize; ++i) {
            const int sampleIndex = offset + i;
            float left = unit->dryLevel * inL[sampleIndex];
            float right = unit->dryLevel * inR[sampleIndex];

            if (unit->earlyLevel > 0.0f) {
                left += unit->earlyLevel * unit->earlyOutL[i];
                right += unit->earlyLevel * unit->earlyOutR[i];
            }

            if (unit->lateLevel > 0.0f) {
                left += roomLateGain * unit->lateLevel * unit->lateOutL[i];
                right += roomLateGain * unit->lateLevel * unit->lateOutR[i];
            }

            outL[sampleIndex] = zapgremlins(left);
            outR[sampleIndex] = zapgremlins(right);
        }
    }
}

void EarlyReflectionsDF_Ctor(EarlyReflectionsDF* unit)
{
    try {
        new (unit) EarlyReflectionsDF;
        initializeEarlyModel(unit);
    } catch (...) {
        ClearUnitOnMemFailed
    }

    SETCALC(EarlyReflectionsDF_next);
    EarlyReflectionsDF_next(unit, 1);
}

void EarlyReflectionsDF_Dtor(EarlyReflectionsDF* unit)
{
    unit->~EarlyReflectionsDF();
}

void HallDF_Ctor(HallDF* unit)
{
    try {
        new (unit) HallDF;
        initializeHallModel(unit);
    } catch (...) {
        ClearUnitOnMemFailed
    }

    SETCALC(HallDF_next);
    HallDF_next(unit, 1);
}

void HallDF_Dtor(HallDF* unit)
{
    unit->~HallDF();
}

void PlateDF_Ctor(PlateDF* unit)
{
    try {
        new (unit) PlateDF;
        initializePlateModel(unit);
    } catch (...) {
        ClearUnitOnMemFailed
    }

    SETCALC(PlateDF_next);
    PlateDF_next(unit, 1);
}

void PlateDF_Dtor(PlateDF* unit)
{
    unit->~PlateDF();
}

void RoomDF_Ctor(RoomDF* unit)
{
    try {
        new (unit) RoomDF;
        initializeRoomModel(unit);
    } catch (...) {
        ClearUnitOnMemFailed
    }

    SETCALC(RoomDF_next);
    RoomDF_next(unit, 1);
}

void RoomDF_Dtor(RoomDF* unit)
{
    unit->~RoomDF();
}
} // namespace

PluginLoad(DragonflyReverbDF)
{
    ft = inTable;
    DefineDtorUnit(EarlyReflectionsDF);
    DefineDtorUnit(HallDF);
    DefineDtorUnit(PlateDF);
    DefineDtorUnit(RoomDF);
}
