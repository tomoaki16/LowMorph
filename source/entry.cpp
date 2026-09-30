#include "public.sdk/source/main/pluginfactory.h"
#include "processor.h"
#include "controller.h"
#include "ids.h"
#define stringPluginName "LowMorph"
BEGIN_FACTORY_DEF("tomoaki16","https://github.com/tomoaki16/LowMorph","")
DEF_CLASS2(INLINE_UID_FROM_FUID(ProcessorUID),PClassInfo::kManyInstances,kVstAudioEffectClass,stringPluginName,Vst::kDistributable,"Fx",FULL_VERSION_STR,kVstVersionString,LowMorphProcessor::createInstance)
DEF_CLASS2(INLINE_UID_FROM_FUID(ControllerUID),PClassInfo::kManyInstances,kVstComponentControllerClass,stringPluginName " Controller",0,"",FULL_VERSION_STR,kVstVersionString,LowMorphController::createInstance)
END_FACTORY
