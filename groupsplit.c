/* groupsplit
 * Copyright (c) 2019 Edward Kelly <morph_2016@yahoo.co.uk>
 * 
 * Part of the gemnotes live music notation system
 */
 
#include "m_pd.h"

#define SPLITS 256
#define GROUPS 64

t_class *groupsplit_class;

typedef struct _groupsplit
{
  t_object x_obj;
  t_atom splits[SPLITS]; // make iSplit the subgroup!
  t_atom numer[GROUPS];
  t_atom denom[GROUPS];
  t_atom tSig[2];
  t_atom gSig[2];
  t_float barNum, barDen, barPhase, tPhase, num, den;
  t_int group, rem, iStep, iRem, iSplit, tied, gCount, nCount, firstStep, mode, nGrps, myBug, nSplits, tSplit, allStep, inter, rested;
  t_outlet *step, *split, *tie, *newGroup, *bar, *groupSig, *beamStart;
} t_groupsplit;

void groupsplit_float(t_groupsplit *y, t_floatarg f)
{ //here is where we will calculate group-remainders
  //and output splits and tie values
  //...we could also make it output beam values
  //if we can set note / rests? How about subgroups?
  if(y->iStep <= 0) post("input must be > 0");
  else if(y->mode == 0 || y->mode == 2)
    {
      y->iStep = (t_int)f;
      if(y->firstStep == 1)
	{
	  outlet_bang(y->newGroup);
	  y->firstStep = 0;
	}
      else if(y->rem == 0)
	{
	  y->rem = y->group;
	  outlet_bang(y->newGroup);
	  y->gCount = 0;
	}
      while(y->iStep > y->rem)
	{
	  y->tied = 1;
	  y->allStep += y->rem;
	  if(y->mode == 2 || y->mode == 3) y->iSplit = (t_int)atom_getfloatarg(y->allStep,SPLITS,y->splits);
	  if(y->mode == 0) y->iSplit = (t_int)atom_getfloatarg(y->gCount,SPLITS,y->splits);
	  if(y->gCount == 0) outlet_float(y->beamStart, 0);
	  else if(y->rested == 0) outlet_float(y->beamStart, 1);
	  else if(y->rested == 1 && y->rem > 1)
	    { //we have to send a rest message before the step float (or bang)
	      //so that the beams do not start
	      outlet_float(y->beamStart, 0);
	      y->rested = 0;
	    }
	  else if(y->rested == 1 && y->rem == 1)
	    { //if there is only one note left give it a tail
	      outlet_float(y->beamStart, 2);
	      y->rested = 0;
	    }
	  outlet_float(y->tie, (t_float)y->tied);
	  outlet_float(y->split, (t_float)y->iSplit);
	  outlet_float(y->step, (t_float)y->rem);
	  outlet_bang(y->newGroup);
	  y->iStep -= y->rem;
	  y->rem = y->group;
	  y->gCount = 0;
	}
      if(y->iStep == y->rem)
	{
	  y->tied = 0;
	  y->allStep += y->rem;
	  if(y->mode == 2 || y->mode == 3) y->iSplit = (t_int)atom_getfloatarg(y->allStep,SPLITS,y->splits);
	  if(y->mode == 0) y->iSplit = (t_int)atom_getfloatarg(y->gCount,SPLITS,y->splits);
	  if(y->gCount == 0) outlet_float(y->beamStart, 0);
	  else if(y->rested == 0) outlet_float(y->beamStart, 1);
	  else if(y->rested == 1 && y->rem > 1)
	    { //we have to send a rest message before the step float (or bang)
	      //so that the beams do not start
	      outlet_float(y->beamStart, 0);
	      y->rested = 0;
	    }
	  else if(y->rested == 1 && y->rem == 1)
	    { //if there is only one note left give it a tail
	      outlet_float(y->beamStart, 2);
	      y->rested = 0;
	    }
	  outlet_float(y->tie, (t_float)y->tied);
	  outlet_float(y->split, (t_float)y->iSplit);
	  outlet_float(y->step, (t_float)y->rem);
	  outlet_bang(y->newGroup);
	  y->rem = y->group;
	  y->gCount = 0;
	}
      else if(y->iStep > 0)
	{
	  y->tied = 0;
	  y->rem -= y->iStep;
	  y->allStep += y->iStep;
	  if(y->mode == 2) y->iSplit = (t_int)atom_getfloatarg(y->allStep,SPLITS,y->splits);
	  if(y->mode == 0) y->iSplit = (t_int)atom_getfloatarg(y->gCount,SPLITS,y->splits);
	  if(y->gCount == 0) outlet_float(y->beamStart, 0);
	  else if(y->rested == 0) outlet_float(y->beamStart, 1);
	  else if(y->rested == 1 && y->rem > 1)
	    { //we have to send a rest message before the step float (or bang)
	      //so that the beams do not start
	      outlet_float(y->beamStart, 0);
	      y->rested = 0;
	    }
	  else if(y->rested == 1 && y->rem == 1)
	    { //if there is only one note left give it a tail
	      outlet_float(y->beamStart, 2);
	      y->rested = 0;
	    }
	  outlet_float(y->tie, (t_float)y->tied);
	  outlet_float(y->split, (t_float)y->iSplit);
	  outlet_float(y->step, (t_float)y->iStep);
	  y->gCount += y->iStep;
	}
    }
  else
    {
      if(y->firstStep == 1)
	{
	  y->tSplit = 0;
	  y->num = atom_getfloatarg(y->nCount, GROUPS, y->numer);
	  y->group = y->num;
	  y->den = atom_getfloatarg(y->nCount, GROUPS, y->denom);
	  SETFLOAT(&y->gSig[0],y->num);
	  SETFLOAT(&y->gSig[1],y->den);
	  outlet_list(y->groupSig, gensym("list"), 2, y->gSig);
	  outlet_bang(y->newGroup);
	  y->firstStep = 0;
	}
      else if(y->rem == 0)
	{
	  y->nCount++;
	  if(y->nCount >= y->nGrps)
	    {
	      y->nCount = 0;
	      y->tSplit = 0;
	    }
	  else y->tSplit += y->rem;
	  if(y->myBug) post("y->tSplit = %d",y->tSplit);
	  y->num = atom_getfloatarg(y->nCount, GROUPS, y->numer);
	  y->den = atom_getfloatarg(y->nCount, GROUPS, y->denom);
	  SETFLOAT(&y->gSig[0],y->num);
	  SETFLOAT(&y->gSig[1],y->den);
	  outlet_list(y->groupSig, gensym("list"), 2, y->gSig);
	  y->group = y->num;
	  y->rem = y->group;
	  y->gCount = 0;
	  outlet_bang(y->newGroup);
	}
      while(y->iStep > y->rem)
	{
	  y->tied = 1;
	  y->iSplit = (t_int)atom_getfloatarg(y->tSplit,SPLITS,y->splits);
	  if(y->gCount == 0) outlet_float(y->beamStart, 0);
	  else if(y->rested == 0) outlet_float(y->beamStart, 1);
	  else if(y->rested == 1 && y->rem > 1)
	    { //we have to send a rest message before the step float (or bang)
	      //so that the beams do not start
	      outlet_float(y->beamStart, 0);
	      y->rested = 0;
	    }
	  else if(y->rested == 1 && y->rem == 1)
	    { //if there is only one note left give it a tail
	      outlet_float(y->beamStart, 2);
	      y->rested = 0;
	    }
	  outlet_float(y->tie, (t_float)y->tied);
	  outlet_float(y->split, (t_float)y->iSplit);
	  outlet_float(y->step, (t_float)y->rem);
	  outlet_bang(y->newGroup);
	  y->iStep -= y->rem;
	  y->nCount++;
	  if(y->nCount >= y->nGrps)
	    {
	      y->nCount = 0;
	      y->tSplit = 0;
	    }
	  else y->tSplit += y->rem;
	  if(y->myBug) post("y->tSplit = %d",y->tSplit);
	  y->num = atom_getfloatarg(y->nCount, GROUPS, y->numer);
	  y->den = atom_getfloatarg(y->nCount, GROUPS, y->denom);
	  SETFLOAT(&y->gSig[0],y->num);
	  SETFLOAT(&y->gSig[1],y->den);
	  outlet_list(y->groupSig, gensym("list"), 2, y->gSig);
	  y->group = y->num;
	  y->rem = y->group;
	  y->gCount = 0;
	}
      if(y->iStep == y->rem)
	{
	  y->tied = 0;
	  y->iSplit = (t_int)atom_getfloatarg(y->tSplit,SPLITS,y->splits);
	  if(y->gCount == 0) outlet_float(y->beamStart, 0);
	  else if(y->rested == 0) outlet_float(y->beamStart, 1);
	  else if(y->rested == 1 && y->rem > 1)
	    { //we have to send a rest message before the step float (or bang)
	      //so that the beams do not start
	      outlet_float(y->beamStart, 0);
	      y->rested = 0;
	    }
	  else if(y->rested == 1 && y->rem == 1)
	    { //if there is only one note left give it a tail
	      outlet_float(y->beamStart, 2);
	      y->rested = 0;
	    }
	  outlet_float(y->tie, (t_float)y->tied);
	  outlet_float(y->split, (t_float)y->iSplit);
	  outlet_float(y->step, (t_float)y->rem);
	  outlet_bang(y->newGroup);
	  y->nCount++;
	  if(y->nCount >= y->nGrps)
	    {
	      y->nCount = 0;
	      y->tSplit = 0;
	    }
	  else y->tSplit += y->rem;
	  if(y->myBug) post("y->tSplit = %d",y->tSplit);
	  y->num = atom_getfloatarg(y->nCount, GROUPS, y->numer);
	  y->den = atom_getfloatarg(y->nCount, GROUPS, y->denom);
	  SETFLOAT(&y->gSig[0],y->num);
	  SETFLOAT(&y->gSig[1],y->den);
	  outlet_list(y->groupSig, gensym("list"), 2, y->gSig);
	  y->group = y->num;
	  y->rem = y->group;
	  y->gCount = 0;
	}
      else if(y->iStep > 0)
	{
	  y->tied = 0;
	  y->rem -= y->iStep;
	  y->iSplit = (t_int)atom_getfloatarg(y->tSplit,SPLITS,y->splits);
	  if(y->gCount == 0) outlet_float(y->beamStart, 0);
	  else if(y->rested == 0) outlet_float(y->beamStart, 1);
	  else if(y->rested == 1 && y->rem > 1)
	    { //we have to send a rest message before the step float (or bang)
	      //so that the beams do not start
	      outlet_float(y->beamStart, 0);
	      y->rested = 0;
	    }
	  else if(y->rested == 1 && y->rem == 1)
	    { //if there is only one note left give it a tail
	      outlet_float(y->beamStart, 2);
	      y->rested = 0;
	    }
	  outlet_float(y->tie, (t_float)y->tied);
	  outlet_float(y->split, (t_float)y->iSplit);
	  outlet_float(y->step, (t_float)y->iStep);
	  y->gCount += y->iStep;
	  y->tSplit += y->iStep;
	  if(y->myBug) post("y->tSplit = %d",y->tSplit);
	}
    }  
}
void groupsplit_bang(t_groupsplit *y)
{
  //just do a step
  if(y->mode == 0 || y->mode == 2)
    {
      if(y->firstStep == 1)
	{
	  outlet_bang(y->newGroup);
	  y->firstStep = 0;
	  y->gCount = 0;
	}
      else if(y->rem == 0)
	{
	  y->rem = y->group;
	  outlet_bang(y->newGroup);
	  y->gCount = 0;
	}
      while(y->iStep > y->rem)
	{
	  y->tied = 1;
	  y->allStep += y->rem;
	  if(y->mode == 2 || y->mode == 3) y->iSplit = (t_int)atom_getfloatarg(y->allStep,SPLITS,y->splits);
	  if(y->mode == 0) y->iSplit = (t_int)atom_getfloatarg(y->gCount,SPLITS,y->splits);
	  if(y->gCount == 0) outlet_float(y->beamStart, 0);
	  else if(y->rested == 0) outlet_float(y->beamStart, 1);
	  else if(y->rested == 1 && y->rem > 1)
	    { //we have to send a rest message before the step float (or bang)
	      //so that the beams do not start
	      outlet_float(y->beamStart, 0);
	      y->rested = 0;
	    }
	  else if(y->rested == 1 && y->rem == 1)
	    { //if there is only one note left give it a tail
	      outlet_float(y->beamStart, 2);
	      y->rested = 0;
	    }
	  outlet_float(y->tie, (t_float)y->tied);
	  outlet_float(y->split, (t_float)y->iSplit);
	  outlet_float(y->step, (t_float)y->rem);
	  outlet_bang(y->newGroup);
	  //how do we know if this is a rest?
	  y->iStep -= y->rem;
	  y->rem = y->group;
	  y->gCount = 0;
	}
      if(y->iStep == y->rem)
	{
	  y->tied = 0;
	  y->allStep += y->rem;
	  if(y->mode == 2 || y->mode == 3) y->iSplit = (t_int)atom_getfloatarg(y->allStep,SPLITS,y->splits);
	  if(y->mode == 0) y->iSplit = (t_int)atom_getfloatarg(y->gCount,SPLITS,y->splits);
	  if(y->gCount == 0) outlet_float(y->beamStart, 0);
	  else if(y->rested == 0) outlet_float(y->beamStart, 1);
	  else if(y->rested == 1 && y->rem > 1)
	    { //we have to send a rest message before the step float (or bang)
	      //so that the beams do not start
	      outlet_float(y->beamStart, 0);
	      y->rested = 0;
	    }
	  else if(y->rested == 1 && y->rem == 1)
	    { //if there is only one note left give it a tail
	      outlet_float(y->beamStart, 2);
	      y->rested = 0;
	    }
	  outlet_float(y->tie, (t_float)y->tied);
	  outlet_float(y->split, (t_float)y->iSplit);
	  outlet_float(y->step, (t_float)y->rem);
	  outlet_bang(y->newGroup);
	  y->rem = y->group;
	  y->gCount = 0;
	}
      else if(y->iStep > 0)
	{
	  y->tied = 0;
	  y->rem -= y->iStep;
	  y->allStep += y->iStep;
	  if(y->mode == 2) y->iSplit = (t_int)atom_getfloatarg(y->allStep,SPLITS,y->splits);
	  if(y->mode == 0) y->iSplit = (t_int)atom_getfloatarg(y->gCount,SPLITS,y->splits);
	  if(y->gCount == 0) outlet_float(y->beamStart, 0);
	  else if(y->rested == 0) outlet_float(y->beamStart, 1);
	  else if(y->rested == 1 && y->rem > 1)
	    { //we have to send a rest message before the step float (or bang)
	      //so that the beams do not start
	      outlet_float(y->beamStart, 0);
	      y->rested = 0;
	    }
	  else if(y->rested == 1 && y->rem == 1)
	    { //if there is only one note left give it a tail
	      outlet_float(y->beamStart, 2);
	      y->rested = 0;
	    }
	  outlet_float(y->tie, (t_float)y->tied);
	  outlet_float(y->split, (t_float)y->iSplit);
	  outlet_float(y->step, (t_float)y->iStep);
	  y->gCount += y->iStep;
	}
    }
  else
    {
      if(y->firstStep == 1)
	{
	  y->tSplit = 0;
	  y->num = atom_getfloatarg(y->nCount, GROUPS, y->numer);
	  y->group = y->num;
	  y->den = atom_getfloatarg(y->nCount, GROUPS, y->denom);
	  SETFLOAT(&y->gSig[0],y->num);
	  SETFLOAT(&y->gSig[1],y->den);
	  outlet_list(y->groupSig, gensym("list"), 2, y->gSig);
	  outlet_bang(y->newGroup);
	  y->firstStep = 0;
	}
      else if(y->rem == 0)
	{
	  y->nCount++;
	  if(y->nCount >= y->nGrps)
	    {
	      y->nCount = 0;
	      y->tSplit = 0;
	    }
	  y->num = atom_getfloatarg(y->nCount, GROUPS, y->numer);
	  y->den = atom_getfloatarg(y->nCount, GROUPS, y->denom);
	  SETFLOAT(&y->gSig[0],y->num);
	  SETFLOAT(&y->gSig[1],y->den);
	  outlet_list(y->groupSig, gensym("list"), 2, y->gSig);
	  y->group = y->num;
	  y->rem = y->group;
	  y->gCount = 0;
	  outlet_bang(y->newGroup);
	}
      while(y->iStep > y->rem)
	{
	  y->tied = 1;
	  y->iSplit = (t_int)atom_getfloatarg(y->tSplit,SPLITS,y->splits);
	  if(y->gCount == 0) outlet_float(y->beamStart, 0);
	  else if(y->rested == 0) outlet_float(y->beamStart, 1);
	  else if(y->rested == 1 && y->rem > 1)
	    { //we have to send a rest message before the step float (or bang)
	      //so that the beams do not start
	      outlet_float(y->beamStart, 0);
	      y->rested = 0;
	    }
	  else if(y->rested == 1 && y->rem == 1)
	    { //if there is only one note left give it a tail
	      outlet_float(y->beamStart, 2);
	      y->rested = 0;
	    }
	  outlet_float(y->tie, (t_float)y->tied);
	  outlet_float(y->split, (t_float)y->iSplit);
	  outlet_float(y->step, (t_float)y->rem);
	  y->iStep -= y->rem;
	  y->nCount++;
	  if(y->nCount >= y->nGrps)
	    {
	      y->nCount = 0;
	      y->tSplit = 0;
	    }
	  else y->tSplit += y->rem;
	  y->num = atom_getfloatarg(y->nCount, GROUPS, y->numer);
	  y->den = atom_getfloatarg(y->nCount, GROUPS, y->denom);
	  SETFLOAT(&y->gSig[0],y->num);
	  SETFLOAT(&y->gSig[1],y->den);
	  outlet_list(y->groupSig, gensym("list"), 2, y->gSig);
	  outlet_bang(y->newGroup);
	  y->group = y->num;
	  y->rem = y->group;
	  y->gCount = 0;
	}
      if(y->iStep == y->rem)
	{
	  y->tied = 0;
	  y->iSplit = (t_int)atom_getfloatarg(y->tSplit,SPLITS,y->splits);
	  if(y->gCount == 0) outlet_float(y->beamStart, 0);
	  else if(y->rested == 0) outlet_float(y->beamStart, 1);
	  else if(y->rested == 1 && y->rem > 1)
	    { //we have to send a rest message before the step float (or bang)
	      //so that the beams do not start
	      outlet_float(y->beamStart, 0);
	      y->rested = 0;
	    }
	  else if(y->rested == 1 && y->rem == 1)
	    { //if there is only one note left give it a tail
	      outlet_float(y->beamStart, 2);
	      y->rested = 0;
	    }
	  outlet_float(y->tie, (t_float)y->tied);
	  outlet_float(y->split, (t_float)y->iSplit);
	  outlet_float(y->step, (t_float)y->rem);
	  y->nCount++;
	  if(y->nCount >= y->nGrps)
	    {
	      y->nCount = 0;
	      y->tSplit = 0;
	    }
	  else y->tSplit += y->rem;
	  y->num = atom_getfloatarg(y->nCount, GROUPS, y->numer);
	  y->den = atom_getfloatarg(y->nCount, GROUPS, y->denom);
	  SETFLOAT(&y->gSig[0],y->num);
	  SETFLOAT(&y->gSig[1],y->den);
	  outlet_list(y->groupSig, gensym("list"), 2, y->gSig);
	  outlet_bang(y->newGroup);
	  y->group = y->num;
	  y->rem = y->group;
	  y->gCount = 0;
	}
      else if(y->iStep > 0)
	{
	  y->tied = 0;
	  y->rem -= y->iStep;
	  y->iSplit = (t_int)atom_getfloatarg(y->tSplit,SPLITS,y->splits);
	  if(y->gCount == 0) outlet_float(y->beamStart, 0);
	  else if(y->rested == 0) outlet_float(y->beamStart, 1);
	  else if(y->rested == 1 && y->rem > 1)
	    { //we have to send a rest message before the step float (or bang)
	      //so that the beams do not start
	      outlet_float(y->beamStart, 0);
	      y->rested = 0;
	    }
	  else if(y->rested == 1 && y->rem == 1)
	    { //if there is only one note left give it a tail
	      outlet_float(y->beamStart, 2);
	      y->rested = 0;
	    }
	  outlet_float(y->tie, (t_float)y->tied);
	  outlet_float(y->split, (t_float)y->iSplit);
	  outlet_float(y->step, (t_float)y->iStep);
	  y->gCount += y->iStep;
	  y->tSplit += y->iStep;
	}
    }  
}

void groupsplit_group(t_groupsplit *y, t_floatarg f)
{
  if(f >= 1)
    {
      y->rem = (t_int)f;
      y->group = (t_int)f;
      y->gCount = 0;
      outlet_bang(y->newGroup);
    }
  else post("group must be at least 1");
}

void groupsplit_rest(t_groupsplit *y, t_floatarg f)
{
  y->rested = 1;
}

void groupsplit_bar(t_groupsplit *y, t_symbol *s, int argc, t_atom *argv)
{
  int i, j;
  if(argc * 0.5 <= 0 || argc * 0.5 >= GROUPS) post("You must specify a number of pairs of items from 1 to %d",GROUPS);
  else
    {
      y->barNum = atom_getfloat(argv);
      y->barDen = atom_getfloat(argv+1);
      if(y->myBug) post("barNum = %d, barDen = %d, we made it here", (t_int)y->barNum, (t_int)y->barDen); 
      if(y->barNum <= 0 || y->barDen <= 0) post("ERROR! Bar time signature values must both be > 0");
      else
	{
	  y->nGrps = 0;
	  SETFLOAT(&y->tSig[0],y->barNum);
	  SETFLOAT(&y->tSig[1],y->barDen);
	  outlet_list(y->bar, gensym("list"), 2, y->tSig);
	  y->barPhase = y->barNum / y->barDen;
	  if(y->myBug) post("barNum = %d, barDen = %d, barPhase = %f", (t_int)y->barNum, (t_int)y->barDen, y->barPhase); 
	  for(i=1; i < argc; i++)
	    {
	      j = i * 2;
	      y->num = atom_getfloat(argv + j);
	      y->den = atom_getfloat(argv + j + 1);
	      if(y->den <= 0)
		{
		  post("denominators for time signature fractions must be > 0!");
		  break;
		}
	      else if(y->num <= 0)
		{
		  post("numerators for time signatures must be > 0!");
		}
	      else
		{
		  if(y->myBug)
		    post("i = %d, j = %d, y->num = %f, y->den = %f",i,j,y->num,y->den);
		  SETFLOAT(&y->numer[i-1],y->num);
		  SETFLOAT(&y->denom[i-1],y->den);
		  y->tPhase += y->num / y->den;
		  y->nGrps++;
		  if(y->myBug)
		    post("y->nGrps = %d",y->nGrps);
		}
	      if(y->tPhase >= y->barPhase)
		break;
	      //anything left over is splits
	      //note that the above contains a > so that if a stupid mistake is made in
	      //the group splits and the last one goes over the bar, 
	    }
	  post("bar set: %d / %d, total bar phase = %f",(int)y->barNum,(int)y->barDen,y->barPhase);
	  if(j + 2 < argc - 1)
	    {//the rest are splits
	      y->nSplits = 0;
	      j += 2;
	      for(i = j; i < argc; i++)
		{
		  y->nSplits++;
		  if(y->myBug) post("i = %d, j = %d, split = %f",i,j,atom_getfloat(argv+i));
		  SETFLOAT(&y->splits[i - j],atom_getfloat(argv+i));
		}
	      post("Set splits after the bar declaration!");
	    }
	}
    }
}


void groupsplit_splits(t_groupsplit *y, t_symbol *s, int argc, t_atom *argv)
{
  int i;
  if(argc <= 0 || argc >= SPLITS) post("You must specify a number of items from 1 to %d",SPLITS);
  else
    {
      for(i = 0; i < argc; i++)
	{
	  SETFLOAT(&y->splits[i], atom_getfloat(argv+i));
	}
    }
}

void groupsplit_mode(t_groupsplit *y, t_floatarg f)
{
  if(f == 0) y->mode = 0;
  else if(f == 1) y->mode = 1;
  else if(f == 2) y->mode = 2;
  else if(f == 3) y->mode = 3;
  else post("groupsplit mode must be 0, 1, 2 or 3!");
}

void groupsplit_reset(t_groupsplit *y, t_floatarg f)
{
  y->allStep = 0;
}

void groupsplit_debug(t_groupsplit *y, t_floatarg f)
{
  y->myBug = f != 0 ? 1 : 0;
}

void *groupsplit_new(t_floatarg f)
{
  t_groupsplit *y = (t_groupsplit *)pd_new(groupsplit_class);
  //  t_int group, rem, iStep, iRem, iSplit, tied, gCount;
  y->group = 4;
  y->rem = 4;
  y->iStep = 1;
  y->iSplit = 0;
  y->tied = 0;
  y->firstStep = 1;
  for(y->gCount = 0; y->gCount < SPLITS; y->gCount++)
    {
      SETFLOAT(&y->splits[y->gCount],0);
    }
  for(y->gCount = 0; y->gCount < GROUPS; y->gCount++)
    {
      SETFLOAT(&y->numer[y->gCount],4);//defaults
      SETFLOAT(&y->denom[y->gCount],16);
    }
  SETFLOAT(&y->tSig[0],4);
  SETFLOAT(&y->tSig[1],4);

  y->gCount = 0;
  y->barNum = 4;
  y->barDen = 4;
  y->barPhase = 1;
  y->tPhase = 0;
  y->num = 4;
  y->den = 16;
  y->nGrps = 4;
  y->nCount = 0;

  y->myBug = 0;
  y->mode = 0;
  
  y->step = outlet_new(&y->x_obj, gensym("float"));
  y->split = outlet_new(&y->x_obj, gensym("float"));
  y->tie = outlet_new(&y->x_obj, gensym("float"));
  y->newGroup = outlet_new(&y->x_obj, gensym("bang"));
  y->bar = outlet_new(&y->x_obj, gensym("list"));
  y->groupSig = outlet_new(&y->x_obj, gensym("list"));
  y->beamStart = outlet_new(&y->x_obj, gensym("float"));
  return(void *)y;
}

void groupsplit_setup(void) 
{
  groupsplit_class = class_new(gensym("groupsplit"),
  (t_newmethod)groupsplit_new,
  0, sizeof(t_groupsplit),
  0, A_DEFFLOAT, 0);
  post("groupsplit - part of the gemnotes system");

  class_addbang(groupsplit_class, groupsplit_bang);
  class_addfloat(groupsplit_class, groupsplit_float);
  class_addmethod(groupsplit_class, (t_method)groupsplit_splits, gensym("splits"), A_GIMME, 0);
  class_addmethod(groupsplit_class, (t_method)groupsplit_group, gensym("group"), A_DEFFLOAT, 0);
  class_addmethod(groupsplit_class, (t_method)groupsplit_mode, gensym("mode"), A_DEFFLOAT, 0);
  class_addmethod(groupsplit_class, (t_method)groupsplit_reset, gensym("reset"), A_DEFFLOAT, 0);
  class_addmethod(groupsplit_class, (t_method)groupsplit_rest, gensym("rest"), A_DEFFLOAT, 0);
  class_addmethod(groupsplit_class, (t_method)groupsplit_bar, gensym("bar"), A_GIMME, 0);
  class_addmethod(groupsplit_class, (t_method)groupsplit_debug, gensym("debug"), A_DEFFLOAT, 0);
}
