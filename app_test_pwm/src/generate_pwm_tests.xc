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

#include "generate_pwm_tests.h"

/*****************************************************************************/
static void parse_control_file( // Parse PWM control file and set up test options
	GENERATE_PWM_TYP &tst_data_s // Reference to structure of PWM test data
)
{
  unsigned char file_buf[FILE_SIZE];
  int char_cnt; // Character counter
  int line_cnt = 0; // line counter
  int test_cnt = 0; // test counter
  int new_line = 0; // flag if new-line found
  int tst_line = 0; // flag set if test-option found
  char curr_char; // Current character
  int file_id; // File identifier
  int status = 0; // Error status


	// Initialise file buffer
  for (char_cnt = 0; char_cnt < FILE_SIZE; ++char_cnt)
	{
    file_buf[char_cnt] = 0;
  } // for char_cnt

  file_id = _open( "pwm_tests.txt" ,O_RDONLY ,0 ); // Open control file for PWM tests

  assert(-1 != file_id); // ERROR: Open file failed (_open)

  _read( file_id ,file_buf ,FILE_SIZE ); // Read file into buffer

  status = _close(file_id);	// Close file
  assert(0 == status);	// ERROR: Close file failed (_close)

	acquire_lock(); // Acquire Display Mutex
	printstrln("Read following Test Options ..." );

	// Parse the file buffer for test options
  for (char_cnt = 0; char_cnt < FILE_SIZE; ++char_cnt)
	{
    curr_char = file_buf[char_cnt]; // Get next character

    if (!curr_char) break; // Check for end of file info.


		switch (curr_char)
		{
    	case '0' : // Opt out of test
				tst_data_s.common.options.flags[test_cnt] = 0;
				tst_line = 1; // Flag test-option found
    	break; // case '0' :

    	case '1' : // Opt for this test
				tst_data_s.common.options.flags[test_cnt] = 1;
				tst_line = 1; // Flag test-option found
    	break; // case '1' :

    	case 'A' : // Opt for this test
				tst_data_s.common.options.flags[test_cnt] = PWM_PHASE_A;
				tst_line = 1; // Flag test-option found
    	break; // case 'A' :

    	case 'B' : // Opt for this test
				tst_data_s.common.options.flags[test_cnt] = PWM_PHASE_B;
				tst_line = 1; // Flag test-option found
    	break; // case 'B' :

    	case 'C' : // Opt for this test
				tst_data_s.common.options.flags[test_cnt] = PWM_PHASE_C;
				tst_line = 1; // Flag test-option found
    	break; // case 'C' :

    	case '#' : // Start of comment
				new_line = 1; // Set flag for new-line
    	break; // case '1' :

    	case '\n' : // End of line.
				new_line = 1; // Set flag for new-line
    	break; // case '\n' :

    	default : // Whitespace
				// Un-determined line
    	break; // case '\n' :
		} // switch (curr_char)

		// Check if we have a test-option line
		if (tst_line)
		{ // Process test-option line
			test_cnt++;
			printchar(curr_char);
			printchar(' ');

			while ('\n' != file_buf[char_cnt])
			{
				char_cnt++;
				assert(char_cnt < FILE_SIZE); // End-of-file found
				printchar( file_buf[char_cnt] );
			} // while ('\n' != file_buf[char_cnt])

			line_cnt++;
			tst_line = 0; // Clear test-option flag
			new_line = 0; // Clear new_line flag
		} // if (tst_line)
		else
		{ // Process other line
			// Check if we need to move to new-line
			if (new_line)
			{ // skip to next line
				while ('\n' != file_buf[char_cnt])
				{
					char_cnt++;
					assert(char_cnt < FILE_SIZE); // End-of-file found
				} // while ('\n' != file_buf[char_cnt])

				line_cnt++;
				new_line = 0; // Clear new_line flag
			} // if (new_line)
		} // else !(tst_line)
  } // for char_cnt

	printcharln(' ');
	release_lock(); // Release Display Mutex

	// Do some checks ...
	assert(test_cnt == NUM_TEST_OPTS); // Check read required number of test options found
	assert(NUM_TEST_OPTS <= line_cnt); // Check enough file lines read
	assert(test_cnt <= line_cnt); // Check no more than one test/line

	return;
} // parse_control_file
/*****************************************************************************/
static void print_progress( // Print progress indicator
	GENERATE_PWM_TYP &tst_data_s // Reference to structure of PWM test data
)
{
	// Check for display-wrap
	if (PRINT_WID > tst_data_s.print_cnt)
	{
		printchar('.');
		tst_data_s.print_cnt++;
	} // if (PRINT_WID > tst_data_s.print_cnt)
	else
	{
		printcharln('.');
		tst_data_s.print_cnt = 1;
	} // if (PRINT_WID > tst_data_s.print_cnt)
} // print_progress
/*****************************************************************************/
static void init_pwm( // Initialise PWM parameters for one motor
	PWM_COMMS_TYP &pwm_comms_s, // reference to structure containing PWM data
	chanend c_pwm, // PWM channel connecting Client & Server
	unsigned motor_id // Unique Motor identifier e.g. 0 or 1
)
{
	int phase_cnt; // phase counter


	pwm_comms_s.buf = 0; // Current double-buffer in use at shared memory address
	pwm_comms_s.params.id = motor_id; // Unique Motor identifier e.g. 0 or 1

	// initialise arrays
	for (phase_cnt = 0; phase_cnt < NUM_PWM_PHASES; phase_cnt++)
	{
		pwm_comms_s.params.widths[phase_cnt] = 0;
	} // for phase_cnt

	// Receive the address of PWM data structure from the PWM server, in case shared memory is used
	c_pwm :> pwm_comms_s.mem_addr; // Receive shared memory address from PWM server

	return;
} // init_pwm
/*****************************************************************************/
static void init_test_data( // Initialise PWM Test data
	GENERATE_PWM_TYP &tst_data_s, // Reference to structure of PWM test data
	streaming chanend c_chk // Channel for sending test vecotrs to test checker
)
{
	init_common_data( tst_data_s.common ); // Initialise data common to Generator and Checker

	tst_data_s.print_on = VERBOSE_PRINT; // Set print mode
	tst_data_s.dbg = 0; // Set debug mode

	tst_data_s.period = PWM_PERIOD; // Set period between generations of PWM Client data
	tst_data_s.curr_vect.comp_state[CNTRL] = SKIP; // Initialise to skipped test for set-up mode
	tst_data_s.prev_vect.comp_state[CNTRL] = QUIT; // Initialise to something that will force an update

	parse_control_file( tst_data_s );

	c_chk <: tst_data_s.common.options; // Send test options to checker core

  tst_data_s.phase_id = tst_data_s.common.options.flags[TST_PHASE]; // Store phase to be tested
} // init_test_data
/*****************************************************************************/
static void assign_test_vector_width( // Assign Width-state of test vector
	GENERATE_PWM_TYP &tst_data_s, // Reference to structure of PWM test data
	WIDTH_PWM_ENUM wid_state // Input Width-state
)
{
	tst_data_s.curr_vect.comp_state[WIDTH] = wid_state; // Update speed-state of test vector

	tst_data_s.width = tst_data_s.common.pwm_wids[wid_state]; // Set pulse width for current width-state
} // assign_test_vector_width
/*****************************************************************************/
static void assign_test_vector_leg( // Assign PWM-leg of test vector
	GENERATE_PWM_TYP &tst_data_s, // Reference to structure of PWM test data
	PWM_LEG_ENUM inp_leg	// Input PWM-leg state
)
{
	tst_data_s.curr_vect.comp_state[LEG] = inp_leg;
} // assign_test_vector_leg
/*****************************************************************************/
static void assign_test_vector_adc( // Assign ADC_trigger state of test vector
	GENERATE_PWM_TYP &tst_data_s, // Reference to structure of PWM test data
	ADC_PWM_ENUM inp_adc	// Input adc-trigger state
)
{
	tst_data_s.curr_vect.comp_state[ADC_TRIG] = inp_adc;
} // assign_test_vector_leg
/*****************************************************************************/
static void assign_test_vector_deadtime( // Assign DeadTime of test vector
	GENERATE_PWM_TYP &tst_data_s, // Reference to structure of PWM test data
	DEAD_PWM_ENUM inp_deadtime	// Input dead-time state
)
{
	tst_data_s.curr_vect.comp_state[DEAD] = inp_deadtime;
} // assign_test_vector_deadtime
/*****************************************************************************/
static void do_pwm_test( // Performs one PWM test
	GENERATE_PWM_TYP &tst_data_s, // Reference to structure of PWM test data
	chanend c_pwm 				// Channel between Client and Server
)
{
	timer chronometer; // timer


	tst_data_s.time += tst_data_s.period; // Update time for next PWM pulse generation

	// Load test data into PWM phase under test
	tst_data_s.pwm_comms.params.widths[tst_data_s.phase_id] = tst_data_s.width;

	if (0 == tst_data_s.print_on)
	{
		print_progress( tst_data_s ); // Print progress indicator
	} // if (0 == tst_data_s.print_on)

#if (USE_XSCOPE)
	// NB These signals have to be registered in the file main.xc for the target application
	xscope_int( 0 ,tst_data_s.pwm_comms.params.widths[PWM_PHASE_A] );
	xscope_int( 1 ,tst_data_s.pwm_comms.params.widths[PWM_PHASE_B] );
	xscope_int( 2 ,tst_data_s.pwm_comms.params.widths[PWM_PHASE_C] );
#endif // (USE_XSCOPE)

	chronometer when timerafter(tst_data_s.time) :> void;	// Wait till test period elapsed

	foc_pwm_put_parameters( tst_data_s.pwm_comms ,c_pwm ); // Update the PWM values

	if (tst_data_s.print_on)
	{
		acquire_lock(); // Acquire Display Mutex
		printstr( "PWM:" ); printintln( tst_data_s.width );
		release_lock(); // Release Display Mutex
	} // if (tst_data_s.print_on)

} // do_pwm_test
/*****************************************************************************/
static int vector_compare( // Check if 2 sets of test vector are different
	TEST_VECT_TYP &vect_a, // Structure of containing 1st set of vectore components
	TEST_VECT_TYP &vect_b  // Structure of containing 2nd set of vectore components
) // return TRUE (1) if vectors are different, FALSE(0) if equal
{
	VECT_COMP_ENUM comp_cnt; // vector component counter


	for (comp_cnt=0; comp_cnt<NUM_VECT_COMPS; comp_cnt++)
	{
		if (vect_a.comp_state[comp_cnt] != vect_b.comp_state[comp_cnt]) return 1;
	} // for comp_cnt=0

	return 0; // No differences found
} // vector_compare
/*****************************************************************************/
static void do_pwm_vector( // Do all tests for one PWM test vector
	GENERATE_PWM_TYP &tst_data_s, // Reference to structure of PWM test data
	streaming chanend c_chk, // Channel for sending test vecotrs to test checker
	chanend c_pwm, 				// Channel between Client and Server
	int test_cnt // count-down test counter
)
{
	int new_vect; // flag set if new test vector detected


	new_vect = vector_compare( tst_data_s.curr_vect ,tst_data_s.prev_vect );

	// Check for new test-vector
	if (new_vect)
	{
		c_chk <: tst_data_s.curr_vect; // transmit new test vector details to test checker

		// Check if verbose printing required
		if (tst_data_s.print_on)
		{
			print_test_vector( tst_data_s.common ,tst_data_s.curr_vect ,"" );
		} // if (tst_data_s.print_on)

		tst_data_s.prev_vect = tst_data_s.curr_vect; // update previous vector

		// Check for termination
		if (QUIT == tst_data_s.curr_vect.comp_state[CNTRL])
		{
			c_pwm <: PWM_CMD_LOOP_STOP; // Signal PWM Server to terminate
		} // if (QUIT == tst_data_s.curr_vect.comp_state[CNTRL])
	} // if (new_vect)

	// Loop through tests for current test vector
	while(test_cnt)
	{
		do_pwm_test( tst_data_s ,c_pwm ); // Performs one PWM test

		test_cnt--; // Decrement test counter
	} // while(test_cnt)

} // do_pwm_vector
/*****************************************************************************/
static void gen_pwm_width_test( // Generate PWM Test data for testing one PWM Pulse-width
	GENERATE_PWM_TYP &tst_data_s, // Reference to structure of PWM test data
	streaming chanend c_chk, // Channel for sending test vecotrs to test checker
	chanend c_pwm, 				// Channel between Client and Server
	WIDTH_PWM_ENUM wid_state // Current PWM Pulse-width state under test
)
{
	assign_test_vector_width( tst_data_s ,wid_state ); // Set width component of test vector

	tst_data_s.curr_vect.comp_state[CNTRL] = SKIP; // Skip start-up
	do_pwm_vector( tst_data_s ,c_chk ,c_pwm ,3 );

	tst_data_s.curr_vect.comp_state[CNTRL] = VALID; // Start-up complete, Switch on testing
	do_pwm_vector( tst_data_s ,c_chk ,c_pwm ,MAX_TESTS );

} // gen_pwm_width_test
/*****************************************************************************/
static void gen_motor_pwm_test_data( // Generate PWM Test data for one motor
	GENERATE_PWM_TYP &tst_data_s, // Reference to structure of PWM test data
	streaming chanend c_chk, // Channel for sending test vecotrs to test checker
	chanend c_pwm 				// Channel between Client and Server
)
{
	timer chronometer; // timer


	chronometer :> tst_data_s.time;	// Get time

	acquire_lock(); // Acquire Display Mutex
	printstr( "Start Test Generation For Phase_");
	printcharln( ('A' + tst_data_s.phase_id) );
	release_lock(); // Release Display Mutex

	// NB These tests assume PWM_FILTER = 0

	if (tst_data_s.common.options.flags[TST_ADC])
	{
		assign_test_vector_adc( tst_data_s ,ADC_ON );
	} // if (tst_data_s.common.options.flags[TST_ADC])
	else
	{
		assign_test_vector_adc( tst_data_s ,NO_ADC );
	} // if (tst_data_s.common.options.flags[TST_ADC])

	assign_test_vector_deadtime( tst_data_s ,DEAD_ON ); // Set test Dead-time
	assign_test_vector_leg( tst_data_s ,NUM_PWM_LEGS ); // Set test both PWM-legs

	// Do pulse-width tests ...
	if (tst_data_s.common.options.flags[TST_NARROW]) gen_pwm_width_test( tst_data_s ,c_chk ,c_pwm ,MINI );

	gen_pwm_width_test( tst_data_s ,c_chk ,c_pwm ,SMALL );

	if (tst_data_s.common.options.flags[TST_EQUAL]) gen_pwm_width_test( tst_data_s ,c_chk ,c_pwm ,EQUAL );

	gen_pwm_width_test( tst_data_s ,c_chk ,c_pwm ,LARGE );

	if (tst_data_s.common.options.flags[TST_NARROW]) gen_pwm_width_test( tst_data_s ,c_chk ,c_pwm ,MAXI );

	tst_data_s.curr_vect.comp_state[CNTRL] = QUIT; // Signal that testing has ended for current motor
	do_pwm_vector( tst_data_s ,c_chk ,c_pwm ,0 );

} // gen_motor_pwm_test_data
/*****************************************************************************/
void gen_all_pwm_test_data( // Generate PWM Test data
	streaming chanend c_chk, // Channel for sending test vecotrs to test checker
	chanend c_pwm 				// Channel between Client and Server
)
{
	GENERATE_PWM_TYP tst_data_s; // Structure of PWM test data
	int cmd; // Control command


	init_pwm( tst_data_s.pwm_comms ,c_pwm ,MOTOR_ID );	// Initialise PWM communication data

	init_test_data( tst_data_s ,c_chk );

	gen_motor_pwm_test_data( tst_data_s ,c_chk ,c_pwm );

	// Wait for PWM Seerver to terminate
	do // while(PWM_CMD_ACK != cmd)
	{
		c_pwm :> cmd; // get next signal from PWM Server
	} while(PWM_CMD_ACK != cmd);

	acquire_lock(); // Acquire Display Mutex
	printstrln("Test Generation Ends" );
	release_lock(); // Release Display Mutex

	// Wait for test checker to terminate
	do // while(PWM_CMD_ACK != cmd)
	{
		c_chk :> cmd; // get next signal from Checker
	} while(PWM_CMD_ACK != cmd);

	_Exit(0); // Exit without house-keeping
} // gen_all_pwm_test_data
/*****************************************************************************/
