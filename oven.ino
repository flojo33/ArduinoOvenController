#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <RotaryEncoder.h>
#include <Button.h>
#include <Adafruit_MAX31865.h>

#define OutputRelais 2
#define DisplayAddress 0x27
#define InputEncoderA 6
#define InputEncoderB 7
#define TemperatureSettingStep 0.5
#define TotalProgramCount 8
#define TotalStepCount 8

//Main Menus:
#define MENU_HOME 0
#define MENU_START_SELECTION 10
#define MENU_PROGRAM_EDIT_SELECTION 20
#define MENU_PROGRAM_EDIT 21
#define MENU_STEP_EDIT 22 
#define MENU_OFFSET_EDIT 30
#define MENU_RUNNING 100
#define MENU_RUNNING_SINGLE 200

#define HOME_MENU_STATES 4
#define HOME_MENU_START_SINGLE 0
#define HOME_MENU_START 1
#define HOME_MENU_SELECT_PROGRAM 2
#define HOME_MENU_EDIT_OFFSET 3

#define STEP_EDIT_STATE_DURATION 0
#define STEP_EDIT_STATE_TEMPERATURE 1


const double RREF = 4300.0;

int programDurations[TotalProgramCount * TotalStepCount];
double programTemperature[TotalProgramCount * TotalStepCount];
double singleTargetTemperature = 40.0;
Adafruit_MAX31865 max = Adafruit_MAX31865(10, 11, 12, 13);

int currentMenu = MENU_HOME;
int currentHomeMenuSelection = HOME_MENU_START_SINGLE;
int currentProgramSelection = 0;
int currentStepSelection = 0;
int currentStepEditState = STEP_EDIT_STATE_DURATION;

float lastProgramSwitch;

bool runningCancelClicked = false;

struct SaveData
{
  double tempOffset;
  int programDurations[TotalProgramCount * TotalStepCount];
  double programTemperature[TotalProgramCount * TotalStepCount];
};

SaveData EEMEM eep_data;

//Display
LiquidCrystal_I2C lcd(DisplayAddress, 16, 2);

//Temperatures
double temperature = 21.0;

//PT1000
uint16_t currentResistance = 0;

// Encoder:
RotaryEncoder encoder(InputEncoderA, InputEncoderB, RotaryEncoder::LatchMode::FOUR3);
Button rotaryButton(3);
int encoderLastState;

//Heating
bool heatingActive = false;
float last_temperature_read = 0.0f;
int updateRate = 1000;

double tempOffset = - 0.6;

bool displayChanged = true;
  
void setup() {
  Serial.begin(115200);

  max.begin(MAX31865_2WIRE);
  rotaryButton.begin();
  // put your setup code here, to run once:
  lcd.init();
  lcd.backlight();
  pinMode(OutputRelais, OUTPUT);
  ReadSettings();
  ReadTempOffset();
  //FactoryReset();
}


void loop() {
  UpdateEncoder();
  UpdateController();
  Render();
}

void UpdateController() {
  if(currentMenu != MENU_RUNNING && currentMenu != MENU_RUNNING_SINGLE) {
    SetHeatingState(false);
    return;
  } else {
    double targetTemperature; 
    if(currentMenu == MENU_RUNNING) {
      if(millis() - lastProgramSwitch >= programDurations[GetSelectedStepId()] * 60 * 1000.0f) {
        lastProgramSwitch = millis();
        currentStepSelection = currentStepSelection + 1;
        UpdateDisplay();
      }
      if(currentStepSelection >= TotalStepCount) {
        //Finished -> Back to home screen
        currentMenu = MENU_HOME;
        return;
      }
      targetTemperature = programTemperature[GetSelectedStepId()];
    } else {
      //SINGLE PROGRAM!
      targetTemperature = singleTargetTemperature;
    }
    

    if(millis() - last_temperature_read > updateRate) {
      last_temperature_read = millis();

      currentResistance = max.readRTD();

      //Serial.print("RTD value: "); Serial.println(currentResistance);
      float ratio = currentResistance;
      ratio /= 32768;
      //Serial.print("Ratio = "); Serial.println(ratio,8);
      //Serial.print("Resistance = "); Serial.println(RREF*ratio,8);
      
      temperature = max.temperature(1000, RREF);
      //Serial.print("Temperatue = "); Serial.println(temperature, 8);

      if(heatingActive)
      {
        SetHeatingState(true);
        if(temperature > targetTemperature + tempOffset) {
          heatingActive = false;
        }
      }
      else  
      {
        SetHeatingState(false);
        if(temperature < targetTemperature + tempOffset) {
          heatingActive = true;
        }
      }
      //Serial.println("BLUB" + String(last_temperature_read));
      UpdateDisplay();
    }

    SetHeatingState(heatingActive);
  }
}

void UpdateEncoder() {
  encoder.tick();
  int encoderState = encoder.getPosition();
  if (encoderState != encoderLastState) {
    bool encoderChange = (int)encoder.getDirection() < 0;
    Serial.print("Encoder Change: ");
    Serial.println(encoderChange ? "1" : "0");
    ProcessEncoderInput(encoderChange);
    encoderLastState = encoderState;
  }
  if(rotaryButton.pressed()) {
    Serial.println("Click");
    ProcessClick();
  }
}

double ReadSettings() {
  EEPROM.get((int)&eep_data.programDurations, programDurations);
  EEPROM.get((int)&eep_data.programTemperature, programTemperature);
}

void SaveSettings() {
  EEPROM.put((int)&eep_data.programDurations, programDurations);
  EEPROM.put((int)&eep_data.programTemperature, programTemperature);
}

void ReadTempOffset() {
  EEPROM.get((int)&eep_data.tempOffset, tempOffset);
}
void SaveTempOffset() {
  EEPROM.put((int)&eep_data.tempOffset, tempOffset);
}

void Render() {
  if(!displayChanged) return;
  displayChanged = false;
  switch(currentMenu) {
    case MENU_HOME:
      UpdateMainMenuDisplay();
    break;
    case MENU_RUNNING:
      UpdateRunningDisplay();
    break;
    case MENU_RUNNING_SINGLE:
      UpdateRunningSingleDisplay();
    break;
    case MENU_START_SELECTION:
      UpdateStartSelectionDisplay();
    break;
    case MENU_PROGRAM_EDIT_SELECTION:
      UpdateProgramEditSelectionDisplay();
    break;
    case MENU_PROGRAM_EDIT:
      UpdateProgramEditDisplay();
    break;
    case MENU_STEP_EDIT:
      UpdateStepEditDisplay();
    break;
    case MENU_OFFSET_EDIT:
      UpdateOffsetEditDisplay();
    break;
    default:
      SetDisplay("ERROR", "MENU_NOT_KNOWN");
    break;
  }
}

void UpdateMainMenuDisplay() {
  switch(currentHomeMenuSelection) {
    case HOME_MENU_START_SINGLE:
      SetDisplay("Manuell", "Starten");
    break;
    case HOME_MENU_START:
      SetDisplay("Programm", "Starten");
    break;
    case HOME_MENU_SELECT_PROGRAM:
      SetDisplay("Programme", "Bearbeiten");
    break;
    case HOME_MENU_EDIT_OFFSET:
      SetDisplay("Relais-Offset", "Bearbeiten");
    break;
    default:
      SetDisplay("ERROR", "PAGE_NOT_KNOWN");
    break;
  }
}

void UpdateStartSelectionDisplay() {
  if(currentProgramSelection < TotalProgramCount) {
    SetDisplay("STARTEN", "P" + String(currentProgramSelection+1));
  } else {
    RenderBack();
  }
}

void UpdateProgramEditSelectionDisplay() {
  if(currentProgramSelection < TotalProgramCount) {
    SetDisplay("BE (BEARBEITEN)", "P" + String(currentProgramSelection+1));
  } else {
    RenderBack();
  }
}

void UpdateProgramEditDisplay() {
  if(currentStepSelection < TotalProgramCount) {
    String info = " ---";
    if(programDurations[GetSelectedStepId()] > 0) {
      info = FormatMinutes(programDurations[GetSelectedStepId()]) + " @ " + FormatTemperature(programTemperature[GetSelectedStepId()]);
    }
    SetDisplay("BE-P" + String(currentProgramSelection+1) + "-S (Step)", "S" + String(currentStepSelection + 1) + ":" + info);
  } else {
    SetDisplay("Fertig", "<-");
  }
}

void UpdateStepEditDisplay() {
  String info;
  if(currentStepEditState == STEP_EDIT_STATE_DURATION) {
    if(programDurations[GetSelectedStepId()] > 0) {
      info = "Dauer: " + FormatMinutes(programDurations[GetSelectedStepId()]);
    } else {
      info = "Deaktiviert";
    }
  } else {
    info = "Temp: " + FormatTemperature(programTemperature[GetSelectedStepId()]);
  }
  SetDisplay("BE-P" + String(currentProgramSelection+1) + "-S" + String(currentStepSelection + 1), info);
}

void UpdateOffsetEditDisplay() {
    SetDisplay("Relais Offset:", String(tempOffset, 1) + "c");
    SaveTempOffset();
}

int GetSelectedStepId() {
  return currentProgramSelection * TotalStepCount + currentStepSelection;
}

String FormatTemperature(double temperature) {
  if(temperature < 10) return "00" + String(temperature, 1) + "c";
  if(temperature < 100) return "0" + String(temperature, 1) + "c";
  return String(temperature, 1) + "c";
}

String FormatMinutes(int minutes) {
  if(minutes < 10) return "00" + String(minutes) + "m";
  if(minutes < 100) return "0" + String(minutes) + "m";
  return String(minutes) + "m";
}

void ProcessEncoderInput(bool positive) {
  int direction;
  if(positive) {
    direction = 1;
    Serial.println("Encoder ++");
  } else {
    direction = -1;
    Serial.println("Encoder --");
  }
  switch(currentMenu) {
    case MENU_HOME:
      currentHomeMenuSelection = Clamp(currentHomeMenuSelection + direction, 0, HOME_MENU_STATES - 1);
      UpdateDisplay();
    break;
    case MENU_RUNNING_SINGLE:
      if(runningCancelClicked) {
        runningCancelClicked = false;
      } else {
        singleTargetTemperature = Clamp(singleTargetTemperature + direction, 30.0, 250.0); // + 1 for back button
      }
      UpdateDisplay();
    break;
    case MENU_RUNNING:
      runningCancelClicked = false;
      UpdateDisplay();
    break;
    case MENU_START_SELECTION:
      currentProgramSelection = Clamp(currentProgramSelection + direction, 0, TotalProgramCount); // + 1 for back button
      UpdateDisplay();
    break;
    case MENU_PROGRAM_EDIT_SELECTION:
      currentProgramSelection = Clamp(currentProgramSelection + direction, 0, TotalProgramCount); // + 1 for back button
      UpdateDisplay();
    break;
    case MENU_PROGRAM_EDIT:
      currentStepSelection = Clamp(currentStepSelection + direction, 0, TotalStepCount); // + 1 for back button
      UpdateDisplay();
    break;
    case MENU_OFFSET_EDIT:
      tempOffset = Clamp(tempOffset + direction * 0.1, -10.0, 10.0); // + 1 for back button
      UpdateDisplay();
    break;
    case MENU_STEP_EDIT:
      switch(currentStepEditState) {
        case STEP_EDIT_STATE_DURATION:
          programDurations[GetSelectedStepId()] = Clamp(programDurations[GetSelectedStepId()] + direction, 0, 999);
          SaveSettings();
        break;
        case STEP_EDIT_STATE_TEMPERATURE:
          programTemperature[GetSelectedStepId()] = Clamp(programTemperature[GetSelectedStepId()] + direction * 1.0, 15.0, 250.0);
          SaveSettings();
        break;
      }
      UpdateDisplay();
    break;
  }
}

void ProcessClick() {
   switch(currentMenu) {
    case MENU_HOME:
      ProcessMainMenuClick();
    break;
    case MENU_START_SELECTION:
      ProcessStartSelectionClick();
    break;
    case MENU_PROGRAM_EDIT_SELECTION:
      ProcessProgramEditSelectionClick();
    break;
    case MENU_PROGRAM_EDIT:
      ProcessProgramEditClick();
    break;
    case MENU_STEP_EDIT:
      ProcessStepEditClick();
    break;
    case MENU_OFFSET_EDIT:
      currentMenu = MENU_HOME;
      UpdateDisplay();
    break;
    case MENU_RUNNING_SINGLE:
      ProcessRunningClick();
    break;
    case MENU_RUNNING:
      ProcessRunningClick();
    break;
  }
}

void FactoryReset() {
  for(int i = 0; i < TotalProgramCount; i++) {
    for(int j = 0; j < TotalStepCount; j++) {
      programDurations[i * TotalStepCount + j] = 0;
      programTemperature[i * TotalStepCount + j] = 30.0;
    }
  }
  SaveSettings();
}

void ProcessMainMenuClick() {
  switch(currentHomeMenuSelection) {
    case HOME_MENU_START_SINGLE:
      currentMenu = MENU_RUNNING_SINGLE;
    break;
    case HOME_MENU_START:
      currentMenu = MENU_START_SELECTION;
    break;
    case HOME_MENU_SELECT_PROGRAM:
      currentMenu = MENU_PROGRAM_EDIT_SELECTION;
    break;
    case HOME_MENU_EDIT_OFFSET:
      currentMenu = MENU_OFFSET_EDIT;
    break;
  }
  currentHomeMenuSelection = 0;
  UpdateDisplay();
}

void ProcessStartSelectionClick() {
  if(currentProgramSelection == TotalProgramCount) {
    currentProgramSelection = 0;
    currentMenu = MENU_HOME;
  } else {
    runningCancelClicked = false;
    currentStepSelection = 0;
    lastProgramSwitch = millis();
    currentMenu = MENU_RUNNING;
  }
  UpdateDisplay();
}

void ProcessProgramEditSelectionClick() {
  if(currentProgramSelection == TotalProgramCount) {
    currentProgramSelection = 0;
    currentMenu = MENU_HOME;
  } else {
    currentMenu = MENU_PROGRAM_EDIT;
  }
  UpdateDisplay();
}

void ProcessProgramEditClick() {
  if(currentStepSelection == TotalStepCount) {
    currentStepSelection = 0;
    currentMenu = MENU_PROGRAM_EDIT_SELECTION;
  } else {
    currentMenu = MENU_STEP_EDIT;
  }
  UpdateDisplay();
}

void ProcessStepEditClick() {
  switch(currentStepEditState) {
    case STEP_EDIT_STATE_DURATION:
      if(programDurations[GetSelectedStepId()] == 0) {
        currentMenu = MENU_PROGRAM_EDIT;
      } else {
        currentStepEditState = STEP_EDIT_STATE_TEMPERATURE;
      }
    break;
    case STEP_EDIT_STATE_TEMPERATURE:
      currentStepEditState = STEP_EDIT_STATE_DURATION;
      currentMenu = MENU_PROGRAM_EDIT;
    break;
  }
  UpdateDisplay();
}

void ProcessRunningClick() {
  runningCancelClicked = !runningCancelClicked;
  if(!runningCancelClicked) {
      currentMenu = MENU_HOME;
      //FactoryReset();
  }
  UpdateDisplay();
}

int Clamp(int value, int min, int max) {
  if(value < min) return max;
  if(value > max) return min;
  return value;
}
double Clamp(double value, double min, double max) {
  if(value < min) return max;
  if(value > max) return min;
  return value;
}

void UpdateDisplay() {
  displayChanged = true;
}

void UpdateRunningDisplay() {
  if(runningCancelClicked) {
    SetDisplay("Ablauf wirklich", "ABBRECHEN?");
  } else {
    SetDisplay("Temp: " + FormatTemperature(temperature) + " P" + String(currentProgramSelection+1), "Ziel: " + FormatTemperature(programTemperature[GetSelectedStepId()]) + " S" + String(currentStepSelection+1));
  }
}


void UpdateRunningSingleDisplay() {
  if(runningCancelClicked) {
    SetDisplay("Ablauf wirklich", "ABBRECHEN?");
  } else {
    SetDisplay("Temp: " + FormatTemperature(temperature), "Ziel: " + FormatTemperature(singleTargetTemperature));
  }
}

void RenderBack() {
  SetDisplay("Zurueck", "<-");
}

void SetDisplay(String topLine, String bottomLine) {
  Serial.println("Set Display:");
  Serial.println(topLine);
  Serial.println(bottomLine);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(topLine);
  lcd.setCursor(0, 1);
  lcd.print(bottomLine);
}

double round_up(double value) {
    const double multiplier = 100;
    return ceil(value * multiplier) / multiplier;
}

void SetHeatingState(bool state) {
  digitalWrite(OutputRelais, state);
}
