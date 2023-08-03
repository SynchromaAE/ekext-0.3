/*
 * wavefolder~ - modulation of triangle->sawtooth wave and PWM generation from modulated
 * tri-saw wave.
 * Copyright (c) 2005-2023 Edward Kelly
 * Forinformaion on usage and distribution, and for a DICLAIMER OF ALL 
 * WARRANTIES, see the file "LICENSE.txt," in this distribution. */

#include "m_pd.h"
#include <math.h>
#define _limit 0.99999


static t_class *wavefolder_tilde_class;

typedef struct _wavefolder_tilde {
  t_object x_obj;
  t_float token, bipolar;
//  t_float token, debug, safety;
  t_outlet *folded, *pulse;
} t_wavefolder_tilde;

static inline t_float saturate( t_float input ) { //clamp without branching
    t_float x1 = fabsf( input + _limit );
    t_float x2 = fabsf( input - _limit );
    return 0.5 * (x1 - x2);
}

t_int *wavefolder_tilde_perform(t_int *w) {
  t_wavefolder_tilde   *x =   (t_wavefolder_tilde *)(w[1]);
    t_sample      *in =       (t_sample *)(w[2]);
    t_sample      *thresh =       (t_sample *)(w[3]);
    t_sample      *pthresh =       (t_sample *)(w[4]);
    t_sample     *out =       (t_sample *)(w[5]);
    t_sample     *pout =       (t_sample *)(w[6]);
  int             n =              (int)(w[7]);
    t_float insample = 0;
    t_float outsample = 0;
    t_float thsample = 0;
    t_float pthsample = 0;
    t_float reciprocal = 0;
    t_float remainder = 0;
    t_float remciprocal = 0;
        
  while (n--) 
  {
      insample = *in++;
      thsample = saturate(*thresh++);
      thsample = thsample * 0.5 + 0.5;
      pthsample = *pthresh++;
      reciprocal = 1 / thsample;
      remainder = 1 - thsample;
      remciprocal = 1 / remainder;
      if(x->bipolar != 0) {
      outsample = *out++ = (insample < thsample ? insample * reciprocal : 1 - ((insample - thsample) * remciprocal)) * 2 - 1;
      }
      else if(x->bipolar == 0) {
          outsample = *out++ = insample < thsample ? insample * reciprocal : 1 - ((insample - thsample) * remciprocal);
      }
      if(x->bipolar == 0) {
          pthsample = pthsample * 0.5 + 0.5;
          *pout++ = outsample > pthsample ? 1 : 0;
      }
      else if(x->bipolar != 0) {
          *pout++ = outsample > pthsample ? 1 : -1;
      }
  }
    return (w+8);
}

void wavefolder_tilde_bipolar(t_wavefolder_tilde *x, t_floatarg f) {
    x->bipolar = f;
}
    
void wavefolder_tilde_dsp(t_wavefolder_tilde *x, t_signal **sp) {
  dsp_add(wavefolder_tilde_perform, 7, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec, sp[0]->s_n);
}
          
void *wavefolder_tilde_new(t_floatarg f) {
  t_wavefolder_tilde *x = (t_wavefolder_tilde *)pd_new(wavefolder_tilde_class);

    inlet_new (&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    inlet_new (&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
//  floatinlet_new (&x->x_obj, &x->threshold);
    outlet_new(&x->x_obj, &s_signal);
    outlet_new(&x->x_obj, &s_signal);
  return (void *)x;
}

void wavefolder_tilde_setup(void) {
  wavefolder_tilde_class = class_new(gensym("wavefolder~"), 
  (t_newmethod)wavefolder_tilde_new, 
  0, sizeof(t_wavefolder_tilde),
  CLASS_DEFAULT, A_DEFFLOAT, 0);

  post("~~~~~~~~~~~~~~~>wavefolder~");
  post("~~~>by Ed Kelly, 2012");

  class_addmethod(wavefolder_tilde_class,
  (t_method)wavefolder_tilde_dsp, gensym("dsp"), 0);
  CLASS_MAINSIGNALIN(wavefolder_tilde_class, t_wavefolder_tilde, token);
  class_addmethod(wavefolder_tilde_class, (t_method)wavefolder_tilde_bipolar, gensym("bipolar"), A_DEFFLOAT, 0);
  //  class_addmethod(wavefolder_tilde_class, (t_method)wavefolder_tilde_mode, gensym("mode"), A_DEFFLOAT, 0);
}
