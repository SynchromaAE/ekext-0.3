/*
 * copy data from one array into another
 * Copyright (c) 2005-2023 Edward Kelly and others
 * Based on arraycopy from maxlib (Olaf Matthes) <olaf.matthes@gmx.de>
 * Forinformaion on usage and distribution, and for a DICLAIMER OF ALL 
 * WARRANTIES, see the file "LICENSE.txt," in this distribution. */

#include "m_pd.h"
#include <stdlib.h>

static char *version = "copyarray v0.2.1, written by Olaf Matthes <olaf.matthes@gmx.de>, adapted by Edward Kelly <synchroma@gmail.com>";

typedef struct copyarray
{
	t_object x_obj;		
	t_symbol *x_destarray;
	t_symbol *x_sourcearray;
	t_garray *x_destbuf;
	t_garray *x_sourcebuf;
	t_int    x_start;
	t_int    x_end;
	t_int    x_pos;
	short    x_print;
} t_copyarray;

	/* choose the destination array to copy to */
static void copyarray_setdestarray(t_copyarray *x, t_symbol *s)
{
	t_garray *b;
	
	if ((b = (t_garray *)pd_findbyclass(s, garray_class)))
	{
		// post("copyarray: destination array set to \"%s\"", s->s_name);
		x->x_destbuf = b;
	} else {
		post("copyarray: no array \"%s\" (error %d)", s->s_name, b);
		x->x_destbuf = 0;
	}
}

static void copyarray_setdest(t_copyarray *x, t_symbol *s)
{
		x->x_destarray = s;
		copyarray_setdestarray(x, x->x_destarray);
}

	/* choose the source array to copy from */
static void copyarray_setsourcearray(t_copyarray *x, t_symbol *s)
{
	t_garray *b;
	
	if ((b = (t_garray *)pd_findbyclass(s, garray_class)))
	{
		// post("copyarray: source array set to \"%s\"", s->s_name);
		x->x_sourcebuf = b;
	} else {
		post("copyarray: no array \"%s\" (error %d)", s->s_name, b);
		x->x_sourcebuf = 0;
	}
}

	/* this is the routine that actually does the copying */
	/* get's called directly when we get a 'bang' */
static void copyarray_docopy(t_copyarray *x)
{
  /* use new 64-bit compatible array API if available */
#if (defined PD_MAJOR_VERSION && defined PD_MINOR_VERSION) && (PD_MAJOR_VERSION > 0 || PD_MINOR_VERSION >= 41)
# define arraynumber_t t_word
# define array_getarray garray_getfloatwords
# define array_get(pointer, index) (pointer[index].w_float)
# define array_set(pointer, index, value) ((pointer[index].w_float)=value)
#else
# define arraynumber_t t_float
# define array_getarray garray_getfloatarray
# define array_get(pointer, index) (pointer[index])
# define array_set(pointer, index, value) ((pointer[index])=value)
#endif

	t_garray *b;		/* make local copy of array */
	arraynumber_t *tab;  /* the content itself */
	int sourcesize, destsize;
	t_int i;
	t_garray *A;
	arraynumber_t *vec;

	if(!x->x_destarray)
	{
		post("copyarray: no destination array specified");
		return;
	}
	if(!x->x_sourcearray)
	{
		post("copyarray: no source array specified");
		return;
	}

	A = x->x_destbuf;

	if ((b = (t_garray *)pd_findbyclass(x->x_sourcearray, garray_class)))
	{
		// post("copyarray: source array set to \"%s\"", x->x_sourcearray->s_name);
	} else {
		post("copyarray: no array \"%s\" (error %d)", x->x_sourcearray->s_name, b);
		return;
	}

		// read from our array
	if (!array_getarray(b, &sourcesize, &tab))
	{
		pd_error(x, "copyarray: couldn't read from source array '%s'!",
                 x->x_sourcearray->s_name);
		return;
	}

	if (!(A = (t_garray *)pd_findbyclass(x->x_destarray, garray_class)))
		error("copyarray: %s: no such array", x->x_destarray->s_name);
	else if (!array_getarray(A, &destsize, &vec))
		error("copyarray: %s: bad template ", x->x_destarray->s_name);
	else
	{
		if(x->x_start > sourcesize)
		{
			pd_error(x, "copyarray: start point %i out of range for source '%s'",
                     (int)x->x_start, x->x_sourcearray->s_name);
			return;
		}
		if(x->x_end - x->x_start > destsize)
		{
		  post("x->x_end %d - x->x_start %d = %d",x->x_end, x->x_start,(int)(x->x_end - x->x_start));
			pd_error(x, "copyarray: start point %i out of range for destination '%s'",
                     (int)x->x_start, x->x_destarray->s_name);
			return;
		}
		if(x->x_end)	// end point is specified
		{
            if(x->x_end > sourcesize)
            {
                logpost(x, 2, "copyarray: end point %i out of range for source '%s', using %i",
                        (int)x->x_end, x->x_sourcearray->s_name, sourcesize);
                x->x_end = sourcesize;
            }
            if(x->x_end - x->x_start > destsize)
            {
		  post("x->x_end %d - x->x_start %d = %d",x->x_end, x->x_start,(int)(x->x_end - x->x_start));
                logpost(x, 2, "copyarray: end point %i out of range for destination '%s', using %i",
                        (int)x->x_end, x->x_destarray->s_name, destsize);
                x->x_end = destsize;
			}
		}
        else
            x->x_end = (sourcesize < destsize ? sourcesize : destsize);

		if(x->x_pos)
			vec += x->x_pos;

		for(i = x->x_start; i < x->x_end; i++)
		{
            array_set(vec, 0, array_get(tab, i));
			vec++;
		}
		garray_redraw(A);
		if(x->x_print)post("copyarray: copied %d values from array \"%s\" to array \"%s\"", 
					        x->x_end-x->x_start, x->x_sourcearray->s_name, x->x_destarray->s_name);
	}
}

static void copyarray_list(t_copyarray *x, t_symbol *s, int argc, t_atom *argv)
{
	if(argc > 1) {
		x->x_sourcearray = atom_getsymbolarg(0, argc, argv);
		x->x_destarray   = atom_getsymbolarg(1, argc, argv);
	}
}

static void copyarray_source(t_copyarray *x, t_symbol *s)
{
	x->x_sourcearray = s;
	x->x_start = x->x_end = x->x_pos = 0;
	copyarray_docopy(x);
}

static void copyarray_print(t_copyarray *x, t_floatarg f)
{
	if(f)
		x->x_print = 1;
	else
		x->x_print = 0;
}

static void copyarray_copy(t_copyarray *x, t_symbol *s, int argc, t_atom *argv)
{
	if(argc == 1)		// source array name supplied
	{
		x->x_sourcearray = atom_getsymbolarg(0, argc, argv);
		x->x_start = x->x_end = x->x_pos = 0;
	}
	else if(argc == 2)	// array name and start point supplied
	{
		x->x_sourcearray = atom_getsymbolarg(0, argc, argv);
		x->x_start = atom_getfloatarg(1, argc, argv);
		x->x_end = x->x_pos = 0;
	}
	else if(argc == 3)	// arrayname and start & end point supplied
	{
		x->x_sourcearray = atom_getsymbolarg(0, argc, argv);
		x->x_start = atom_getfloatarg(1, argc, argv);
		if(argv[2].a_type == A_FLOAT)	// real position
		{
			x->x_end = atom_getfloatarg(2, argc, argv);
		}
		else	// offset given
		{
			t_symbol *offset = atom_getsymbolarg(2, argc, argv);
			x->x_end = (t_int)atoi(offset->s_name) + x->x_start;
		}
		x->x_pos = 0;
	}
	else if(argc == 4)	// as above & dest. array
	{
		x->x_sourcearray = atom_getsymbolarg(0, argc, argv);
		x->x_start = atom_getfloatarg(1, argc, argv);
		if(argv[2].a_type == A_FLOAT)	// real position
		{
			x->x_end = atom_getfloatarg(2, argc, argv);
		}
		else	// offset given
		{
			t_symbol *offset = atom_getsymbolarg(2, argc, argv);
			x->x_end = (t_int)atoi(offset->s_name) + x->x_start;
		}
		x->x_destarray = atom_getsymbolarg(3, argc, argv);
		copyarray_setdestarray(x, x->x_destarray);
		x->x_pos = 0;
	}
	else if(argc == 5)	// as above & dest. array & pos. in dest.
	{
		x->x_sourcearray = atom_getsymbolarg(0, argc, argv);
		x->x_start = atom_getfloatarg(1, argc, argv);
		if(argv[2].a_type == A_FLOAT)	// real position
		{
			x->x_end = atom_getfloatarg(2, argc, argv);
		}
		else	// offset given
		{
			t_symbol *offset = atom_getsymbolarg(2, argc, argv);
			x->x_end = (t_int)atoi(offset->s_name) + x->x_start;
		}
		x->x_destarray = atom_getsymbolarg(3, argc, argv);
		copyarray_setdestarray(x, x->x_destarray);
		x->x_pos = atom_getfloatarg(4, argc, argv);
	}
	else post("copyarray: copy: wrong number of arguments");
	
	copyarray_docopy(x);
}

static t_class *copyarray_class;

static void *copyarray_new(t_symbol *s, int argc, t_atom *argv)
{
    t_copyarray *x = (t_copyarray *)pd_new(copyarray_class);

	if (argc > 0) {
		x->x_destarray = atom_getsymbolarg(0, argc, argv);
		copyarray_setdestarray(x, x->x_destarray);
	}
	inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("symbol"), gensym("dest"));
	x->x_start = x->x_end = x->x_pos = x->x_print = 0;
	return (x);
}

#ifndef MAXLIB
void copyarray_setup(void)
{
	/* the object's class: */
    copyarray_class = class_new(gensym("copyarray"), (t_newmethod)copyarray_new,
    	0, sizeof(t_copyarray), 0, A_GIMME, 0);
#else
void maxlib_copyarray_setup(void)
{
	/* the object's class: */
    copyarray_class = class_new(gensym("maxlib_copyarray"), (t_newmethod)copyarray_new,
    	0, sizeof(t_copyarray), 0, A_GIMME, 0);
	class_addcreator((t_newmethod)copyarray_new, gensym("copyarray"), A_GIMME, 0);
#endif
	class_addmethod(copyarray_class, (t_method)copyarray_copy, gensym("copy"), A_GIMME, 0);
	class_addmethod(copyarray_class, (t_method)copyarray_print, gensym("print"), A_FLOAT, 0);
	class_addmethod(copyarray_class, (t_method)copyarray_setdest, gensym("dest"), A_SYMBOL, 0);
    class_addsymbol(copyarray_class, copyarray_source);
	class_addbang(copyarray_class, copyarray_docopy);
	// class_addlist(copyarray_class, copyarray_list);
#ifndef MAXLIB
    
    logpost(NULL, 4, version);
#else
    class_sethelpsymbol(copyarray_class, gensym("maxlib/copyarray\\xs-help.pd"));
#endif
}
