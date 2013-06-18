/**
 * The copyrights, all other intellectual and industrial 
 * property rights are retained by XMOS and/or its licensors. 
 * Terms and conditions covering the use of this code can
 * be found in the Xmos End User License Agreement.
 *
 * Copyright XMOS Ltd 2013
 *
 * In the case where this code is a modification of existing code
 * under a separate license, the separate license terms are shown
 * below. The modifications to the code are still covered by the 
 * copyright notice above.
 **/ 

#include "test_hall_common.h"

/*****************************************************************************/
static void init_error_component( // Initialise HALL Test data for error_status test vector component
	VECT_COMP_TYP &vect_comp_s, // Reference to structure of common data for one test vector component
	int inp_states, // No. of states for this test vector component
	const char inp_name[] // input name for current test vector component
)
{
	// Check enough room for all states
	if (MAX_COMP_STATES < inp_states)
	{
		acquire_lock(); // Acquire Display Mutex
		printstrln( "ERROR: MAX_COMP_STATES < inp_states, Update value for MAX_COMP_STATES in test_hall_common.h" );
		release_lock(); // Release Display Mutex
		assert(0 == 1); // Force Abort
	} // if (MAX_COMP_STATES < inp_states)

	vect_comp_s.num_states = inp_states; // Assign number of states for current component
	safestrcpy( vect_comp_s.comp_name.str ,inp_name );

	safestrcpy( vect_comp_s.state_names[HALL_ERR_OFF].str ," No_Err" );
	safestrcpy( vect_comp_s.state_names[HALL_ERR_ON].str ," Err_On" );

	// Add any new component states here 
} // init_error_component
/*****************************************************************************/
static void init_phase_component( // Initialise HALL Test data for phase-change test vector component
	VECT_COMP_TYP &vect_comp_s, // Reference to structure of common data for one test vector component
	int inp_states, // No. of states for this test vector component
	const char inp_name[] // input name for current test vector component
)
{
	// Check enough room for all states
	if (MAX_COMP_STATES < inp_states)
	{
		acquire_lock(); // Acquire Display Mutex
		printstrln( "ERROR: MAX_COMP_STATES < inp_states, Update value for MAX_COMP_STATES in test_hall_common.h" );
		release_lock(); // Release Display Mutex
		assert(0 == 1); // Force Abort
	} // if (MAX_COMP_STATES < inp_states)

	vect_comp_s.num_states = inp_states; // Assign number of states for current component
	safestrcpy( vect_comp_s.comp_name.str ,inp_name );

	safestrcpy( vect_comp_s.state_names[CHANGE].str ,"" );

	// Add any new component states here 
} // init_phase_component
/*****************************************************************************/
static void init_spin_component( // Initialise HALL Test data for spin-direction test vector component
	VECT_COMP_TYP &vect_comp_s, // Reference to structure of common data for one test vector component
	int inp_states, // No. of states for this test vector component
	const char inp_name[] // input name for current test vector component
)
{
	// Check enough room for all states
	if (MAX_COMP_STATES < inp_states)
	{
		acquire_lock(); // Acquire Display Mutex
		printstr( "ERROR on line "); printint( __LINE__ ); printstr( " of "); printstr( __FILE__ );
		printstrln( ": MAX_COMP_STATES < inp_states, Update value for MAX_COMP_STATES in test_hall_common.h" );
		release_lock(); // Release Display Mutex
		assert(0 == 1);
	} // if (MAX_COMP_STATES < inp_states)

	vect_comp_s.num_states = inp_states; // Assign number of states for current component
	safestrcpy( vect_comp_s.comp_name.str ,inp_name );

	safestrcpy( vect_comp_s.state_names[ANTI].str ," Anti-Clock" );
	safestrcpy( vect_comp_s.state_names[CLOCK].str ," Clock-Wise" );

	// Add any new component states here 
} // init_spin_component
/*****************************************************************************/
static void init_speed_component( // Initialise HALL Test data for speed test vector component
	VECT_COMP_TYP &vect_comp_s, // Reference to structure of common data for one test vector component
	int inp_states, // No. of states for this test vector component
	const char inp_name[] // input name for current test vector component
)
{
	// Check enough room for all states
	if (MAX_COMP_STATES < inp_states)
	{
		acquire_lock(); // Acquire Display Mutex
		printstr( "ERROR on line "); printint( __LINE__ ); printstr( " of "); printstr( __FILE__ );
		printstrln( ": MAX_COMP_STATES < inp_states, Update value for MAX_COMP_STATES in test_hall_common.h" );
		release_lock(); // Release Display Mutex
		assert(0 == 1);
	} // if (MAX_COMP_STATES < inp_states)

	vect_comp_s.num_states = inp_states; // Assign number of states for current component
	safestrcpy( vect_comp_s.comp_name.str ,inp_name );

	safestrcpy( vect_comp_s.state_names[ACCEL].str	," Accelerating" );
	safestrcpy( vect_comp_s.state_names[FAST].str		," Fast-steady " );
	safestrcpy( vect_comp_s.state_names[DECEL].str	," Decelerating" );
	safestrcpy( vect_comp_s.state_names[SLOW].str		," Slow-steady " );

	// Add any new component states here 
} // init_speed_component
/*****************************************************************************/
static void init_control_component( // Initialise HALL Test data for Control/Communications test vector component
	VECT_COMP_TYP &vect_comp_s, // Reference to structure of common data for one test vector component
	int inp_states, // No. of states for this test vector component
	const char inp_name[] // input name for current test vector component
)
{
	// Check enough room for all states
	if (MAX_COMP_STATES < inp_states)
	{
		acquire_lock(); // Acquire Display Mutex
		printstr( "ERROR on line "); printint( __LINE__ ); printstr( " of "); printstr( __FILE__ );
		printstrln( ": MAX_COMP_STATES < inp_states, Update value for MAX_COMP_STATES in test_hall_common.h" );
		release_lock(); // Release Display Mutex
		assert(0 == 1);
	} // if (MAX_COMP_STATES < inp_states)

	vect_comp_s.num_states = inp_states; // Assign number of states for current component
	safestrcpy( vect_comp_s.comp_name.str ,inp_name );

	safestrcpy( vect_comp_s.state_names[QUIT].str	,"QUIT " );
	safestrcpy( vect_comp_s.state_names[VALID].str	,"VALID" );
	safestrcpy( vect_comp_s.state_names[SKIP].str		,"SKIP " );

	// Add any new component states here 
} // init_speed_component
/*****************************************************************************/
void print_test_vector( // print test vector details
	COMMON_HALL_TYP &comm_hall_s, // Reference to structure of common HALL data
	TEST_VECT_TYP inp_vect, // Structure containing current HALL test vector to be printed
	const char prefix_str[] // prefix string
)
{
	int comp_cnt; // Counter for Test Vector components
	int comp_state; // state of current component of input test vector


	acquire_lock(); // Acquire Display Mutex
	printstr( prefix_str ); // Print prefix string

	// loop through NON-control test vector components
	for (comp_cnt=1; comp_cnt<NUM_VECT_COMPS; comp_cnt++)
	{
		comp_state = inp_vect.comp_state[comp_cnt];  // Get state of current component

		if (comp_state < comm_hall_s.comp_data[comp_cnt].num_states)
		{
			printstr( comm_hall_s.comp_data[comp_cnt].state_names[comp_state].str ); // Print component status
		} //if (comp_state < comm_hall_s.comp_data[comp_cnt].num_states)
		else
		{
			printcharln(' ');
			printstr( "ERROR: Invalid state. Found ");
			printint( comp_state );
			printstr( " for component ");
			printstrln( comm_hall_s.comp_data[comp_cnt].comp_name.str );
			assert(0 == 1); // Force abort
		} //if (comp_state < comm_hall_s.comp_data[comp_cnt].num_states)
	} // for comp_cnt

	printchar( ':' );
	comp_state = inp_vect.comp_state[CNTRL];  // Get state of Control/Comms. status
	printstrln( comm_hall_s.comp_data[CNTRL].state_names[comp_state].str ); // Print Control/Comms. status

	release_lock(); // Release Display Mutex
} // print_test_vector
/*****************************************************************************/
void init_common_data( // Initialise HALL Test data
	COMMON_HALL_TYP &comm_hall_s // Reference to structure of common HALL data
)
{
	// Array of Hall Phase values [CBA} (NB Increment for clock-wise rotation)
	int clkwise[HALL_PER_POLE] = { 1 ,3 ,2 ,6 ,4 ,5 };
	int phase_val; // phase value
	int phase_cnt; // phase counter


 	comm_hall_s.inverse[0] = HALL_PHASE_MASK; // Unused value

	for (phase_cnt=0; phase_cnt<HALL_PER_POLE; phase_cnt++)
	{
	 	phase_val = clkwise[phase_cnt]; // Get current phase value
	 	comm_hall_s.phases[phase_cnt] = phase_val; // Assign Hall Phase values
	 	comm_hall_s.inverse[phase_val] = phase_cnt; // Assign array offset
	} // for (phase_cnt=0; phase_cnt<HALL_PHASE_MASK; phase_cnt++)

	init_error_component(		comm_hall_s.comp_data[ERROR]	,NUM_HALL_ERRS		," Status" );
	init_phase_component(		comm_hall_s.comp_data[PHASE]	,NUM_HALL_PHASES	," Phase " );
	init_spin_component(		comm_hall_s.comp_data[SPIN]		,NUM_HALL_SPINS		,"  Spin " );
	init_speed_component(		comm_hall_s.comp_data[SPEED]	,NUM_HALL_SPEEDS	," Speed " );
	init_control_component(	comm_hall_s.comp_data[CNTRL]	,NUM_HALL_CNTRLS	," Comms." );

	// Add any new test vector components here
} // init_common_data
/*****************************************************************************/
