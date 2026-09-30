#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace lowmorph {

struct Params {
    float body=.68f, attack=.52f, string=.50f, tone=.55f, mix=1.f;
    float pluck=.22f, pickup=.28f, damping=.45f;
};

class Core {
    double sr=48000.0;
    Params p{};

    // Continuous input analysis. No look-ahead/windowed autocorrelation.
    float inLP1=0.f, inLP2=0.f;
    float env=0.f, fast=0.f, slow=0.f, prevFast=0.f;
    bool edgeHigh=false;
    int edgeSamples=0;
    std::array<int,3> periods{{0,0,0}};
    int periodPos=0, periodCount=0;
    float guitarHz=82.4f, bassHz=41.2f, targetHz=41.2f;
    bool havePitch=false;

    // Previous-version additive bass voice retained as the sound generator.
    double phase[8]{};
    float amp[8]{};
    float masterGain=0.f;
    float lp=0.f;
    float dcX=0.f, dcY=0.f;

    static float clamp(float x,float a,float b){ return std::max(a,std::min(b,x)); }
    static float median3(float a,float b,float c){
        if(a>b) std::swap(a,b); if(b>c) std::swap(b,c); if(a>b) std::swap(a,b); return b;
    }

    void trackPitch(float x){
        constexpr double pi=3.14159265358979323846;
        const float fc=420.f;
        const float a=1.f-(float)std::exp(-2.0*pi*fc/sr);
        inLP1 += a*(x-inLP1);
        inLP2 += a*(inLP1-inLP2);
        ++edgeSamples;

        const float threshold=std::max(.0012f, env*.10f);
        if(!edgeHigh && inLP2>threshold){
            edgeHigh=true;
            const int period=edgeSamples;
            edgeSamples=0;
            const int minP=(int)(sr/430.0);
            const int maxP=(int)(sr/65.0);
            if(period>=minP && period<=maxP){
                periods[periodPos]=period;
                periodPos=(periodPos+1)%3;
                periodCount=std::min(3,periodCount+1);
                if(periodCount>=2){
                    float per=(float)periods[(periodPos+2)%3];
                    if(periodCount==3) per=median3((float)periods[0],(float)periods[1],(float)periods[2]);
                    const float candidate=(float)(sr/per);
                    if(!havePitch || (candidate>guitarHz*.72f && candidate<guitarHz*1.38f)){
                        guitarHz=candidate;
                        targetHz=clamp(candidate*.5f,35.f,220.f);
                        havePitch=true;
                    }
                }
            }
        } else if(edgeHigh && inLP2 < -threshold){
            edgeHigh=false;
        }
        if(edgeSamples>(int)(sr*.075)) {
            edgeSamples=(int)(sr*.075);
            periodCount=0;
        }
    }

public:
    void prepare(double s){ sr=s; reset(); }

    void reset(){
        inLP1=inLP2=env=fast=slow=prevFast=0.f;
        edgeHigh=false; edgeSamples=0; periods={{0,0,0}}; periodPos=periodCount=0;
        guitarHz=82.4f; bassHz=targetHz=41.2f; havePitch=false;
        std::fill(phase,phase+8,0.0); std::fill(amp,amp+8,0.f);
        masterGain=lp=dcX=dcY=0.f;
    }

    void set(const Params&v){ p=v; }
    float detectedHz()const{return bassHz;}

    void process(const float* in,float* out,int n){
        static constexpr float H[8]={1.f,.58f,.39f,.27f,.19f,.13f,.09f,.06f};
        constexpr double pi=3.14159265358979323846;

        const float envAttack=1.f-(float)std::exp(-1.0/(.0015*sr));
        const float envRelease=1.f-(float)std::exp(-1.0/(.055*sr));
        const float fastCoef=1.f-(float)std::exp(-1.0/(.00065*sr));
        const float slowCoef=1.f-(float)std::exp(-1.0/(.025*sr));
        const float gainAttack=1.f-(float)std::exp(-1.0/(.0025*sr));
        const float gainRelease=1.f-(float)std::exp(-1.0/(.045*sr));
        const float pitchCoef=1.f-(float)std::exp(-1.0/(.006*sr));

        for(int i=0;i<n;++i){
            const float x=in[i], q=std::fabs(x);
            env += (q>env?envAttack:envRelease)*(q-env);
            fast += fastCoef*(q-fast);
            slow += slowCoef*(q-slow);
            const float slope=fast-prevFast;
            prevFast=fast;

            trackPitch(x);
            if(havePitch) bassHz += pitchCoef*(targetHz-bassHz);

            // A pick adds/rebalances harmonic energy, but never hard-resets the output.
            const float onsetThreshold=std::max(.00035f,slow*.020f);
            if(slope>onsetThreshold && fast>std::max(.0025f,slow*1.02f)){
                const float velocity=clamp(fast*7.5f,.08f,1.f);
                for(int k=0;k<8;++k){
                    const float desired=velocity*H[k];
                    amp[k]=std::max(amp[k],desired);
                }
            }

            // Continuous amplitude follows the player's actual sustain/mute.
            // No silence threshold, no voiceActive flag, no hard choke.
            const float desiredGain=clamp(env/.018f,0.f,1.f);
            masterGain += (desiredGain>masterGain?gainAttack:gainRelease)*(desiredGain-masterGain);

            float y=0.f;
            const float pp=clamp(p.pluck,.05f,.48f);
            for(int k=0;k<8;++k){
                const float B=.000045f*(.35f+p.string);
                const float harmonic=(k+1)*std::sqrt(1.f+B*(k+1)*(k+1));
                phase[k]+=2*pi*bassHz*harmonic/sr;
                while(phase[k]>2*pi) phase[k]-=2*pi;

                const float tau=(1.20f+2.45f*p.body)/
                    (1.f+.32f*k*(.5f+p.string)+.28f*p.damping*k);
                amp[k]*=std::exp(-1.f/(tau*(float)sr));

                const float pluckWeight=.55f+.45f*std::fabs((float)std::sin(pi*(k+1)*pp));
                y+=(float)std::sin(phase[k])*amp[k]*pluckWeight;
            }

            // Smoothly couple the synthesized sustain to the DI envelope.
            y*=masterGain;

            const float fc=1500.f+4000.f*p.tone*(1.f-.22f*p.damping);
            const float fa=1.f-(float)std::exp(-2*pi*fc/sr);
            lp+=fa*(y-lp);
            y=lp;

            const float drive=1.25f+1.75f*p.body;
            y=std::tanh(y*drive)/std::tanh(drive);

            // DC blocker instead of hard-zeroing tiny samples.
            const float dc=y-dcX+.995f*dcY;
            dcX=y; dcY=dc;

            out[i]=(1.f-p.mix)*x+p.mix*.52f*dc;
        }
    }
};

}
