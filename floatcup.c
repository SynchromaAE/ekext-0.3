/* floatcup counts up in floating-point steps ^_-
 * Copyright (c) 2005-2023 Edward Kelly
 * Forinformaion on usage and distribution, and for a DICLAIMER OF ALL 
 * WARRANTIES, see the file "LICENSE.txt," in this distribution.
 * you can set a negative step for it to count down. */
 
#include "m_pd.h"

t_class *floatcup_class;

typedef struct _floatcup
{
  t_object x_obj;
  t_float f_count, fa, step;
  t_outlet *count;
} t_floatcup;

void floatcup_float(t_floatcup *y, t_floatarg f)
{
  y->f_count = f;
}

void floatcup_bang(t_floatcup *y)
{
  outlet_float(y->count, y->f_count);
  y->f_count += y->step;
}

void floatcup_setbang(t_floatcup *y, t_floatarg f)
{
  y->f_count = f;
  outlet_float(y->count, y->f_count);
  y->f_count += 1;
}

void floatcup_step(t_floatcup *y, t_floatarg f)
{
  y->step = f;
}

void *floatcup_new(t_floatarg f)
{
  t_floatcup *y = (t_floatcup *)pd_new(floatcup_class);
  y->step = 1;
  y->fa = f;
  y->f_count = 0;
  y->count = outlet_new(&y->x_obj, gensym("float"));
  return(void *)y;
}

void floatcup_setup(void) 
{
  floatcup_class = class_new(gensym("floatcup"),
  (t_newmethod)floatcup_new,
  0, sizeof(t_floatcup),
  0, A_DEFFLOAT, 0);
  post("floatcup counts up ^_^");

  class_addbang(floatcup_class, floatcup_bang);
  class_addfloat(floatcup_class, floatcup_float);
  class_addmethod(floatcup_class, (t_method)floatcup_step, gensym("step"), A_DEFFLOAT, 0);
  class_addmethod(floatcup_class, (t_method)floatcup_setbang, gensym("setbang"), A_DEFFLOAT, 0);}
