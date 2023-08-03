/* only allow standard lengths for rhythmic notation
 * part of the gemnotes system
 * Copyright (c) 2005-2023 Edward Kelly
 * Forinformaion on usage and distribution, and for a DICLAIMER OF ALL 
 * WARRANTIES, see the file "LICENSE.txt," in this distribution.
 */
 
#include "m_pd.h"

static t_int regroup_splits[264] =
  /* 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 */
  {0,1,2,3,4,3,3,4,4,3,3, 4, 4, 4, 4, 3, 4, 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 6, 4, 4, 4, 4, 4,
   0,0,0,0,0,2,3,3,4,3,3, 4, 4, 4, 4, 3, 4, 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 3, 4, 4, 4, 4, 4,
   0,0,0,0,0,0,0,0,0,3,4, 3, 4, 3, 4, 3, 4, 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 6, 4, 4, 4, 4, 4,
   0,0,0,0,0,0,0,0,0,0,0, 0, 0, 2, 2, 3, 4, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 3, 4, 4, 4, 4, 4,
   0,0,0,0,0,0,0,0,0,0,0, 0, 0, 0, 0, 3, 0, 2, 3, 3, 4, 3, 3, 4, 4, 4, 4, 6, 4, 4, 4, 4, 4,
   0,0,0,0,0,0,0,0,0,0,0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 2, 3, 3, 4, 3, 3, 3, 4, 4, 4, 4, 4,
   0,0,0,0,0,0,0,0,0,0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 0, 4, 3, 4, 4, 4,
   0,0,0,0,0,0,0,0,0,0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 3, 4};

t_class *regroup_class;

typedef struct _regroup
{
  t_object x_obj;
  t_int input, thisVal, nextVal, tiedon;
  t_outlet *out, *tie;
} t_regroup;

void regroup_float(t_regroup *y, t_floatarg f)
{
  int i;
  if(f < 1 || f > 32) post("regroup: input an integer from 1 to 32");
  else if(f == 0) outlet_float(y->out, 0);
  else
    {
      y->input = (t_int)f;
      for(i = 0; i < 8; i++)
	{
	  y->thisVal = regroup_splits[y->input + i*33];
	  if(i < 7)
	    {
	      y->nextVal = regroup_splits[y->input + (i+1)*33];
	      if(y->nextVal > 0)
		{
		  y->tiedon = 1;
		  outlet_float(y->tie, y->tiedon);
		  outlet_float(y->out, (t_float)y->thisVal);
		}
	      else if(y->nextVal == 0 && y->thisVal > 0)
		{
		  y->tiedon = 0;
		  outlet_float(y->tie, y->tiedon);
		  outlet_float(y->out, (t_float)y->thisVal);
		}
	    }
	  else if(y->thisVal > 0)
	    {
	      y->tiedon = 0;
	      outlet_float(y->tie, y->tiedon);
	      outlet_float(y->out, (t_float)y->thisVal);
	    }
	}
    }
}

void regroup_bang(t_regroup *y)
{
  int i;
  for(i = 0; i > 8; i++)
    {
      y->thisVal = regroup_splits[y->input + i*33];
      if(i < 7)
	{
	  y->nextVal = regroup_splits[y->input + (i+1)*33];
	  if(y->nextVal > 0)
	    {
	      y->tiedon = 1;
	      outlet_float(y->tie, y->tiedon);
	      outlet_float(y->out, (t_float)y->thisVal);
	    }
	  else if(y->nextVal == 0 && y->thisVal > 0)
	    {
	      y->tiedon = 0;
	      outlet_float(y->tie, y->tiedon);
	      outlet_float(y->out, (t_float)y->thisVal);
	    }
	}
      else if(y->thisVal > 0)
	{
	  y->tiedon = 0;
	  outlet_float(y->tie, y->tiedon);
	  outlet_float(y->out, (t_float)y->thisVal);
	}
    }
}

void *regroup_new()
{
  t_regroup *y = (t_regroup *)pd_new(regroup_class);
  y->input = 4; //dummy default value
  y->out = outlet_new(&y->x_obj, gensym("float"));
  y->tie = outlet_new(&y->x_obj, gensym("float"));
  return(void *)y;
}

void regroup_setup(void) 
{
  regroup_class = class_new(gensym("regroup"),
  (t_newmethod)regroup_new,
  0, sizeof(t_regroup),
  0, A_DEFFLOAT, 0);
  post("regroup split notes into 2s and 3s - part of gemnotes 0.3");

  class_addbang(regroup_class, regroup_bang);
  class_addfloat(regroup_class, regroup_float);
}
