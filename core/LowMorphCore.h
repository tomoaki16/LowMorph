#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
namespace lowmorph {
struct Params { float body=.68f, attack=.52f, string=.50f, tone=.55f, mix=1.f, pluck=.22f, pickup=.28f, damping=.45f; };
class Core {
 double sr=48000.0; Params p{}; std::vector<float> hist; size_t w=0; int hop=0;
 float env=0, slow=0, hz=41.2f, target=41.2f, lp=0; double phase[8]{}; float amp[8]{};
 static float clamp(float x,float a,float b){return std::max(a,std::min(b,x));}
 void analyse(){
  int N=(int)hist.size(), lo=(int)(sr/400.0), hi=std::min((int)(sr/70.0),N/2), bestLag=lo; float best=-1.f;
  for(int lag=lo;lag<=hi;++lag){double xy=0,xx=0,yy=0;
   for(int j=0;j<N-lag;++j){float a=hist[(w+j)%N],b=hist[(w+j+lag)%N];xy+=a*b;xx+=a*a;yy+=b*b;}
   float c=(float)(xy/(std::sqrt(xx*yy)+1e-12)); if(c>best){best=c;bestLag=lag;}}
  if(best>.55f) target=clamp((float)(sr/bestLag)*.5f,35.f,220.f);
 }
public:
 void prepare(double s){sr=s;hist.assign((size_t)(sr*.060),0.f);reset();}
 void reset(){std::fill(hist.begin(),hist.end(),0.f);w=0;hop=0;env=slow=lp=0;hz=target=41.2f;std::fill(phase,phase+8,0.0);std::fill(amp,amp+8,0.f);}
 void set(const Params&v){p=v;} float detectedHz()const{return hz;}
 void process(const float*in,float*out,int n){
  static constexpr float A[8]={1.f,.58f,.39f,.27f,.19f,.13f,.09f,.06f};
  constexpr double pi=3.14159265358979323846;
  for(int i=0;i<n;++i){float x=in[i];hist[w]=x;w=(w+1)%hist.size();float q=std::fabs(x);
   float ca=std::exp(-1.f/(.003f*(float)sr)),cr=std::exp(-1.f/(.060f*(float)sr));
   env=q>env?ca*env+(1-ca)*q:cr*env+(1-cr)*q; slow=.998f*slow+.002f*env;
   bool onset=env>std::max(.008f,slow*1.7f); if(++hop>=int(sr*.010)){hop=0;analyse();} hz+=.0035f*(target-hz);
   if(onset){float v=clamp(env*7.f,.15f,1.f);for(int k=0;k<8;++k)amp[k]=std::max(amp[k],v*A[k]);}
   float y=0,pp=clamp(p.pluck,.05f,.48f);
   for(int k=0;k<8;++k){float B=.000045f*(.35f+p.string),r=(k+1)*std::sqrt(1+B*(k+1)*(k+1));
    phase[k]+=2*pi*hz*r/sr;if(phase[k]>2*pi)phase[k]-=2*pi;
    float tau=(1.55f+2.8f*p.body)/(1+.32f*k*(.5f+p.string)+.22f*p.damping*k);
    amp[k]*=std::exp(-1.f/(tau*(float)sr));
    y+=(float)std::sin(phase[k])*amp[k]*(.55f+.45f*std::fabs((float)std::sin(pi*(k+1)*pp)));}
   float fc=1500.f+4000.f*p.tone*(1-.22f*p.damping),a=1-(float)std::exp(-2*pi*fc/sr);lp+=a*(y-lp);y=lp;
   float drive=1.25f+1.75f*p.body;y=std::tanh(y*drive)/std::tanh(drive);
   out[i]=(1-p.mix)*x+p.mix*.52f*y;
  }
 }
};}
