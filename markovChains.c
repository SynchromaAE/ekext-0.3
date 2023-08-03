/*
 * Copyright (c) 2005-2023 Edward Kelly
 * Forinformaion on usage and distribution, and for a DICLAIMER OF ALL 
 * WARRANTIES, see the file "LICENSE.txt," in this distribution. */

#ifdef __APPLE__
#include <sys/types.h>
#include <sys/time.h>
#include <sys/math.h>
#endif

#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "m_pd.h"

#define ARRAYSIZE 1024
#define HALFARRAY 512

static t_class *markovChains_class;

typedef struct drand48_data *randomChoice;

typedef struct _map
{
  t_atom values[ARRAYSIZE];
  t_int current[ARRAYSIZE];
  t_atom boundary[ARRAYSIZE];
  t_int length[HALFARRAY];
  t_int starts[HALFARRAY]; // start of each slot (index offset)
  t_atom normalize[HALFARRAY];
  t_atom listOut[ARRAYSIZE];
  //  t_atom infoPos[HALFARRAY]; // position of slot start
} t_map;

typedef struct _markovChains {
  t_object x_obj;
  t_clock *x_block; // do I need this?
  t_map map;
  unsigned short int seed16v[3];
  t_float seed1, seed2, seed3;
  t_float normValue, runningTot; //do we need runningTot ?
  t_int current, totLen, isInit;
  t_int thisOffset, thisSlot, thisLen, maxLen, addLen, addOffset, addSlot, noZero;
  t_int nextSlot, maxSlot, nextOffset, nextLen, maxOffset;
  t_int autoNorm;

  t_int thisIndex, isChange;
  t_float val1, valNorm, thisRand, thisTot, addTot, totValue, thisBound, lastBound;

  //outList
  t_int outLen, outOffset, outVal;

  t_int myBug;
  
  t_outlet *index, *value, *slot, *randOut, *listValues, *debugList;

} t_markovChains;

/*
 * double randNum;
 * randNum = drand48();
 */
  
void markovChains_seed(t_markovChains *x, t_symbol *s, int argc, t_atom *argv)
{
  int i;
  if(argc > 0)
    {
      argc = argc > 3 ? 3 : argc;
      for(i = 0; i < argc; i++)
	{
	  switch(i)
	    {
	    case 0:
	      x->seed1 = atom_getfloat(argv);
	      break;
	    case 1:
	      x->seed2 = atom_getfloat(argv+1);
	      break;
	    case 2:
	      x->seed3 = atom_getfloat(argv+2);
	      break;
	    default:
	      break;
	    }
	}
      x->seed16v[2] = (unsigned short int)x->seed3;
      x->seed16v[1] = (unsigned short int)x->seed2;
      x->seed16v[0] = (unsigned short int)x->seed1;
      seed48(x->seed16v);
    }
}

void markovChains_bang(t_markovChains *x)
{
  double randNum = drand48();
  x->thisRand = (t_float)randNum;
  int i;
  t_float bVal, lastB, thisBound;

  if(x->autoNorm && x->thisTot > 0)
    {
      lastB = 0;
      if(x->thisTot < 1 && x->thisTot > 0) x->thisRand = x->thisRand / x->thisTot;
      for(i=0; i < x->thisLen; i++)
	{
	  bVal = atom_getfloatarg(x->thisOffset + i + x->noZero, ARRAYSIZE, x->map.boundary);
	  bVal = bVal / x->thisTot;
	  if(x->thisRand > lastB && x->thisRand < bVal)
	    {
	      x->lastBound = lastB;
	      x->thisIndex = i > 0 ? i : 0;
	    }
	  lastB = bVal;
	}
      outlet_float(x->randOut, x->thisRand);
      outlet_float(x->slot, (t_float)x->thisSlot);
      outlet_float(x->value, x->lastBound);
      outlet_float(x->index, (t_float)x->thisIndex);
    }
  else if(x->autoNorm == 0)
    {
      lastB = 0;
      for(i=0; i < x->thisLen; i++)
	{
	  bVal = atom_getfloatarg(x->thisOffset + i, ARRAYSIZE, x->map.boundary);
	  if(x->thisRand > lastB && x->thisRand < bVal)
	    {
	      x->lastBound = lastB;
	      x->thisIndex = i > 0 ? i : 0;
	    }
	  lastB = bVal;
	}
      outlet_float(x->randOut, x->thisRand);
      outlet_float(x->slot, (t_float)x->thisSlot);
      outlet_float(x->value, x->lastBound);
      outlet_float(x->index, (t_float)x->thisIndex); 
    }
  else if(x->autoNorm == 2)
    {
      lastB = 0;
      x->thisRand = x->thisRand * x->thisTot;
      if(x->myBug == 3)
	{
	  post("x->thisTot = %f", x->thisTot);
	}
      for(i=0; i < x->thisLen; i++)
	{
	  bVal = atom_getfloatarg(x->thisOffset + i, ARRAYSIZE, x->map.boundary);
	  if(x->thisRand > lastB && x->thisRand < bVal)
	    {
	      x->lastBound = lastB;
	      x->thisIndex = i > 0 ? i - 1 : 0;
	    }
	  lastB = bVal;
	}
      outlet_float(x->randOut, x->thisRand);
      outlet_float(x->slot, (t_float)x->thisSlot);
      outlet_float(x->value, x->lastBound);
      outlet_float(x->index, (t_float)x->thisIndex);
    }
}

void markovChains_float(t_markovChains *x, t_floatarg f) // go to a slot and make a markov choice from there
{
  t_int iIn = (t_int)f;
  double randNum = drand48();
  x->thisRand = (t_float)randNum;
  int i;
  t_float bVal, lastB, thisBound;
  if(iIn >= 0 && iIn <= x->maxSlot)
    {
      lastB = 0;
      t_float lBound;
      x->thisSlot = iIn;
      // what about if x->thisOffset + x->thisLen >= 1024?
      x->thisOffset = x->map.starts[x->thisSlot];
      x->thisLen = x->map.length[x->thisSlot];
      x->thisTot = atom_getfloatarg(x->thisOffset + x->thisLen - 1 + x->noZero, ARRAYSIZE, x->map.boundary);
      if(x->autoNorm && x->thisTot > 0)
	{
	  x->thisRand = x->thisRand / x->thisTot;
	  for(i = 0; i < x->thisLen; i++)
	    {
	      bVal = atom_getfloatarg(x->thisOffset + i + x->noZero, ARRAYSIZE, x->map.boundary);
	      bVal = bVal / x->thisTot;
	      if(x->thisRand > lastB && x->thisRand < bVal)
		{
		  x->lastBound = lastB;
		  x->thisIndex = i > 0 ? i - 1 : 0;
		}
	      lastB = bVal;
	    }
	}
      else
	{
	  for(i = 0; i < x->thisLen; i++)
	    {
	      bVal = atom_getfloatarg(x->thisOffset + i + x->noZero, ARRAYSIZE, x->map.boundary);
	      if(x->thisRand > lastB && x->thisRand < bVal)
		{
		  x->lastBound = lastB;
		  x->thisIndex = i > 0 ? i - 1 : 0;
		}
	      lastB = bVal;
	    }
	}
      outlet_float(x->randOut, x->thisRand);
      outlet_float(x->slot, (t_float)x->thisSlot);
      outlet_float(x->value, x->lastBound);
      outlet_float(x->index, (t_float)x->thisIndex);
    }
}

void markovChains_outList(t_markovChains *x, t_floatarg f)
{
  t_int ndx = (t_int)f;
  t_int i, j;
  t_float val2;
  if(ndx > -1 && ndx <= x->maxSlot)
    {
      x->outLen = x->map.length[ndx];
      x->outOffset = x->map.starts[ndx];
      for(i=0; i < x->outLen; i++)
	{
	  j = x->outOffset + i;
	  val2 = atom_getfloatarg(j, ARRAYSIZE, x->map.values);
	  if(x->myBug == 3)
	    {
	      post("j = i + outOffset = %d, val2 = %f", j, val2);
	    }
	  //t_atom listOut[ARRAYSIZE];
	  SETFLOAT(&x->map.listOut[i], val2);
	}
      outlet_list(x->listValues, gensym("list"), x->outLen, x->map.listOut);
    }
  else if(ndx == -1)
    {
      for(i=0; i < x->thisLen; i++)
	{
	  j = x->thisOffset + i;
	  val2 = atom_getfloatarg(j, ARRAYSIZE, x->map.values);
	  if(x->myBug == 3)
	    {
	      post("j = i + outOffset = %d, val2 = %f", j, val2);
	    }
	  //t_atom listOut[ARRAYSIZE];
	  SETFLOAT(&x->map.listOut[i], val2);
	}
      outlet_list(x->listValues, gensym("list"), x->thisLen, x->map.listOut);
    }  
}

void markovChains_addSeq(t_markovChains *x, t_symbol *s, int argc, t_atom *argv) //after nextSlot (above)
{
  t_int i;
  t_int newMaxSlot = x->maxSlot + 1;
  t_float val1;
  // minumum length = 2
  if(argc >= 2 && ARRAYSIZE - x->nextOffset > argc && argc <= x->maxLen)
    {
      
      x->addLen = (t_int)argc;
      if(x->addLen > x->maxLen) x->addLen = x->maxLen;
      x->map.length[newMaxSlot] = x->addLen;
      x->addOffset = x->map.starts[x->maxSlot] + x->map.length[x->maxSlot];
      if(x->myBug == 3)
	{
	  post("addOffset = %d", x->addOffset);
	}
      x->map.starts[newMaxSlot] = x->addOffset;
      x->addTot = 0;
      // addSeq code goes in here
      if(x->addOffset + x->addLen < 1023)
	{
	  for(i=0; i< x->addLen; i++)  
	    {
	      val1 = atom_getfloat(argv+i);
	      val1 = fabs(val1);
	      if(val1 < 0.001) val1 = 0.001;
	      x->addTot += val1;
	      x->map.current[x->addOffset+i] = i;
	      if(x->addTot > 0)
		{
		  x->normValue = 1 / x->addTot;
		}
	      SETFLOAT(&x->map.values[x->addOffset + i], val1);
	      SETFLOAT(&x->map.boundary[x->addOffset + i], x->addTot);
	      if (x->addOffset + x->addLen * 2 < 1023)
		SETFLOAT(&x->map.values[x->addOffset + i + x->maxLen], 1);
	    }
	  if(x->myBug == 3)
	    {
	      post("thisTot = %f, x->addOffset = %d, boundary = %f", x->thisTot, x->addOffset, atom_getfloatarg(x->addOffset + i, ARRAYSIZE, x->map.boundary));
	    }
	}
      SETFLOAT(&x->map.normalize[newMaxSlot], 1 / x->normValue);
      x->maxSlot = newMaxSlot;
      x->addOffset += x->addLen;
    }
  else if (ARRAYSIZE - x->addOffset <= argc)
    {
      post("markovChains: sequence too long for available memory");
    }
  else post ("markovChains: sequence too short - minimum length = 2");
}

void markovChains_noZeros(t_markovChains *x, t_floatarg f)
{
  if(f == 0)
    {

    }
  else if(f > 0)
    {

    }
}

void markovChains_clear(t_markovChains *x, t_floatarg f)
{
  t_int i;
  for(i=0; i<ARRAYSIZE; i++)
    {
      x->map.current[i] = 0;
      //SETFLOAT[&x->map.values[i], 1];
      SETFLOAT(&x->map.values[i], 1);
      SETFLOAT(&x->map.listOut[i], 1);
      if(i < HALFARRAY)
	{
	  x->map.length[i] = x->thisLen;
	  x->map.starts[i] = 0;
	  //SETFLOAT[&x->map.normalize[i], 100];
	  SETFLOAT(&x->map.normalize[i], 100);
	}
    }
  
  x->map.current[0] = 0; // slot 0 will be a buffer with maxlength = f
  x->map.length[0] = x->thisLen;
  for(i = 0; i < x->thisLen; i++)
    {
      SETFLOAT(&x->map.values[i], 1);
    }
  SETFLOAT(&x->map.normalize[0], 100);
  x->map.starts[0] = 0;

  x->maxSlot = 0;
  x->maxOffset = x->thisLen;
  x->thisSlot = 0;
  x->thisOffset = 0;
  x->nextOffset = x->thisLen;
  x->nextSlot = 1;
  x->thisTot = f > 0 ? f : 16;

  x->addSlot = 0;
  x->addOffset = 16;
  x->addLen = 16;

}

void markovChains_setSlot(t_markovChains *x, t_floatarg f)
{
  if(f > 0 && f < x->maxSlot)
    {
      x->thisSlot = (t_int)f;
      x->thisOffset = x->map.starts[x->thisSlot];
      x->thisLen = x->map.length[x->thisSlot];
      x->normValue = atom_getfloatarg(x->thisSlot, HALFARRAY, x->map.normalize);
      x->thisTot = atom_getfloatarg(x->thisOffset + x->thisLen - 1, ARRAYSIZE, x->map.boundary);
    }
  else if(f >= x->maxSlot)
    {
      x->thisSlot = x->maxSlot;
      x->thisOffset = x->map.starts[x->thisSlot];
      x->thisLen = x->map.length[x->thisSlot];
      x->normValue = atom_getfloatarg(x->thisSlot, HALFARRAY, x->map.normalize);
      x->thisTot = atom_getfloatarg(x->thisOffset + x->thisLen - 1, ARRAYSIZE, x->map.boundary);
    }
}

void markovChains_autoNorm(t_markovChains *x, t_floatarg f)
{
  x->autoNorm = f > 0 ? f > 1 ? 2 : 1 : 0;
}

void markovChains_noZero(t_markovChains *x, t_floatarg f)
{
  x->noZero = f == 0 ? 0 : 1;
}

void markovChains_debug(t_markovChains *x, t_floatarg f)
{
  if(f >= 0)
    {
      x->myBug = (t_int)f;
      if(x->myBug == 1)
	{
	  //t_atom listOut[ARRAYSIZE];
	  SETFLOAT(&x->map.listOut[0], (t_float)x->thisSlot);
	  SETFLOAT(&x->map.listOut[1], (t_float)x->thisLen);
	  SETFLOAT(&x->map.listOut[2], (t_float)x->thisOffset);
	  SETFLOAT(&x->map.listOut[3], (t_float)x->thisIndex);
	  SETFLOAT(&x->map.listOut[4], (t_float)x->map.starts[x->thisSlot]);
	  SETFLOAT(&x->map.listOut[5], (t_float)x->map.length[x->thisSlot]);
	  SETFLOAT(&x->map.listOut[6], atom_getfloatarg(x->thisOffset + x->thisIndex, ARRAYSIZE, x->map.values));
	  SETFLOAT(&x->map.listOut[7], atom_getfloatarg(x->thisSlot, HALFARRAY, x->map.normalize));
	  outlet_list(x->debugList, gensym("list"), 8, x->map.listOut);
	}
      else if(x->myBug == 2)
	{
	  //t_atom listOut[ARRAYSIZE];
	  post("x->thisSlot = %d, x->thisLen = %d, x->thisOffset = %d, x->thisIndex = %d, x->map.starts = %d, x->map.length = %d, x->value = %f, x->normalize = %f", x->thisSlot, x->thisLen, x->thisOffset, x->thisIndex, x->map.starts[x->thisSlot], x->map.length[x->thisSlot], atom_getfloatarg(x->thisOffset +x->thisIndex, ARRAYSIZE, x->map.values), atom_getfloatarg(x->thisSlot, HALFARRAY, x->map.normalize));
	}
      else if(x->myBug == 3)
	{
	  x->myBug = 3;
	  post("x->thisTot = %f", x->thisTot);
	}
    }
  else x->myBug = 0;
}

void *markovChains_new(t_floatarg f) // f = maxlength
{
  int i;
  t_markovChains *x = (t_markovChains *)pd_new(markovChains_class);

  x->seed1 = 16384;
  x->seed2 = 8192;
  x->seed3 = 512;

  x->seed16v[2] = (unsigned short int)x->seed3;
  x->seed16v[1] = (unsigned short int)x->seed2;
  x->seed16v[0] = (unsigned short int)x->seed1;
  seed48(x->seed16v);

  x->isInit = 1;
  if(f > 0)
    {
      x->thisLen = (t_int)f;
      x->maxLen = x->thisLen;
    }
  else
    {
      x->thisLen = 16;
      x->maxLen = 16;
    }
  
  for(i=0; i<ARRAYSIZE; i++)
    {
      x->map.current[i] = 0;
      //SETFLOAT[&x->map.values[i], 1];
      SETFLOAT(&x->map.values[i], 1);
      SETFLOAT(&x->map.listOut[i], 1);
      if(i < HALFARRAY)
	{
	  x->map.length[i] = x->thisLen;
	  x->map.starts[i] = 0;
	  //SETFLOAT[&x->map.normalize[i], 100];
	  SETFLOAT(&x->map.normalize[i], 100);
	}
      x->noZero = 1;
    }
  
  x->map.current[0] = 0; // slot 0 will be a buffer with maxlength = f
  x->map.length[0] = x->thisLen;
  for(i = 0; i < x->thisLen; i++)
    {
      SETFLOAT(&x->map.values[i], 1);
    }
  SETFLOAT(&x->map.normalize[0], 100);
  x->map.starts[0] = 0;

  x->maxSlot = 0;
  x->maxOffset = x->thisLen;
  x->thisSlot = 0;
  x->thisOffset = 0;
  x->nextOffset = x->thisLen;
  x->nextSlot = 1;
  x->thisTot = f > 0 ? f : 16;

  x->addSlot = 0;
  x->addOffset = 16;
  x->addLen = 16;
  
  x->myBug = 0;
  x->thisIndex = 0;
  //t_outlet *index, *value, *slot, randOut;
  x->index = outlet_new(&x->x_obj, &s_float);
  x->value = outlet_new(&x->x_obj, &s_float);
  x->slot = outlet_new(&x->x_obj, &s_float);
  x->randOut = outlet_new(&x->x_obj, &s_float);
  x->listValues = outlet_new(&x->x_obj, &s_list);
  x->debugList = outlet_new(&x->x_obj, &s_list);

  return (void *)x;
}

void markovChains_setup(void)
{
  markovChains_class = class_new(gensym("markovChains"),
  (t_newmethod)markovChains_new,
  0, sizeof(t_markovChains),
  0, A_DEFFLOAT, 0);
  post("markovChains: chains of markov chains");
  post("|^^^^^^^--Edward Kelly 2021--^^^^^^^|");

  class_addfloat(markovChains_class, markovChains_float);
  class_addbang(markovChains_class, markovChains_bang);
  class_addmethod(markovChains_class, (t_method)markovChains_setSlot, gensym("setSlot"), A_DEFFLOAT, 0);
  class_addmethod(markovChains_class, (t_method)markovChains_addSeq, gensym("addSeq"), A_GIMME, 0);
  class_addmethod(markovChains_class, (t_method)markovChains_autoNorm, gensym("autoNorm"), A_DEFFLOAT, 0);
  class_addmethod(markovChains_class, (t_method)markovChains_seed, gensym("seed"), A_GIMME, 0);
  class_addmethod(markovChains_class, (t_method)markovChains_outList, gensym("outList"), A_DEFFLOAT, 0);

  class_addmethod(markovChains_class, (t_method)markovChains_debug, gensym("debug"), A_DEFFLOAT, 0);
  class_addmethod(markovChains_class, (t_method)markovChains_clear, gensym("clear"), A_DEFFLOAT, 0);
  class_addmethod(markovChains_class, (t_method)markovChains_noZero, gensym("noZero"), A_DEFFLOAT, 0);
  
}
