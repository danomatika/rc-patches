#include "m_pd.h"
#ifdef NT
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

#define MAX_SIZE 2048

/* ------------------------ d_pitchiness~ for pd----------------------------- */

/* tilde object to take absolute value. */

static t_class *d_pitchiness_class;

typedef struct _d_pitchiness
{
    t_object x_obj; 	   /* obligatory header */    
	t_float x_f;	
	t_outlet* d_pitchiness;		   /* m: place for outlet */
	float* storage;
	
} t_d_pitchiness;

    /* this is the actual performance routine which acts on the samples.
    It's called with a single pointer "w" which is our location in the
    DSP call list.  We return a new "w" which will point to the next item
    after us.  Meanwhile, w[0] is just a pointer to dsp-perform itself
    (no use to us), w[1] and w[2] are the input and output vector locations,
    and w[3] is the number of points to calculate. */

static t_int *d_pitchiness_perform(t_int *w)
{
	float pitchiness = 0.0f;
	t_d_pitchiness *x = (t_d_pitchiness *)(w[1]);
    t_float *in = (t_float *)(w[2]);
	
	//int size=(int)(w[3]); 
	//int size=WINSIZE;
	int size = (int)(w[3])/2;
	int i= 0;
	// check for max size condition - bail out if not met
	if ( size > MAX_SIZE )
	{
		error("blocksize %d too big - max block size is 4096", size*2);
		return (w+4);
	}


	// first copy results to pitchfinding
	for( i=1; i<size/2; i++ )
	{
		x->storage[i] = in[i];
	}

	// downsample by factor of 2
	for ( i=1; i<size/4; i++ )
	{
		float val = in[i*2] + in[i*2+1];
		val /= 2.0f;
		x->storage[i] *= val; 
	}
	// and again by factor of 3
	for ( i=1; i<size/6; i++ )
	{
		float val = in[i*3] + in[i*3+1] + in[i*3+2];
		val /= 3.0f;
		x->storage[i] *= val; 
	}

	// count up pitchiness
	float total = 0.0f;
	float max = 0.0f;
	for ( i=1; i<size/6; i++ )
	{
		float val = x->storage[i];
		if ( val > 10.0f )
			val = 10.0f;
		if ( val > max )
			max = val;
		total += val;
	}
	total /= (float)(size/4);
	total /= max;
	//pitchiness = pitchiness * 0.8f + total*0.2f;
	pitchiness = total;

	// output	
        outlet_float(x->d_pitchiness,pitchiness);

	return (w+4);
}

    /* called to start DSP.  Here we call Pd back to add our perform
    routine to a linear callback list which Pd in turn calls to grind
    out the samples. */
static void d_pitchiness_dsp(t_d_pitchiness *x, t_signal **sp)
{
    dsp_add(d_pitchiness_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

static void *d_pitchiness_new(void)
{
    t_d_pitchiness *x = (t_d_pitchiness *)pd_new(d_pitchiness_class);
	x->d_pitchiness=outlet_new(&x->x_obj, &s_float);
	x->x_f = 0;
	x->storage = malloc(MAX_SIZE*sizeof(float));
	
	return (x);
}

static void d_pitchiness_free(t_d_pitchiness *x) {
	
	free( x->storage );
}


    /* this routine, which must have exactly this name (with the "~" replaced
    by "_tilde) is called when the code is first loaded, and tells Pd how
    to build the "class". */
void d_pitchiness_tilde_setup(void)
{
    d_pitchiness_class = class_new(gensym("d_pitchiness~"), (t_newmethod)d_pitchiness_new, (t_method)d_pitchiness_free,
    	sizeof(t_d_pitchiness), 0, A_DEFFLOAT, 0);

	    /* this is magic to declare that the leftmost, "main" inlet
	    takes signals; other signal inlets are done differently... */

    CLASS_MAINSIGNALIN(d_pitchiness_class, t_d_pitchiness, x_f);
    	/* here we tell Pd about the "dsp" method, which is called back
	when DSP is turned on. */
    
	class_addmethod(d_pitchiness_class, (t_method)d_pitchiness_dsp, gensym("dsp"), 0);
	post("d_pitchiness version 0.1");
}
