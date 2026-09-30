#include "processor.h"
#include "ids.h"
using namespace Steinberg; using namespace Steinberg::Vst;
LowMorphProcessor::LowMorphProcessor(){setControllerClass(ControllerUID);}
tresult PLUGIN_API LowMorphProcessor::initialize(FUnknown* c){
 auto r=AudioEffect::initialize(c);if(r!=kResultOk)return r;
 addAudioInput(STR16("Guitar In"),SpeakerArr::kMono);addAudioOutput(STR16("Bass Out"),SpeakerArr::kMono);return kResultOk;}
tresult PLUGIN_API LowMorphProcessor::setActive(TBool s){if(s)core.prepare(processSetup.sampleRate);else core.reset();return AudioEffect::setActive(s);}
tresult PLUGIN_API LowMorphProcessor::process(ProcessData& d){
 if(d.inputParameterChanges){int32 n=d.inputParameterChanges->getParameterCount();for(int32 i=0;i<n;++i){auto*q=d.inputParameterChanges->getParameterData(i);if(!q||q->getPointCount()==0)continue;int32 off=0;ParamValue v=0;if(q->getPoint(q->getPointCount()-1,off,v)!=kResultTrue)continue;float f=(float)v;
 switch(q->getParameterId()){case kBody:params.body=f;break;case kAttack:params.attack=f;break;case kString:params.string=f;break;case kTone:params.tone=f;break;case kMix:params.mix=f;break;case kPluck:params.pluck=.05f+.43f*f;break;case kPickup:params.pickup=.08f+.38f*f;break;case kDamping:params.damping=f;break;}}core.set(params);}
 if(d.numInputs<1||d.numOutputs<1)return kResultOk;if(d.symbolicSampleSize!=kSample32)return kResultFalse;
 core.process(d.inputs[0].channelBuffers32[0],d.outputs[0].channelBuffers32[0],d.numSamples);return kResultOk;}
