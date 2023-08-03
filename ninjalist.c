/* takes a map like 0 1 3 4 7 and only returns the number if it is present */
/* based on sieve by Edward Kelly
 * Copyright (c) 2005-2023 Edward Kelly
 * Forinformaion on usage and distribution, and for a DICLAIMER OF ALL 
 * WARRANTIES, see the file "LICENSE.txt," in this distribution. */

#include "m_pd.h"
#define MAXENTRIES 1024
#define LASTENTRY 1023

static t_class *ninjalist_class;

typedef struct _map
{
  t_atom map[MAXENTRIES];
  t_atom nomap[MAXENTRIES];
} t_map;

typedef struct _ninjalist
{
  t_object x_obj;
  t_map x_map;
  t_float input, max, outmap;
  t_outlet *mapped, *value, *mapout, *inst;
} t_ninjalist;

void ninjalist_float(t_ninjalist *x, t_floatarg fin)
{
  int arg = 0;
  t_float test = 0;
  x->input = arg = fin;
  test = atom_getfloatarg(arg, MAXENTRIES, x->x_map.map);
  outlet_bang(x->inst);
  outlet_float(x->value, test);
  outlet_float(x->mapped, arg);
}

void ninjalist_set(t_ninjalist *x, t_floatarg fmap, t_floatarg fval) /* set one value 
                                                                in the array */
{
  t_float fvaller;
  if(fmap < MAXENTRIES && fmap >= 0)
    {
      int imap = (int)fmap;
      fvaller = fval != 0 ? 0 : 1;
      SETFLOAT(&x->x_map.map[imap], fval);
      SETFLOAT(&x->x_map.nomap[imap], fvaller);
      x->max = fmap > x->max ? fmap : x->max;
    }
}

void ninjalist_delete(t_ninjalist *x, t_floatarg loc) /* remove a value */
{
  int addloc = (int)loc + 1;
  int maxentry = (int)x->max;
  int i;
  t_float buffer;
  if(loc<x->max && loc>=0)
    {
      for(i=addloc;i<=maxentry;i++)
	{
	  buffer = atom_getfloatarg(i,MAXENTRIES,x->x_map.map);
	  SETFLOAT(&x->x_map.map[i-1],buffer);
	  if(buffer!=0)
	    {
	      SETFLOAT(&x->x_map.nomap[i-1],0);
	    }
	  else
	    {
	      SETFLOAT(&x->x_map.nomap[i-1],1);
	    }
	}
      SETFLOAT(&x->x_map.map[maxentry],0);
      x->max--;
    }
  else if(loc==x->max)
    {
      x->max--;
      SETFLOAT(&x->x_map.map[maxentry],0);
    }
}

void ninjalist_insert(t_ninjalist *x, t_floatarg loc, t_floatarg val)
/* insert a value at specific location, moving subsequent values up */
{
  int location = (int)loc;
  int maxentry = (int)x->max+1;
  int i;
  t_float buffer;
  if(loc>=0 && maxentry < MAXENTRIES)
    {
      for(i=maxentry;i>=location;i--)
	{
	  buffer = atom_getfloatarg(i-1,MAXENTRIES,x->x_map.map);
	  SETFLOAT(&x->x_map.map[i],buffer);
	  if(buffer!=0)
	    {
	      SETFLOAT(&x->x_map.nomap[i],0);
	    }
	  else
	    {
       	      SETFLOAT(&x->x_map.nomap[i],1);
	    }
	}
      x->max++;
      SETFLOAT(&x->x_map.map[location], val);
      if(val) 
	{
	  SETFLOAT(&x->x_map.nomap[location],0);
	}
      else
	{
	  SETFLOAT(&x->x_map.nomap[location],1);
	}
    }
}

void ninjalist_get(t_ninjalist *x, t_floatarg inv) /* outlet to map or inverse */
{
  if(inv!=0) 
    {
      outlet_list(x->mapout, gensym("list"), x->max+1, x->x_map.nomap);
    }
  else outlet_list(x->mapout, gensym("list"), x->max+1, x->x_map.map);
  x->outmap = inv;
}

void ninjalist_clear(t_ninjalist *x)
{
  int i;
  for(i=0;i<MAXENTRIES;i++) 
    {
      SETFLOAT(&x->x_map.map[i], 0);
      SETFLOAT(&x->x_map.nomap[i], 1);
    }
  x->max = 0;
}

void ninjalist_map(t_ninjalist *x, t_symbol *s, int argc, t_atom *argv) /* set the whole map */
{
  int i;
  for(i=0;i<MAXENTRIES;i++) 
    {
      SETFLOAT(x->x_map.map+i, 0);
      SETFLOAT(x->x_map.nomap+i, 1);
    }
  x->max = 0;
  t_float arg;
  for(i=0;i<argc;i++) 
    {
      arg = atom_getfloat(argv+i);
      if(arg != 0)
	{
	  SETFLOAT(&x->x_map.map[i], arg);
	  SETFLOAT(&x->x_map.nomap[i], 0);
	  x->max = i;
	}
    }
  if (x->max > 0 && x->outmap == 0)
    {
      outlet_list(x->mapout, gensym("list"), x->max+1, x->x_map.map);
    }
  else if (x->max > 0 && x->outmap == 1)
    {
      outlet_list(x->mapout, gensym("list"), x->max+1, x->x_map.nomap);
    }
}

void *ninjalist_new(t_floatarg f) 
{
  t_ninjalist *x = (t_ninjalist *)pd_new(ninjalist_class);
  x->max = 0;
  x->outmap = 0;
  int i;
  for(i=0;i<MAXENTRIES;i++) 
    {
      SETFLOAT(x->x_map.map+i, 0);
      SETFLOAT(x->x_map.nomap+i, 1);
    }
  x->mapped = outlet_new(&x->x_obj, &s_float);
  x->value = outlet_new(&x->x_obj, &s_float);
  x->mapout = outlet_new(&x->x_obj, &s_list);
  x->inst = outlet_new(&x->x_obj, &s_bang);
  return (void *)x;
}

void ninjalist_setup(void) 
{
  ninjalist_class = class_new(gensym("ninjalist"),
  (t_newmethod)ninjalist_new,
  0, sizeof(t_ninjalist),
  0, A_DEFFLOAT, 0);
  post("|^^^^^^^^^^^ninjalist^^^^^^^^^^^|");
  post("|->^^^integer map to floats^^^<-|");
  post("|^^^^^^^Edward Kelly 2012^^^^^^^|");

  class_addfloat(ninjalist_class, ninjalist_float);
  class_addmethod(ninjalist_class, (t_method)ninjalist_set, gensym("set"), A_DEFFLOAT, A_DEFFLOAT, 0);
  class_addmethod(ninjalist_class, (t_method)ninjalist_map, gensym("map"), A_GIMME, 0);
  class_addmethod(ninjalist_class, (t_method)ninjalist_clear, gensym("clear"), A_DEFFLOAT, 0);
  class_addmethod(ninjalist_class, (t_method)ninjalist_get, gensym("get"), A_DEFFLOAT, 0);
  class_addmethod(ninjalist_class, (t_method)ninjalist_delete, gensym("delete"), A_DEFFLOAT, 0);
  class_addmethod(ninjalist_class, (t_method)ninjalist_insert, gensym("insert"), A_DEFFLOAT, A_DEFFLOAT, 0);
}
