/*
 * framespect~ : Weighted alpha comparison, block-by-block.
 * Copyright (c) 2005-2023 Edward Kelly
 * Forinformaion on usage and distribution, and for a DICLAIMER OF ALL 
 * WARRANTIES, see the file "LICENSE.txt," in this distribution. */

#include "m_pd.h"
#include <math.h>

static t_class *framespect_tilde_class;

typedef struct _framespect_tilde 
{
  t_object x_obj;
  t_float f;
  t_float f_max, f_win, f_accum;
  t_outlet *f_score;
} t_framespect_tilde;

t_int *framespect_tilde_perform(t_int *w)
{
  t_framespect_tilde *x = (t_framespect_tilde *)(w[1]);  
  t_sample       *real1 =           (t_sample *)(w[2]);
  t_sample       *imag1 =           (t_sample *)(w[3]);
  t_sample       *real2 =           (t_sample *)(w[4]);
  t_sample       *imag2 =           (t_sample *)(w[5]);
  int                 n =                  (int)(w[6]);
  t_float r1, i1, r2, i2;
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
      r1 = (*real1++);
      i1 = (*imag1++);
      r2 = (*real2++);
      i2 = (*imag2++);
      vector1 = sqrt((r1 * r1) + (i1 * i1));
      vector2 = sqrt((r2 * r2) + (i2 * i2));
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
  
  return(w+7);
}

void framespect_tilde_dsp(t_framespect_tilde *x, t_signal **sp)
{
  dsp_add(framespect_tilde_perform, 6, x,
	  sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
}

void *framespect_tilde_new(t_floatarg f)
{
  t_framespect_tilde *x = (t_framespect_tilde *)pd_new(framespect_tilde_class);

  x->f_win = f;

  inlet_new (&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  inlet_new (&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  inlet_new (&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  floatinlet_new (&x->x_obj, &x->f_win);
  x->f_score = outlet_new(&x->x_obj, gensym("float"));

  return (void *)x;
}


void framespect_tilde_setup(void)
{
  framespect_tilde_class = class_new(gensym("framespect~"),
				     (t_newmethod)framespect_tilde_new,
				     0, sizeof(t_framespect_tilde),
				     CLASS_DEFAULT, A_DEFFLOAT, 0);

  post("|'''''''''''framespect~'''''''''''''|");
  post("|'''''weighted alpha comparison'''''|");
  post("|'''edward'''''''kelly'''''''2005'''|");

  class_addmethod(framespect_tilde_class, (t_method)framespect_tilde_dsp,
		  gensym("dsp"), 0);

  CLASS_MAINSIGNALIN(framespect_tilde_class, t_framespect_tilde, f);
}
