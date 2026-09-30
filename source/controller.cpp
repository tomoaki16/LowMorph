#include "controller.h"
#include "ids.h"
using namespace Steinberg;using namespace Steinberg::Vst;
tresult PLUGIN_API LowMorphController::initialize(FUnknown*c){auto r=EditController::initialize(c);if(r!=kResultOk)return r;
 parameters.addParameter(STR16("Body"),nullptr,0,.68,ParameterInfo::kCanAutomate,kBody);
 parameters.addParameter(STR16("Attack"),nullptr,0,.52,ParameterInfo::kCanAutomate,kAttack);
 parameters.addParameter(STR16("String"),nullptr,0,.50,ParameterInfo::kCanAutomate,kString);
 parameters.addParameter(STR16("Tone"),nullptr,0,.55,ParameterInfo::kCanAutomate,kTone);
 parameters.addParameter(STR16("Mix"),nullptr,0,1.,ParameterInfo::kCanAutomate,kMix);
 parameters.addParameter(STR16("Pluck Position"),nullptr,0,.40,ParameterInfo::kCanAutomate,kPluck);
 parameters.addParameter(STR16("Pickup Position"),nullptr,0,.53,ParameterInfo::kCanAutomate,kPickup);
 parameters.addParameter(STR16("Damping"),nullptr,0,.45,ParameterInfo::kCanAutomate,kDamping);return kResultOk;}
