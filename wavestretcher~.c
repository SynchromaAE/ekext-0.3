/*
 * wavestretcher~ - above and below a threshold, the waveform is stretched or squashed
 * Copyright (c) 2005-2023 Edward Kelly
 * Forinformaion on usage and distribution, and for a DICLAIMER OF ALL 
 * WARRANTIES, see the file "LICENSE.txt," in this distribution. */

#include "m_pd.h"
#include <math.h>
#define _limit 0.99999


static t_class *wavestretcher_tilde_class;

typedef struct _wavestretcher_tilde {
  t_object x_obj;
  t_float token, bipolar;
//  t_float token, debug, safety;
  t_outlet *stretched;
} t_wavestretcher_tilde;

static inline t_float saturate( t_float input ) { //clamp without branching
    t_float x1 = fabsf( input + _limit );
    t_float x2 = fabsf( input - _limit );
    return 0.5 * (x1 - x2);
}

t_int *wavestretcher_tilde_perform(t_int *w) {
  t_wavestretcher_tilde   *x =   (t_wavestretcher_tilde *)(w[1]);
    t_sample      *in =       (t_sample *)(w[2]);
    t_sample      *thresh =       (t_sample *)(w[3]);
    t_sample      *stretch =       (t_sample *)(w[4]);
    t_sample     *out =       (t_sample *)(w[5]);
  int             n =              (int)(w[6]);
    t_float insample = 0;
    t_float outsample = 0;
    t_float thsample = 0;
    t_float stsample = 0;
    t_float stnegative = 0;
    t_float stosample = 0;
    t_float stusample = 0;
    t_float reciprocal = 0;
    t_float remainder = 0;
    t_float remciprocal = 0;
        
  while (n--) 
  {
      insample = *in++;
      thsample = saturate(*thresh++);
      thsample = thsample * 0.5 + 0.5;
      stsample = *stretch++;
      stnegative = 0 - stsample;
      reciprocal = 1 / thsample;
      remainder = 1 - thsample;
      remciprocal = 1 / remainder;
      if(stsample > 0) {
          stusample = stsample * reciprocal + (1 - stsample);
          stosample = 1 - stsample;
          outsample = *out++ = insample < thsample && insample > -thsample ? insample * stusample : insample >= thsample ? insample * stosample + stsample : insample * stosample - stsample;
      } else if (stsample < 0) {
          stusample = 1 - stnegative;
          stosample = stnegative * remciprocal + (1 - stnegative);
          outsample = *out++ = insample < thsample && insample > -thsample ? insample * stusample : insample >= thsample ? (insample - thsample) * stosample + thsample * (1 - stnegative) : (insample + thsample) * stosample - thsample * (1 - stnegative);
      } else if (stsample == 0) {
          outsample = *out++ = insample;
      }
  }
    return (w+7);
}

void wavestretcher_tilde_bipolar(t_wavestretcher_tilde *x, t_floatarg f) {
    x->bipolar = f;
}
    
void wavestretcher_tilde_dsp(t_wavestretcher_tilde *x, t_signal **sp) {
  dsp_add(wavestretcher_tilde_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
}
          
void *wavestretcher_tilde_new(t_floatarg f) {
  t_wavestretcher_tilde *x = (t_wavestretcher_tilde *)pd_new(wavestretcher_tilde_class);

    inlet_new (&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    inlet_new (&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    outlet_new(&x->x_obj, &s_signal);
  return (void *)x;
}

void wavestretcher_tilde_setup(void) {
  wavestretcher_tilde_class = class_new(gensym("wavestretcher~"), 
  (t_newmethod)wavestretcher_tilde_new, 
  0, sizeof(t_wavestretcher_tilde),
  CLASS_DEFAULT, A_DEFFLOAT, 0);

  post("~~~~~~~~~~~~~~~>wavestretcher~");
  post("~~~>by Ed Kelly, 2012");

  class_addmethod(wavestretcher_tilde_class,
  (t_method)wavestretcher_tilde_dsp, gensym("dsp"), 0);
  CLASS_MAINSIGNALIN(wavestretcher_tilde_class, t_wavestretcher_tilde, token);
  class_addmethod(wavestretcher_tilde_class, (t_method)wavestretcher_tilde_bipolar, gensym("bipolar"), A_DEFFLOAT, 0);
  //  class_addmethod(wavestretcher_tilde_class, (t_method)wavestretcher_tilde_mode, gensym("mode"), A_DEFFLOAT, 0);
}
