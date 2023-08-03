/* root-integer of a value
 * Copyright (c) 2005-2023 Edward Kelly
 * Forinformaion on usage and distribution, and for a DICLAIMER OF ALL 
 * WARRANTIES, see the file "LICENSE.txt," in this distribution. */
 
#include "m_pd.h"

t_class *rootint_class;

typedef struct _rootint
{
  t_object x_obj;
  t_int isInt, iDiv;
  t_float lastDiv, thisDiv;
  t_outlet *rootInteger;//, *pot;
} t_rootint;

void rootint_float(t_rootint *y, t_floatarg f)
{
  if(f != 0 && f > 1 && (t_float)((t_int)f) == f)
    {
      y->thisDiv = y->lastDiv = f;
      y->iDiv = (t_int)y->thisDiv;
      while((t_float)y->iDiv == y->thisDiv)
	{
	  y->lastDiv = y->thisDiv;
	  y->thisDiv *= 0.5;
	  y->iDiv = (t_int)y->thisDiv;
	}
      //if(y->lastDiv == 1) y->lastDiv = 2; //musically sensible but mathematically erroneous
      //if(y->lastDiv == 1) outlet_bang(y->pot); //again, if the output is 1 we know it's a power-of-two, so another outlet is irrelevant!
      outlet_float(y->rootInteger, y->lastDiv);
    }
  else post("input must be an integer greater than 1!");
}

void rootint_bang(t_rootint *y)
{
  outlet_float(y->rootInteger, y->lastDiv);
}

void *rootint_new()
{
  t_rootint *y = (t_rootint *)pd_new(rootint_class);
  y->thisDiv = y->lastDiv = 2;
  y->rootInteger = outlet_new(&y->x_obj, gensym("float"));
  //y->pot = outlet_new(&y->x_obj, gensym("bang"));
  return(void *)y;
}

void rootint_setup(void) 
{
  rootint_class = class_new(gensym("rootint"),
  (t_newmethod)rootint_new,
  0, sizeof(t_rootint),
  0, A_DEFFLOAT, 0);
  post("rootint - is it a power of two?");

  class_addbang(rootint_class, rootint_bang);
  class_addfloat(rootint_class, rootint_float);
}
