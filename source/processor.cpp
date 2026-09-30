#include "processor.h"
#include "ids.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

LowMorphProcessor::LowMorphProcessor() { setControllerClass(ControllerUID); }

tresult PLUGIN_API LowMorphProcessor::initialize(FUnknown* context) {
    auto result = AudioEffect::initialize(context);
    if (result != kResultOk) return result;
    addAudioInput(STR16("Guitar In"), SpeakerArr::kMono);
    addAudioOutput(STR16("Bass Out"), SpeakerArr::kMono);
    return kResultOk;
}

tresult PLUGIN_API LowMorphProcessor::setActive(TBool state) {
    if (state) core.prepare(processSetup.sampleRate); else core.reset();
    return AudioEffect::setActive(state);
}

tresult PLUGIN_API LowMorphProcessor::process(ProcessData& data) {
    if (data.inputParameterChanges) {
        const int32 count = data.inputParameterChanges->getParameterCount();
        for (int32 i = 0; i < count; ++i) {
            IParamValueQueue* queue = data.inputParameterChanges->getParameterData(i);
            if (!queue || queue->getPointCount() == 0) continue;
            int32 sampleOffset = 0;
            ParamValue value = 0.0;
            if (queue->getPoint(queue->getPointCount() - 1, sampleOffset, value) != kResultTrue) continue;
            const float v = static_cast<float>(value);
            switch (queue->getParameterId()) {
                case kBody: params.body = v; break;
                case kAttack: params.attack = v; break;
                case kString: params.string = v; break;
                case kTone: params.tone = v; break;
                case kMix: params.mix = v; break;
                case kPluck: params.pluck = 0.05f + 0.43f * v; break;
                case kPickup: params.pickup = 0.08f + 0.38f * v; break;
                case kDamping: params.damping = v; break;
                default: break;
            }
        }
        core.set(params);
    }
    if (data.numInputs < 1 || data.numOutputs < 1 || data.numSamples <= 0) return kResultOk;
    if (data.symbolicSampleSize != kSample32) return kResultFalse;
    if (!data.inputs[0].channelBuffers32 || !data.outputs[0].channelBuffers32) return kResultFalse;
    float* input = data.inputs[0].channelBuffers32[0];
    float* output = data.outputs[0].channelBuffers32[0];
    if (!input || !output) return kResultFalse;
    core.process(input, output, data.numSamples);
    return kResultOk;
}
