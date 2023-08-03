/* sieve takes a map like 0 1 3 4 7 and only returns the number if it is present 
 * in the map, or returns the closest, or the next up or down (wrapped)
 * Copyright (c) 2005-2023 Edward Kelly
 * Forinformaion on usage and distribution, and for a DICLAIMER OF ALL 
 * WARRANTIES, see the file "LICENSE.txt," in this distribution. */

#ifdef __APPLE__
#include <sys/types.h>
#include <sys/time.h>
#endif

#include <stdlib.h>
#include <time.h>

#include "m_pd.h"
#include <math.h>
#include <string.h>
#define MAXENTRIES 8192 // increased to 8192 in 2019
#define LASTENTRY 8191
#define DEFAULTTARGET 100

static t_class *sieve_class;

typedef struct drand48_data *markovBuffer;

typedef struct {
  long tv_sec;
  long tv_usec;
} timeval;

/* mode = 0 : block when absent, 1: nearest when absent, 2: shunt when absent */
typedef struct _map
{
  t_atom map[MAXENTRIES];
  t_atom umap[MAXENTRIES];
  t_atom nomap[MAXENTRIES];
  t_atom omap[MAXENTRIES];
  t_atom onomap[MAXENTRIES]; //do we need this?
} t_map;

typedef struct _sieve
{
  timeval tv;
  t_object x_obj;
  t_map x_map;
  t_float input, mode, outmap, myBug, aim, markovResult, weight, slotVal;
  t_int markovIndex, max, favourite, urnRem, rLoc, uLoc, umax;
  t_outlet *mapped, *value, *mapout, *inst;
  unsigned short int seed16v[3];
  double random;  
  long int mod1, mod2, mod3, seed1, seed2, seed3;
} t_sieve;

void sieve_markov(t_sieve *x) //maybe move this in the code when it's finished!
{
  if(x->aim != 0)
    {
      t_float lastResult = 0;
      t_float thisResult = 0;
      t_float accumulator = 0;
      x->markovResult = 0;
      x->random = (drand48() * x->aim);
      if(x->myBug == 1) post("dRand48 == %f",(t_float)x->random);
      int i;
      for(i = 0; i <= x->max; i++)
	{
	  thisResult = atom_getfloatarg(i,MAXENTRIES,x->x_map.map);
	  accumulator += thisResult;
	  if(x->random > lastResult && x->random <= accumulator)
	    {
	      x->markovIndex = (t_float)i;
	      x->markovResult = x->random;
	    }
	  lastResult += thisResult;
	}
      if(x->myBug == 4) post("lastResult = %f, thisResult = %f, random = %f",lastResult, thisResult, x->random);
      outlet_float(x->value, (t_float)x->random);
      outlet_float(x->mapped, x->markovIndex);
    }
}

void sieve_float(t_sieve *x, t_floatarg fin)
{
  int i, ip, in, arg, arga, argb, argaout, argbout, argxa, argxb, itest, itesta, itestb, iresult;
  itest = itesta = itestb = iresult = arga = argb = arg = 0;
  t_float test, testa, testb, fresult;
  test = testa = testb = fresult = 0;
  x->input = arg = fin;
  if (x->mode == 0) /* only let through floats when the corresponding 
                       index contains != 0 */
    {
      test = fin < 0 ? 0 : atom_getfloatarg(arg, MAXENTRIES, x->x_map.map);
      if(test!=0) 
	{
	  outlet_bang(x->inst);
	  outlet_float(x->value, test);
	  outlet_float(x->mapped, arg);
	}
    }
  else if (x->mode == 1) /* find the nearest float whose (int) index is != 0 */
    {
      test =  fin < 0 ? 0 : atom_getfloatarg(arg, MAXENTRIES, x->x_map.map);
      if(test!=0)
	{
	  outlet_bang(x->inst);
	  outlet_float(x->value, test);
	  outlet_float(x->mapped, arg);
	}
      else
	{
	  arga = argb = arg;
	  while(itest == 0 && (arga > -1 || argb < MAXENTRIES))
	    {
	      arga--;
	      argb++;
	      argxa = arga >= 0 ? arga : 0;
	      argxb = argb <= LASTENTRY ? argb : LASTENTRY;
	      testa = atom_getfloatarg(argxa, MAXENTRIES, x->x_map.map);
	      testb = atom_getfloatarg(argxb, MAXENTRIES, x->x_map.map);
	      itesta = testa != 0 ? 1 : 0;
	      itestb = testb != 0 ? 1 : 0;
	      itest =  fin < 0 ? 0 : itesta + itestb;
	    }
	  switch(itest)
	    {
	    case 2: /* if we find two at equal distance, output the higher */
	      if (x->mode == 1)
		{
		  outlet_float(x->value, testb);
		  outlet_float(x->mapped, argb);
		}
	      else
		{
		  outlet_float(x->value, testa);
		  outlet_float(x->mapped, arga);
		}
	    case 1:
	      iresult = itesta == 1 ? arga : argb;
	      fresult = itesta == 1 ? testa : testb;
	      outlet_float(x->value, fresult);
	      outlet_float(x->mapped, iresult);
	    case 0:
	      break;
	    }
	}
    }
  else if (x->mode==2) /* if the index is 0, find the next highest */
    {
      itest = 0;
      test =  fin < 0 ? 0 : atom_getfloatarg(arg, MAXENTRIES, x->x_map.map);
      if(test!=0)
	{
	  outlet_bang(x->inst);
	  outlet_float(x->value, test);
	  outlet_float(x->mapped, arg);
	}
      else
	{
	  arga =  arg;
	  while(itest == 0 && (x->max > 0))
	    {
	      arga = (arga + 1) <= LASTENTRY ? (arga + 1) : 0;
	      testa = atom_getfloatarg(arga, MAXENTRIES, x->x_map.map);
	      itest = testa != 0 ? 1 : 0;
	    }
	  if(x->max > 0 && fin >= 0)
	    {
	      outlet_float(x->value, testa);
	      outlet_float(x->mapped, arga);
	    }
	}
    }
  else if (x->mode == 3) /* if the index is 0, find the next lowest */
    {
      itest = 0;
      test =  fin < 0 ? 0 : atom_getfloatarg(arg, MAXENTRIES, x->x_map.map);
      if(test!=0)
	{
	  outlet_bang(x->inst);
	  outlet_float(x->value, test);
	  outlet_float(x->mapped, arg);
	}
      else
	{
	  arga =  arg;
	  while(itest == 0 && (x->max > 0))
	    {
	      argb = arga - 1;
	      arga = argb >= 0 ? argb : LASTENTRY;
	      testa = atom_getfloatarg(arga, MAXENTRIES, x->x_map.map);
	      itest = testa != 0 ? 1 : 0;
	    }
	}
      outlet_float(x->value, testa);
      outlet_float(x->mapped, arga);
    }
  else if ( x->mode == 4 )
    {
    }
}

t_int randLoc(t_sieve *x, t_float range)
{
  //timeval tv;
  gettimeofday(&x->tv, 0);

  x->seed1 = (x->tv.tv_usec % x->mod1) + (x->tv.tv_sec % x->mod2);
  x->seed1 = x->seed1 % x->mod3;
  x->seed2 = (x->tv.tv_usec % x->mod2) + (x->tv.tv_sec % x->mod3);
  x->seed2 = x->seed2 % x->mod1;
  x->seed3 = (x->tv.tv_usec % x->mod3) + (x->tv.tv_sec % x->mod1);
  x->seed3 = x->seed3 % x->mod2;
    
  x->seed16v[2] = (unsigned short int)x->seed3;
  x->seed16v[1] = (unsigned short int)x->seed2;
  x->seed16v[0] = (unsigned short int)x->seed1;
  seed48(x->seed16v);

  x->random = (drand48() * range) + 0.9; // I need to check whether the 0.9 is necessary to get the last value!
  if(x->myBug == 3) post("dRand48 == %f",(t_float)x->random);
  
  return((t_int)x->random);
}

void sieve_umap(t_sieve *x)
{
  t_int i;
  t_float f;
  x->umax = 0;
  for(i = 0; i <= x->max; i++)
    {
      f = atom_getfloatarg(i, MAXENTRIES, x->x_map.map);
      if(f != 0)
	{
	  SETFLOAT(&x->x_map.umap[x->umax], (t_float)i);
	  x->umax++;
	}
    }
  x->urnRem = x->umax;
}

void sieve_urn(t_sieve *x)
{
  if(x->umax > 0)
    {
      x->rLoc = 0;
      x->rLoc = randLoc(x, x->umax);
      x->slotVal = -1;
      
      t_int i = 0;
      t_int uLoc = 0;

      //x->urnRem = x->umax; // set this in sieve_umap, not here!
      while(x->urnRem > 0)
	{
	  //the umap table cross-references the map table
	  uLoc = (t_int)atom_getfloatarg(x->rLoc, MAXENTRIES, x->x_map.umap);
	  x->slotVal = atom_getfloatarg(uLoc, MAXENTRIES, x->x_map.map);
	  if(x->slotVal != -1 && x->urnRem > 0)
	    {
	      outlet_float(x->mapped, (t_float)x->rLoc);
	      outlet_float(x->value, x->slotVal);
	      SETFLOAT(&x->x_map.map[uLoc], -1);
	      x->urnRem--;
	      if(x->urnRem > 0) x->rLoc = randLoc(x, x->urnRem);
	      break;
	    }
	  else if(x->urnRem == 0)
	    {
	      sieve_umap(x);
	      uLoc = (t_int)atom_getfloatarg(x->rLoc, MAXENTRIES, x->x_map.umap);
	      x->slotVal = atom_getfloatarg(uLoc, MAXENTRIES, x->x_map.map);
	      outlet_float(x->mapped, (t_float)x->rLoc);
	      outlet_float(x->value, x->slotVal);
	      SETFLOAT(&x->x_map.map[uLoc], -1);
	      x->urnRem--;
	      break;
	    }
	}
      /*if(x->urnRem > 0)
	{
	  outlet_float(x->mapped, (t_float)x->rLoc);
	  outlet_float(x->value, x->slotVal);
	  x->urnRem--;
	}
	}*/
      /*
      if(x->urnRem == 0)
	{
	  if(x->max == 0) x->max == 1;
	  else
	{
	  x->urnRem = x->max;
	  // then copy omap (and inverse) to x_map.map (and .nomap)
	  for(i = 0; i < x->rLoc; i++)
	    {
	      //how to anneal afterwards?
	      //or do we only keep the annealed copy? (prefereable - put it in the anneal routine)
	      //maybe we need an amap array (annealed map)
	    }
	  // reset array for urn-like behaviour!
	  {
	  }
	}
    }
  else if(x->slotVal != 0 && x->urnRem > 0)
    {
      //umap is new - urn map - need to trace back through other routines for initialization etc.
      SETFLOAT(&x->x_map.umap[x->rLoc], x->slotVal);
      outlet_float(x->value, x->slotVal);
      outlet_float(x->mapped, (t_float)x->rLoc);
      SETFLOAT(&x->x_map.map[x->rLoc], 0);
      SETFLOAT(&x->x_map.nomap[x->rLoc], x->slotVal);
      x->urnRem--;
      }*/
  // we need to work out whether there is a value at a randomly generated location (using the randLoc() function above)
  // if there is we need to output the location and value, then delete the value
  // a de-accumulator would be good to use, check for "no non-values (0) left" status - i.e. starts off full and ends up 0
  // when there are no values left, the omap table resets the map table (or what do we do if it is annealed? - another array?)
  // we can probably do away with the onomap array
  // it can be worked out from the omap table with some math (1-f, or 1/f? perhaps new modes, but always !=0)
  // also to reset annealing in map table when urn mode is reset
    }
}

void sieve_set(t_sieve *x, t_floatarg fmap, t_floatarg fval) /* set one value in the array */
{
  t_float fvaller;
  t_int mapper = (t_int)fmap;
  if(mapper < MAXENTRIES && mapper >= 0)
    {
      fvaller = fval != 0 ? 0 : 1;
      if(fval != 0)
	{
	  x->umax++;
	  SETFLOAT(&x->x_map.umap[x->umax], (t_float)mapper);
	  //x->umax++;
	  
	}
      SETFLOAT(&x->x_map.map[mapper], fval);
      SETFLOAT(&x->x_map.omap[mapper], fval);
      SETFLOAT(&x->x_map.nomap[mapper], fvaller);
      SETFLOAT(&x->x_map.onomap[mapper], fvaller);
      x->max = mapper > x->max ? mapper : x->max;
      if(x->myBug == 4) post("x->max == %d, mapper = %d",x->max,mapper);
    }
}

void sieve_anneal(t_sieve *x, t_floatarg aim) /* reduce all values so that they all add-up to aim */
// this is to add markov chains to sieve, so that they all add up to whatever the aim is (total sum of probabilities)
{
  if(aim != 0) x->aim = aim;
  else x->aim = DEFAULTTARGET; 
  t_int index;
  x->aim = aim;
  t_float totalValue = 0;
  t_float multiplier = 1;
  //t_float divider = 1;
  t_float indexValue = 0;
  for(index = 0; index <= x->max; index++) // get the total of all filled slots
    {
      totalValue += atom_getfloatarg(index,MAXENTRIES,x->x_map.map);
    }
  if(x->myBug == 2)
    {
      post("total = %f, x->aim = %f",totalValue, x->aim);
      post("annealing");
    }
  //why would we put a clock delay in here?
  // divide the aim by the total, then multiply each value by the result
  multiplier = x->aim / totalValue;
  if(x->myBug == 2) post("multipler = %f",multiplier);
  for(index = 0; index <= x->max; index++) // annealing
    {
      indexValue = atom_getfloatarg(index,MAXENTRIES,x->x_map.map);
      SETFLOAT(&x->x_map.map[index],indexValue * multiplier);
      if(indexValue != 0)
	{
	  SETFLOAT(&x->x_map.nomap[index],1 / (indexValue * multiplier));
	}
      else SETFLOAT(&x->x_map.nomap[index],1);
    }
  if(x->myBug == 3)
    {
      post("ANNEALING!");
      for(index = 0; index <= x->max; index+=6)
	{
	  post("%d, %d, %d, %d, %d, %d",atom_getfloatarg(index, MAXENTRIES, x->x_map.map),atom_getfloatarg(index+1, MAXENTRIES, x->x_map.map),atom_getfloatarg(index+2, MAXENTRIES, x->x_map.map),atom_getfloatarg(index+3, MAXENTRIES, x->x_map.map),atom_getfloatarg(index+4, MAXENTRIES, x->x_map.map),atom_getfloatarg(index+5, MAXENTRIES, x->x_map.map),atom_getfloatarg(index+6, MAXENTRIES, x->x_map.map));
	}
    }
  if(x->myBug == 2)
    {
      t_float testTotal = 0;
      for(index = 0; index <= x->max; index++)
	{
	  testTotal += atom_getfloatarg(index, MAXENTRIES, x->x_map.map);
	}
      post("testTotal == %d", testTotal);
    }
}

void sieve_weight(t_sieve *x, t_floatarg which, t_floatarg weighting)// make a certain location more likely by weighting
{ // re-weight the array so that it is more likely to be one than another, in its Markov probability, by a fractional amount
  x->favourite = (t_int)which;
  x->weight = weighting;
  t_int index;
  t_float thisValue = 0;
  t_float inverse = 1;
  if(x->favourite >= x->max) post("that location is out-of-range. favour = %d, x->max = %d",x->favourite,x->max);
  if(x->weight >= 1.0) post("warning - you are making it impossible for outher outcomes! weight == %f", x->weight);
  else if(x->weight <= 0) post("warning - you can't have a weight of 0 - it would crash! weight == %f", x->weight);
  else
    {
      t_float adjustUp = 1 + x->weight;
      t_float adjustDown = 1 - x->weight;
      if(adjustUp > 0)
	{
	  adjustDown = 1 / adjustUp;
	}
      else
	{
	  adjustDown = 1 - adjustUp;
	}
      for(index = 0; index <= x->max; index++) // is x->max the last one? or the one after?
	{
	  thisValue = atom_getfloatarg(index,MAXENTRIES,x->x_map.omap);
	  if(index == x->favourite)
	    {
	      SETFLOAT(&x->x_map.map[index], thisValue * adjustUp);
	      SETFLOAT(&x->x_map.nomap[index], thisValue * adjustDown);
	    }
	  else
	    {
	      SETFLOAT(&x->x_map.map[index], thisValue * adjustDown);
	      SETFLOAT(&x->x_map.nomap[index], thisValue * adjustUp);
	    }
	}
    }
}

void sieve_delete(t_sieve *x, t_floatarg loc) /* remove a value */
{
  int addloc = (int)loc + 1;
  int maxentry = x->max;
  int i;
  t_float buffer;
  if(loc<x->max && loc>=0)
    {
      for(i=addloc; i <= maxentry; i++)
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

void sieve_shunt(t_sieve *x, t_floatarg loc) /* move down and decrement subsequent values */
{
  int addloc = (int)loc + 1;
  int maxentry = (int)x->max;
  int i;
  t_float buffer, shunt;
  if(loc<x->max && loc>=0)
    {
      for(i=addloc;i<=maxentry;i++)
	{
	  buffer = atom_getfloatarg(i,MAXENTRIES,x->x_map.map);
	  shunt = buffer - 1;
	  SETFLOAT(&x->x_map.map[i-1],shunt);
	  if(shunt!=0)
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

void sieve_shift(t_sieve *x, t_floatarg loc) /* move up and increment subsequent */
{
  int location = (int)loc;
  int addloc;
  int maxentry = x->max+1;
  int i;
  t_float buffer, shift;
  if(location>=0 && maxentry < MAXENTRIES)
    {
      for(i=maxentry;i>=location;i--)
	{
	  buffer = atom_getfloatarg(i-1,MAXENTRIES,x->x_map.map);
	  shift = buffer + 1;
	  SETFLOAT(&x->x_map.map[i],shift);
	  if(shift!=0)
	    {
	      SETFLOAT(&x->x_map.nomap[i],0);
	    }
	  else
	    {
	      SETFLOAT(&x->x_map.nomap[i],1);
	    }
	}
      x->max++;
    }
}

void sieve_insert(t_sieve *x, t_floatarg loc, t_floatarg val)
/* insert a value at specific location, moving subsequent values up */
{
  int location = (int)loc;
  int maxentry = x->max+1;
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

void sieve_get(t_sieve *x, t_floatarg inv) /* outlet to map or inverse */
{
  if(inv == 1) 
    {
      outlet_list(x->mapout, gensym("list"), x->max+1, x->x_map.nomap);
    }
  else if (inv == 0)
    {
      outlet_list(x->mapout, gensym("list"), x->max+1, x->x_map.map);
    }
  else if (inv == 2)
    {
      outlet_list(x->mapout, gensym("list"), x->umax+1, x->x_map.umap);
    }
  x->outmap = inv;
}

void sieve_clear(t_sieve *x)
{
  int i;
  for(i=0; i< MAXENTRIES; i++) 
    {
      SETFLOAT(&x->x_map.map[i], 0);
      SETFLOAT(&x->x_map.nomap[i], 1);
      SETFLOAT(&x->x_map.omap[i], 0);
      SETFLOAT(&x->x_map.onomap[i], 1);
      SETFLOAT(&x->x_map.umap[i], -1);
    }
  x->max = x->umax = 0;
}

void sieve_map(t_sieve *x, t_symbol *s, int argc, t_atom *argv) /* set the whole map */
{
  int i;
  for(i=0;i<MAXENTRIES;i++) 
    {
      SETFLOAT(x->x_map.map+i, 0);
      SETFLOAT(x->x_map.omap+i, 0);
      SETFLOAT(x->x_map.nomap+i, 1);
      SETFLOAT(x->x_map.onomap+i, 1);
      SETFLOAT(x->x_map.umap+i, -1);
    }
  x->max = x->umax = 0;
  t_float arg;
  for(i=0;i<argc;i++)
    {
      arg = atom_getfloat(argv+i);
      if(arg != 0)
	{
	  SETFLOAT(&x->x_map.map[i], arg);
	  SETFLOAT(&x->x_map.omap[i], arg);
	  SETFLOAT(&x->x_map.nomap[i], 0);
	  SETFLOAT(&x->x_map.onomap[i], 0);
	  SETFLOAT(&x->x_map.umap[x->umax], (t_float)i);
	  x->umax++;
	  x->max = i;
	}
    }
  if (x->max > 0 && x->outmap == 0)
    {
      outlet_list(x->mapout, gensym("list"), x->max+1, x->x_map.map);
      outlet_list(x->mapout, gensym("list"), x->max+1, x->x_map.omap);
    }
  else if (x->max > 0 && x->outmap == 1)
    {
      outlet_list(x->mapout, gensym("list"), x->max+1, x->x_map.nomap);
      outlet_list(x->mapout, gensym("list"), x->max+1, x->x_map.onomap);
    }
}

void sieve_mode(t_sieve *x, t_floatarg fmode)
{
  x->mode = fmode < 0 ? 0 : fmode > 3 ? 3 : fmode;
}

void sieve_debug(t_sieve *x, t_floatarg bug)
{
  t_float ele0, ele1, ele2, ele3, ele4, ele5, ele6, ele7, ele8, ele9;
  t_float nle0, nle1, nle2, nle3, nle4, nle5, nle6, nle7, nle8, nle9;
  ele0 = atom_getfloatarg(0, MAXENTRIES, x->x_map.map);
  ele1 = atom_getfloatarg(1, MAXENTRIES, x->x_map.map);
  ele2 = atom_getfloatarg(2, MAXENTRIES, x->x_map.map);
  ele3 = atom_getfloatarg(3, MAXENTRIES, x->x_map.map);
  ele4 = atom_getfloatarg(4, MAXENTRIES, x->x_map.map);
  ele5 = atom_getfloatarg(5, MAXENTRIES, x->x_map.map);
  ele6 = atom_getfloatarg(6, MAXENTRIES, x->x_map.map);
  ele7 = atom_getfloatarg(7, MAXENTRIES, x->x_map.map);
  ele8 = atom_getfloatarg(8, MAXENTRIES, x->x_map.map);
  ele9 = atom_getfloatarg(9, MAXENTRIES, x->x_map.map);
  nle0 = atom_getfloatarg(0, MAXENTRIES, x->x_map.nomap);
  nle1 = atom_getfloatarg(1, MAXENTRIES, x->x_map.nomap);
  nle2 = atom_getfloatarg(2, MAXENTRIES, x->x_map.nomap);
  nle3 = atom_getfloatarg(3, MAXENTRIES, x->x_map.nomap);
  nle4 = atom_getfloatarg(4, MAXENTRIES, x->x_map.nomap);
  nle5 = atom_getfloatarg(5, MAXENTRIES, x->x_map.nomap);
  nle6 = atom_getfloatarg(6, MAXENTRIES, x->x_map.nomap);
  nle7 = atom_getfloatarg(7, MAXENTRIES, x->x_map.nomap);
  nle8 = atom_getfloatarg(8, MAXENTRIES, x->x_map.nomap);
  nle9 = atom_getfloatarg(9, MAXENTRIES, x->x_map.nomap);
  post("mode = %f, max = %f", x->mode, x->max);
  post("first 10 elements = %f, %f, %f, %f, %f, %f, %f, %f, %f, %f", ele0, ele1, ele2, ele3, ele4, ele5, ele6, ele7, ele8, ele9);
  post("first 10 elements = %f, %f, %f, %f, %f, %f, %f, %f, %f, %f", nle0, nle1, nle2, nle3, nle4, nle5, nle6, nle7, nle8, nle9);
  x->myBug = bug;
}

void sieve_init(t_sieve *x, t_floatarg f)
{
  x->mod1 = 320;
  x->mod2 = 26364;
  x->mod3 = 1202;
  x->seed1 = 21717;
  x->seed2 = 23129;
  x->seed3 = 7886;
  //timeval tv;
  gettimeofday(&x->tv, 0);

  x->seed1 = (x->tv.tv_usec % x->mod1) + (x->tv.tv_sec % x->mod2);
  x->seed1 = x->seed1 % x->mod3;
  x->seed2 = (x->tv.tv_usec % x->mod2) + (x->tv.tv_sec % x->mod3);
  x->seed2 = x->seed2 % x->mod1;
  x->seed3 = (x->tv.tv_usec % x->mod3) + (x->tv.tv_sec % x->mod1);
  x->seed3 = x->seed3 % x->mod2;
    
  x->seed16v[2] = (unsigned short int)x->seed3;
  x->seed16v[1] = (unsigned short int)x->seed2;
  x->seed16v[0] = (unsigned short int)x->seed1;
  seed48(x->seed16v);
}

void *sieve_new(t_floatarg f, timeval tv) 
{
  t_sieve *x = (t_sieve *)pd_new(sieve_class);
  x->mode = f;
  x->max = x->umax = 0;
  x->outmap = 0;
  int i;
  for(i=0;i<MAXENTRIES;i++) 
    {
      SETFLOAT(x->x_map.map+i, 0);
      SETFLOAT(x->x_map.omap+i, 0);
      SETFLOAT(x->x_map.nomap+i, 1);
      SETFLOAT(x->x_map.onomap+i, 1);
      SETFLOAT(x->x_map.umap+i, -1);
    }

  x->mod1 = 1120;
  x->mod2 = 12364;
  x->mod3 = 3003;
  x->seed1 = 21217;
  x->seed2 = 33129;
  x->seed3 = 9286;
  //timeval x->tv;
  gettimeofday(&x->tv, 0);

  x->seed1 = (x->tv.tv_usec % x->mod1) + (x->tv.tv_sec % x->mod2);
  x->seed1 = x->seed1 % x->mod3;
  x->seed2 = (x->tv.tv_usec % x->mod2) + (x->tv.tv_sec % x->mod3);
  x->seed2 = x->seed2 % x->mod1;
  x->seed3 = (x->tv.tv_usec % x->mod3) + (x->tv.tv_sec % x->mod1);
  x->seed3 = x->seed3 % x->mod2;

    
  x->seed16v[2] = (unsigned short int)x->seed3;
  x->seed16v[1] = (unsigned short int)x->seed2;
  x->seed16v[0] = (unsigned short int)x->seed1;
  seed48(x->seed16v);
  //  srand(time(NULL));
  post("initializing pseudo-random number generator: %d, %d, %d",(int)x->seed1,(int)x->seed2,(int)x->seed3);

  x->mapped = outlet_new(&x->x_obj, &s_float);
  x->value = outlet_new(&x->x_obj, &s_float);
  x->mapout = outlet_new(&x->x_obj, &s_list);
  x->inst = outlet_new(&x->x_obj, &s_bang);
  return (void *)x;
}

void sieve_setup(void) 
{
  sieve_class = class_new(gensym("sieve"),
  (t_newmethod)sieve_new,
  0, sizeof(t_sieve),
  0, A_DEFFLOAT, 0);
  post("|^^^^^^^^^^^^^sieve^^^^^^^^^^^^^|");
  post("|->^^^integer map to floats^^^<-|");
  post("|^^^^^Edward Kelly 2006-2022^^^^|");

  class_addfloat(sieve_class, sieve_float);
  class_addmethod(sieve_class, (t_method)sieve_init, gensym("init"), A_DEFFLOAT, 0);
  class_addmethod(sieve_class, (t_method)sieve_markov, gensym("markov"), A_DEFFLOAT, 0);
  class_addmethod(sieve_class, (t_method)sieve_set, gensym("set"), A_DEFFLOAT, A_DEFFLOAT, 0);
  class_addmethod(sieve_class, (t_method)sieve_anneal, gensym("anneal"), A_DEFFLOAT, 0);
  class_addmethod(sieve_class, (t_method)sieve_weight, gensym("weight"), A_DEFFLOAT, A_DEFFLOAT, 0);
  class_addmethod(sieve_class, (t_method)sieve_map, gensym("map"), A_GIMME, 0);
  class_addmethod(sieve_class, (t_method)sieve_clear, gensym("clear"), A_DEFFLOAT, 0);
  class_addmethod(sieve_class, (t_method)sieve_get, gensym("get"), A_DEFFLOAT, 0);
  class_addmethod(sieve_class, (t_method)sieve_delete, gensym("delete"), A_DEFFLOAT, 0);
  class_addmethod(sieve_class, (t_method)sieve_shunt, gensym("shunt"), A_DEFFLOAT, 0);
  class_addmethod(sieve_class, (t_method)sieve_shift, gensym("shift"), A_DEFFLOAT, 0);
  class_addmethod(sieve_class, (t_method)sieve_insert, gensym("insert"), A_DEFFLOAT, A_DEFFLOAT, 0);
  class_addmethod(sieve_class, (t_method)sieve_mode, gensym("mode"), A_DEFFLOAT, 0);
  class_addmethod(sieve_class, (t_method)sieve_debug, gensym("debug"), A_DEFFLOAT, 0);
  class_addmethod(sieve_class, (t_method)sieve_umap, gensym("umap"), A_DEFFLOAT, 0);
  class_addmethod(sieve_class, (t_method)sieve_urn, gensym("urn"), A_DEFFLOAT, 0);
}
