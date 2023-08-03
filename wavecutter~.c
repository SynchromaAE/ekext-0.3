/*
 * wavecutter~ - peak threshold waveshaper and odd/even transformation
 * Copyright (c) 2005-2023 Edward Kelly
 * Forinformaion on usage and distribution, and for a DICLAIMER OF ALL 
 * WARRANTIES, see the file "LICENSE.txt," in this distribution. */

#include "m_pd.h"
#include <math.h>
#define _limit 0.99999


static t_class *wavecutter_tilde_class;

typedef struct _wavecutter_tilde {
  t_object x_obj;
  t_float token;
  t_outlet *folded;
} t_wavecutter_tilde;

static inline t_float saturate( t_float input ) { //clamp without branching
    t_float x1 = fabsf( input + _limit );
    t_float x2 = fabsf( input - _limit );
    return 0.5 * (x1 - x2);
}

t_int *wavecutter_tilde_perform(t_int *w) {
  t_wavecutter_tilde   *x =   (t_wavecutter_tilde *)(w[1]);
    t_sample      *in =       (t_sample *)(w[2]);
    t_sample      *thresh =       (t_sample *)(w[3]);
    t_sample      *negate =       (t_sample *)(w[4]);
    t_sample     *out =       (t_sample *)(w[5]);
  int             n =              (int)(w[6]);
    t_float insample = 0;
    t_float outsample = 0;
    t_float thsample = 0;
    t_float nthsample = 0;
    t_float nsample = 0;
        
  while (n--) 
  {
      insample = *in++;
      thsample = saturate(*thresh++);
      nthsample = 0 - thsample;
      nsample = *negate++ * -1;
      outsample = *out++ = (insample > thsample ? insample : insample < nthsample ? insample * nsample : 0);
  }
    return (w+7);
}
    
void wavecutter_tilde_dsp(t_wavecutter_tilde *x, t_signal **sp) {
  dsp_add(wavecutter_tilde_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
}
          
void *wavecutter_tilde_new(t_floatarg f) {
  t_wavecutter_tilde *x = (t_wavecutter_tilde *)pd_new(wavecutter_tilde_class);

    inlet_new (&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    inlet_new (&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
//  floatinlet_new (&x->x_obj, &x->threshold);
    outlet_new(&x->x_obj, &s_signal);
  return (void *)x;
}

void wavecutter_tilde_setup(void) {
  wavecutter_tilde_class = class_new(gensym("wavecutter~"), 
  (t_newmethod)wavecutter_tilde_new, 
  0, sizeof(t_wavecutter_tilde),
  CLASS_DEFAULT, A_DEFFLOAT, 0);

  post("~~~~~~~~~~~~~~~>wavecutter~");
  post("~~~>by Ed Kelly, 2012");

  class_addmethod(wavecutter_tilde_class,
  (t_method)wavecutter_tilde_dsp, gensym("dsp"), 0);
  CLASS_MAINSIGNALIN(wavecutter_tilde_class, t_wavecutter_tilde, token);
}
