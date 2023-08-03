/* stavelines
 * part of the gemnotes system
 * Copyright (c) 2005-2023 Edward Kelly
 * Forinformaion on usage and distribution, and for a DICLAIMER OF ALL 
 * WARRANTIES, see the file "LICENSE.txt," in this distribution. */
 
#include "m_pd.h"

#define STAVELINES 32

t_class *stavelines_class;

typedef struct _stavelines
{
  t_object x_obj;
  t_int perclines[STAVELINES]; // number of lines in each stave
  t_int staves[STAVELINES]; // staves used
  t_atom breaks[STAVELINES]; // break before stave (after?)
  t_int staveLinesNum, breakLinesNum, thisStaveNum, thisBreakNum;
  t_int stavenum;
  t_float staveVal, breakVal, fullStaveVal, fullBreakVal;
  t_float stepMult, groupMult;
  t_outlet *linesOff, *breaksOff, *linesNum;
  // if staveMode > 0 accumulate staves and breaks
} t_stavelines;

void stavelines_float(t_stavelines *y, t_floatarg f)
{

}

void stavelines_bang(t_stavelines *y)
{

}

void stavelines_setlines(t_stavelines *z, t_symbol *s, int argc, t_atom *argv)
{
  int i, lines;
  if(argc > 0)
    {
      for(i = 0; i < argc; i++)
	{
	  lines = (int)atom_getfloat(argv+i);
	  z->perclines[i] = lines;
	}
      z->staveLinesNum = argc;
    }
  else post("No elements to setlines list!");
}

void stavelines_setbreaks(t_stavelines *z, t_symbol *s, int argc, t_atom *argv)
{
  //break styles: 0.0 = no break (e.g. first stave), 0.5 = simple break after stave, 1.0 = break + space below this stave, 1.5 = break + space above and below this stave
  //methinks perhaps this doesn't quite work this way!
  int i; t_float breaks;
  if(argc > 0)
    {
      breaks = (int)atom_getfloat(argv+i);
      SETFLOAT(&z->breaks[i],breaks);
    }
}

void stavelines_staves(t_stavelines *z, t_symbol *s, int argc, t_atom *argv)
{ // 1 = stave is being used, 0 means it is not
  int i, staveUsed;
  if(argc > 0)
    {
      for(i=0;i<argc;i++)
	{
	  staveUsed = (int)atom_getfloat(argv+i);
	  z->staves[i] = staveUsed;
	}
    }
  else post("Empty list to staves!");
}

void stavelines_liststaves(t_stavelines *z, t_floatarg f)
{
  int i = (int)f;
  if(f == 0)
    {
      post("stavelines: %d %d %d %d %d %d %d %d",z->perclines[0],z->perclines[1],z->perclines[2],z->perclines[3],z->perclines[4],z->perclines[5],z->perclines[6],z->perclines[7]);
    }
  else if(f == 1)
    {
      post("stavesUsed: %d %d %d %d %d %d %d %d",z->staves[0],z->staves[1],z->staves[2],z->staves[3],z->staves[4],z->staves[5],z->staves[6],z->staves[7]);
    }
  else if(f == 2)
    {
      post("breaks: %f, %f, %f, %f, %f, %f, %f, %f",atom_getfloatarg(0, STAVELINES, z->breaks),atom_getfloatarg(1, STAVELINES, z->breaks),atom_getfloatarg(2, STAVELINES, z->breaks),atom_getfloatarg(3, STAVELINES, z->breaks),atom_getfloatarg(4, STAVELINES, z->breaks),atom_getfloatarg(5, STAVELINES, z->breaks),atom_getfloatarg(6, STAVELINES, z->breaks),atom_getfloatarg(7, STAVELINES, z->breaks));
    }
  else post("WRONG ARGUMENT TO LISTSTAVES");
}

/*void stavelines_stavemode(t_stavelines *z, t_floatarg f)
{
  if(f < 3 && f >= 0)
    {
      z->staveMode = (int)f;
    }
  else post("staveMode %f out of bounds!",f);
  }*/

void stavelines_thisstave(t_stavelines *z, t_floatarg f)
{
  int i;
  if(f >= 0 && f < STAVELINES)
    {
      z->thisStaveNum = (int)f;
      if(z->staves[z->thisStaveNum] == 0) post("This stave is not being shown. Notes will be invisible!");
      else
	{
	  if(z->thisStaveNum < z->staveLinesNum)
	    {
	      z->fullBreakVal = z->fullStaveVal = 0;
	      if(z->staveLinesNum < STAVELINES)
		{
		  for(i = 0; i < z->thisStaveNum; i++)
		    {
		      if(z->staves[i] == 1)
			{
			  z->staveVal = z->perclines[i];
			  z->breakVal = atom_getfloatarg(i, STAVELINES, z->breaks);
			  z->fullStaveVal += (t_float)z->staveVal;
			  z->fullBreakVal += z->breakVal;
			}
		    }
		}
	      else post("z->staveLinesNum > maximum, %d >= %d",z->staveLinesNum,STAVELINES);
	    }
	  else post("x->thisStaveNum > z->staveLinesNum");
	  if(z->staves[z->thisStaveNum] == 1) outlet_float(z->linesNum, (t_float)z->perclines[z->thisStaveNum]);
	  outlet_float(z->breaksOff, z->fullBreakVal);
	  outlet_float(z->linesOff, z->fullStaveVal);
	  post("breaksOff = %f, linesOff = %f",z->fullBreakVal,z->fullStaveVal);
	}
    }
  else post("this stave %f out of bounds!",f);
}

void *stavelines_new(t_floatarg f)
{
  t_stavelines *y = (t_stavelines *)pd_new(stavelines_class);
  int i;
  y->stavenum = 0;
  //y->staveMode = 0;
  y->staveVal = y->breakVal = y->fullStaveVal = y->fullBreakVal = 0;
  y->staveLinesNum = y->breakLinesNum = 0;
  for(i=0;i<STAVELINES;i++)
    {
      y->staves[i] = 0;
      SETFLOAT(&y->breaks[i],0);
      y->perclines[i] = 0;
    }
  y->linesOff = outlet_new(&y->x_obj, gensym("float"));
  y->breaksOff = outlet_new(&y->x_obj, gensym("float"));
  y->linesNum = outlet_new(&y->x_obj, gensym("float"));
  return(void *)y;
}

void stavelines_setup(void) 
{
  stavelines_class = class_new(gensym("stavelines"),
  (t_newmethod)stavelines_new,
  0, sizeof(t_stavelines),
  0, A_DEFFLOAT, 0);
  //post("stavelines");

  class_addbang(stavelines_class, stavelines_bang);
  class_addfloat(stavelines_class, stavelines_float);
  class_addmethod(stavelines_class, (t_method)stavelines_setlines, gensym("setlines"), A_GIMME, 0);
  class_addmethod(stavelines_class, (t_method)stavelines_setbreaks, gensym("setbreaks"), A_GIMME, 0);
  class_addmethod(stavelines_class, (t_method)stavelines_staves, gensym("staves"), A_GIMME, 0);
  //class_addmethod(stavelines_class, (t_method)stavelines_stavemode, gensym("stavemode"), A_DEFFLOAT, 0);
  class_addmethod(stavelines_class, (t_method)stavelines_thisstave, gensym("thisstave"), A_DEFFLOAT, 0);
  class_addmethod(stavelines_class, (t_method)stavelines_liststaves, gensym("liststaves"), A_DEFFLOAT, 0);
}
