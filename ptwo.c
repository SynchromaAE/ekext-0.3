/* power of two?(1) or not?(0)
 * Copyright (c) 2005-2023 Edward Kelly
 * Forinformaion on usage and distribution, and for a DICLAIMER OF ALL 
 * WARRANTIES, see the file "LICENSE.txt," in this distribution.
 */
 
#include "m_pd.h"
#include <math.h>

t_class *ptwo_class;

typedef struct _ptwo
{
  t_object x_obj;
  t_int isTwoYes;
  unsigned long inVal;
  t_outlet *isTwo;
} t_ptwo;

void ptwo_float(t_ptwo *y, t_floatarg f)
{
  y->inVal = (unsigned long)f;
  if(y->inVal != 0 && (y->inVal & (y->inVal - 1)) == 0) y->isTwoYes = 1;
  else y->isTwoYes = 0;
  outlet_float(y->isTwo, (t_float)y->isTwoYes);
}

void ptwo_bang(t_ptwo *y)
{
  outlet_float(y->isTwo, (t_float)y->isTwoYes);
}

void *ptwo_new(t_floatarg f)
{
  t_ptwo *y = (t_ptwo *)pd_new(ptwo_class);
  y->inVal = (unsigned long)f;
  y->isTwo = outlet_new(&y->x_obj, gensym("float"));
  return(void *)y;
}

void ptwo_setup(void) 
{
  ptwo_class = class_new(gensym("ptwo"),
  (t_newmethod)ptwo_new,
  0, sizeof(t_ptwo),
  0, A_DEFFLOAT, 0);
  post("ptwo - is it a power of two?");

  class_addbang(ptwo_class, ptwo_bang);
  class_addfloat(ptwo_class, ptwo_float);
}
