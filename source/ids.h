#pragma once
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"
using namespace Steinberg;
static const FUID ProcessorUID(0x4C4F574D,0x4F525048,0x42415353,0x00000003);
static const FUID ControllerUID(0x4C4F574D,0x4F525048,0x4354524C,0x00000003);
enum ParamIDs : Vst::ParamID { kBody=100,kAttack,kString,kTone,kMix,kPluck,kPickup,kDamping };
