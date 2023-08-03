/* wfold~
 * control rate adjusted wavefolder~
 * Copyright (c) 2005-2023 Edward Kelly
 * Forinformaion on usage and distribution, and for a DICLAIMER OF ALL 
 * WARRANTIES, see the file "LICENSE.txt," in this distribution. */

#include "m_pd.h"
#include <math.h>

static t_class *wfold_tilde_class;

typedef struct _wfold_tilde {
  t_object x_obj;
  t_float threshold, reciprocal, remainder, remciprocal, mode, token, bipolar;
//  t_float token, debug, safety;
  t_outlet *folded;
} t_wfold_tilde;

t_int *wfold_tilde_perform(t_int *w) {
  t_wfold_tilde   *x =   (t_wfold_tilde *)(w[1]);
  t_sample      *in =       (t_sample *)(w[2]);
  t_sample     *out =       (t_sample *)(w[3]);
  int             n =              (int)(w[4]);
    t_float insample = 0;
    
    x->threshold = x->threshold > 0 ? x->threshold < 1 ? x->threshold : 0.5 : 0.5;
    x->reciprocal = 1 / x->threshold;
    x->remainder = 1 - x->threshold;
    x->remciprocal = 1 / x->remainder;
    
  while (n--) 
  {
      insample = *in++;
      if(x->bipolar == 0) {
          *out++ = insample < x->threshold ? insample * x->reciprocal : 1 - ((insample - x->threshold) * x->remciprocal);
      } else if(x->bipolar != 0) {
          *out++ = (insample < x->threshold ? insample * x->reciprocal : 1 - ((insample - x->threshold) * x->remciprocal)) * 2 - 1;
      }
  }
    return (w+5);
}

void wfold_tilde_bipolar(t_wfold_tilde *x, t_floatarg f) {
    x->bipolar = f;
}

void wfold_tilde_dsp(t_wfold_tilde *x, t_signal **sp) {
  dsp_add(wfold_tilde_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}
/*
void wfold_tilde_mode(t_wfold_tilde *x, t_floatarg f) {
    x->mode = f;
} */
          
void *wfold_tilde_new(t_floatarg f) {
  t_wfold_tilde *x = (t_wfold_tilde *)pd_new(wfold_tilde_class);

    x->threshold = f > 0 ? f < 1 ? f : 0.5 : 0.5;
    x->reciprocal = 1 / x->threshold;
    x->remainder = 1 - x->threshold;
    x->remciprocal = 1 / x->remainder;

//  inlet_new (&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  floatinlet_new (&x->x_obj, &x->threshold);
  outlet_new(&x->x_obj, &s_signal);
  return (void *)x;
}

void wfold_tilde_setup(void) {
  wfold_tilde_class = class_new(gensym("wfold~"), 
  (t_newmethod)wfold_tilde_new, 
  0, sizeof(t_wfold_tilde),
  CLASS_DEFAULT, A_DEFFLOAT, 0);

  post("~~~~~~~~~~~~~~~>wfold~");
  post("~~~>by Ed Kelly, 2012");

  class_addmethod(wfold_tilde_class,
  (t_method)wfold_tilde_dsp, gensym("dsp"), 0);
  CLASS_MAINSIGNALIN(wfold_tilde_class, t_wfold_tilde, token);
    class_addmethod(wfold_tilde_class, (t_method)wfold_tilde_bipolar, gensym("bipolar"), A_DEFFLOAT, 0);
  //  class_addmethod(wfold_tilde_class, (t_method)wfold_tilde_mode, gensym("mode"), A_DEFFLOAT, 0);
}
