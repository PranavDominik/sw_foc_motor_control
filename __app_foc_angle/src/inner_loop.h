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
 *
 *****************************************************************************
 *
 * During normal operation, the inner_loop() function loops through the following states:
 *  START: Initial entry state.
 *		Move to SEARCH state.
 *
 *  SEARCH:	Motor runs open-loop, spinning the magnetic field around at a fixed torque,
 *		until the QEI reports that it has an accurate position measurement.
 *		Then the Hall sensors and QEI data are used to calculate the phase
 *		difference between the motors coils and the QEI origin.
 *		Move to FOC state.
 *
 *  FOC:		Motor runs in closed-loop, the full Field Oriented Control (FOC)
 *		is used to commutate the rotor, until a 'stop' or 'stall' condition is detected.
 *		Move to STOP or STALL state.
 *
 *	STALL: Currently, error message is issued.
 *		Move to STOP state.
 *
 *	STOP: End loop
 *
 *	For each iteration of the FOC state, the following actions are performed:-
 *		Read the QEI and ADC data.
 *		Estimate the angular velocity from the QEI data.
 *		Estimate the Iq value from either the ADC data or angular velocity.
 *		Optionally estimate the Id value from the ADC data
 *		Optionally use the demand and estimated velocity to produce a demand Iq value, using a PID
 *		Use the demand and estimated Iq to produce a requested Iq value, using a PID
 *  	Transform the requested Iq and Id values into voltages for the motor coils
 *  	Convert voltages into PWM duty cycles.
 *
 * This is a standard FOC algorithm, with the current and speed control loops combined.
 *
 * Notes:
 *
 *	The Leading_Angle is the angle between the position of maximum Iq
 *	(tangential magnetic field), and the position of the magnetic pole.
 *	This allows the coil to be pulled towards the pole in an efficient manner.
 *
 *	The Phase_Lag, is the angular difference between the applied voltage (PWM),
 *	and the measured current (ADC).
 *
 *	Due to the effects of back EMF and inductance, both the Leading_Angle and
 *	Phase_Lag vary with angular velocity.
 *
 **/

#ifndef _INNER_LOOP_H_
#define _INNER_LOOP_H_

#include <stdlib.h> // Required for abs()

#include <xs1.h>
#include <print.h>
#include <assert.h>
#include <safestring.h>

#include "app_global.h"
#include "mathuint.h"
#include "hall_client.h"
#include "qei_client.h"
#include "adc_client.h"
#include "pwm_client.h"
#include "pid_regulator.h"
#include "clarke.h"
#include "park.h"
#include "watchdog.h"
#include "shared_io.h"

// Timing definitions
#define MILLI_400_SECS (400 * MILLI_SEC) // 400 ms. Start-up settling time
#define ALIGN_PERIOD (8 * MILLI_SEC) // 24ms. Time to allow for Motor colis to align opposite magnets WARNING depends on START_VQ_OPENLOOP

#define PWM_MIN_LIMIT (PWM_MAX_VALUE >> 4) // Min PWM value allowed (1/16th of max range)
#define PWM_MAX_LIMIT (PWM_MAX_VALUE - PWM_MIN_LIMIT) // Max. PWM value allowed

#define VOLT_RES_BITS 14 // No. of bits used to define No. of different Voltage magnitude levels
#define VOLT_MAX_MAG (1 << VOLT_RES_BITS) // No.of different Voltage Magnitudes. NB Voltage is -VOLT_MAX_MAG..(VOLT_MAX_MAG-1)

// Check precision data
#if (VOLT_RES_BITS < PWM_RES_BITS)
#error  VOLT_RES_BITS < PWM_RES_BITS
#endif // (VOLT_RES_BITS < PWM_RES_BITS)

#define VOLT_TO_PWM_BITS (VOLT_RES_BITS - PWM_RES_BITS + 1) // bit shift required for converting volts to PWM pulse-widths
#define HALF_VOLT_TO_PWM (1 << (VOLT_TO_PWM_BITS - 1)) // Used for rounding
#define VOLT_OFFSET (VOLT_MAX_MAG + HALF_VOLT_TO_PWM) // Offset required to make PWM pulse-width +ve

#define STALL_TRIP_COUNT 5000

#define FIRST_HALL_STATE 0b001 // [CBA] 1st Hall state of 6-state cycle
#define POSI_LAST_HALL_STATE 0b101 // [CBA] last Hall state of 6-state cycle if spinning in Positive direction
#define NEGA_LAST_HALL_STATE 0b011 // [CBA] last Hall state of 6-state cycle if spinning in Negative direction

// Choose last Hall state of 6-state cycle, depending on spin direction
#if (LDO_MOTOR_SPIN == 1)
	#define LAST_HALL_STATE 0b011
#else
#endif

#define INIT_THETA 0 // Initial start-up angle

#if (LOAD_VAL == NO_LOAD)
#define START_VOLT_OPENLOOP 4000 // 2000 Voltage (Vh) magnitude for start of open-loop state
#define START_GAMMA_OPENLOOP 64  // Voltage angle for start of open-loop state (ie Vq = Vh.cos(angle)

#define END_VOLT_OPENLOOP 5000 // 3000  Voltage (Vh) magnitude for end of open-loop state
#define END_GAMMA_OPENLOOP 19 //  19 32 Voltage angle for end of open-loop state (ie Vq = Vh.cos(angle)

#define REQ_VOLT_CLOSEDLOOP 1000 // Default starting value
#define REQ_GAMMA_CLOSEDLOOP 19     // Used to tune IQ PID

#define MIN_VQ 1600 // Motor will stall if abs(Vq) falls below this value
#define MAX_VQ_OPENLOOP 5800 // MB~ Max Vq value for open-loop tuning
#define MIN_VQ_OPENLOOP 1000 // MB~ Min Vq value for open-loop tuning

#define START_SPEED 800 // Speed used to start motor
#define INIT_SPEED 400 // Initial motor speed, before external request received

#define MIN_SPEED 300 // This value is derived from experience
#elif (LOAD_VAL == BIG_LOAD)

#define START_VOLT_OPENLOOP 4000 // 2000 Voltage (Vh) magnitude for start of open-loop state
#define START_GAMMA_OPENLOOP 64  // Voltage angle for start of open-loop state (ie Vq = Vh.cos(angle)

#define END_VOLT_OPENLOOP 2500 // 3000  Voltage (Vh) magnitude for end of open-loop state
#define END_GAMMA_OPENLOOP 19 //  19 32 Voltage angle for end of open-loop state (ie Vq = Vh.cos(angle)

#define REQ_VOLT_CLOSEDLOOP 400 // Default starting value
#define REQ_GAMMA_CLOSEDLOOP 19     // Used to tune IQ PID

#define MIN_VQ 1600 // Motor will stall if abs(Vq) falls below this value
#define MAX_VQ_OPENLOOP 5800 // MB~ Max Vq value for open-loop tuning
#define MIN_VQ_OPENLOOP 1000 // MB~ Min Vq value for open-loop tuning

#define START_SPEED 100 // Speed used to start motor
#define INIT_SPEED 100 // Initial motor speed, before external request received

#define MIN_SPEED 30 // This value is derived from experience
#endif // (LOAD_VAL == BIG_LOAD)


#define SYNC_SPEED 400 // Speed below which angular synchronisation used
#define SPEC_MAX_SPEED 4000 // This value is derived from the LDO Motor Max. spec. speed
#define SAFE_MAX_SPEED 5800 // This value is derived from the Optical Encoder Max. rate of 100kHz (gives 5860)
#define SPEED_INC 100 // If speed change requested, this is the amount of change

// Test definitions
#define ITER_INC 50000 // No. of FOC iterations between speed increments

// MB~ Cludge to stop velocity spikes. Needs proper fix. Changed Power board, seemed to clear up QEI data
#define VELOC_FILT 1
#define MAX_VELOC_INC 1 // MB~ Maximum allowed velocity increment.

// Set-up defines for scaling ...
#define SHIFT_20 20
#define SHIFT_16 16
#define SHIFT_13 13
#define SHIFT_12 12
#define SHIFT_9   9

#define PHASE_BITS SHIFT_20 // No of bits in phase offset scaling factor
#define PHASE_DENOM (1 << PHASE_BITS)
#define HALF_PHASE (PHASE_DENOM >> 1)

#define VEL_GRAD 10000 // (Estimated_Current)^2 = VEL_GRAD * Angular_Velocity

#define XTR_SCALE_BITS SHIFT_16 // Used to generate 2^n scaling factor
#define XTR_HALF_SCALE (1 << (XTR_SCALE_BITS - 1)) // Half Scaling factor (used in rounding)

#define XTR_COEF_BITS SHIFT_9 // Used to generate filter coef divisor. coef_div = 1/2^n
#define XTR_COEF_DIV (1 << XTR_COEF_BITS) // Coef divisor
#define XTR_HALF_COEF (XTR_COEF_DIV >> 1) // Half of Coef divisor

#define PROPORTIONAL 1 // Selects between 'proportional' and 'offset' error corrections
#define VELOC_CLOSED 1 // MB~ 1 Selects fully closed loop (both velocity, Iq and Id)
#define IQ_ID_CLOSED 1 // MB~ 1 Selects Iq/Id closed-loop, velocity open-loop

// TRANSIT state uses at least one electrical cycle per 1024 Vq values ...
#define VOLT_DIFF_BITS 10 // NB 2^10 = 1024, Used to down-scale Voltage differences in blending function
#define VOLT_DIFF_MASK ((1 << VOLT_DIFF_BITS ) - 1) // Used to mask out Voltage difference bits

// Linear conversion are used to relate 2 parameters. E.G. PWM Voltage applied to the coil current produced  (I = m*V + c)
#define S2V_BITS SHIFT_12 // No of bits in Voltage to current scaling factor
#define S2V_DENOM (1 << S2V_BITS)
#define HALF_S2V (S2V_DENOM >> 1)
#define S2V_MUX 2575 // Speed multiplier. NB 0.6286 ~= 2575/2^12

#define V2I_BITS SHIFT_16 // No of bits in Voltage to current scaling factor
#define V2I_DENOM (1 << V2I_BITS)
#define HALF_V2I (V2I_DENOM >> 1)
#define V2I_MUX 1341 // Voltage multiplier. NB 0.02045 ~= 1341/2^16

// Used to smooth demand Voltage
#define SMOOTH_VOLT_INC 2 // Maximum allowed increment in demand voltage
#define HALF_SMOOTH_VOLT (SMOOTH_VOLT_INC >> 1)  // Half max. allowed increment

// Defines for Field Weakening
#define FW_SPEED ((3*SPEC_MAX_SPEED + 2) >> 2) // Only use field-weakening if above 3/4 of SPEC_MAX_SPEED
#define IQ_LIM 55 // 55 Maximum allowed target Iq value, before field weakening applied
#define IQ_ID_BITS 2 // NB Near stability, 1 unit change in Id is equivalent to a 4 unit change in Iq
#define IQ_ID_RATIO (1 << IQ_ID_BITS) // NB Near stability, 1 unit change in Id is equivalent to a 4 unit change in Iq
#define IQ_ID_HALF (IQ_ID_RATIO >> 1) // Quantisation error is half IQ_ID_RATIO

#define VEL_SCALE_BITS 16 // Used to generate 2^n scaling factor
#define VEL_HALF_SCALE (1 << (VEL_SCALE_BITS - 1)) // Half Scaling factor (used in rounding)

#define VEL_COEF_BITS 8 // Used to generate filter coef divisor. coef_div = 1/2^n
#define VEL_COEF_DIV (1 << VEL_COEF_BITS) // Coef divisor
#define VEL_HALF_COEF (VEL_COEF_DIV >> 1) // Half of Coef divisor

#define PERIOD_COEF_BITS 6 // Used to generate filter coef divisor. coef_div = 1/2^n
#define PERIOD_COEF_DIV (1 << PERIOD_COEF_BITS) // Coef divisor
#define PERIOD_HALF_COEF (PERIOD_COEF_DIV >> 1) // Half of Coef divisor

#define SYNC_SCALE_BITS 19 // Used to generate 2^n scaling factor
#define SYNC_SCALE_DIVR (1 << SYNC_SCALE_BITS) // Scaling factor used to scale velocity in sync. mode
#define HALF_SYNC_SCALE (SYNC_SCALE_DIVR >> 1) // Half scaling factor used in rounding

#define ANG_BUF_SIZ 625 // This is a chosen to be a factor of (SECS_IN_MIN * PLATFORM_REFERENCE_HZ)

#define VEL2ANG_MUX 229065 // Velocity to Angle conversion factor
#define VEL2ANG_RES 19 // Resolution of Velocity to Angle conversion factor
#define VEL2ANG_DIV (1 << VEL2ANG_RES) // Divisor for Velocity to Angle conversion factor
#define VEL2ANG_HALF (VEL2ANG_DIV >> 1) // Half Divisor (used for rounding)

#define ANG2VEL_MUX 9375// Angle to Velocity conversion factor
#define ANG2VEL_RES 12 // Resolution of Angle to Velocity conversion factor
#define ANG2VEL_DIV (1 << ANG2VEL_RES) // Divisor for Angle to Velocity conversion factor
#define ANG2VEL_HALF (ANG2VEL_DIV >> 1) // Half Divisor (used for rounding)

#if (USE_XSCOPE)
//MB~	#define DEMO_LIMIT 100000 // XSCOPE
#define DEMO_LIMIT 400000 // XSCOPE
//MB~	#define DEMO_LIMIT 800000 // XSCOPE
#else // if (USE_XSCOPE)
//MB~	#define DEMO_LIMIT 4000000
	#define DEMO_LIMIT 4000
#endif // else !(USE_XSCOPE)

#define STR_LEN 80 // String Length

// Debug/Tuning switches
#define GAMMA_SWEEP 0 // IQ/ID Open-loop, Gamma swept through electrical cycle

// Parameters for filtering estimated rotational current values.
#define ROTA_FILT_BITS 9 // WARNING: Using larger values will increase the response time of the motor
#if (1 <  ROTA_FILT_BITS)
#define ROTA_HALF_FILT (1 << (ROTA_FILT_BITS - 1))
#else
#define ROTA_HALF_FILT 0
#endif

#define RF_DIV_RPM_BITS 24 // Bit resolution for Reference_Freq/Start_Speed = (100 MHz)/(358 RPM/60) as power of 2

#define BLEND_BITS (RF_DIV_RPM_BITS - PWM_RES_BITS - POLE_PAIR_BITS) // Bit resolution for Blending denominator
#define BLEND_DENOM (1 << BLEND_BITS) // Up-scaling factor
#define BLEND_HALF (BLEND_DENOM >> 1) // Half Up-scaling factor. Used in rounding

#define QEI_UPSCALE_BITS 2
#define QEI_UPSCALE_DENOM (1 << QEI_UPSCALE_BITS) // Factor for up-scaling QEI values
#define QEI_HALF_UPSCALE (QEI_UPSCALE_DENOM >> 1) // Half QEI up-scaling Factor (used for rounding)
#define UQ_PER_PAIR (QEI_PER_PAIR << QEI_UPSCALE_BITS) // No. of different Up-scaled QEI values per pole pair
#define UQ_PER_REV (QEI_PER_REV << QEI_UPSCALE_BITS) // No. of different Up-scaled QEI values per revolution
#define UQ_REV_MASK (UQ_PER_REV - 1) // (16-bit) Mask used to extract Up-scaled QEI bits

#define OC_ERR_LIM 512 // Number of Hall Over-current errors allowed before Board powered down

#define USE_VEL 0 // Flag set if velocity estimate used (NOT angular-difference)

#pragma xta command "add exclusion foc_loop_motor_fault"
#pragma xta command "add exclusion foc_loop_speed_comms"
#pragma xta command "add exclusion foc_loop_shared_comms"
#pragma xta command "add exclusion foc_loop_startup"
#pragma xta command "analyze loop foc_loop"
#pragma xta command "set required - 40 us"

/** Different Motor Phases */
typedef enum MOTOR_STATE_ETAG
	{ // WARNING: Maintain the order of the following state
  WAIT_START = 0, // Wait for motor to start (receives non-zero request velocity)
  ALIGN, // Align Coils opposite magnet
  SEARCH, // Turn motor until FOC start conditions found
  TRANSIT,	// Transit state
  FOC,		  // Normal FOC state
	STALL,		// State where motor stalled
	WAIT_STOP, // Wait for motor to stop (measures small velocity)
	POWER_OFF, // Error state where motor powered off
  NUM_MOTOR_STATES	// Handy Value!-)
} MOTOR_STATE_ENUM;

// WARNING: If altering Error types. Also update error-message in init_error_data()
/** Different Motor Phases */
typedef enum ERROR_ETAG
{
	OVERCURRENT_ERR = 0,
	UNDERVOLTAGE_ERR,
	STALLED_ERR,
	DIRECTION_ERR,
	SPEED_ERR,
  NUM_ERR_TYPS	// Handy Value!-)
} ERROR_ENUM;

/** Different Rotating Vector components (in rotor frame of reference) */
typedef enum ROTA_VECT_ETAG
{
  D_ROTA = 0,  // Radial Component
  Q_ROTA,      // Tangential (Torque) Component
  NUM_ROTA_COMPS     // Handy Value!-)
} ROTA_VECT_ENUM;

typedef struct ROTA_DATA_TAG // Structure containing data for one rotating vector component
{
	int start_open_V;	// Voltage at start of open-loop state (to generate magnetic field).
	int end_open_V;	// Voltage at end of open-loop state (to generate magnetic field).
	int trans_V;	// voltage during TRANSIT state
	int req_closed_V;	// Requested closed-loop voltage (to generate magnetic field).
	int set_V;	// Demand voltage set by control loop
	int err_V;	// Set Voltage remainder, used in error diffusion
	int prev_V;	// Previous Demand voltage
	int diff_V;	// Difference between Start and Requested Voltage
	int rem_V;	// Voltage remainder, used in error diffusion
	int inp_I;	// Unfiltered input current value (generated by rotor motion)
	int est_I;	// (Possibly filtered) Estimated current value (generated by rotor motion)
	int prev_I;	// Previous estimated current value
	int rem_I;	// Electrical current remainder used in error diffusion
} ROTA_DATA_TYP;

typedef struct STRING_TAG // Structure containing string
{
	char str[STR_LEN]; // Array of characters
} STRING_TYP;

typedef struct ERR_DATA_TAG // Structure containing Error handling data
{
	STRING_TYP err_strs[NUM_ERR_TYPS]; // Array messages for each error type
	unsigned err_cnt[NUM_ERR_TYPS];	// Count No of Errors.
	unsigned err_lim[NUM_ERR_TYPS];	// Error count Limit
	int line[NUM_ERR_TYPS];	// Array of line number for NEWEST occurance of error type.
	unsigned err_flgs;	// Set of Fault detection flags for each error type
} ERR_DATA_TYP;

typedef struct MOTOR_DATA_TAG // Structure containing motor state data
{
	ADC_PARAM_TYP adc_params; // Structure containing measured data from ADC
	HALL_PARAM_TYP hall_params; // Structure containing measured data from Hall sensors
	PWM_COMMS_TYP pwm_comms; // Structure containing PWM communication data between Client/Server.
	QEI_PARAM_TYP qei_params; // Structure containing measured data from QEI sensors
	PID_CONST_TYP pid_consts[NUM_PIDS]; // array of PID const data for different IQ Estimate algorithms
	PID_REGULATOR_TYP pid_regs[NUM_PIDS]; // array of pid regulators used for motor control
	ERR_DATA_TYP err_data; // Structure containing data for error-handling
	ROTA_DATA_TYP vect_data[NUM_ROTA_COMPS]; // Array of structures holding data for each rotating vector component
	int this_angs[ANG_BUF_SIZ];	// Array of previous raw total angle values, (delivered by QEI Client) for this motor
	int othr_angs[ANG_BUF_SIZ];	// Array of previous raw total angle values, (delivered by QEI Client) for other motor
	int diff_angs[ANG_BUF_SIZ];	// Array of previous raw total angle values, (delivered by QEI Client) for other motor
	int cnts[NUM_MOTOR_STATES]; // array of counters for each motor state
	MOTOR_STATE_ENUM state; // Current motor state
	CMD_IO_ENUM cmd_id; // Speed Command
	int meas_speed;	// speed, i.e. magnitude of angular velocity
	int stall_speed;	// Speed below which motor is assumed to have stalled
	int req_veloc;	// (External) Requested angular velocity
	int req_diff;	// (External) Requested angular difference
	int strt_diff;	// ang-diff of motors at start of synchronisation period
	int sum_err_diff;	// sum of differece errors
	int rem_err_diff;	// Remainder after scaling sum of differece errors
	int old_diff;	// Old Requested angular-difference
	int old_veloc;	// Old Requested angular velocity
	int targ_vel;	// (Internal) Target angular velocity
	int targ_diff;	// (Internal) Target angular difference
	int est_veloc;	// Estimated angular velocity (from QEI data)
	int speed_inc; // Speed increment when commanded
	int prev_veloc;	// previous velocity
	unsigned est_period;	// Estimate of QEI period: ticks per QEI position (in Reference Frequency Cycles)
	unsigned filt_period; // Filtered QEI period
	unsigned restart_time; // Re-start time-stamp
	int prev_period;	// previous value of QEI period
	int open_period;	// Time between updates PWM data during open-loop phase
	int open_uq_inc;	// Increment to Upscaled theta value during open-loop phase
	int pid_veloc;	// Output of angular velocity PID
	int pid_Id;	// Output of 'radial' current PID
	int pid_Iq;	// Output of 'tangential' current PID
	int prev_Id;	// previous target 'radial' current value
	int raw_ang;	// raw total angle (delivered by QEI Client)
	int this_diff_ang;	// angular difference between complete cycle of buffer entries, for this motor
	int othr_diff_ang;	// angular difference between complete cycle of buffer entries, for other motor
	int spec_max_diff;	// Angular-Difference above which QEI data unreliable
	int min_diff;	// Angular-Difference below which motor is assumed to have stalled
	int sync_diff;	// Angular-Difference below which motor synchronisation is switched on
	int buf_full;	// Flag set ang-diff buffer full
	int tot_up_ang;	// Upscaled Total angle traversed (NB accounts for multiple revolutions)
	int prev_up_ang;	// Upscaled previous total angle traversed (NB accounts for multiple revolutions)
	int diff_up_ang;	// Upscaled difference angle QEI between updates
	int prev_ang_this;	// Previous raw total angle (delivered by QEI Client) for this motor
	int prev_ang_othr;	// Previous raw total angle (delivered by QEI Client) for other motor
	int corr_ang;	// Correction angle (when QEI origin detected)
	int prev_diff;	// Previous Non-zero Difference angle QEI between updates
	int est_theta;		// Estimated Angular position (from QEI data)
	int est_revs;	// Estimated No of revolutions (No. of origin traversals) (from QEI data)
	int prev_revs;	// Previous No of revolutions
	int set_theta;	// PWM theta value
	int open_theta;	// Open-loop theta value
	int foc_theta;	// FOC theta value
	int pid_preset; // Flag set if PID needs preseting
	int search_theta;	// theta value at end of 'SEARCH state'
	int trans_theta;	// theta value at end of 'TRANSIT state'
	int trans_cycles;	// Number of electrical cycles spent in 'TRANSIT state'
	int trans_cnt;	// Incremented every time trans_theta is updated
	int blend_weight;	// Current value of blending weight
	int blend_inc;	// Increment to blending weight
	int fw_on; // Flag set if Field Weakening required
	int ws_cnt; // Wrong-Spin count //MB~

	int iters; // Iterations of inner_loop
	int buf_cnt; // Angular buffer counter
	unsigned id; // Unique Motor identifier e.g. 0 or 1
	unsigned prev_hall; // previous hall state value
	unsigned end_hall; // hall state at end of cycle. I.e. next value is first value of cycle (001)
	unsigned xscope;	// Flag set when xscope output required

	int qei_offset;	// Phase difference between the QEI origin and PWM theta origin
	int hall_offset;	// Phase difference between the Hall sensor origin and PWM theta origin
	int qei_calib; // Flag set when QEI offset has been calibrated
	int calib_off; // Calibration angular offset
	int hall_found;	// Flag set when QEI orign found
	int Iq_err;	// Error diffusion value for scaling of measured Iq
	int adc_err;	// Error diffusion value for ADC extrema filter

	timer tymer;	// Timer
	unsigned prev_time; 	// previous time stamp

	int filt_adc; // filtered ADC value
	int coef_err; // Coefficient diffusion error
	int scale_err; // Scaling diffusion error

	int half_qei; // Half QEI points per revolution (used for rounding)
	int filt_veloc; // filtered velocity value
	int period_coef_err; // QEI-Period filter coefficient diffusion error
	int period_scale_err; // QEI-Period scaling diffusion error
	int coef_vel_err; // velocity filter coefficient diffusion error
	int scale_vel_err; // Velocity scaling diffusion error
	int veloc_calc_err; // Velocity calculation diffusion error

	int sync_on; // Flag indicating angular synchronisation in operation
	int prev_sync; // Previous value of sync-flag
	int start_ang_this;	// Total angle at start of sync. for this motor
	int start_ang_othr;	// Total angle at start of sync. for other motor

	int tmp; // MB~
	int temp; // MB~ Dbg

timer dbg_tmr; // MB~
unsigned dbg_orig; // MB~
unsigned dbg_strt;
unsigned dbg_end;
unsigned dbg_prev;
unsigned dbg_diff;
unsigned dbg_sum; // MB~
int dbg_err; // MB~

} MOTOR_DATA_TYP;

/*****************************************************************************/
void run_motor ( // run the motor inner loop
	unsigned motor_id,
	chanend c_wd,
	chanend c_pwm,
	streaming chanend c_hall,
	streaming chanend c_qei,
	streaming chanend c_adc,
	chanend c_speed,
	chanend c_can_eth_shared
);
/*****************************************************************************/

#endif /* _INNER_LOOP_H_ */
