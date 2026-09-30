#pragma once
#include "public.sdk/source/vst/vsteditcontroller.h"
class LowMorphController: public Steinberg::Vst::EditController {
public:
 static Steinberg::FUnknown* createInstance(void*){return (Steinberg::Vst::IEditController*)new LowMorphController;}
 Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown*) override;
};
