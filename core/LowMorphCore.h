#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace lowmorph {

struct Params {
    float body=.68f, attack=.52f, string=.50f, tone=.55f, mix=1.f;
    float pluck=.22f, pickup=.28f, damping=.45f;
};

class Core {
    double sr=48000.0;
    Params p{};

    // Low-latency monophonic period tracker. No block autocorrelation, FFT, or look-ahead.
    float det1=0.f, det2=0.f, detEnv=0.f;
    bool schmittHigh=false;
    int samplesSinceRise=0;
    std::array<int,3> periods{{0,0,0}};
    int periodCount=0, periodPos=0;
    float inputHz=82.4f, targetHz=41.2f, modelHz=41.2f;
    bool pitchValid=false;

    // Continuous performance controls.
    float inputEnv=0.f, fastEnv=0.f, slowEnv=0.f, prevFast=0.f;
    float articulation=0.f;   // smooth excitation amount
    float contact=1.f;        // smooth finger/mute loss
    int excitationLeft=0;

    // One-delay-loop waveguide / extended Karplus-Strong string.
    std::vector<float> delay;
    int writePos=0;
    float lossState=0.f, pickupState=0.f, dcX=0.f, dcY=0.f;
    uint32_t rng=0x12345678u;

    static float clamp(float x,float a,float b){ return std::max(a,std::min(b,x)); }
    static float median3(float a,float b,float c){
        return std::max(std::min(a,b),std::min(std::max(a,b),c));
    }
    float noise(){
        rng ^= rng<<13; rng ^= rng>>17; rng ^= rng<<5;
        return ((rng & 0x00ffffffu)/8388608.f)-1.f;
    }

    void updatePitch(float x){
        // Two cascaded one-pole LPFs suppress upper guitar harmonics before edge timing.
        const float fc=330.f;
        const float a=1.f-(float)std::exp(-2.0*3.14159265358979323846*fc/sr);
        det1 += a*(x-det1);
        det2 += a*(det1-det2);

        const float absd=std::fabs(det2);
        const float ea=1.f-(float)std::exp(-1.0/(.006*sr));
        const float er=1.f-(float)std::exp(-1.0/(.050*sr));
        detEnv += (absd>detEnv?ea:er)*(absd-detEnv);
        ++samplesSinceRise;

        // Adaptive Schmitt trigger: hysteresis follows the filtered signal level.
        const float th=std::max(.0008f,detEnv*.16f);
        if(!schmittHigh && det2>th){
            schmittHigh=true;
            const int period=samplesSinceRise;
            samplesSinceRise=0;
            const int minP=(int)(sr/430.0);
            const int maxP=(int)(sr/65.0);
            if(period>=minP && period<=maxP){
                periods[periodPos]=period;
                periodPos=(periodPos+1)%3;
                periodCount=std::min(periodCount+1,3);
                if(periodCount>=2){
                    float chosen=(float)periods[(periodPos+2)%3];
                    if(periodCount==3)
                        chosen=median3((float)periods[0],(float)periods[1],(float)periods[2]);
                    const float candidate=(float)(sr/chosen);
                    // Reject implausible single-edge octave jumps; accept sustained changes quickly.
                    if(!pitchValid || (candidate>inputHz*.62f && candidate<inputHz*1.62f)){
                        inputHz=candidate;
                        targetHz=clamp(inputHz*.5f,35.f,220.f);
                        pitchValid=true;
                    } else {
                        inputHz += .18f*(candidate-inputHz);
                        targetHz=clamp(inputHz*.5f,35.f,220.f);
                    }
                }
            }
        } else if(schmittHigh && det2 < -th) {
            schmittHigh=false;
        }

        if(samplesSinceRise>(int)(sr*.060)){
            pitchValid=false;
            periodCount=0;
        }
    }

    float readDelay(float d) const {
        const int size=(int)delay.size();
        float rp=(float)writePos-d;
        while(rp<0) rp+=size;
        const int i0=(int)rp;
        const int i1=(i0+1)%size;
        const float f=rp-i0;
        return delay[i0]+f*(delay[i1]-delay[i0]);
    }

public:
    void prepare(double s){
        sr=s;
        delay.assign((size_t)(sr/30.0)+8,0.f);
        reset();
    }

    void reset(){
        std::fill(delay.begin(),delay.end(),0.f);
        writePos=0; det1=det2=detEnv=0.f; schmittHigh=false; samplesSinceRise=0;
        periods={{0,0,0}}; periodCount=periodPos=0;
        inputHz=82.4f; targetHz=modelHz=41.2f; pitchValid=false;
        inputEnv=fastEnv=slowEnv=prevFast=articulation=0.f; contact=1.f;
        excitationLeft=0; lossState=pickupState=dcX=dcY=0.f; rng=0x12345678u;
    }

    void set(const Params&v){ p=v; }
    float detectedHz()const{return modelHz;}

    void process(const float*in,float*out,int n){
        const float envA=1.f-(float)std::exp(-1.0/(.0015*sr));
        const float envR=1.f-(float)std::exp(-1.0/(.035*sr));
        const float fastA=1.f-(float)std::exp(-1.0/(.00055*sr));
        const float slowA=1.f-(float)std::exp(-1.0/(.018*sr));
        const float contactCoef=1.f-(float)std::exp(-1.0/(.018*sr));
        const float artRelease=1.f-(float)std::exp(-1.0/(.030*sr));

        for(int i=0;i<n;++i){
            const float x=in[i];
            const float ax=std::fabs(x);
            updatePitch(x);

            inputEnv += (ax>inputEnv?envA:envR)*(ax-inputEnv);
            fastEnv += fastA*(ax-fastEnv);
            slowEnv += slowA*(ax-slowEnv);
            const float slope=fastEnv-prevFast;
            prevFast=fastEnv;

            // Onsets only inject energy. They never hard-reset or hard-stop the resonator.
            const float onsetThreshold=std::max(.00045f,slowEnv*.018f);
            if(slope>onsetThreshold && fastEnv>std::max(.0025f,slowEnv*1.025f)){
                articulation=std::max(articulation,clamp(fastEnv*9.f,.08f,1.f));
                excitationLeft=std::max(excitationLeft,(int)(sr*(.0025+.0045*p.attack)));
            }
            articulation += artRelease*(0.f-articulation);

            // Finger mute is a continuous loss control, not a gate.
            const float energyNorm=clamp(inputEnv/.020f,0.f,1.f);
            const float desiredContact=energyNorm;
            contact += contactCoef*(desiredContact-contact);

            // Smooth pitch motion; repeated notes start immediately at the previous valid pitch.
            const float pitchCoef=1.f-(float)std::exp(-1.0/(.0045*sr));
            if(pitchValid) modelHz += pitchCoef*(targetHz-modelHz);

            const float delaySamples=clamp((float)(sr/modelHz)-.5f,8.f,(float)delay.size()-3.f);
            float loop=readDelay(delaySamples);

            // Frequency-dependent loss: treble dies faster, while mute increases broadband loss smoothly.
            const float lossFc=900.f+4200.f*p.tone;
            const float la=1.f-(float)std::exp(-2.0*3.14159265358979323846*lossFc/sr);
            lossState += la*(loop-lossState);

            const float openFeedback=.9978f-.0022f*p.damping;
            const float muteLoss=.955f+.044f*contact;
            const float feedback=clamp(openFeedback*muteLoss,.90f,.9994f);

            float excite=0.f;
            if(excitationLeft>0){
                --excitationLeft;
                // Band-limited-ish excitation: DI transient supplies playing character,
                // noise supplies string broadband energy without copying guitar sustain.
                const float transient=x-det2;
                excite=(.55f*transient+.018f*noise())*articulation*(.55f+.75f*p.attack);
            }

            const float next=feedback*lossState+excite;
            delay[writePos]=clamp(next,-1.4f,1.4f);
            writePos=(writePos+1)%delay.size();

            // Pickup/body stage. No discontinuous zeroing anywhere in the audio path.
            const float bodyFc=650.f+2600.f*p.body;
            const float ba=1.f-(float)std::exp(-2.0*3.14159265358979323846*bodyFc/sr);
            pickupState += ba*(loop-pickupState);
            float y=pickupState;

            // DC blocker for stable silence and no residual step/click.
            const float dc=y-dcX+.995f*dcY;
            dcX=y; dcY=dc;
            y=std::tanh(dc*(1.05f+1.15f*p.body));

            out[i]=(1.f-p.mix)*x+p.mix*.72f*y;
        }
    }
};

}
