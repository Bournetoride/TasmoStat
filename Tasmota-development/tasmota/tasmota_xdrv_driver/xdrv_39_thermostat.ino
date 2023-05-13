/*
  xdrv_39_thermostat.ino - Thermostat controller for Tasmota

  Copyright (C) 2021  Javier Arigita

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
 #define USE_THERMOSTAT 
#ifdef USE_THERMOSTAT

#define XDRV_39              39

// Enable/disable debugging
#define DEBUG_THERMOSTAT

// Commands
#define D_CMND_THERMOSTATMODESET "ThermostatModeSet"
#define D_CMND_CLIMATEMODESET "ClimateModeSet"
#define D_CMND_TEMPFROSTPROTECTSET "TempFrostProtectSet"
//#define D_CMND_CONTROLLERMODESET "ControllerModeSet"
#define D_CMND_OUTPUTRELAYSET "OutputRelaySet"
//#define D_CMND_TIMEALLOWRAMPUPSET "TimeAllowRampupSet"
#define D_CMND_TEMPFORMATSET "TempFormatSet"
//#define D_CMND_TEMPMEASUREDSET "TempMeasuredSet"
#define D_CMND_TEMPTARGETSET "Setpoint"
//#define D_CMND_TEMPMEASUREDGRDREAD "TempMeasuredGrdRead"
#define D_CMND_STATEEMERGENCYSET "StateEmergencySet"
#define D_CMND_TIMEMANUALTOAUTOSET "TimeManualToAutoSet"
//#define D_CMND_TIMEONLIMITSET "TimeOnLimitSet"
#define D_CMND_TEMPHYSTSET "TempHystSet"
//#define D_CMND_TIMEMAXACTIONSET "TimeMaxActionSet"
//#define D_CMND_TIMEMINACTIONSET "TimeMinActionSet"
//#define D_CMND_TIMEMINTURNOFFACTIONSET "TimeMinTurnoffActionSet"
#define D_CMND_TIMESENSLOSTSET "TimeSensLostSet"
#define D_CMND_DIAGNOSTICMODESET "DiagnosticModeSet"
#define D_CMND_CTRDUTYCYCLEREAD "CtrDutyCycleRead"
#define D_CMND_ENABLEOUTPUTSET "EnableOutputSet"

enum ThermostatModes { THERMOSTAT_AUTOMATIC_OP,  THERMOSTAT_MANUAL_OP, THERMOSTAT_MODES_MAX };
enum ClimateModes { CLIMATE_HEATING, CLIMATE_COOLING, CLIMATE_MODES_MAX };
enum InterfaceStates { IFACE_OFF, IFACE_ON };
enum InputUsage { INPUT_NOT_USED, INPUT_USED };
enum CtrCycleStates { CYCLE_OFF, CYCLE_ON };
enum EmergencyStates { EMERGENCY_OFF, EMERGENCY_ON };
enum SensorType { SENSOR_MQTT, SENSOR_LOCAL, SENSOR_MAX };
enum TempFormat { TEMP_CELSIUS, TEMP_FAHRENHEIT };
enum TempConvType { TEMP_CONV_ABSOLUTE, TEMP_CONV_RELATIVE };
enum DiagnosticModes { DIAGNOSTIC_OFF, DIAGNOSTIC_ON };
enum ThermostatSupportedInputSwitches {
  THERMOSTAT_INPUT_NONE,
  THERMOSTAT_INPUT_SWT1 = 1,            // Buttons
  THERMOSTAT_INPUT_SWT2,
  THERMOSTAT_INPUT_SWT3,
  THERMOSTAT_INPUT_SWT4,
  THERMOSTAT_INPUT_SWT5,
  THERMOSTAT_INPUT_SWT6,
  THERMOSTAT_INPUT_SWT7,
  THERMOSTAT_INPUT_SWT8
};
enum ThermostatSupportedOutputRelays {
  THERMOSTAT_OUTPUT_NONE,
  THERMOSTAT_OUTPUT_REL1 = 1,           // Relays
  THERMOSTAT_OUTPUT_REL2,
  THERMOSTAT_OUTPUT_REL3,
  THERMOSTAT_OUTPUT_REL4,
  THERMOSTAT_OUTPUT_REL5,
  THERMOSTAT_OUTPUT_REL6,
  THERMOSTAT_OUTPUT_REL7,
  THERMOSTAT_OUTPUT_REL8
};

typedef union {
  uint32_t data;
  struct {
    uint32_t thermostat_mode : 2;       // Operation mode of the thermostat system
    //uint32_t controller_mode : 2;       // Operation mode of the thermostat controller
    uint32_t climate_mode : 1;          // Climate mode of the thermostat (0 = heating / 1 = cooling)
    uint32_t sensor_alive : 1;          // Flag stating if temperature sensor is alive (0 = inactive, 1 = active)
    uint32_t sensor_type : 1;           // Sensor type: MQTT/local
    uint32_t temp_format : 1;           // Temperature format (0 = Celsius, 1 = Fahrenheit)
    uint32_t command_output : 1;        // Flag stating the desired command to the output (0 = inactive, 1 = active)
    uint32_t status_output : 1;         // Flag stating state of the output (0 = inactive, 1 = active)
    uint32_t status_input : 1;          // Flag stating state of the input (0 = inactive, 1 = active)
    uint32_t use_input : 1;             // Flag stating if the input switch shall be used to switch to manual mode
    //uint32_t phase_hybrid_ctr : 2;      // Phase of the hybrid controller (Ramp-up, PI or Autotune)
    uint32_t status_cycle_active : 1;   // Status showing if cycle is active (Output ON) or not (Output OFF)
    uint32_t counter_seconds : 6;       // Second counter used to track minutes
    uint32_t output_relay_number : 4;   // Output relay number
    uint32_t input_switch_number : 3;   // Input switch number
    uint32_t enable_output : 1;         // Enables / disables the physical output
    uint32_t free : 3;                  // Free bits
  };
} ThermostatStateBitfield;

typedef union {
  uint8_t data;
  struct {
    uint8_t state_emergency : 1;       // State for thermostat emergency
    uint8_t diagnostic_mode : 1;       // Diagnostic mode selected
    uint8_t output_inconsist_ctr : 2;  // Counter of the minutes where the output state is inconsistent with the command
  };
} ThermostatDiagBitfield;

const char kThermostatCommands[] PROGMEM = "|" D_CMND_THERMOSTATMODESET "|"
  D_CMND_TEMPFROSTPROTECTSET "|" D_CMND_TEMPFORMATSET "|" D_CMND_STATEEMERGENCYSET "|"
   D_CMND_TIMEMANUALTOAUTOSET "|" D_CMND_TEMPHYSTSET "|" D_CMND_TIMESENSLOSTSET "|"
   D_CMND_DIAGNOSTICMODESET "|" D_CMND_ENABLEOUTPUTSET;
  
void (* const ThermostatCommand[])(void) PROGMEM = {
  &CmndThermostatModeSet, &CmndTempFrostProtectSet, &CmndTempFormatSet,
   &CmndStateEmergencySet, &CmndTimeManualToAutoSet, &CmndTempHystSet, 
   &CmndTimeSensLostSet, &CmndDiagnosticModeSet, &CmndEnableOutputSet
   };

struct THERMOSTAT {
  ThermostatStateBitfield status;                                             // Bittfield including states as well as several flags
  uint32_t timestamp_temp_measured_update = 0;                                // Timestamp of latest measurement update
  uint32_t timestamp_temp_meas_change_update = 0;                             // Timestamp of latest measurement value change (> or < to previous)
  uint32_t timestamp_output_off = 0;                                          // Timestamp of latest thermostat output Off state
  uint32_t timestamp_input_on = 0;                                            // Timestamp of latest input On state
  uint32_t time_thermostat_total = 0;                                         // Time thermostat on within a specific timeframe
  uint32_t time_ctr_checkpoint = 0;                                           // Time to finalize the control cycle within the PI strategy or to switch to PI from Rampup in seconds
  float temp_step = 0;                                                       // Setpoint increase per minute until sunset 
  float temp_offset = 0;                                                     // Temperature setpoint above tank bottom
  float temp_target_level = THERMOSTAT_TEMP_INIT;                            // Target level of the thermostat in tenths of degrees
  float temp_target_level_ctr = THERMOSTAT_TEMP_INIT;                         // Target level set for the controller
  float temp_delta = 0;                                                       // Temperature difference between top & bottom of tank
  float temp_manifold = 0;                                                    // Temperature measurement received from manifold sensor in tenths of degrees celsius
  float temp_top = 0;                                                          // Temperature measurement received from tank top sensor in tenths of degrees celsius
  float temp_bottom = 0;                                                      // Temperature measurement received from tank bottom sensor in tenths of degrees celsius
  uint16_t time_delta = 0;                                                      // Number of minutes before sunset
  uint8_t time_output_delay = THERMOSTAT_TIME_OUTPUT_DELAY;                     // Output delay between state change and real actuation event (f.i. valve open/closed)
  uint16_t time_sunset = 0;                                                      // Sunset time in minutes
  uint16_t time_now = 0;                                                         //Now time in minutes
  uint16_t time_sens_lost = THERMOSTAT_TIME_SENS_LOST;                        // Maximum time w/o sensor update to set it as lost in minutes
  uint16_t time_manual_to_auto = THERMOSTAT_TIME_MANUAL_TO_AUTO;              // Time without input switch active to change from manual to automatic in minutes
  int16_t temp_frost_protect = THERMOSTAT_TEMP_FROST_PROTECT;                 // Minimum temperature for frost protection, in tenths of degrees celsius
  float temp_hysteresis = 4;                                                   // Range hysteresis for temperature controller, in degrees celsius
  ThermostatDiagBitfield diag;                                                // Bittfield including diagnostic flags
} Thermostat[THERMOSTAT_CONTROLLER_OUTPUTS];

/*********************************************************************************************/

void ThermostatInit(uint8_t ctr_output)
{
  // Init Thermostat[ctr_output].status bitfield:
  Thermostat[ctr_output].status.thermostat_mode = THERMOSTAT_AUTOMATIC_OP;
  //Thermostat[ctr_output].status.controller_mode = CTR_PI;
  Thermostat[ctr_output].status.climate_mode = CLIMATE_HEATING;
  Thermostat[ctr_output].status.sensor_alive = IFACE_OFF;
  Thermostat[ctr_output].status.sensor_type = SENSOR_LOCAL;
  Thermostat[ctr_output].status.temp_format = TEMP_CELSIUS;
  Thermostat[ctr_output].status.command_output = IFACE_OFF;
  Thermostat[ctr_output].status.status_output = IFACE_OFF;
  Thermostat[ctr_output].status.status_cycle_active = CYCLE_OFF;
  Thermostat[ctr_output].diag.state_emergency = EMERGENCY_OFF;
  Thermostat[ctr_output].status.counter_seconds = 0;
  Thermostat[ctr_output].status.output_relay_number = (THERMOSTAT_RELAY_NUMBER + ctr_output);
  //Thermostat[ctr_output].status.input_switch_number = (THERMOSTAT_SWITCH_NUMBER + ctr_output);
  Thermostat[ctr_output].status.use_input = INPUT_NOT_USED;
  Thermostat[ctr_output].status.enable_output = IFACE_ON;
  Thermostat[ctr_output].diag.output_inconsist_ctr = 0;
  Thermostat[ctr_output].diag.diagnostic_mode = DIAGNOSTIC_ON;
  // Make sure the Output is OFF
  if (Thermostat[ctr_output].status.enable_output == IFACE_ON) {
    ExecuteCommandPower(Thermostat[ctr_output].status.output_relay_number, POWER_OFF, SRC_THERMOSTAT);
  }
}

bool ThermostatMinuteCounter(uint8_t ctr_output)
{
  bool result = false;
  Thermostat[ctr_output].status.counter_seconds++;    // increment time

  if ((Thermostat[ctr_output].status.counter_seconds % 60) == 0) {
    result = true;
    Thermostat[ctr_output].status.counter_seconds = 0;
  }
  return result;
}

inline bool ThermostatSwitchIdValid(uint8_t switchId)
{
  return (switchId >= THERMOSTAT_INPUT_SWT1 && switchId <= THERMOSTAT_INPUT_SWT8);
}

inline bool ThermostatRelayIdValid(uint8_t relayId)
{
  return (relayId >= THERMOSTAT_OUTPUT_REL1 && relayId <= THERMOSTAT_OUTPUT_REL8);
}

uint8_t ThermostatInputStatus(uint8_t input_switch)
{
  bool ifId = ThermostatSwitchIdValid(input_switch);
  uint8_t value = 0;
  if(ifId) {
    value = SwitchGetState(ifId - THERMOSTAT_INPUT_SWT1);
  }
  return value;
}

uint8_t ThermostatOutputStatus(uint8_t output_switch)
{
  return (uint8_t)bitRead(TasmotaGlobal.power, (output_switch - 1));
}

int16_t ThermostatCelsiusToFahrenheit(const int32_t deg, uint8_t conv_type) {
  int32_t value;
  value = (int32_t)(((int32_t)deg * (int32_t)90) / (int32_t)50);
  if (conv_type == TEMP_CONV_ABSOLUTE) {
    value += (int32_t)320;
  }

  // Protect overflow
  if (value <= (int32_t)(INT16_MIN)) {
    value = (int32_t)(INT16_MIN);
  }
  else if (value >= (int32_t)INT16_MAX) {
    value = (int32_t)INT16_MAX;
  }

  return (int16_t)value;
}

int16_t ThermostatFahrenheitToCelsius(const int32_t deg, uint8_t conv_type) {
  int16_t offset = 0;
  int32_t value;
  if (conv_type == TEMP_CONV_ABSOLUTE) {
    offset = 320;
  }

  value = (int32_t)(((deg - (int32_t)offset) * (int32_t)50) / (int32_t)90);

  // Protect overflow
  if (value <= (int32_t)(INT16_MIN)) {
    value = (int32_t)(INT16_MIN);
  }
  else if (value >= (int32_t)INT16_MAX) {
    value = (int32_t)INT16_MAX;
  }

  return (int16_t)value;
}

void ThermostatSignalPreProcessingSlow(uint8_t ctr_output)
{
  Thermostat[ctr_output].temp_offset = Thermostat[ctr_output].temp_hysteresis;
  Thermostat[ctr_output].time_delta = TasmotaGlobal.min_sunset - TasmotaGlobal.min_time;          //number of minutes between now and sunset
  Thermostat[ctr_output].time_sunset = TasmotaGlobal.min_sunset - 800;                            //number of minutes between 800 and sunset
  if (TasmotaGlobal.min_time > 800 && TasmotaGlobal.min_time < 1111) {
    Thermostat[ctr_output].temp_delta = Thermostat[ctr_output].temp_top - (Thermostat[ctr_output].temp_bottom - Thermostat[ctr_output].temp_hysteresis);
    Thermostat[ctr_output].temp_delta = max(Thermostat[ctr_output].temp_delta, (3*Thermostat[ctr_output].temp_hysteresis));
   
    //Thermostat[ctr_output].time_sunset = max(&Thermostat[ctr_output].time_sunset, 100);   
    Thermostat[ctr_output].temp_step = (Thermostat[ctr_output].temp_delta / Thermostat[ctr_output].time_sunset);
    Thermostat[ctr_output].temp_offset = Thermostat[ctr_output].temp_step * Thermostat[ctr_output].time_delta;
    Thermostat[ctr_output].temp_target_level = Thermostat[ctr_output].temp_bottom + Thermostat[ctr_output].temp_offset + Thermostat[ctr_output].temp_hysteresis ;}
   else {Thermostat[ctr_output].temp_target_level = Thermostat[ctr_output].temp_offset + Thermostat[ctr_output].temp_top;}
   
}

void ThermostatSignalPostProcessingSlow(uint8_t ctr_output)
{
  // Increase counter when inconsistent output state exists
  if ((Thermostat[ctr_output].status.status_output != Thermostat[ctr_output].status.command_output)
    &&(Thermostat[ctr_output].status.enable_output == IFACE_ON)) {
    Thermostat[ctr_output].diag.output_inconsist_ctr++;
  }
  else {
    Thermostat[ctr_output].diag.output_inconsist_ctr = 0;
  }
}

void ThermostatSignalProcessingFast(uint8_t ctr_output)
{
  // Update real status of the input
  Thermostat[ctr_output].status.status_input = (uint32_t)ThermostatInputStatus(Thermostat[ctr_output].status.input_switch_number);
  // Update timestamp of last input
  if (Thermostat[ctr_output].status.status_input == IFACE_ON) {
    Thermostat[ctr_output].timestamp_input_on = TasmotaGlobal.uptime;
  }
  // Update real status of the output
  Thermostat[ctr_output].status.status_output = (uint32_t)ThermostatOutputStatus(Thermostat[ctr_output].status.output_relay_number);
}


bool ThermostatStateAutoToManual(uint8_t ctr_output)
{
  bool change_state = false;
  // If input is used
  // AND switch input is active
  //  OR temperature sensor is not alive
  // then go to manual
  if (((Thermostat[ctr_output].status.status_input == IFACE_ON)
      || (Thermostat[ctr_output].status.sensor_alive == IFACE_OFF))) {
    change_state = true;
  }
  return change_state;
}

bool ThermostatStateManualToAuto(uint8_t ctr_output)
{
  bool change_state = false;

  // If switch input inactive
  // AND sensor alive
  // AND no switch input action (time in current state) bigger than a pre-defined time
  // then go to automatic
  if ((Thermostat[ctr_output].status.status_input == IFACE_OFF)
    &&(Thermostat[ctr_output].status.sensor_alive ==  IFACE_ON)
    && ((TasmotaGlobal.uptime - Thermostat[ctr_output].timestamp_input_on) > ((uint32_t)Thermostat[ctr_output].time_manual_to_auto * 60))) {
    change_state = true;
  }
  return change_state;
}

void ThermostatEmergencyShutdown(uint8_t ctr_output)
{
  // Emergency switch to THERMOSTAT_ON
 // Thermostat[ctr_output].status.thermostat_mode = THERMOSTAT_OFF;
  Thermostat[ctr_output].status.command_output = IFACE_ON;
  if (Thermostat[ctr_output].status.enable_output == IFACE_ON) {
    ThermostatOutputRelay(ctr_output, Thermostat[ctr_output].status.command_output);
  }
}

void ThermostatState(uint8_t ctr_output)
{
  switch (Thermostat[ctr_output].status.thermostat_mode) {
        // State automatic, thermostat active following the command target temp.
    case THERMOSTAT_AUTOMATIC_OP:
      if (ThermostatStateAutoToManual(ctr_output)) {
        // If sensor not alive change to THERMOSTAT_MANUAL_OP
        Thermostat[ctr_output].status.thermostat_mode = THERMOSTAT_MANUAL_OP;
      }
      //ThermostatCtrState(ctr_output);
      break;
    // State manual operation following input switch
    case THERMOSTAT_MANUAL_OP:
      if (ThermostatStateManualToAuto(ctr_output)) {
        // Input switch inactive and timeout reached change to THERMOSTAT_AUTOMATIC_OP
        Thermostat[ctr_output].status.thermostat_mode = THERMOSTAT_AUTOMATIC_OP;
      }
      break;
  }
}

void ThermostatOutputRelay(uint8_t ctr_output, uint32_t command)
{
  // If command received to enable output
  // AND current output status is OFF
  // then switch output to ON
  if ((command == IFACE_ON)
    && (Thermostat[ctr_output].status.status_output == IFACE_OFF)) {

    if (Thermostat[ctr_output].status.enable_output == IFACE_ON) {
      ExecuteCommandPower(Thermostat[ctr_output].status.output_relay_number, POWER_ON, SRC_THERMOSTAT);
    }

    Thermostat[ctr_output].status.status_output = IFACE_ON;

  }
  // If command received to disable output
  // AND current output status is ON
  // then switch output to OFF
  else if ((command == IFACE_OFF) && (Thermostat[ctr_output].status.status_output == IFACE_ON)) {
//#ifndef DEBUG_THERMOSTAT
    if (Thermostat[ctr_output].status.enable_output == IFACE_ON) {
      ExecuteCommandPower(Thermostat[ctr_output].status.output_relay_number, POWER_OFF, SRC_THERMOSTAT);
    }
//#endif // DEBUG_THERMOSTAT
    Thermostat[ctr_output].timestamp_output_off = TasmotaGlobal.uptime;
    Thermostat[ctr_output].status.status_output = IFACE_OFF;

  }
}

void ThermostatCtrWork(uint8_t ctr_output)  // Solar
{
  //bool flag_heating = (Thermostat[ctr_output].status.climate_mode == CLIMATE_HEATING);
  if  (Thermostat[ctr_output].temp_manifold < Thermostat[ctr_output].temp_target_level) {
    Thermostat[ctr_output].temp_target_level_ctr = Thermostat[ctr_output].temp_target_level;
    Thermostat[ctr_output].status.command_output = IFACE_OFF;                              // Switch OFF
  }
 
  if (Thermostat[ctr_output].temp_manifold > Thermostat[ctr_output].temp_target_level) {   // Manifold > Setpoint
    Thermostat[ctr_output].status.status_cycle_active = CYCLE_ON;
    Thermostat[ctr_output].status.command_output = IFACE_ON;                              // Switch ON
  }
}


void ThermostatWork(uint8_t ctr_output)
{
  switch (Thermostat[ctr_output].status.thermostat_mode) {
    
    // State automatic thermostat active following to command target temp.
    case THERMOSTAT_AUTOMATIC_OP:
      ThermostatCtrWork(ctr_output);

      break;
    // State manual operation following input switch
    case THERMOSTAT_MANUAL_OP:
      Thermostat[ctr_output].time_ctr_checkpoint = 0;
      Thermostat[ctr_output].status.command_output = Thermostat[ctr_output].status.status_input;
      break;
  }
  ThermostatOutputRelay(ctr_output, Thermostat[ctr_output].status.command_output);
}

void ThermostatDiagnostics(uint8_t ctr_output)
{
  // Diagnostic related to the plausibility of the output state
  if ((Thermostat[ctr_output].diag.diagnostic_mode == DIAGNOSTIC_ON)
    &&(Thermostat[ctr_output].diag.output_inconsist_ctr >= THERMOSTAT_TIME_MAX_OUTPUT_INCONSIST)) {
    //Thermostat[ctr_output].status.thermostat_mode = THERMOSTAT_OFF;
    Thermostat[ctr_output].diag.state_emergency = EMERGENCY_ON;
  }

    // If diagnostics fail, emergency enabled and thermostat shutdown triggered
  if (Thermostat[ctr_output].diag.state_emergency == EMERGENCY_ON) {
    ThermostatEmergencyShutdown(ctr_output);
  }
}

void ThermostatController(uint8_t ctr_output)
{
  ThermostatState(ctr_output);
  ThermostatWork(ctr_output);
}

#ifdef DEBUG_THERMOSTAT

void ThermostatDebug(uint8_t ctr_output)
{
  char result_chr[FLOATSZ];
  AddLog(LOG_LEVEL_DEBUG, PSTR(""));
  AddLog(LOG_LEVEL_DEBUG, PSTR("------ Thermostat Start ------"));
  dtostrfd(Thermostat[ctr_output].temp_bottom, 1, result_chr);
  AddLog(LOG_LEVEL_DEBUG, PSTR("Thermostat[ctr_output].temp_bottom: %s"), result_chr);
  dtostrfd(Thermostat[ctr_output].temp_top, 1, result_chr);
  AddLog(LOG_LEVEL_DEBUG, PSTR("Thermostat[ctr_output].temp_top: %s"), result_chr);
  dtostrfd(Thermostat[ctr_output].temp_manifold, 1, result_chr);
  AddLog(LOG_LEVEL_DEBUG, PSTR("Thermostat[ctr_output].temp_manifold: %s"), result_chr);
  dtostrfd(Thermostat[ctr_output].status.command_output, 0, result_chr);
  AddLog(LOG_LEVEL_DEBUG, PSTR("Thermostat[ctr_output].status.command_output: %s"), result_chr);
  dtostrfd(Thermostat[ctr_output].status.status_output, 0, result_chr);
  AddLog(LOG_LEVEL_DEBUG, PSTR("Thermostat[ctr_output].status.status_output: %s"), result_chr);
  dtostrfd(Thermostat[ctr_output].temp_offset, 2, result_chr);
  AddLog(LOG_LEVEL_DEBUG, PSTR("Thermostat[ctr_output].temp_offset: %s"), result_chr);
  dtostrfd(Thermostat[ctr_output].temp_delta, 1, result_chr);
  AddLog(LOG_LEVEL_DEBUG, PSTR("Thermostat[ctr_output].temp_delta: %s"), result_chr);
  dtostrfd(Thermostat[ctr_output].time_delta, 0, result_chr);
  AddLog(LOG_LEVEL_DEBUG, PSTR("Thermostat[ctr_output].time_delta: %s"), result_chr);
  dtostrfd(TasmotaGlobal.min_sunset, 0, result_chr);
  AddLog(LOG_LEVEL_DEBUG, PSTR("min_sunset: %s"), result_chr);
  dtostrfd(TasmotaGlobal.min_time, 0, result_chr);
  AddLog(LOG_LEVEL_DEBUG, PSTR("min_time: %s"), result_chr);
  dtostrfd(TasmotaGlobal.uptime, 0, result_chr);
  AddLog(LOG_LEVEL_DEBUG, PSTR("uptime: %s"), result_chr);
  dtostrfd(TasmotaGlobal.power, 0, result_chr);
  AddLog(LOG_LEVEL_DEBUG, PSTR("power: %s"), result_chr);
  AddLog(LOG_LEVEL_DEBUG, PSTR("------ Thermostat End ------"));
  AddLog(LOG_LEVEL_DEBUG, PSTR(""));
}
#endif // DEBUG_THERMOSTAT


void ThermostatGetLocalSensor(uint8_t ctr_output) {
  String buf = ResponseData();   // copy the string into a new buffer that will be modified
  JsonParser parser((char*)buf.c_str());
  JsonParserObject root = parser.getRootObject();
  if (root) {
    String sensor_name = THERMOSTAT_MANIFOLD_NAME;
    const char* value_c;
    
    JsonParserToken value_token = root[sensor_name].getObject()[PSTR("Temperature")];
    if (value_token.isNum()) {
      float value = value_token.getFloat();
        if ( (value >= -100)
        && (value <= 1000)
        ) {           
        Thermostat[ctr_output].temp_manifold = value;
        }
      }  
    }
  
  if (root) {
    String sensor_name = THERMOSTAT_TOP_NAME;
    const char* value_c;
    
    JsonParserToken value_token = root[sensor_name].getObject()[PSTR("Temperature")];
    if (value_token.isNum()) {
      float value = value_token.getFloat();
        if ( (value >= -100)
        && (value <= 1000)
        ) {          
          Thermostat[ctr_output].temp_top = value;
        }
      }   
    }
  
  if (root) {
    String sensor_name = THERMOSTAT_BOTTOM_NAME;
    const char* value_c;
    
    JsonParserToken value_token = root[sensor_name].getObject()[PSTR("Temperature")];
    if (value_token.isNum()) {
      float value = value_token.getFloat();
        if ( (value >= -100)
        && (value <= 1000)
        ) {          
           Thermostat[ctr_output].temp_bottom = value;
        }
      }
    }  
 
     //D_TIMER_TIME   
  
 
          uint32_t timestamp = TasmotaGlobal.uptime;
          Thermostat[ctr_output].timestamp_temp_meas_change_update = timestamp;
          Thermostat[ctr_output].status.sensor_alive = IFACE_ON;
      
//}

 
}
/*********************************************************************************************\
 * Commands
\*********************************************************************************************/

void CmndThermostatModeSet(void)
{
  if ((XdrvMailbox.index > 0) && (XdrvMailbox.index <= THERMOSTAT_CONTROLLER_OUTPUTS)) {
    uint8_t ctr_output = XdrvMailbox.index - 1;
    if (XdrvMailbox.data_len > 0) {
      uint8_t value = (uint8_t)(CharToFloat(XdrvMailbox.data));
      if ((value >= THERMOSTAT_AUTOMATIC_OP) && (value < THERMOSTAT_MODES_MAX)) {
        Thermostat[ctr_output].status.thermostat_mode = value;
        Thermostat[ctr_output].timestamp_input_on = 0;     // Reset last manual switch timer if command set externally
      }
    }
    ResponseCmndIdxNumber((int)Thermostat[ctr_output].status.thermostat_mode);
  }
}


void CmndTempFrostProtectSet(void)
{
  if ((XdrvMailbox.index > 0) && (XdrvMailbox.index <= THERMOSTAT_CONTROLLER_OUTPUTS)) {
    uint8_t ctr_output = XdrvMailbox.index - 1;
    int16_t value;
    if (XdrvMailbox.data_len > 0) {
      if (Thermostat[ctr_output].status.temp_format == TEMP_FAHRENHEIT) {
        value = (int16_t)ThermostatFahrenheitToCelsius((int32_t)(CharToFloat(XdrvMailbox.data)), TEMP_CONV_ABSOLUTE);//*10
      }
      else {
        value = (int16_t)(CharToFloat(XdrvMailbox.data));//*10
      }
      if ( (value >= -100)
        && (value <= 1000)) {
        Thermostat[ctr_output].temp_frost_protect = value;
      }
    }
    if (Thermostat[ctr_output].status.temp_format == TEMP_FAHRENHEIT) {
      value = ThermostatCelsiusToFahrenheit((int32_t)Thermostat[ctr_output].temp_frost_protect, TEMP_CONV_ABSOLUTE);
    }
    else {
      value = Thermostat[ctr_output].temp_frost_protect;
    }
    ResponseCmndIdxFloat((float)value , 1);
  }
}

void CmndTempFormatSet(void)
{
  if ((XdrvMailbox.index > 0) && (XdrvMailbox.index <= THERMOSTAT_CONTROLLER_OUTPUTS)) {
    uint8_t ctr_output = XdrvMailbox.index - 1;
    if (XdrvMailbox.data_len > 0) {
      uint8_t value = (uint8_t)(XdrvMailbox.payload);
      if ((value >= 0) && (value <= TEMP_FAHRENHEIT)) {
        Thermostat[ctr_output].status.temp_format = value;
      }
    }
    ResponseCmndIdxNumber((int)Thermostat[ctr_output].status.temp_format);
  }
}

void CmndStateEmergencySet(void)
{
  if ((XdrvMailbox.index > 0) && (XdrvMailbox.index <= THERMOSTAT_CONTROLLER_OUTPUTS)) {
    uint8_t ctr_output = XdrvMailbox.index - 1;
    if (XdrvMailbox.data_len > 0) {
      uint8_t value = (uint8_t)(XdrvMailbox.payload);
      if ((value >= 0) && (value <= 1)) {
        Thermostat[ctr_output].diag.state_emergency = (uint16_t)value;
      }
    }
    ResponseCmndIdxNumber((int)Thermostat[ctr_output].diag.state_emergency);
  }
}

void CmndTimeManualToAutoSet(void)
{
  if ((XdrvMailbox.index > 0) && (XdrvMailbox.index <= THERMOSTAT_CONTROLLER_OUTPUTS)) {
    uint8_t ctr_output = XdrvMailbox.index - 1;
    if (XdrvMailbox.data_len > 0) {
      uint32_t value = (uint32_t)(XdrvMailbox.payload);
      if ((value >= 0) && (value <= 1440)) {
        Thermostat[ctr_output].time_manual_to_auto = (uint16_t)value;
      }
    }
    ResponseCmndIdxNumber((int)((uint32_t)Thermostat[ctr_output].time_manual_to_auto));
  }
}

void CmndTempHystSet(void)    //TempHystSet
{
  if ((XdrvMailbox.index > 0) && (XdrvMailbox.index <= THERMOSTAT_CONTROLLER_OUTPUTS)) {
    uint8_t ctr_output = XdrvMailbox.index - 1;
    if (XdrvMailbox.data_len > 0) {
    float value = float(CharToFloat(XdrvMailbox.data));
    
      if ( (value >= -10)
        && (value <= 10)) {
        Thermostat[ctr_output].temp_hysteresis = value + 0.5;
      }
    //}
    ResponseCmndIdxFloat(Thermostat[ctr_output].temp_hysteresis, 1);
    }
  }
}

void CmndTimeSensLostSet(void)
{
  if ((XdrvMailbox.index > 0) && (XdrvMailbox.index <= THERMOSTAT_CONTROLLER_OUTPUTS)) {
    uint8_t ctr_output = XdrvMailbox.index - 1;
    if (XdrvMailbox.data_len > 0) {
      uint32_t value = (uint32_t)(XdrvMailbox.payload);
      if ((value >= 0) && (value <= 1440)) {
        Thermostat[ctr_output].time_sens_lost = (uint16_t)value;
      }
    }
    ResponseCmndIdxNumber((int)((uint32_t)Thermostat[ctr_output].time_sens_lost));
  }
}


void CmndDiagnosticModeSet(void)
{
  if ((XdrvMailbox.index > 0) && (XdrvMailbox.index <= THERMOSTAT_CONTROLLER_OUTPUTS)) {
    uint8_t ctr_output = XdrvMailbox.index - 1;
    if (XdrvMailbox.data_len > 0) {
      uint8_t value = (uint8_t)(CharToFloat(XdrvMailbox.data));
      if ((value >= DIAGNOSTIC_OFF) && (value <= DIAGNOSTIC_ON)) {
        Thermostat[ctr_output].diag.diagnostic_mode = value;
      }
    }
    ResponseCmndIdxNumber((int)Thermostat[ctr_output].diag.diagnostic_mode);
  }
}

void CmndEnableOutputSet(void)
{
  if ((XdrvMailbox.index > 0) && (XdrvMailbox.index <= THERMOSTAT_CONTROLLER_OUTPUTS)) {
    uint8_t ctr_output = XdrvMailbox.index - 1;
    if (XdrvMailbox.data_len > 0) {
      uint8_t value = (uint8_t)(CharToFloat(XdrvMailbox.data));
      if ((value >= IFACE_OFF) && (value <= IFACE_ON)) {
        Thermostat[ctr_output].status.enable_output = value;
      }
    }
    ResponseCmndIdxNumber((int)Thermostat[ctr_output].status.enable_output);
  }
}



/*********************************************************************************************\
 * Web UI
\*********************************************************************************************/


// To be done, add all of this defines in according languages file when all will be finished
// Avoid multiple changes on all language files during developement
// --------------------------------------------------
// xdrv_39_thermostat.ino
#define D_THERMOSTAT              "Pump"
#define D_THERMOSTAT_SET_POINT    "Set Point"
#define D_THERMOSTAT_MANIFOLD     "Manifold"
#define D_THERMOSTAT_TOP          "Tank Top"
#define D_THERMOSTAT_STEP         "Temp Step"
#define D_THERMOSTAT_OFFSET       "Temp Offset"
#define D_THERMOSTAT_DIFF         "Temp Delta"
#define D_THERMOSTAT_HYSTERESIS   "Hysteresis"
#define D_THERMOSTAT_TEMPERATURE  "Temperature"
#define D_THERMOSTAT_TIME         "Time"
//#define D_SUNSET                "Sunset"
// --------------------------------------------------


#ifdef USE_WEBSERVER
const char HTTP_THERMOSTAT_INFO[]        PROGMEM = "{s}" D_THERMOSTAT "{m}%s{e}";
const char HTTP_THERMOSTAT_TEMPERATURE[] PROGMEM = "{s}%s " D_TEMPERATURE "{m}%*_f " D_UNIT_DEGREE "%c{e}";
const char HTTP_THERMOSTAT_TIME[] PROGMEM = "{s}%s " D_THERMOSTAT_TIME "{m}%*_f " D_UNIT_MINUTE "%S{e}";
const char HTTP_THERMOSTAT_MANIFOLD[] PROGMEM = "{s}%s " D_THERMOSTAT_MANIFOLD"{m}%*_f " D_UNIT_DEGREE "%c{e}";
const char HTTP_THERMOSTAT_STEP[] PROGMEM = "{s}%s " D_THERMOSTAT_STEP "{m}%*_f " D_UNIT_DEGREE "%c{e}";
const char HTTP_THERMOSTAT_SUNSET[]  PROGMEM = "{s}%s" D_SUNSET "{m}%*_f" D_UNIT_MINUTE "%s{e}";
const char HTTP_THERMOSTAT_HYSTERESIS[]  PROGMEM = "{s}" D_THERMOSTAT_HYSTERESIS "{m}%*_f" D_UNIT_DEGREE "%c{e}";
const char HTTP_THERMOSTAT_HL[]          PROGMEM = "{s}<hr>{m}<hr>{e}";

#endif  // USE_WEBSERVER

void ThermostatShow(uint8_t ctr_output, bool json)        //console
{
  
    float f_target_temp = Thermostat[ctr_output].temp_target_level ;
    float f_manifold = Thermostat[ctr_output].temp_manifold;
    float f_tank_diff = Thermostat[ctr_output].temp_delta ;
    float f_tank_step = Thermostat[ctr_output].temp_step ;
    float f_time_sunset = TasmotaGlobal.min_sunset ;
    float f_time_now = TasmotaGlobal.min_time ;
    float f_offset = Thermostat[ctr_output].temp_offset ;
    float f_hysteresis = Thermostat[ctr_output].temp_hysteresis ;
  if (json) {  
    ResponseAppend_P(PSTR(",\"Thermostat%i\":{"), ctr_output);
    ResponseAppend_P(PSTR("%s\"%s\":%i"), "", D_CMND_OUTPUTRELAYSET, Thermostat[ctr_output].status.status_output);
    ResponseAppend_P(PSTR("%s\"%s\":%2_f"), ",", D_CMND_TEMPTARGETSET, &f_target_temp);
    ResponseAppend_P(PSTR("%s\"%s\":%2_f"), ",", D_THERMOSTAT_MANIFOLD, &f_manifold);
    ResponseAppend_P(PSTR("%s\"%s\":%2_f"), ",", D_THERMOSTAT_DIFF, &f_tank_diff);
    ResponseAppend_P(PSTR("%s\"%s\":%4_f"), ",", D_THERMOSTAT_STEP, &f_tank_step);
    ResponseAppend_P(PSTR("%s\"%s\":%i"), ",", D_SUNSET, TasmotaGlobal.min_sunset);
    ResponseAppend_P(PSTR("%s\"%s\":%i"), ",", D_THERMOSTAT_TIME, TasmotaGlobal.min_time);
    ResponseAppend_P(PSTR("%s\"%s\":%2_f"), ",", D_THERMOSTAT_OFFSET, &f_offset);
    ResponseAppend_P(PSTR("%s\"%s\":%2_f"), ",", D_THERMOSTAT_HYSTERESIS, &f_hysteresis);
    ResponseJsonEnd();
    return;
  }
#ifdef USE_WEBSERVER

  WSContentSend_P(HTTP_THERMOSTAT_HL);

  {
    char c_unit = Thermostat[ctr_output].status.temp_format==TEMP_CELSIUS ? D_UNIT_CELSIUS[0] : D_UNIT_FAHRENHEIT[0];
    float f_temperature ;
    uint16_t t_sunset;                   //sunset time in minutes
    float f_step;                   //now time in minutes

    //f_temperature = Thermostat[ctr_output].temp_target_level ;
    WSContentSend_PD(HTTP_THERMOSTAT_TEMPERATURE, D_THERMOSTAT_SET_POINT, Settings->flag2.temperature_resolution, &f_target_temp, c_unit);

    //f_temperature = Thermostat[ctr_output].temp_manifold ;
    WSContentSend_PD(HTTP_THERMOSTAT_TEMPERATURE, D_THERMOSTAT_MANIFOLD, Settings->flag2.temperature_resolution, &f_manifold, c_unit);

    //t_sunset = Thermostat[ctr_output].time_sunset ;   //Thermostat[ctr_output].time_sunset
    WSContentSend_PD(HTTP_THERMOSTAT_TIME, D_SUNSET, Settings->flag2.temperature_resolution, &f_time_sunset);

    //f_step = Thermostat[ctr_output].temp_step ;        //Thermostat[ctr_output].temp_step
    WSContentSend_PD(HTTP_THERMOSTAT_TEMPERATURE, D_THERMOSTAT_STEP, Settings->flag2.energy_resolution, &f_tank_step, c_unit);

    //f_temperature = Thermostat[ctr_output].temp_hysteresis ;
    WSContentSend_PD(HTTP_THERMOSTAT_TEMPERATURE, D_THERMOSTAT_HYSTERESIS, Settings->flag2.temperature_resolution, &f_hysteresis, c_unit);

    
    //WSContentSend_PD(HTTP_THERMOSTAT_TEMPERATURE, D_THERMOSTAT_GRADIENT, Settings->flag2.temperature_resolution, &f_temperature, c_unit);
    
    //WSContentSend_P(HTTP_THERMOSTAT_PUMP_TIME, Thermostat[ctr_output].time_pi_cycle );

   }

#endif  // USE_WEBSERVER
}



/*********************************************************************************************\
 * Interface
\*********************************************************************************************/

bool Xdrv39(uint32_t function)
{
  bool result = false;
  uint8_t ctr_output;
  //uint8_t sensor;

  switch (function) {
    case FUNC_INIT:
      for (ctr_output = 0; ctr_output < THERMOSTAT_CONTROLLER_OUTPUTS; ctr_output++) {
        ThermostatInit(ctr_output);
      }
      break;
    case FUNC_LOOP:
      for (ctr_output = 0; ctr_output < THERMOSTAT_CONTROLLER_OUTPUTS; ctr_output++) {
        if (Thermostat[ctr_output].status.thermostat_mode != THERMOSTAT_MANUAL_OP) {
          ThermostatSignalProcessingFast(ctr_output);
          ThermostatGetLocalSensor(ctr_output);
          ThermostatDiagnostics(ctr_output);
        }
      }
      break;
    case FUNC_SERIAL:
      break;
    case FUNC_EVERY_SECOND:
      for (ctr_output = 0; ctr_output < THERMOSTAT_CONTROLLER_OUTPUTS; ctr_output++) {
        ThermostatGetLocalSensor(ctr_output);
        ThermostatController(ctr_output);
        if ((ThermostatMinuteCounter(ctr_output))
          ) {                 //&& (Thermostat[ctr_output].status.thermostat_mode != THERMOSTAT_MANUAL_OP)
          ThermostatSignalPreProcessingSlow(ctr_output);
          ThermostatSignalPostProcessingSlow(ctr_output);
#ifdef DEBUG_THERMOSTAT
          ThermostatDebug(ctr_output);
#endif // DEBUG_THERMOSTAT
        }
      }
      break;
    case FUNC_SHOW_SENSOR:
      for (ctr_output = 0; ctr_output < THERMOSTAT_CONTROLLER_OUTPUTS; ctr_output++) {
          ThermostatGetLocalSensor(ctr_output);
 
      }
      
      break;
    case FUNC_JSON_APPEND:
      for (ctr_output = 0; ctr_output < THERMOSTAT_CONTROLLER_OUTPUTS; ctr_output++) {
        ThermostatShow(ctr_output, true);
      }
      break;

#ifdef USE_WEBSERVER
    case FUNC_WEB_SENSOR:
      for (ctr_output = 0; ctr_output < THERMOSTAT_CONTROLLER_OUTPUTS; ctr_output++) {
        ThermostatShow(ctr_output, false);
      }
      break;
#endif  // USE_WEBSERVER

    case FUNC_COMMAND:
      result = DecodeCommand(kThermostatCommands, ThermostatCommand);
      break;
  }
  return result;
}

#endif // USE_THERMOSTAT