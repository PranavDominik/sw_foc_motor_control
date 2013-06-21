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

#ifndef _CHECK_PWM_TESTS_H_
#define _CHECK_PWM_TESTS_H_

#include <stdlib.h>

#include <xs1.h>
#include <assert.h>
#include <print.h>
#include <safestring.h>

#include "app_global.h"
#include "use_locks.h"
#include "capture_pwm_data.h"
#include "test_pwm_common.h"

/** Define Input buffer size in bits */
#define INP_BUF_BITS 6 // NB This may need to be increased if extra printed output added to checker

/** Define allowed PWM-width delay */
#define WID_TIMEOUT 2 // Allowed PWM-width delay

#define NUM_INP_BUFS (1 << INP_BUF_BITS) // No. of input buffers used for storing PWM widths, NB Can probably use 4, but sailing cloase to the wind
#define INP_BUF_MASK (NUM_INP_BUFS - 1) // Bit-mask used to wrap input buffer offset

#define PWM_MASK (PWM_MAX_VALUE - 1) // Used to force into range [0..PWM_MASK]

#define ADC_DELAY 8 // Fudge-Factor: delay between receiving ADC-trigger from channel and outputting to dummy port

/** Classes of PWM sample patterns*/
typedef enum PWM_PATN_ETAG
{
  PWM_LOW = 0,	// All-bits low (Low portion of pulse)
  PWM_RISE,			// Rising Edge of pulse
  PWM_FALL,			// Falling Edge of pulse
  PWM_HIGH,			// All-bits high (High portion of pulse)
  NUM_PATN_STATES    // Handy Value!-)
} PWM_PATN_ENUM;

/** Type containing data for one pulse sample */
typedef struct PWM_SAMP_TAG // Structure containing data for one PWM sample
{
	PWM_PORT_TYP port_data; // PWM port data
	unsigned first; // first pattern bit received (LS bit)
	unsigned last; // last pattern bit received (MS bit)
} PWM_SAMP_TYP;

/** Type containing data for one PWM-leg */
typedef struct PWM_WAVE_TAG // Structure containing data for one PWM Wave
{
	PWM_SAMP_TYP curr_data; // data for current PWM sample 
	PWM_SAMP_TYP prev_data; // data for previous PWM sample
	int meas_wid;	// measured PWM width
	unsigned hi_time; // Time accumulated during high (one) period of pulse
	unsigned lo_time; // Time accumulated during low (zero) period of pulse
	int hi_sum;	// sum of high-times
	int lo_sum;	// sum of low-times
	int hi_num;	// No. of high-times
	int lo_num;	// No. of high-times
	int new_rise;	// Flag set when new rising-edge found
	int new_fall;	// Flag set when new falling-edge found
	PORT_TIME_TYP rise_time; // Time-stamp of rising edge
	PORT_TIME_TYP fall_time; // Time-stamp of falling edge
} PWM_WAVE_TYP;

/** Type containing data for a balanced line (pair of PWM wave trains) */
typedef struct PWM_LINE_TAG
{
	PWM_WAVE_TYP waves[NUM_PWM_LEGS]; // Array of structures containing PWM wave data for each leg of a balanced line
} PWM_LINE_TYP;

/** Type containing data for all PWM motor data (3 phases + ADC) */
typedef struct PWM_MOTOR_TAG
{
	PWM_LINE_TYP lines[NUM_TST_PHASES]; // Array of structures containing PWM wave data for a balanced-line (PWM phase)
	PORT_TIME_TYP adc_time; // port_time when ADC trigger received
} PWM_MOTOR_TYP;

/** Type containing all check data */
typedef struct CHECK_PWM_TAG // Structure containing PWM check data
{
	COMMON_PWM_TYP common; // Structure of PWM data common to Generator and Checker
	char padstr1[STR_LEN]; // Padding string used to format display output
	char padstr2[STR_LEN]; // Padding string used to format display output
	TEST_VECT_TYP curr_vect; // Structure of containing current PWM test vector (PWM conditions to be tested)
	TEST_VECT_TYP prev_vect; // Structure of containing previous PWM test vector
	int motor_errs[NUM_VECT_COMPS]; // Array of error counters for one motor
	int motor_tsts[NUM_VECT_COMPS]; // Array of test counters for one motor
	PWM_LEG_ENUM curr_leg; // Current PWM-leg under test
	PWM_LEG_ENUM prev_leg; // Previous PWM-leg under test
	int bound; // error bound for PWM-width measurement
	int print;  // Print flag
	int dbg;  // Debug flag
} CHECK_PWM_TYP;

/*****************************************************************************/
/** Check PWM results 
 * \param c_hi_leg[], // Array of Channels for receiving PWM High-Leg data
 * \param c_lo_leg[], // Array of Channels for receiving PWM Low-Leg data
 * \param c_adc, // Channel for receiving PWM ADC-trigger data
 * \param c_tst // Channel for sending test vecotrs to test checker
 */
void check_pwm_server_data( // Checks PWM results for all motors
	streaming chanend c_hi_leg[], // Array of Channels for receiving PWM High-Leg data
	streaming chanend c_lo_leg[], // Array of Channels for receiving PWM Low-Leg data
	streaming chanend c_adc, // Channel for receiving PWM ADC-trigger data
	streaming chanend c_tst // Channel for receiving test vectors from test generator
);
/*****************************************************************************/
#endif /* _CHECK_PWM_TESTS_H_ */
