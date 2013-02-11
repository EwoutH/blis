/*

   BLIS    
   An object-based framework for developing high-performance BLAS-like
   libraries.

   Copyright (C) 2012, The University of Texas

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    - Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    - Neither the name of The University of Texas nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "blis2.h"
#include "test_libblis.h"


// Static variables.
static char*     op_str                    = "randv";
static char*     o_types                   = "v";  // x
static char*     p_types                   = "";   // (no parameters)
static thresh_t  thresh[BLIS_NUM_FP_TYPES] = { { 1e-04, 1e-05 },   // warn, pass for s
                                               { 1e-04, 1e-05 },   // warn, pass for c
                                               { 1e-13, 1e-14 },   // warn, pass for d
                                               { 1e-13, 1e-14 } }; // warn, pass for z

// Local prototypes.
void libblis_test_randv_deps( test_params_t* params,
                              test_op_t*     op );

void libblis_test_randv_experiment( test_params_t* params,
                                    test_op_t*     op,
                                    mt_impl_t      impl,
                                    num_t          datatype,
                                    char*          pc_str,
                                    char*          sc_str,
                                    dim_t          p_cur,
                                    double*        perf,
                                    double*        resid );

void libblis_test_randv_impl( mt_impl_t impl,
                              obj_t*    x );

void libblis_test_randv_check( obj_t*  x,
                               double* resid );



void libblis_test_randv_deps( test_params_t* params, test_op_t* op )
{
	// No dependencies.
}



void libblis_test_randv( test_params_t* params, test_op_t* op )
{

	// Return early if this test has already been done.
	if ( op->test_done == TRUE ) return;

	// Call dependencies first.
	if ( TRUE ) libblis_test_randv_deps( params, op );

	// Execute the test driver for each implementation requested.
	if ( op->front_seq == ENABLE )
	{
		libblis_test_op_driver( params,
		                        op,
		                        BLIS_TEST_SEQ_FRONT_END,
		                        op_str,
		                        p_types,
		                        o_types,
		                        thresh,
		                        libblis_test_randv_experiment );
	}
}



void libblis_test_randv_experiment( test_params_t* params,
                                    test_op_t*     op,
                                    mt_impl_t      impl,
                                    num_t          datatype,
                                    char*          pc_str,
                                    char*          sc_str,
                                    dim_t          p_cur,
                                    double*        perf,
                                    double*        resid )
{
	unsigned int n_repeats = params->n_repeats;
	unsigned int i;

	double       time_min  = 1e9;
	double       time;

	dim_t        m;

	char         x_store;

	obj_t        x;


	// Map the dimension specifier to an actual dimension.
	m = libblis_test_get_dim_from_prob_size( op->dim_spec[0], p_cur );

	// Map parameter characters to BLIS constants.


	// Extract the storage character for each operand.
	x_store = sc_str[0];

	// Create the test objects.
	libblis_test_vobj_create( params, datatype, x_store, m, &x );

	// Repeat the experiment n_repeats times and record results. 
	for ( i = 0; i < n_repeats; ++i )
	{
		time = bl2_clock();

		libblis_test_randv_impl( impl, &x );

		time_min = bl2_clock_min_diff( time_min, time );
	}

	// Estimate the performance of the best experiment repeat.
	*perf = ( 2.0 * m ) / time_min / FLOPS_PER_UNIT_PERF;
	if ( bl2_obj_is_complex( x ) ) *perf *= 2.0;

	// Perform checks.
	// For randv(), we don't return a meaningful residual/diff, since we can't
	// really say for sure what is "random" and what is not, so instead we
	// manually perform some checks that will fail under some scenarios whic
	// we consider to be likely.
	libblis_test_randv_check( &x, resid );

	// Free the test objects.
	bl2_obj_free( &x );
}



void libblis_test_randv_impl( mt_impl_t impl,
                              obj_t*    x )
{
	switch ( impl )
	{
		case BLIS_TEST_SEQ_FRONT_END:
		bl2_randv( x );
		break;

		default:
		libblis_test_printf_error( "Invalid implementation type.\n" );
	}
}



void libblis_test_randv_check( obj_t*  x,
                               double* resid )
{
	dim_t m_x   = bl2_obj_vector_dim( *x );
	inc_t inc_x = bl2_obj_vector_inc( *x );
	void* buf_x = bl2_obj_buffer_at_off( *x );

	*resid = 0.0;

	//
	// The two most likely ways that randv would fail is if all elements
	// were zero, or if all elements were greater than or equal to one.
	// We check both of these conditions by computing the sum of the
	// absolute values of the elements of x.
	//

	if      ( bl2_obj_is_float( *x ) )
	{
		float  sum_x;

		bl2_sabsumv( m_x,
		             buf_x, inc_x,
		             &sum_x );

		if      ( sum_x == *bl2_s0   ) *resid = 1.0;
		else if ( sum_x >= 1.0 * m_x ) *resid = 2.0;
	}
	else if ( bl2_obj_is_double( *x ) )
	{
		double sum_x;

		bl2_dabsumv( m_x,
		             buf_x, inc_x,
		             &sum_x );

		if      ( sum_x == *bl2_d0   ) *resid = 1.0;
		else if ( sum_x >= 1.0 * m_x ) *resid = 2.0;
	}
	else if ( bl2_obj_is_scomplex( *x ) )
	{
		float  sum_x;

		bl2_cabsumv( m_x,
		             buf_x, inc_x,
		             &sum_x );

		if      ( sum_x == *bl2_s0   ) *resid = 1.0;
		else if ( sum_x >= 2.0 * m_x ) *resid = 2.0;
	}
	else // if ( bl2_obj_is_dcomplex( *x ) )
	{
		double sum_x;

		bl2_zabsumv( m_x,
		             buf_x, inc_x,
		             &sum_x );

		if      ( sum_x == *bl2_d0   ) *resid = 1.0;
		else if ( sum_x >= 2.0 * m_x ) *resid = 2.0;
	}
}
