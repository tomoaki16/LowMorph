#pragma once
#include <algorithm>
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
    std::vector<float> hist;
    size_t w=0;
    int hop=0;

    float env=0.f, fastEnv=0.f, slowEnv=0.f, prevFast=0.f;
    float hz=41.2f, target=41.2f, lp=0.f;
    double phase[8]{};
    float amp[8]{};

    int retriggerSamples=0;
    int silenceSamples=0;
    bool voiceActive=false;

    static float clamp(float x,float a,float b){ return std::max(a,std::min(b,x)); }

    void analyse() {
        const int N=(int)hist.size();
        const int lo=(int)(sr/400.0);
        const int hi=std::min((int)(sr/70.0),N/2);
        int bestLag=lo;
        float best=-1.f;
        for(int lag=lo;lag<=hi;++lag){
            double xy=0,xx=0,yy=0;
            for(int j=0;j<N-lag;++j){
                const float a=hist[(w+j)%N], b=hist[(w+j+lag)%N];
                xy+=a*b; xx+=a*a; yy+=b*b;
            }
            const float c=(float)(xy/(std::sqrt(xx*yy)+1e-12));
            if(c>best){ best=c; bestLag=lag; }
        }
        if(best>.55f)
            target=clamp((float)(sr/bestLag)*.5f,35.f,220.f);
    }

    void trigger(float strength) {
        static constexpr float A[8]={1.f,.58f,.39f,.27f,.19f,.13f,.09f,.06f};
        const float v=clamp(strength, .10f, 1.f);
        // A new pick replaces the previous excitation instead of waiting for the old note to decay.
        for(int k=0;k<8;++k) amp[k]=v*A[k];
        voiceActive=true;
        silenceSamples=0;
        retriggerSamples=(int)(sr*.018); // only reject double-triggering inside ~18 ms
    }

    void choke() {
        // Guitar mute should stop the synthetic string too; do not leave autonomous oscillators ringing.
        for(float &a : amp) a*=0.82f;
        lp*=0.82f;
        bool alive=false;
        for(float a : amp) if(a>1e-5f){ alive=true; break; }
        if(!alive){
            std::fill(amp,amp+8,0.f);
            lp=0.f;
            voiceActive=false;
        }
    }

public:
    void prepare(double s){ sr=s; hist.assign((size_t)(sr*.045),0.f); reset(); }

    void reset(){
        std::fill(hist.begin(),hist.end(),0.f);
        w=0; hop=0;
        env=fastEnv=slowEnv=prevFast=0.f;
        hz=target=41.2f; lp=0.f;
        retriggerSamples=silenceSamples=0;
        voiceActive=false;
        std::fill(phase,phase+8,0.0);
        std::fill(amp,amp+8,0.f);
    }

    void set(const Params&v){ p=v; }
    float detectedHz()const{return hz;}

    void process(const float*in,float*out,int n){
        constexpr double pi=3.14159265358979323846;
        const float envAttack=std::exp(-1.f/(.0015f*(float)sr));
        const float envRelease=std::exp(-1.f/(.028f*(float)sr));
        const float fastCoef=std::exp(-1.f/(.0007f*(float)sr));
        const float slowCoef=std::exp(-1.f/(.018f*(float)sr));
        const int muteHold=(int)(sr*.012); // require ~12 ms of near-silence before choking

        for(int i=0;i<n;++i){
            const float x=in[i];
            hist[w]=x; w=(w+1)%hist.size();
            const float q=std::fabs(x);

            env = q>env ? envAttack*env+(1-envAttack)*q
                        : envRelease*env+(1-envRelease)*q;
            fastEnv = fastCoef*fastEnv+(1-fastCoef)*q;
            slowEnv = slowCoef*slowEnv+(1-slowCoef)*q;

            // Attack detector: respond to the positive transient slope, not the residual note envelope.
            const float slope=fastEnv-prevFast;
            prevFast=fastEnv;
            if(retriggerSamples>0) --retriggerSamples;

            const float dynamicThreshold=std::max(.0025f, slowEnv*.08f);
            const bool transient = slope>dynamicThreshold && fastEnv>std::max(.006f, slowEnv*1.06f);
            if(transient && retriggerSamples==0)
                trigger(clamp(fastEnv*8.f,.10f,1.f));

            if(++hop>=std::max(1,(int)(sr*.006))){
                hop=0;
                if(env>.004f) analyse();
            }
            hz += .006f*(target-hz);

            // Detect a real left-hand/right-hand mute from the DI input.
            if(q<.0012f && env<.0045f) ++silenceSamples;
            else silenceSamples=0;
            if(voiceActive && silenceSamples>muteHold) choke();

            float y=0.f;
            const float pp=clamp(p.pluck,.05f,.48f);
            for(int k=0;k<8;++k){
                const float B=.000045f*(.35f+p.string);
                const float r=(k+1)*std::sqrt(1+B*(k+1)*(k+1));
                phase[k]+=2*pi*hz*r/sr;
                if(phase[k]>2*pi) phase[k]-=2*pi;

                const float tau=(1.20f+2.45f*p.body)/
                    (1+.32f*k*(.5f+p.string)+.28f*p.damping*k);
                amp[k]*=std::exp(-1.f/(tau*(float)sr));

                y+=(float)std::sin(phase[k])*amp[k]*
                    (.55f+.45f*std::fabs((float)std::sin(pi*(k+1)*pp)));
            }

            const float fc=1500.f+4000.f*p.tone*(1-.22f*p.damping);
            const float a=1-(float)std::exp(-2*pi*fc/sr);
            lp+=a*(y-lp);
            y=lp;

            const float drive=1.25f+1.75f*p.body;
            y=std::tanh(y*drive)/std::tanh(drive);

            // Hard floor prevents denormal/residual hiss after the synthetic voice is killed.
            if(!voiceActive || std::fabs(y)<1e-7f) y=0.f;
            out[i]=(1-p.mix)*x+p.mix*.52f*y;
        }
    }
};

}
