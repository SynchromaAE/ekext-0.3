/* scalefilter - maps incoming numbers onto the closest member of a 12TET scale
 * Copyright (c) 2005-2023 Edward Kelly
 * Forinformaion on usage and distribution, and for a DICLAIMER OF ALL 
 * WARRANTIES, see the file "LICENSE.txt," in this distribution. */

#include "m_pd.h"
#include <string.h>
#define MAXSIZE 256 
static t_class *scalefilter_class;

typedef struct _scalefilter
{
  t_object x_obj;

  t_atom chroma[MAXSIZE], instance[MAXSIZE];
  int bufsize, isscale, elements, foundelements;
  t_float maxinstances;
  t_outlet *note, *scale, *fill, *nomatch, *full;
} t_scalefilter;

static void scalefilter_set(t_scalefilter *x, t_symbol *s, int size, t_atom *thisscale)
{
  int i;
  t_float arg;
  if(size >= 1) {
    for(i=0;i<size;i++) {
      arg = atom_getfloat(thisscale+i);
      SETFLOAT(&x->chroma[i],arg);
      SETFLOAT(&x->instance[i],0);
    }
    x->isscale = 1;
    x->bufsize = x->elements = size;
    x->foundelements = 0;
    outlet_float(x->fill,(t_float)x->elements);
  }
}

static void scalefilter_zero(t_scalefilter *x, t_floatarg size)
{
  int i;
  int newsize = 12;
  if(size >= 1) {
    newsize = (int)size;
  }
  x->bufsize = newsize;
  for(i=0;i<newsize;i++) {
    SETFLOAT(&x->chroma[i],-9999);
    SETFLOAT(&x->instance[i],0);
  }
  x->isscale = 0;
  x->elements = 0;
  x->foundelements = 0;
}

static void scalefilter_reset(t_scalefilter *x) 
{
  int i;
  for(i=0;i<x->bufsize;i++) {
    SETFLOAT(&x->instance[i],0);
  }
  if(x->isscale == 0) {
    x->elements = 0;
  }
  if(x->isscale == 1) {
    x->elements = x->bufsize;
  }
  x->foundelements = 0;
}

static void scalefilter_float(t_scalefilter *x, t_floatarg fin)
{
  int max = x->maxinstances >= 1 ? (int)x->maxinstances : 1;
  int i, instances;
  int found = 0;
  t_float element;
  if(x->isscale == 1) { // if there is already a scale, just compare
    for(i=0;i<x->bufsize;i++) {
      element = atom_getfloatarg(i, MAXSIZE, x->chroma);
      if(element == fin) {
	found = 1;
	instances = (int)atom_getfloatarg(i, MAXSIZE, x->instance);
	if(instances < max) {
	  SETFLOAT(&x->instance[i],instances + 1);
	  outlet_float(x->note,fin);
	  x->foundelements++;
	}
      }
    }
  }
  if(x->isscale == 0) { // if there is no scale, add new notes to the end
    for(i=0;i<x->elements;i++) {
      element = atom_getfloatarg(i, MAXSIZE, x->chroma);
      if(element == fin) {
	found = 1;
	instances = (int)atom_getfloatarg(i, MAXSIZE, x->instance);
	if(instances < max) {
	  SETFLOAT(&x->instance[i],instances + 1);
	  outlet_float(x->note, fin);
	  x->foundelements++;
	}
      } 
    }
    if(x->bufsize > x->elements && found == 0) {
      SETFLOAT(&x->chroma[x->elements],fin);
      SETFLOAT(&x->instance[i],1);
      found = 1;
      x->elements++;
      outlet_float(x->note, fin);
      outlet_float(x->fill, (t_float)x->elements);
      x->foundelements++;
    }
    if(x->elements == x->bufsize) {
      x->isscale = 1;
    }
  }
  if(x->foundelements >= x->bufsize * (int)x->maxinstances) {
    outlet_bang(x->full);
  }
  if(found == 0) {
    outlet_float(x->nomatch, fin);
  }
}

static void scalefilter_get(t_scalefilter *x)
{
  if(x->isscale == 1) {
    outlet_list(x->scale, gensym("list"), x->bufsize, x->chroma);
  }
  if(x->isscale == 0) {
    outlet_list(x->scale, gensym("list"), x->elements, x->chroma);
  }
}

static void *scalefilter_new(t_floatarg length, t_floatarg maximum)
{
  t_float len = 12;
  t_float max = 2;
  t_scalefilter *x = (t_scalefilter *)pd_new(scalefilter_class);
  if(length >= 1) {
    len = length;
  }
  max = maximum < MAXSIZE ? maximum >= 1 ? maximum : 2 : MAXSIZE;
  x->bufsize = len;
  x->maxinstances = max;
  x->foundelements = 0;
  int i;
  for(i=0;i<MAXSIZE;i++) {
    SETFLOAT(&x->chroma[i],-1);
    SETFLOAT(&x->instance[i],0);
  }
  x->isscale = 0;
  x->note = outlet_new(&x->x_obj, gensym("float"));
  x->scale = outlet_new(&x->x_obj, gensym("list"));
  x->nomatch = outlet_new(&x->x_obj, gensym("float"));
  x->fill = outlet_new(&x->x_obj, gensym("float"));
  x->full = outlet_new(&x->x_obj, gensym("bang"));
  floatinlet_new(&x->x_obj, &x->maxinstances);
  return (void *)x;
}

void scalefilter_setup(void) 
{
  scalefilter_class = class_new(gensym("scalefilter"),
  (t_newmethod)scalefilter_new,
  0, sizeof(t_scalefilter),
  0, A_DEFFLOAT, A_DEFFLOAT, 0);
  post("_-^scalefilter^-_ by Ed Kelly, 2013");

  class_addfloat(scalefilter_class, scalefilter_float);
  class_addmethod(scalefilter_class, (t_method)scalefilter_set, gensym("set"), A_GIMME, 0);
  class_addmethod(scalefilter_class, (t_method)scalefilter_zero, gensym("zero"), A_DEFFLOAT, 0);
  class_addmethod(scalefilter_class, (t_method)scalefilter_reset, gensym("reset"), A_DEFFLOAT, 0);
  class_addmethod(scalefilter_class, (t_method)scalefilter_get, gensym("get"), A_DEFFLOAT, 0);
}
