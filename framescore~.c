/*
 * framescore~ : Weighted block comparison.
 * Copyright (c) 2005-2023 Edward Kelly
 * Forinformaion on usage and distribution, and for a DICLAIMER OF ALL 
 * WARRANTIES, see the file "LICENSE.txt," in this distribution. */
 
#include "m_pd.h"

static t_class *framescore_tilde_class;

typedef struct _framescore_tilde 
{
  t_object x_obj;
  t_float f;
  t_float f_max, f_win, f_accum;
  t_outlet *f_score;
} t_framescore_tilde;

t_int *framescore_tilde_perform(t_int *w)
{
  t_framescore_tilde *x = (t_framescore_tilde *)(w[1]);
  t_sample         *in1 =           (t_sample *)(w[2]);
  t_sample         *in2 =           (t_sample *)(w[3]);
  int                 n =                  (int)(w[4]);
  t_float vector1;
  t_float vector2;
  x->f_accum = 0;
  t_float block_accum = 0;
  x->f_max = 0;
  t_float score = 0;
  t_float avg = 0;
  int block = n;
  x->f_win = x->f_win > 0 ? x->f_win : 0.01;

  while (n--)
    {
      vector1 = (*in1++);
      vector2 = (*in2++);
      vector1 = vector1 > 0 ? vector1 : 0 - vector1;
      vector2 = vector2 > 0 ? vector2 : 0 - vector2;
      block_accum += vector2;
      x->f_max = vector2 > x->f_max ? vector2 : x->f_max;
      t_float diff = vector1 > vector2 ? vector1 - vector2 : vector2 - vector1;
      x->f_accum += (1/((diff/x->f_win)+1)) * vector2;
    }
  score = x->f_accum / x->f_max;
  block_accum /= x->f_max;
  avg = score / block_accum;
  outlet_float(x->f_score, avg);
  
  return(w+5);
}

void framescore_tilde_dsp(t_framescore_tilde *x, t_signal **sp)
{
  dsp_add(framescore_tilde_perform, 4, x,
	  sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

void *framescore_tilde_new(t_floatarg f)
{
  t_framescore_tilde *x = (t_framescore_tilde *)pd_new(framescore_tilde_class);

  x->f_win = f;

  inlet_new (&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  floatinlet_new (&x->x_obj, &x->f_win);
  x->f_score = outlet_new(&x->x_obj, gensym("float"));

  return (void *)x;
}


void framescore_tilde_setup(void)
{
  framescore_tilde_class = class_new(gensym("framescore~"),
				     (t_newmethod)framescore_tilde_new,
				     0, sizeof(t_framescore_tilde),
				     CLASS_DEFAULT, A_DEFFLOAT, 0);

  post("|+++++++++++framescore~+++++++++++++|");
  post("|+++++weighted block comparison+++++|");
  post("|+++edward+++++++kelly+++++++2005+++|");

  class_addmethod(framescore_tilde_class, (t_method)framescore_tilde_dsp,
		  gensym("dsp"), 0);

  CLASS_MAINSIGNALIN(framescore_tilde_class, t_framescore_tilde, f);
}
