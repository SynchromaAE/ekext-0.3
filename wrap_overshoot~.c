#include "m_pd.h"

typedef struct wrap_overshoot_tilde
{
    t_object x_obj;
    t_float x_f;
  int k_i, lastK, wLimit;
  t_float token, shootFlag, limitFlag, offset, lastLimit;
  t_sample f_s;
  t_outlet *phase;
} t_wrap_overshoot_tilde;

t_class *wrap_overshoot_tilde_class;


static void *wrap_overshoot_tilde_new(void)
{
    t_wrap_overshoot_tilde *x = (t_wrap_overshoot_tilde *)pd_new(wrap_overshoot_tilde_class);
    outlet_new(&x->x_obj, gensym("signal"));
    x->phase = outlet_new(&x->x_obj, &s_float);
    floatinlet_new (&x->x_obj, &x->shootFlag);
    x->x_f = 0;
    x->shootFlag = 1;
    x->f_s = 0;
    x->k_i = 0;
    x->lastK = 0;
    x->offset = 0;
    x->limitFlag = 0;
    x->lastLimit = 0;
    return (x);
}

static t_int *wrap_overshoot_tilde_perform(t_int *w)
{
    t_wrap_overshoot_tilde *x = (t_wrap_overshoot_tilde *)(w[1]);
    t_sample *in  = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    t_int n       = (t_int)(w[4]);
    while(n--)
    {
        x->f_s = *in++;
        x->k_i = x->f_s;
	if(x->k_i > x->lastK && x->shootFlag != 0 && x->limitFlag == 0)
	  {
	    *out++ = x->f_s - x->k_i+1 + x->offset;
	  }
	else if(x->k_i > x->lastK && x->shootFlag != 0 && x->limitFlag != 0)
	  {
	    *out++ = x->f_s - x->k_i + x->offset;
	  }
	else if (x->f_s > 0) *out++ = x->f_s - x->k_i + x->offset;
        else *out++ = x->f_s - (x->k_i - 1) + x->offset;
    }
    if(x->wLimit != 0 && x->k_i > 1)
      {
	x->limitFlag = 1;
      }
    else if(x->wLimit != 0 && x->k_i < 1 && x->lastLimit == 1)
      {
	x->limitFlag = 0;
	x->wLimit = 0;
      }
    x->lastK = x->k_i;
    x->lastLimit = x->limitFlag;
    outlet_float(x->phase, x->f_s - (float)x->k_i + x->offset);
    return (w + 5);
}

static void wrap_overshoot_tilde_offset(t_wrap_overshoot_tilde *x, t_floatarg f)
{
  x->offset = f;
}

static void wrap_overshoot_tilde_limit(t_wrap_overshoot_tilde *x, t_floatarg f)
{
  x->wLimit = (int)f != 0 ? 1 : 0;
}

static void wrap_overshoot_tilde_dsp(t_wrap_overshoot_tilde *x, t_signal **sp)
{
  dsp_add((t_perfroutine)wrap_overshoot_tilde_perform, 4, (t_int)x, (t_int)sp[0]->s_vec, (t_int)sp[1]->s_vec, (t_int)sp[0]->s_n);
}

void wrap_overshoot_tilde_setup(void)
{
  wrap_overshoot_tilde_class = class_new(gensym("wrap_overshoot~"), 
  (t_newmethod)wrap_overshoot_tilde_new, 
  0, sizeof(t_wrap_overshoot_tilde),
  CLASS_DEFAULT, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(wrap_overshoot_tilde_class, t_wrap_overshoot_tilde, x_f);
    class_addmethod(wrap_overshoot_tilde_class, (t_method)wrap_overshoot_tilde_dsp,
        gensym("dsp"), A_CANT, 0);
  class_addmethod(wrap_overshoot_tilde_class, (t_method)wrap_overshoot_tilde_offset, gensym("offset"), A_DEFFLOAT, 0);
  class_addmethod(wrap_overshoot_tilde_class, (t_method)wrap_overshoot_tilde_limit, gensym("limit"), A_DEFFLOAT, 0);
}
