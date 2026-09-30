#pragma once
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "core/LowMorphCore.h"
class LowMorphProcessor: public Steinberg::Vst::AudioEffect {
public:
 LowMorphProcessor();
 static Steinberg::FUnknown* createInstance(void*){return (Steinberg::Vst::IAudioProcessor*)new LowMorphProcessor;}
 Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown*) override;
 Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool) override;
 Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData&) override;
 Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream*) override{return Steinberg::kResultOk;}
 Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream*) override{return Steinberg::kResultOk;}
private: lowmorph::Core core; lowmorph::Params params;
};
