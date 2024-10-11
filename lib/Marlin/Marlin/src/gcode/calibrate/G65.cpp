/**
 * @file G65.cpp
 * @author Tadeas Pilar
 * 
 * G65 - Advanced homing
 * Home any axis in any direction targeting any endstop.
 * 
 */




#include "../../inc/MarlinConfig.h"

#ifdef ADVANCED_HOMING

#include "../gcode.h"
#include "../../module/stepper.h"
#include "../../module/endstops.h"

#include "../../module/motion.h"
#include "../../module/endstops.h"
#include "../../module/stepper.h"
#include "../../module/planner.h"
#include "../../module/temperature.h"

#include "metric.h"
#include <gcode/parser.h>
#include <cmsis_os.h>



#ifdef ADVANCED_HOMING_INFO
#define PRINT_INFO(X) serialprintPGM(X)
#else
#define PRINT_INFO(X)
#endif


/*Sanity check*/

#ifndef ENDSTOP_AXIS_MAPPING
#error ENDSTOP_AXIS_MAPPING must be defined
#endif
#ifndef ENDSTOP_POSITIONS
#error ENDSTOP_POSITIONS must be defined
#endif



/*Declare functions*/
static void homeaxis();
static int do_endstop_move(AxisEnum axis, int min_endstop, int max_endstop, int target_endstop, float distance, float speed);
static void update_position(int endstop, int coordinate_system);
static int endstop_to_pin(int endstop);
static int pin_to_endstop(int pin);
static int read_endstop(int endstop_pin);

/*Load endstop inverting from config*/
constexpr bool ENDSTOP_INVERTING[] = {

  #ifdef X_MIN_ENDSTOP_INVERTING
    X_MIN_ENDSTOP_INVERTING,
  #else
    false, 
  #endif

  #ifdef Y_MIN_ENDSTOP_INVERTING
    Y_MIN_ENDSTOP_INVERTING,
  #else
    false, 
  #endif

  #ifdef Z_MIN_ENDSTOP_INVERTING
    Z_MIN_ENDSTOP_INVERTING,
  #else
    false, 
  #endif

  #ifdef Z_MIN_PROBE_ENDSTOP_INVERTING
    Z_MIN_PROBE_ENDSTOP_INVERTING,
  #else
    false, 
  #endif

  #ifdef X_MAX_ENDSTOP_INVERTING
    X_MAX_ENDSTOP_INVERTING,
  #else
    false, 
  #endif

  #ifdef Y_MAX_ENDSTOP_INVERTING
    Y_MAX_ENDSTOP_INVERTING,
  #else
    false, 
  #endif

  #ifdef Z_MAX_ENDSTOP_INVERTING
    Z_MAX_ENDSTOP_INVERTING,
  #else
    false, 
  #endif

  #ifdef X2_MIN_ENDSTOP_INVERTING
    X2_MIN_ENDSTOP_INVERTING,
  #else
    false, 
  #endif

  #ifdef X2_MAX_ENDSTOP_INVERTING
    X2_MAX_ENDSTOP_INVERTING,
  #else
    false, 
  #endif

  #ifdef Y2_MIN_ENDSTOP_INVERTING
    Y2_MIN_ENDSTOP_INVERTING,
  #else
    false, 
  #endif

  #ifdef Y2_MAX_ENDSTOP_INVERTING
    Y2_MAX_ENDSTOP_INVERTING,
  #else
    false, 
  #endif

  #ifdef Z2_MIN_ENDSTOP_INVERTING
    Z2_MIN_ENDSTOP_INVERTING,
  #else
    false, 
  #endif

  #ifdef Z2_MAX_ENDSTOP_INVERTING
    Z2_MAX_ENDSTOP_INVERTING,
  #else
    false, 
  #endif

  #ifdef Z3_MIN_ENDSTOP_INVERTING
    Z3_MIN_ENDSTOP_INVERTING,
  #else
    false, 
  #endif

  #ifdef Z3_MAX_ENDSTOP_INVERTING
    Z3_MAX_ENDSTOP_INVERTING
  #else
    false,
  #endif
};

/**
 * Advanced homing/part measurement (ONLY FOR iX)
 * Homing to any endstop.
 * Supports separate home positions in CNC workspaces(G54-G59.3)(enabled by defining CNC_COORDINATE_SYSTEMS in advanced config)
 * Only supports homing of one axis at a time
 *
 * Use:
 * G65 [AXIS][ENDSTOP] D[DIRECTION][DISTANCE]
 * [AXIS]  Axis to home(X, Y, Z, E)
 * [ENDSTOP]  Target endstop number.
 * [DIRECTION] Direction of homing '+' for positive or '-' for negative direction
 * [DISTANCE] Distance to travel. If endstop is not reached within this distance, motion will stop and position will not be updated to endstop position.
 * 
 */
void GcodeSuite::G65(){
    
    planner.synchronize();
    remember_feedrate_and_scaling();
    endstops.enable(true); // Enable endstops for next homing move

    homeaxis();

    endstops.not_homing();
    restore_feedrate_and_scaling();
    report_current_position();
}

/*Create home positions arrays*/
constexpr AxisEnum endstop_axis_mapping[] = ENDSTOP_AXIS_MAPPING;
constexpr float endstop_positions[] = ENDSTOP_POSITIONS;

#ifdef ENDSTOP_POSITIONS_W0
constexpr float endstop_positions_w0[] = ENDSTOP_POSITIONS_W0;
#endif
#ifdef ENDSTOP_POSITIONS_W1
constexpr float endstop_positions_w1[] = ENDSTOP_POSITIONS_W1;
#endif
#ifdef ENDSTOP_POSITIONS_W2
constexpr float endstop_positions_w2[] = ENDSTOP_POSITIONS_W2;
#endif
#ifdef ENDSTOP_POSITIONS_W3
constexpr float endstop_positions_w3[] = ENDSTOP_POSITIONS_W3;
#endif
#ifdef ENDSTOP_POSITIONS_W4
constexpr float endstop_positions_w4[] = ENDSTOP_POSITIONS_W4;
#endif
#ifdef ENDSTOP_POSITIONS_W5
constexpr float endstop_positions_w5[] = ENDSTOP_POSITIONS_W5;
#endif
#ifdef ENDSTOP_POSITIONS_W6
constexpr float endstop_positions_w6[] = ENDSTOP_POSITIONS_W6;
#endif
#ifdef ENDSTOP_POSITIONS_W7
constexpr float endstop_positions_w7[] = ENDSTOP_POSITIONS_W7;
#endif
#ifdef ENDSTOP_POSITIONS_W8
constexpr float endstop_positions_w8[] = ENDSTOP_POSITIONS_W8;
#endif


/**
 * Read endstop state. Apply ENDSTOP_NOISE_THRESHOLD if defined.
 * Currently does not support ENDSTOP_INVERTING
 * @param endstop_pin MARLIN_PIN on which endstop is attached
 */

int read_endstop(int endstop_pin){
bool inverting = ENDSTOP_INVERTING[pin_to_endstop(endstop_pin)];

#ifdef ENDSTOP_NOISE_THRESHOLD
  int res = 0;
  for(int i = 0; i < ENDSTOP_NOISE_THRESHOLD; i++){
    res += READ(endstop_pin);
    osDelay(1);
  }
  if(res>=ENDSTOP_NOISE_THRESHOLD/2){
    return inverting ? 0 : 1;
  }else{
    return inverting ? 1 : 0;
  }
#else
return READ(endstop_pin) != inverting;
#endif
}


/**
 * Run homing rutine.
 * This function takes parameters from G-Code parser
 */ 
void homeaxis() {
  
   
  /*Parse target endstop. Set min/max endstops for axis*/
    int endstop = -1;
    AxisEnum axis;
    int minpin = X_MIN_PIN;
    int maxpin = X_MAX_PIN;

  if(parser.seen('X')){
    endstop = parser.intval('X');
    axis = X_AXIS;
    minpin = X_MIN_PIN;
    maxpin = X_MAX_PIN;

  }else if(parser.seen('Y')){
    endstop = parser.intval('Y');
    axis = Y_AXIS;
    minpin = Y_MIN_PIN;
    maxpin = Y_MAX_PIN;

  }else if(parser.seen('Z')){
    endstop = parser.intval('Z');
    axis = Z_AXIS;
    minpin = Z_MIN_PIN;
    maxpin = Z_MAX_PIN;

  }else if(parser.seen('E')){
    endstop = parser.intval('E');
    axis = E_AXIS;

  }else{
    serialprintPGM("Error: Unknown axis");
    return;
  }

  /*Check that target endstop position is defined*/
  if(endstop > (int)(sizeof(endstop_positions)/sizeof(endstop_positions[0]))-1){
      serialprintPGM("Error: Unknown endstop");
      return;
  }



  /*Set max travel distance*/
  float distance;
  if(parser.seen('D')){
    distance = parser.floatval('D');
  }else{
    distance=0;
  }

  /*Parse endstop number int endstop*/
  int target_endstop = endstop_to_pin(endstop);
  if(target_endstop<0){ 
    serialprintPGM("Error: Unknown endstop");
    return;     //Trying to target nonexisting endstop
  }


  /*Allow homing move out of triggered endstop*/
  if(distance>0){//Moving in positive direction, ignore MIN endstop if triggered
    if(read_endstop(minpin)){
      minpin = maxpin;
    }
  }else{//Moving in negative direction, ignore MAX endstop if triggered
    if(read_endstop(maxpin)){
      maxpin = minpin;
    }
  }


  
  /*Start tracking raw movement of stepper
  Used for moving current position if homing to alternate workspace*/
  int initial_steps = stepper.position_from_startup(axis);

  const feedRate_t real_fr_mm_s = homing_feedrate(axis);
  target_endstop = do_endstop_move(axis, minpin, maxpin, target_endstop, distance, real_fr_mm_s);
  report_current_position();


  #ifdef ADVANCED_HOMING_BUMP
    if(distance<0){
      distance = 10;
    }else{
      distance = -10;
    }
    do_endstop_move(axis, -1, -1, -1, distance, real_fr_mm_s);
    do_endstop_move(axis, minpin, maxpin, target_endstop, -distance*2, real_fr_mm_s/div[0]);
  #endif //ADVANCED_HOMING_BUMP

  float movement = (stepper.position_from_startup(axis)-initial_steps)*planner.mm_per_step[axis];
  current_position[axis] += movement;
  sync_plan_position();

  if(target_endstop != -1){
    int8_t active_coordinate_system = GcodeSuite::get_coordinate_system();
    update_position(pin_to_endstop(target_endstop), active_coordinate_system);  //Update position based on endstop that got hit.
  }

}


/**
 * Update position of axis mapped to endstop
 * @param endstop Axis will be chosen based on mapping of this endstop.
 * @param coordinate_system If CNC_COORDINATE_SYSTEMS defined, choose endstop position for this coordinate system.
 */
void update_position(int endstop, int coordinate_system){
  
  AxisEnum axis = endstop_axis_mapping[endstop];  //Update position of axis based on endstop to axis mapping.
    
  if(coordinate_system == -1){
    current_position[axis] = endstop_positions[endstop];
    planner.set_position_mm(current_position);
    sync_plan_position();
  }else{
    switch(coordinate_system){

      #ifdef ENDSTOP_POSITIONS_W0
        case 0:
        position_shift[axis] = - current_position[axis] +  endstop_positions_w0[endstop];
        break;
      #endif
      #ifdef ENDSTOP_POSITIONS_W1
        case 1:
        position_shift[axis] = - current_position[axis] +  endstop_positions_w1[endstop];
        break;
      #endif
      #ifdef ENDSTOP_POSITIONS_W2
        case 2:
        position_shift[axis] = - current_position[axis] +  endstop_positions_w2[endstop];
        break;
      #endif
      #ifdef ENDSTOP_POSITIONS_W3
        case 3:
        position_shift[axis] = - current_position[axis] +  endstop_positions_w3[endstop];
        break;
      #endif
      #ifdef ENDSTOP_POSITIONS_W4
        case 4:
        position_shift[axis] = - current_position[axis] +  endstop_positions_w4[endstop];
        break;
      #endif
      #ifdef ENDSTOP_POSITIONS_W5
        case 5:
        position_shift[axis] = - current_position[axis] +  endstop_positions_w5[endstop];
        break;
      #endif
      #ifdef ENDSTOP_POSITIONS_W6
        case 6:
        position_shift[axis] = - current_position[axis] +  endstop_positions_w6[endstop];
        break;
      #endif
      #ifdef ENDSTOP_POSITIONS_W7
        case 7:
        position_shift[axis] = - current_position[axis] +  endstop_positions_w7[endstop];
        break;
      #endif
      #ifdef ENDSTOP_POSITIONS_W8
        case 8:
        position_shift[axis] = - current_position[axis] +  endstop_positions_w8[endstop];
        break;
      #endif
      default:
        break;
        
    }

    GcodeSuite::set_coordinate_system_offset(coordinate_system, axis, position_shift[axis]);
    update_workspace_offset(axis); 

  }
}

/**
 * Single homing move with set parameters.
 * Homing will end if either @param min_endstop or @param max_endstop or @param target_endstop is hit or @param distance is reached.
 * @param axis Axis to move
 * @param min_endstop Endstop pin on negative side of axis
 * @param max_endstop Endstop pin on positive side of axis
 * @param target_endstop Endstop beeing targeted. Can be any endstop.
 * @param distance Maximal distance to move. 
 * @param Move speed
 * @return Pin of endstop that was hit. -1 if distanced was reached.
 */
int do_endstop_move(AxisEnum axis, int min_endstop, int max_endstop, int target_endstop, float distance, float speed){
   
    
    abce_pos_t target = { {{planner.get_axis_position_mm(A_AXIS), planner.get_axis_position_mm(B_AXIS), planner.get_axis_position_mm(C_AXIS), planner.get_axis_position_mm(E_AXIS)}} };
    target[axis] = 0;
    planner.set_machine_position_mm(target);
    target[axis] += distance;

    planner.buffer_segment(target, speed, active_extruder);

    int tes = 0;  //Target endstop
    int xes = 0;  //maX endstop
    int nes = 0;  //miN endstop

    if(target_endstop!=-1){
      tes = read_endstop(target_endstop);
      if(max_endstop != target_endstop){
        xes = read_endstop(max_endstop);
      }
      if(min_endstop != target_endstop){
        nes = read_endstop(min_endstop);
      }
      int distanced_reach_count = 0;
      
        while(1){
          if(tes){
            PRINT_INFO("Stop homing on target endstop\n");
            break;
          }
          if(xes){
            PRINT_INFO("Stop homing on max endstop\n");
            break;
          }
          if(nes){
            PRINT_INFO("Stop homing on min endstop\n");
            break;
          }
          if(!planner.processing()){
            distanced_reach_count++;

            if(distanced_reach_count>1){
              PRINT_INFO("Stop homing on distance reached twice\n");
              break;
            }else{
              PRINT_INFO("Distance reached once\n");
            }
          }
          tes = read_endstop(target_endstop);   //Check target endstop
          if(max_endstop != target_endstop){    //Only check max endstop if it's not targeted
            xes = read_endstop(max_endstop);  
          }
          if(min_endstop != target_endstop){    //Only check min endstop if it's not targeted
            nes = read_endstop(min_endstop);
          }
          idle(true);
      }
    planner.quick_stop();

    }else{
      planner.synchronize();
    }

    if(tes){
      return target_endstop;
    }else if(xes){
      return max_endstop;
    }else if(nes){
      return min_endstop;
    }
    return -1;
}

/**
 * Get pin number from endstop number.
 * @return Pin number, or -1 if endstop undefined 
 */
int endstop_to_pin(int endstop){
    if(0){
#ifdef X_MIN_PIN
    }else if(endstop == 0){
      return X_MIN_PIN;
#endif
#ifdef Y_MIN_PIN
    }else if(endstop == 1){
      return Y_MIN_PIN;
#endif
#ifdef Z_MIN_PIN
    }else if(endstop == 2){
      return Z_MIN_PIN;
#endif
#ifdef Z_MIN_PROBE_PIN
    }else if(endstop == 3){
      return Z_MIN_PROBE_PIN;
#endif
#ifdef X_MAX_PIN
    }else if(endstop == 4){
      return X_MAX_PIN;
#endif
#ifdef Y_MAX_PIN
    }else if(endstop == 5){
      return Y_MAX_PIN;
#endif
#ifdef Z_MAX_PIN
    }else if(endstop == 6){
      return Z_MAX_PIN;
#endif
#ifdef X2_MIN_PIN
    }else if(endstop == 7){
      return X2_MIN_PIN;
#endif
#ifdef X2_MAX_PIN
    }else if(endstop == 8){
      return X2_MAX_PIN;
#endif
#ifdef Y2_MIN_PIN
    }else if(endstop == 9){
      return Y2_MIN_PIN;
#endif
#ifdef Y2_MAX_PIN
    }else if(endstop == 10){
      return Y2_MAX_PIN;
#endif
#ifdef Z2_MIN_PIN
    }else if(endstop == 11){
      return Z2_MIN_PIN;
#endif
#ifdef Z2_MAX_PIN
    }else if(endstop == 12){
      return Z2_MAX_PIN;
#endif
#ifdef Z3_MIN_PIN
    }else if(endstop == 13){
      return Z3_MIN_PIN;
#endif
#ifdef Z3_MAX_PIN
    }else if(endstop == 14){
      return Z3_MAX_PIN;
#endif
    }else{
      return -1;
    }
}


/**
 * Get endstop number from pin number
 * @return Endstop number, or -1 if undefined
 */
int pin_to_endstop(int pin){
    if(0){
#ifdef X_MIN_PIN
    }else if(pin == X_MIN_PIN){
      return 0;
#endif
#ifdef Y_MIN_PIN
    }else if(pin == Y_MIN_PIN){
      return 1;
#endif
#ifdef Z_MIN_PIN
    }else if(pin == Z_MIN_PIN){
      return 2;
#endif
#ifdef Z_MIN_PROBE_PIN
    }else if(pin == Z_MIN_PROBE_PIN){
      return 3;
#endif
#ifdef X_MAX_PIN
    }else if(pin == X_MAX_PIN){
      return 4;
#endif
#ifdef Y_MAX_PIN
    }else if(pin == Y_MAX_PIN){
      return 5;
#endif
#ifdef Z_MAX_PIN
    }else if(pin == Z_MAX_PIN){
      return 6;
#endif
#ifdef X2_MIN_PIN
    }else if(pin == X2_MIN_PIN){
      return 7;
#endif
#ifdef X2_MAX_PIN
    }else if(pin == X2_MAX_PIN){
      return 8;
#endif
#ifdef Y2_MIN_PIN
    }else if(pin == Y2_MIN_PIN){
      return 9;
#endif
#ifdef Y2_MAX_PIN
    }else if(pin == Y2_MAX_PIN){
      return 10;
#endif
#ifdef Z2_MIN_PIN
    }else if(pin == Z2_MIN_PIN){
      return 11;
#endif
#ifdef Z2_MAX_PIN
    }else if(pin == Z2_MAX_PIN){
      return 12;
#endif
#ifdef Z3_MIN_PIN
    }else if(pin == Z3_MIN_PIN){
      return 13;
#endif
#ifdef Z3_MAX_PIN
    }else if(pin == Z3_MAX_PIN){
      return 14;
#endif
    }else{
      return -1;
    }
}


#endif // Advanced homing
