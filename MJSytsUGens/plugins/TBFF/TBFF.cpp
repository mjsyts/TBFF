// PluginTBFF.hpp
// M. Josiah Sytsma (sytsma.music@icloud.com)

#include "TBFF.h"
#include "SC_PlugIn.h"

#include "SC_PlugIn.hpp"

static InterfaceTable* ft;

TBFF::TBFF() {
    reset();
    
    m_Freq = in0(FREQ_IN);
    m_Res = in0(RQ_IN);
    
    set_calc_function<TBFF, &TBFF::next>();
}

void TBFF::reset() {
    z0 = z1 = z2 = z3 = 0.f;
}

void TBFF::next(int inNumSamples) {
    float wc, wc2, wc3, wc4;
    float b, g, s;
    float y0, y1, y2, y3, y4;
    
    //input
    const float* xn = in(SIG_IN);
    SlopeSignal<float> slopedFreq = makeSlope(in0(FREQ_IN), m_Freq);
    SlopeSignal<float> slopedRes = makeSlope(in0(RQ_IN), m_Res);
    
    //output
    float* outBuf = out(0);
    
    for (int i = 0; i < inNumSamples; i++) {
        const float freq = slopedFreq.consume();
        const float res = slopedRes.consume();
        const float currentSample = xn[i];
        
        float k = (1.01 - sc_clip(res, 0.01, 1.0)) * 20.f;
        float a = 1.f + 0.5 * k;
        
        wc = M_PI * freq * sampleDur();
        wc = 2 * tan(0.5 * wc);
        wc2 = wc * wc;
        wc3 = wc2 * wc;
        wc4 = wc3 * wc;
        
        b = 1 / zapgremlins(1 + 8*wc + 20*wc2 + 16*wc3 + 2*wc4);
        g = 2 * wc4 * b;
        
        //calculate filter state
        s = (z0*wc3 + z1*(wc2 + 2*wc3) + z2*(wc + 4*wc2 + 2*wc3) + z3*(1 + 6*wc + 9*wc2 + 2*wc3)) * b;
        
        //solve feedback
        y4 = (g * currentSample + s) / (1 + g * k);
        
        //saturate output
        y0 = tanh(currentSample - k * y4);
        
        y1 = (y0 * (2*wc + 12*wc2 + 20*wc + 8*wc4) + z0*(1 + 6*wc + 10*wc2 + 4*wc3) + z1*(2*wc + 8*wc2 + 6*wc3) + z2*(2*wc2 + 4*wc3) + z3 * 2 * wc3) * b;
        y2 = (y0*(2*wc2 + 8*wc3 + 6*wc4) + z0*(wc + 4*wc + 3*wc3) + z1*(1 + 6*wc + 11*wc + 6*wc3) + z2*(wc + 4*wc2 + 4*wc3) + z3*(wc2 + 2*wc3)) * b;
        y3 = (y0*(2*wc3 + 4*wc4) + z0*(wc2 + 2*wc3) + z1*(wc + 4*wc2 + 4*wc3) + z2*(1 + 6*wc + 10*wc2 + 4*wc3) + z3*(wc + 4*wc2 + 2*wc3)) * b;
        y4 = g * y0 + s;
        
        //update filter states
        z0 += 4 * wc * (y0 - y1 + y2);
        z1 += 2 * wc * (y1 - 2 * y2 + y3);
        z2 += 2 * wc * (y2 - 2 * y3 + y4);
        z3 += 2 * wc * (y3 - 2 * y4);
        
        //output
        outBuf[i] = y4 * a;
    }
    
    m_Freq = slopedFreq.value;
    m_Res = slopedRes.value;
}

PluginLoad(TBFFUGens) {
    ft = inTable;
    registerUnit<TBFF>(ft, "TBFF", false);
}
