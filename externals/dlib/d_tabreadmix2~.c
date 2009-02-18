/*
 *   tabreadmix.c  -  an overlap add tabread~ clone
 *   Copyright (c) 2000-2003 by Tom Schouten
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <m_pd.h>
#include <math.h>

/******************** tabreadmix~ ***********************/

static t_class *d_tabreadmix2_tilde_class;

typedef struct _d_tabreadmix2_tilde
{
    t_object x_obj;
    int x_npoints;
    float *x_vec;
    t_symbol *x_arrayname;
    float x_f;

    /* file position vars */
    float x_currpos;
    float x_currpos_fracpart;
    float x_prevpos;
    float x_posinc;
    
    /* cross fader state vars */
    int x_xfade_size;
    int x_xfade_phase;
    float x_xfade_cos;
    float x_xfade_sin;
    float x_xfade_state_c;
    float x_xfade_state_s;
    
    int x_wrap_start;
    int x_wrap_end;
    int x_wrap_length;

} t_d_tabreadmix2_tilde;



inline void d_tabreadmix2_tilde_wrapindices(t_d_tabreadmix2_tilde *x)
{
    int max;

    /* modulo */
/*    while ( x->x_currpos > x->x_npoints )
	x->x_currpos -= x->x_npoints;
    while ( x->x_prevpos > x->x_npoints )
	x->x_prevpos -= x->x_npoints;*/
    
    while ( x->x_currpos > x->x_wrap_end )
	x->x_currpos -= x->x_wrap_length;
    while ( x->x_prevpos > x->x_wrap_end )
	x->x_prevpos -= x->x_wrap_length;

    /* make sure 0<=..<x->x_npoints */
    //if (x->x_currpos < 0) x->x_currpos += x->x_npoints;
    //if (x->x_prevpos < 0) x->x_prevpos += x->x_npoints;
    x->x_currpos += (x->x_currpos < 0) * x->x_npoints;
    x->x_prevpos += (x->x_prevpos < 0) * x->x_npoints;

}



#define min(x,y) ((x)<(y)?(x):(y))

static t_int *d_tabreadmix2_tilde_perform(t_int *w)
{
    t_d_tabreadmix2_tilde *x = (t_d_tabreadmix2_tilde *)(w[1]);
    t_float *pos = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    int n = (int)(w[4]);    
    int maxxindex;
    float *buf = x->x_vec;
    int i;
    float currgain, prevgain;
    float c,s;
    int chunk;
    int leftover;
    int newpos = (int)*pos;

//    maxxindex = x->x_npoints;
    maxxindex = x->x_wrap_end;
    if (!buf) goto zero;
    if (maxxindex <= 0) goto zero;


    while (n){

	/* process as much data as possible */
	leftover = x->x_xfade_size - x->x_xfade_phase;
	chunk = min(n, leftover);

	for (i = 0; i < chunk; i++){
	    /* compute crossfade gains from oscillator state */
	    currgain = 0.5f - x->x_xfade_state_c;
	    prevgain = 0.5f + x->x_xfade_state_c;
	    
	    /* check indices & wrap */
	    d_tabreadmix2_tilde_wrapindices(x);

	    /* mix and write */
	    newpos = (int)(*pos++);
	    *out++ = currgain * buf[(int)x->x_currpos] + prevgain * buf[(int)x->x_prevpos];	    
	    x->x_currpos += x->x_posinc;
	    x->x_prevpos += x->x_posinc;
	    
	    /* advance oscillator */
	    c =  x->x_xfade_state_c * x->x_xfade_cos -  x->x_xfade_state_s * x->x_xfade_sin;
	    s =  x->x_xfade_state_c * x->x_xfade_sin +  x->x_xfade_state_s * x->x_xfade_cos;
	    x->x_xfade_state_c = c;
	    x->x_xfade_state_s = s;
	}

	/* update indices */
	x->x_xfade_phase += chunk;
	n -= chunk;
	//pos += chunk;

	/* check if prev chunk is finished */
	if (x->x_xfade_size == x->x_xfade_phase){
	    x->x_prevpos = x->x_currpos;
	    x->x_currpos = newpos;
	    x->x_xfade_state_c = 0.5f;
	    x->x_xfade_state_s = 0.0f;
	    x->x_xfade_phase = 0;
	}

    }

    /* return if we ran out of data */
    return (w+5);


 zero:
    while (n--) *out++ = 0;
    return (w+5);
}


static void d_tabreadmix2_tilde_blocksize(t_d_tabreadmix2_tilde *x, t_float size)
{
    double prev_phase;
    int max;
    //float fmax = (float)x->x_npoints * 0.5f;
    float fmax = (float)x->x_wrap_length * 0.5f;

    if (size < 1.0) size = 1.0;
//    post( "got input size %f\n", size );
    

    prev_phase = (double)x->x_xfade_phase;
    prev_phase *= size;
    prev_phase /= (double)x->x_xfade_size;

    
    /* preserve the crossfader state */
    x->x_xfade_phase = (int)prev_phase;
    x->x_xfade_size = (int)size;


    x->x_xfade_cos = cos(M_PI / (float)x->x_xfade_size);
    x->x_xfade_sin = sin(M_PI / (float)x->x_xfade_size);


    /* make sure indices are inside array */
    if (x->x_npoints == 0){
	x->x_currpos = 0;
	x->x_prevpos = 0;
    }

    //else d_tabreadmix2_tilde_wrapindices(x);



}

void d_tabreadmix2_tilde_wrap(t_d_tabreadmix2_tilde *x, t_float start, t_float end )
{
    if ( start > end )
    	error( "d_tabreadmix2~: wrap: start %i must be lower than end %i", (int)start, (int)end );
    else if ( 0 > start || start > x->x_npoints )
    	error( "d_tabreadmix2~: wrap: start %i must be between 0 and bufsize (%d)", (int)start, (int)x->x_npoints );
    else if ( 0 > end || end > x->x_npoints )
    	error( "d_tabreadmix2~: wrap: end %i must be between 0 and bufsize (%d)", (int)end, (int)x->x_npoints );
    else
    {
    	x->x_wrap_start = start;
    	x->x_wrap_end = end;
    	x->x_wrap_length = end - start;
    }
}

void d_tabreadmix2_tilde_pitch(t_d_tabreadmix2_tilde *x, t_float f)
{
    if (f < 1) f = 1;

    d_tabreadmix2_tilde_blocksize(x, sys_getsr() / f);
}


void d_tabreadmix2_tilde_audiorate(t_d_tabreadmix2_tilde *x, t_float f)
{
    if ( f<0.001 ) f = 0.001;

    x->x_posinc = f;
}


void d_tabreadmix2_tilde_chunks(t_d_tabreadmix2_tilde *x, t_float f)
{
    if (f < 1.0f) f = 1.0f;
    d_tabreadmix2_tilde_blocksize(x, (float)x->x_npoints / f);
}

void d_tabreadmix2_tilde_bang(t_d_tabreadmix2_tilde *x, t_float f)
{
    //trigger a chunk reset on next dsp call
    x->x_xfade_phase = x->x_xfade_size;
}

void d_tabreadmix2_tilde_set(t_d_tabreadmix2_tilde *x, t_symbol *s)
{
    t_garray *a;
    
    x->x_arrayname = s;
    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    {
        if (*s->s_name)
            error("tabreadmix~: %s: no such array", x->x_arrayname->s_name);
        x->x_vec = 0;
    }
    else if (!garray_getfloatarray(a, &x->x_npoints, &x->x_vec))
    {
        error("%s: bad template for tabreadmix~", x->x_arrayname->s_name);
        x->x_vec = 0;
        // reset wrappage
        x->x_wrap_start = 0;
        x->x_wrap_end = x->x_npoints;
        x->x_wrap_length = x->x_npoints;
    }
    else garray_usedindsp(a);

    /* make sure indices are inside array */
    if (x->x_npoints == 0){
	x->x_currpos = 0;
	x->x_prevpos = 0;
    }

    //else d_tabreadmix2_tilde_wrapindices(x);

}

static void d_tabreadmix2_tilde_dsp(t_d_tabreadmix2_tilde *x, t_signal **sp)
{
    d_tabreadmix2_tilde_set(x, x->x_arrayname);

    dsp_add(d_tabreadmix2_tilde_perform, 4, x,
        sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);

}

static void d_tabreadmix2_tilde_free(t_d_tabreadmix2_tilde *x)
{
}

static void *d_tabreadmix2_tilde_new(t_symbol *s)
{
    t_d_tabreadmix2_tilde *x = (t_d_tabreadmix2_tilde *)pd_new(d_tabreadmix2_tilde_class);
    x->x_arrayname = s;
    x->x_vec = 0;
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("blocksize"));  
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_f = 0;
    x->x_xfade_phase = 0;
    x->x_xfade_size = 1024;
    x->x_currpos = 0;
    x->x_prevpos = 0;
    x->x_posinc = 1.0f;
    x->x_xfade_state_c = 0.5f;
    x->x_xfade_state_s = 0.0f;
    d_tabreadmix2_tilde_blocksize(x, 1024);
    return (x);
}

void d_tabreadmix2_tilde_setup(void)
{
    d_tabreadmix2_tilde_class = class_new(gensym("d_tabreadmix2~"),
        (t_newmethod)d_tabreadmix2_tilde_new, (t_method)d_tabreadmix2_tilde_free,
        sizeof(t_d_tabreadmix2_tilde), 0, A_DEFSYM, 0);
    CLASS_MAINSIGNALIN(d_tabreadmix2_tilde_class, t_d_tabreadmix2_tilde, x_f);
    class_addmethod(d_tabreadmix2_tilde_class, (t_method)d_tabreadmix2_tilde_dsp,
        gensym("dsp"), 0);
    class_addmethod(d_tabreadmix2_tilde_class, (t_method)d_tabreadmix2_tilde_set,
        gensym("set"), A_SYMBOL, 0);
    class_addmethod(d_tabreadmix2_tilde_class, (t_method)d_tabreadmix2_tilde_blocksize,
        gensym("blocksize"), A_FLOAT, 0);
    class_addmethod(d_tabreadmix2_tilde_class, (t_method)d_tabreadmix2_tilde_pitch,
        gensym("pitch"), A_FLOAT, 0);
    class_addmethod(d_tabreadmix2_tilde_class, (t_method)d_tabreadmix2_tilde_chunks,
        gensym("chunks"), A_FLOAT, 0);
    class_addmethod(d_tabreadmix2_tilde_class, (t_method)d_tabreadmix2_tilde_audiorate,
	gensym("audiorate"), A_FLOAT, 0 );
    class_addmethod(d_tabreadmix2_tilde_class, (t_method)d_tabreadmix2_tilde_wrap,
	gensym("wrap"), A_FLOAT, A_FLOAT, 0 );	
    class_addmethod(d_tabreadmix2_tilde_class, (t_method)d_tabreadmix2_tilde_bang,
        gensym("bang"), 0);
    
}
