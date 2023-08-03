/* phasorbars~
 * gives 8th, 16th and 24th pulses out from a phasor based signal
 * so that sections can be used in the beatslicing of audio
 * sychronized to changing tempos for the table-reading signal
 * across multiple bars**

 **but of course limited to 4000000 samples (about 90s). Bring on pd-double!
 * Copyright (c) 2005-2023 Edward Kelly
 * Forinformaion on usage and distribution, and for a DICLAIMER OF ALL 
 * WARRANTIES, see the file "LICENSE.txt," in this distribution. */


#include "m_pd.h"
#include <math.h>

#define UNITBIT32 1572864.  /* 3*2^19; bit 32 has place value 1 */

    /* machine-dependent definitions.  These ifdefs really
    should have been by CPU type and not by operating system! */
#ifdef IRIX
    /* big-endian.  Most significant byte is at low address in memory */
#define HIOFFSET 0    /* word offset to find MSB */
#define LOWOFFSET 1    /* word offset to find LSB */
#define int32 long  /* a data type that has 32 bits */
#endif /* IRIX */

#ifdef MSW
    /* little-endian; most significant byte is at highest address */
#define HIOFFSET 1
#define LOWOFFSET 0
#define int32 long
#endif

#if defined(__FreeBSD__) || defined(__APPLE__)
#include <machine/endian.h>
#endif

#ifdef __linux__
#include <endian.h>
#endif

#if defined(__unix__) || defined(__APPLE__)
#if !defined(BYTE_ORDER) || !defined(LITTLE_ENDIAN)                         
#error No byte order defined                                                    
#endif                                                                          

#if BYTE_ORDER == LITTLE_ENDIAN                                             
#define HIOFFSET 1                                                              
#define LOWOFFSET 0                                                             
#else                                                                           
#define HIOFFSET 0    /* word offset to find MSB */                             
#define LOWOFFSET 1    /* word offset to find LSB */                            
#endif /* __BYTE_ORDER */                                                       
#include <sys/types.h>
#define int32 int32_t
#endif /* __unix__ or __APPLE__*/

union tabfudge
{
    double tf_d;
    int32 tf_i[2];
};

/* -------------------------- phasorbars~ ------------------------------ */
static t_class *phasorbars_class;

typedef struct _phasorbars
{
  t_object x_obj;
  double x_phase;

  t_float x_conv; /* 1 / the sample rate */
  t_float x_f;      /* scalar frequency */
  // RAMP variables
  t_float value, prevalue, ivalue, cycle, loop, pflag, nextreset, lastcycle, setphase;
  // BAR variables
  t_float looping, wasLooping, barnumber, bflag, nextbar, length, prevLength, baroffset, countstart, lastbar, cflag, forcebar;
  // BEAT variables
  t_float sxtnths,twfths,egths,evns,eght,sqLimit,sqtLimit;
  int sixtn, twfr, getEight, evn, hasLooped, abletonLink;

  t_outlet *x_bar, *x_cycle, *x_8a, *x_8b, *x_16, *x_24, *evens, *looped, *linked;
  t_clock *x_clock;           /* wakeup for message output */

  int semiquavers, sqtriplets;
  int isEight, isSixteen, isTwentyFour, isEven, isBar, isCycle;
  int dspon;
} t_phasorbars;

void phasorbars_tick(t_phasorbars *y)  
{
  if(y->countstart)
    {
      /*      if(y->hasLooped){
	outlet_bang(y->looped);
	y->hasLooped = 0;
	} */
      if(y->isBar){
	outlet_float(y->x_bar,y->barnumber);
	y->isBar = 0;
      }
      if(y->isCycle){
	outlet_float(y->x_cycle, y->cycle);
	y->isCycle = 0;
      }
      if(y->isEight){
	outlet_float(y->x_8b, y->egths);
	outlet_float(y->x_8a, y->egths);
	y->isEight = 0;
      }
      if(y->isEven){
	outlet_float(y->evens,y->evns);
	y->isEven = 0;
      }
      if(y->isSixteen){
	outlet_float(y->x_16, y->sxtnths);
	y->isSixteen = 0;
      }
      if(y->isTwentyFour){
	outlet_float(y->x_24, y->twfths);
	y->isTwentyFour = 0;
      }
    } else {
	y->isBar = 0;
	y->isCycle = 0;
	y->isEight = 0;
	y->isEven = 0;
	y->isSixteen = 0;
	y->isTwentyFour = 0;
  }
  /*if(y->cflag)
    {
      float f_phase = y->setphase;
      int pos = (int)f_phase;
      float position = (float)pos;
      f_phase -= position;
      y->x_phase = f_phase;
      y->cycle = position;
      y->bflag = 1;
      }*/
    //post("bang");
}

t_int *phasorbars_perform(t_int *w)
{
  t_phasorbars *y = (t_phasorbars *)(w[1]);
  t_float *in = (t_float *)(w[2]);
  t_float *out = (t_float *)(w[3]);
  int n = (int)(w[4]);
  double dphase = y->x_phase + UNITBIT32;
  union tabfudge tf;
  int normhipart;

  t_float conv = y->x_conv;

  //  float pflag = y->pflag;

  tf.tf_d = UNITBIT32;
  normhipart = tf.tf_i[HIOFFSET];
  tf.tf_d = dphase;
  while (n--) 
    {
      //    if(y->dspon) 
      //	{ 
	  tf.tf_i[HIOFFSET] = normhipart;
	  y->ivalue = *in++;
	  dphase += y->ivalue * conv; //increment
	  y->value = tf.tf_d - UNITBIT32;
      //      y->barnumber = y->cycle + y->baroffset;

	  //	  if(y->pflag && y->countstart)
	  if(y->countstart)
	    {
	      //	      if(y->ivalue > 0)
	      //		{
		  if(!y->bflag)
		    {
		      if(y->abletonLink > 0) // Added July 2016 to facilitate Ableton Link
			{
			  if(y->cycle == 0 && y->twfths == 0)
			    {
			      outlet_float(y->linked,y->value);
			      //			      y->cycle = 0;
			      y->value == 0;
			      /*			      if(!y->isBar)
				{
				  y->isBar = 1;
				  y->isCycle = 1;
				  y->isEight = 1;
				  y->isEven = 1;
				  y->isSixteen = 1;
				  y->isTwentyFour = 1;
				  clock_delay(y->x_clock,0L);
				  } */
			    }
			  else if(y->cycle == y->length - 1 && y->twfths == 23)
			    {
			      outlet_float(y->linked,y->value);
			      //			      y->cycle = 0;
			      y->value == 0;
			      /*			      y->barnumber = y->cycle + y->baroffset;
			      if(!y->looping)
				{
				  y->barnumber = y->baroffset + y->length;
				  y->cycle = 0;
				}
			      else
				{
				  y->barnumber = y->cycle + y->baroffset;
				  if(y->looping != y->wasLooping)
				    {
				      y->cycle = (float)((int)y->cycle % (int)y->length);
				      y->wasLooping = y->looping;
				    }
				}
			      if(!y->isBar)
				{
				  y->isBar = 1;
				  y->isCycle = 1;
				  y->isEight = 1;
				  y->isEven = 1;
				  y->isSixteen = 1;
				  y->isTwentyFour = 1;
				  clock_delay(y->x_clock,0L);
				  } */

			    }
			  y->abletonLink = 0;
			}
		      if(y->value < y->prevalue) 
			{
			  //post("y->cycle start = %f, y->baroffset start = %f",y->cycle,y->baroffset);
			  y->cycle++;
			  y->barnumber = y->cycle + y->baroffset;

			  //			  if(!y->looping) 
			  //			    {
			      if(y->nextbar >= 0) 
				{
				  y->barnumber = y->nextbar;
				  y->baroffset = y->nextbar;
				  y->cycle = 0;
				  y->nextbar = -1;
				  y->forcebar = -1;
				}
			      else if(y->forcebar >= 0)
				{
				  y->barnumber = y->forcebar;
				  y->cycle = y->forcebar - y->baroffset;
				  y->forcebar = -1;
				  y->nextbar = -1;
				}
			      //			    }

			      if(y->cycle >= y->length)
				{
				  y->cycle = 0;
			      //y->hasLooped = 1;
				  if(!y->looping)
				    {
				      y->barnumber = y->baroffset + y->length;
				      y->cycle = 0;
				    }
				  else
				    {
				      y->barnumber = y->cycle + y->baroffset;
				      if(y->looping != y->wasLooping)
					{
					  y->cycle = (float)((int)y->cycle % (int)y->length);
					  y->wasLooping = y->looping;
					}
				    }
				}
			  //post("y->cycle end = %f, y->baroffset end = %f",y->cycle,y->baroffset);
			      if(!y->isBar)
				{
				  y->isBar = 1;
				  y->isCycle = 1;
				  y->isEight = 1;
				  y->isEven = 1;
				  y->isSixteen = 1;
				  y->isTwentyFour = 1;
				  clock_delay(y->x_clock,0L);
				}
			}
		      /*		      else
			{
			  y->barnumber = y->cycle + y->baroffset;
			  } */
		    }
		  else 
		    {
		      y->bflag=0;
		    }
		  //		}
		  //  }
  
	/*	else if(y->ivalue < 0) {
	//	    } */
	  //	  y->pflag = 1;
	  *out++ = y->value + y->cycle;
    // here are the CLOCKS
    // and you can use the EVENS outlet to make SWING happen
	  int squs = (int)(y->value * 16);
	  int squts = (int)(y->value * 24);
	  if(squs != y->semiquavers)
	    {
	      int even = squs % 2;
	      int get8ths = squs * 0.5;
	      t_float eighths = (float)get8ths;
	      if (!even)
		{
	    //	    outlet_float(y->x_8b, eighths);
	    //	    outlet_float(y->x_8a, eighths);
	    //	    outlet_float(y->evens, 0);
		  y->egths = eighths;
		  y->evns = 0;
		  if(!y->isEight) 
		    {
		      y->isEight = 1;
		      y->isEven = 1;
		      y->isSixteen = 1;
		      y->isTwentyFour = 1;
		      clock_delay(y->x_clock,0L);
		    }
		}
	      else
		{
	  //	  outlet_float(y->evens, 1);
		  y->evns = 1;
		  if(!y->isEven)
		    {
		      y->isEven = 1;
		      clock_delay(y->x_clock, 0L);
		    }
		}
	//	outlet_float(y->x_16, (t_float)squs);
	      y->sxtnths = (t_float)squs;
	      if(!y->isSixteen) 
		{
		  y->isSixteen = 1;
		  if(!(squs % 2)) 
		    {
		      y->isTwentyFour = 1;
		    }
		  clock_delay(y->x_clock, 0L);
		}
	    }
	  else if(squts != y->sqtriplets) 
	    {
      //      outlet_float(y->x_24, (float)squts);
	      y->twfths = (t_float)squts;
	      if(!y->isTwentyFour) 
		{
		  y->isTwentyFour = 1;
		  clock_delay(y->x_clock, 0L);
		}
	    }
	  y->semiquavers = squs;
	  y->sqtriplets = squts;
	  tf.tf_d = dphase;
	  y->prevalue = y->value;
	}
    
    // Here is (a possible) problem!
    // When (x->dspon == 0) this runs...
    // maybe this should go! And countstart should be used to make the sound happen??
      else
	{
	  *out++ = y->value + y->cycle;
	}
      tf.tf_i[HIOFFSET] = normhipart;
      y->x_phase = tf.tf_d - UNITBIT32;
    // ...and when while(n--) isn't happening, nothing runs!
    }
  return (w+5);
}
/*
typedef struct _phasorbars
{

  // RAMP variables
  t_float value, prevalue, ivalue, cycle, loop, pflag, nextreset, lastcycle;
  // BAR variables
  t_float looping, barnumber, bflag, nextbar, length, baroffset, countstart, lastbar, cflag;
*/
void phasorbars_debug(t_phasorbars *y, t_floatarg mybug)
{
  if(mybug==0)
    {
      post("y->baroffset = %d",(int)y->baroffset);
      post("y->cycle     = %d",(int)y->cycle);
      post("y->barnumber = %d",(int)y->barnumber);
      post("y->nextbar   = %d",(int)y->nextbar);
    }
}

void phasorbars_dsp(t_phasorbars *y, t_signal **sp)
{
  y->x_conv = 1./sp[0]->s_sr;
  dsp_add(phasorbars_perform, 4, y, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

void phasorbars_nextcycle(t_phasorbars *y, t_float f)
{
  y->nextreset = (int)f;
  //  post("nextcycle = %d,",y->nextreset);
}

void phasorbars_on(t_phasorbars *y, t_float f)
{
  y->dspon = (int)f;
  //  post("nextcycle = %d,",y->nextreset);
}

void phasorbars_setcycle(t_phasorbars *y, t_float f)
{
  y->cycle = f;
  //  post("cycle = %d,",y->cycle);
}

void phasorbars_loop(t_phasorbars *y, t_float f)
{
  y->loop = f;
  //  post("loop = %f,",y->loop);
}

/*
void phasorbars_ft1(t_phasorbars *y, float f)
{
    float fcycles = 0;
    int cycles = 0;
    if (f < 0) f = 0;
    cycles = (int)f;
    y->cycle = cycles % (int)y->length;
    fcycles = (t_float)cycles;
    y->barnumber = (t_float)cycles;
    f = f - fcycles;
    y->x_phase = f;
    //    y->cycle = fcycles;
    //    y->pflag = 0;
    y->cflag = 1;
    //    outlet_float(y->x_bar, fcycles + y->baroffset);
    //    post("phase = %f, cycles = %f, bar = %f",y->x_phase,fcycles,y->baroffset);
}
*/
void phasorbars_forcebar(t_phasorbars *y, t_float f) // this is a KLUDGE
{
  y->forcebar = f;
}

void phasorbars_forgetit(t_phasorbars *y, t_float f) // this is a KLUDGE
{
  y->forcebar = 0;
}

void phasorbars_setphase(t_phasorbars *y, t_float f)
{
  //y->setphase = f;
  //y->cflag = 1;
  int position = (int)f;
  t_float pos = (float)position;
  t_float phase = f - position;
  y->cycle = pos;
  y->x_phase = phase;
  y->bflag=1;
  //clock_delay(y->x_clock, 0L);
}

void phasorbars_loopbars(t_phasorbars *y, t_float f)
{
  y->wasLooping = y->looping;
  y->looping = f != 0 ? 1 : 0;
}

void phasorbars_setbar(t_phasorbars *y, t_float f)
{
  y->barnumber = f;
  outlet_float(y->x_bar,y->barnumber);
}

void phasorbars_length(t_phasorbars *y, t_float f)
{
  y->prevLength = y->length;
  y->length = f;
  //added on 2nd May 2016
  if(y->cycle >= y->length)
    {
      y->cycle = (float)((int)y->cycle % (int)y->length);
    }
  //  post("loop length = %f",y->length);
}

void phasorbars_nextbar(t_phasorbars *y, t_float f)
{
  y->nextbar = f;
  //  post("nextbar = %f,",y->nextbar);
}

void phasorbars_baroffset(t_phasorbars *y, t_float f)
{
  //  float ext_cycles;
  /*
  post("before: cycle = %f, barnumber = %f, baroffset = %f",y->cycle,y->barnumber,y->baroffset);
  y->cycle = (float)((int)(y->barnumber - f) % (int)y->length);
  y->baroffset = f;
  y->barnumber = y->baroffset + y->cycle;
  y->barnumber++;
  post("after: cycle = %f, barnumber = %f, baroffset = %f",y->cycle,y->barnumber,y->baroffset);
  */
  y->baroffset = f >= 0 ? f : 0;
  //  post("bar_offset = %f,",y->baroffset);
}

void phasorbars_countstart(t_phasorbars *y, t_float f)
{
  y->countstart = f != 0 ? 1 : 0;
  /*  if(f == 0) {
    y->lastbar = y->barnumber;
    y->lastcycle = y->cycle; // because this is incremented at the beginning (so no -1)
    //    post("stopped:bar = %f",y->barnumber);
  } else {
    if(y->nextbar >= 0) {
      y->barnumber = y->nextbar;
      y->baroffset = y->nextbar;
      y->cycle = 0;
    } else {
      y->barnumber = y->lastbar;
      if(y->pflag) y->cycle = (int)(y->lastcycle + y->length) % (int)y->length;
    }
    //    post("startcount:bar = %f, nextbar = %f",y->barnumber,y->nextbar);
  }
  if(y->barnumber < 0) y->barnumber = 0; */
}


void phasorbars_free(t_phasorbars *x)
{
    clock_free(x->x_clock);
}

void phasorbars_link(t_phasorbars *x)
{
  //set so that on a downbeat, if link is received, phase is set to 0, cycle to 0 and the
  //linked outlet is banged
  //or, if we are at the end of a loop and y->twfths == 23
  x->abletonLink = 1;
}

void *phasorbars_new(t_symbol *s,int argc,t_atom* argv)
{
    t_phasorbars *y = (t_phasorbars *)pd_new(phasorbars_class);
    
    y->x_f = 0;
    if (argc) y->x_f = atom_getfloat(argv++),argc--;
    
    inlet_new(&y->x_obj, &y->x_obj.ob_pd, &s_float, gensym("setphase"));
    //    floatinlet_new(&y->x_obj, &y->cycle);

    y->x_phase = 0;
    y->x_conv = 0;
    y->cycle = 0;
    y->pflag = 0;
    y->nextreset = -1;
    y->nextbar = -1;
    y->semiquavers = -1;
    y->sqtriplets = -1;

    y->ivalue = 10;
    y->value = -1;
    y->prevalue = -2;
    y->length = 8;
    y->prevLength = 8;
    y->baroffset = 0;
    y->barnumber = 0;
    y->bflag = 0;
    y->cflag = 0;
    y->countstart = 0;
    y->forcebar = -1;
    y->wasLooping = 0;

    y->isEight = 0;
    y->isSixteen = 0;
    y->isTwentyFour = 0;
    y->isEven = 0;
    y->isBar = 0;
    y->isCycle = 0;
    y->hasLooped = 0;
    y->dspon = 1;

    y->abletonLink = 0;

    outlet_new(&y->x_obj, gensym("signal"));
    
    y->loop = 0;
    if (argc) y->length = atom_getfloat(argv++),argc--;
    if (argc) y->loop = atom_getfloat(argv++),argc--;
     
    //  y->state = 0;

    y->x_24 = outlet_new(&y->x_obj, &s_float);
    y->x_16 = outlet_new(&y->x_obj, &s_float);
    y->evens = outlet_new(&y->x_obj, &s_float);
    y->x_8a = outlet_new(&y->x_obj, &s_float);
    y->x_8b = outlet_new(&y->x_obj, &s_float);
    y->x_cycle = outlet_new(&y->x_obj, &s_float);
    y->x_bar = outlet_new(&y->x_obj, &s_float);
    y->looped = outlet_new(&y->x_obj, &s_bang);
    y->linked = outlet_new(&y->x_obj, &s_float);

    y->x_clock = clock_new(y, (t_method)phasorbars_tick);

    return (y);
}

void phasorbars_tilde_setup(void)
{
    phasorbars_class = class_new(gensym("phasorbars~"), (t_newmethod)phasorbars_new, 0, sizeof(t_phasorbars), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(phasorbars_class, t_phasorbars, x_f);
    class_addmethod(phasorbars_class, (t_method)phasorbars_dsp, gensym("dsp"), 0);
    //    class_addmethod(phasorbars_class, (t_method)phasorbars_ft1,
    //        gensym("ft1"), A_FLOAT, 0);
    class_addmethod(phasorbars_class, (t_method)phasorbars_setphase,
        gensym("setphase"), A_FLOAT, 0);
    class_addmethod(phasorbars_class, (t_method)phasorbars_forcebar,
        gensym("forcebar"), A_FLOAT, 0);
    class_addmethod(phasorbars_class, (t_method)phasorbars_forgetit,
        gensym("forgetit"), A_FLOAT, 0);
    class_addmethod(phasorbars_class, (t_method)phasorbars_loop,
        gensym("loop"), A_FLOAT, 0);
    class_addmethod(phasorbars_class, (t_method)phasorbars_loopbars,
        gensym("loopbars"), A_FLOAT, 0);
    class_addmethod(phasorbars_class, (t_method)phasorbars_length,
        gensym("length"), A_FLOAT, 0);
    class_addmethod(phasorbars_class, (t_method)phasorbars_setcycle,
        gensym("setcycle"), A_FLOAT, 0);
    class_addmethod(phasorbars_class, (t_method)phasorbars_nextcycle,
        gensym("nextcycle"), A_FLOAT, 0);
    class_addmethod(phasorbars_class, (t_method)phasorbars_setbar,
        gensym("setbar"), A_FLOAT, 0);
    class_addmethod(phasorbars_class, (t_method)phasorbars_nextbar,
        gensym("nextbar"), A_FLOAT, 0);
    class_addmethod(phasorbars_class, (t_method)phasorbars_baroffset,
        gensym("baroffset"), A_FLOAT, 0);
    class_addmethod(phasorbars_class, (t_method)phasorbars_countstart,
        gensym("countstart"), A_FLOAT, 0);
    class_addmethod(phasorbars_class, (t_method)phasorbars_on,
        gensym("on"), A_FLOAT, 0);
    class_addmethod(phasorbars_class, (t_method)phasorbars_link,
        gensym("link"), A_GIMME, 0);

//    post("phasorbars~ - a bar/beat counter unified with");
//    post("a multi-bar phasor");

}

