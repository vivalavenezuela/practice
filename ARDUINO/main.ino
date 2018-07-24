
#define KEEP_SETTINGS 1    
#define KEEP_STATE 1		   
#define RESET_SETTINGS 0  



#define NUM_LEDS 24        
byte BRIGHTNESS = 200;      
 
#define SOUND_R A2         
#define SOUND_L A1         
#define SOUND_R_FREQ A3    
#define BTN_PIN 3         
#define LED_PIN 12 
#define POT_GND A0         // пин земля для потенциометра
#define IR_PIN 2      


float RAINBOW_STEP = 5.5;   


#define MODE 0              
#define MAIN_LOOP 5         


#define MONO 1              
#define EXP 1.4             
#define POTENT 1            
byte EMPTY_BRIGHT = 30;           
#define EMPTY_COLOR HUE_PURPLE   


uint16_t LOW_PASS = 100;         
uint16_t SPEKTR_LOW_PASS = 40;   
#define AUTO_LOW_PASS 0     
#define EEPROM_LOW_PASS 1  
#define LOW_PASS_ADD 13    
#define LOW_PASS_FREQ_ADD 3 


float SMOOTH = 0.5;        
#define MAX_COEF 1.8       


float SMOOTH_FREQ = 0.8;          
float MAX_COEF_FREQ = 1.2;        
#define SMOOTH_STEP 20            
#define LOW_COLOR HUE_RED         
#define MID_COLOR HUE_GREEN       


int STROBE_PERIOD = 100;          
#define STROBE_DUTY 20            
#define STROBE_COLOR HUE_YELLOW   
#define STROBE_SAT 0             
byte STROBE_SMOOTH = 100;        


byte LIGHT_COLOR = 0;              
byte LIGHT_SAT = 200;             
byte COLOR_SPEED = 100;
int RAINBOW_PERIOD = 3;
float RAINBOW_STEP_2 = 5.5;


byte RUNNING_SPEED = 60;


byte HUE_START = 0;
byte HUE_STEP = 5;
#define LIGHT_SMOOTH 2


#define BUTT_UP     0xE51CA6AD
#define BUTT_DOWN   0xD22353AD
#define BUTT_LEFT   0x517068AD
#define BUTT_RIGHT  0xAC2A56AD
#define BUTT_OK     0x1B92DDAD
#define BUTT_1      0x68E456AD
#define BUTT_2      0xF08A26AD
#define BUTT_3      0x151CD6AD
#define BUTT_4      0x18319BAD
#define BUTT_5      0xF39EEBAD
#define BUTT_6      0x4AABDFAD
#define BUTT_7      0xE25410AD
#define BUTT_8      0x297C76AD
#define BUTT_9      0x14CE54AD
#define BUTT_0      0xC089F6AD
#define BUTT_STAR   0xAF3F1BAD
#define BUTT_HASH   0x38379AD


#define MODE_AMOUNT 9      

#define STRIPE NUM_LEDS / 5
float freq_to_stripe = NUM_LEDS / 40;

#define FHT_N 64       
#define LOG_OUT 1
#include <FHT.h>         

#include <EEPROMex.h>

#define FASTLED_ALLOW_INTERRUPTS 1
#include "FastLED.h"
CRGB leds[NUM_LEDS];

#include "GyverButton.h"
GButton butt1(BTN_PIN);

#include "IRLremote.h"
CHashIR IRLremote;
uint32_t IRdata;


DEFINE_GRADIENT_PALETTE(soundlevel_gp) {
  0,    0,    255,  0,  // green
  100,  255,  255,  0,  // yellow
  150,  255,  100,  0,  // orange
  200,  255,  50,   0,  // red
  255,  255,  0,    0   // red
};
CRGBPalette32 myPal = soundlevel_gp;

byte Rlenght, Llenght;
float RsoundLevel, RsoundLevel_f;
float LsoundLevel, LsoundLevel_f;

float averageLevel = 50;
int maxLevel = 100;
byte MAX_CH = NUM_LEDS / 2;
int hue;
unsigned long main_timer, hue_timer, strobe_timer, running_timer, color_timer, rainbow_timer, eeprom_timer;
float averK = 0.006;
byte count;
float index = (float)255 / MAX_CH;   
boolean lowFlag;
byte low_pass;
int RcurrentLevel, LcurrentLevel;
int colorMusic[3];
float colorMusic_f[3], colorMusic_aver[3];
boolean colorMusicFlash[3], strobeUp_flag, strobeDwn_flag;
byte this_mode = MODE;
int thisBright[3], strobe_bright = 0;
unsigned int light_time = STROBE_PERIOD * STROBE_DUTY / 100;
volatile boolean ir_flag;
boolean settings_mode, ONstate = true;
int8_t freq_strobe_mode, light_mode;
int freq_max;
float freq_max_f, rainbow_steps;
int freq_f[32];
int this_color;
boolean running_flag[3], eeprom_flag;

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))


void setup() 
{
  Serial.begin(9600);
  FastLED.addLeds<WS2811, LED_PIN, GRB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(BRIGHTNESS);

  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  pinMode(POT_GND, OUTPUT);
  digitalWrite(POT_GND, LOW);
  butt1.setTimeout(900);

  IRLremote.begin(IR_PIN);


  if (POTENT) analogReference(EXTERNAL);
  else analogReference(INTERNAL);


  sbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  sbi(ADCSRA, ADPS0);

  if (RESET_SETTINGS) EEPROM.write(100, 0);        

  if (AUTO_LOW_PASS && !EEPROM_LOW_PASS) 
  {         
    autoLowPass();
  }
  if (EEPROM_LOW_PASS) 
  {                
    LOW_PASS = EEPROM.readInt(70);
    SPEKTR_LOW_PASS = EEPROM.readInt(72);
  }


  if (KEEP_SETTINGS) 
  {
    if (EEPROM.read(100) != 100) 
    {
      Serial.println(F("First start"));
      EEPROM.write(100, 100);
      updateEEPROM();
    } else readEEPROM();
  }
}

void loop() 
{
  buttonTick();     
  remoteTick();     
  mainLoop();       
  eepromTick();     
}

void mainLoop() 
{
  if (ONstate) 
  {
    if (millis() - main_timer > MAIN_LOOP) 
    {
      RsoundLevel = 0;
      LsoundLevel = 0;

      if (this_mode == 0 || this_mode == 1) 
      {
        for (byte i = 0; i < 100; i ++) 
        {                                 
          RcurrentLevel = analogRead(SOUND_R);                            
          if (!MONO) LcurrentLevel = analogRead(SOUND_L);                 

          if (RsoundLevel < RcurrentLevel) RsoundLevel = RcurrentLevel;   
          if (!MONO) if (LsoundLevel < LcurrentLevel) LsoundLevel = LcurrentLevel;   
        }

        RsoundLevel = map(RsoundLevel, LOW_PASS, 1023, 0, 500);
        if (!MONO)LsoundLevel = map(LsoundLevel, LOW_PASS, 1023, 0, 500);

        RsoundLevel = constrain(RsoundLevel, 0, 500);
        if (!MONO)LsoundLevel = constrain(LsoundLevel, 0, 500);


        RsoundLevel = pow(RsoundLevel, EXP);
        if (!MONO)LsoundLevel = pow(LsoundLevel, EXP);

        RsoundLevel_f = RsoundLevel * SMOOTH + RsoundLevel_f * (1 - SMOOTH);
        if (!MONO)LsoundLevel_f = LsoundLevel * SMOOTH + LsoundLevel_f * (1 - SMOOTH);

        if (MONO) LsoundLevel_f = RsoundLevel_f;  


        if (EMPTY_BRIGHT > 5) 
        {
          for (int i = 0; i < NUM_LEDS; i++)
            leds[i] = CHSV(EMPTY_COLOR, 255, EMPTY_BRIGHT);
        }

        if (RsoundLevel_f > 15 && LsoundLevel_f > 15) 
        {
          averageLevel = (float)(RsoundLevel_f + LsoundLevel_f) / 2 * averK + averageLevel * (1 - averK);
          maxLevel = (float)averageLevel * MAX_COEF;
          Rlenght = map(RsoundLevel_f, 0, maxLevel, 0, MAX_CH);
          Llenght = map(LsoundLevel_f, 0, maxLevel, 0, MAX_CH);
          Rlenght = constrain(Rlenght, 0, MAX_CH);
          Llenght = constrain(Llenght, 0, MAX_CH);

          animation();      
        }
      }

      if (this_mode == 2 || this_mode == 3 || this_mode == 4 || this_mode == 7 || this_mode == 8) 
      {
        analyzeAudio();
        colorMusic[0] = 0;
        colorMusic[1] = 0;
        colorMusic[2] = 0;
        for (int i = 0 ; i < 32 ; i++) 
        {
          if (fht_log_out[i] < SPEKTR_LOW_PASS) fht_log_out[i] = 0;
        }
        for (byte i = 2; i < 6; i++) 
        {
          if (fht_log_out[i] > colorMusic[0]) colorMusic[0] = fht_log_out[i];
        }

        for (byte i = 6; i < 11; i++) 
        {
          if (fht_log_out[i] > colorMusic[1]) colorMusic[1] = fht_log_out[i];
        }
        for (byte i = 11; i < 32; i++) 
        {
          if (fht_log_out[i] > colorMusic[2]) colorMusic[2] = fht_log_out[i];
        }
        freq_max = 0;
        for (byte i = 0; i < 30; i++) 
        {
          if (fht_log_out[i + 2] > freq_max) freq_max = fht_log_out[i + 2];
          if (freq_max < 5) freq_max = 5;
          if (freq_f[i] < fht_log_out[i + 2]) freq_f[i] = fht_log_out[i + 2];
          if (freq_f[i] > 0) freq_f[i] -= LIGHT_SMOOTH;
          else freq_f[i] = 0;
        }
        freq_max_f = freq_max * averK + freq_max_f * (1 - averK);
        for (byte i = 0; i < 3; i++) 
        {
          colorMusic_aver[i] = colorMusic[i] * averK + colorMusic_aver[i] * (1 - averK); 
          colorMusic_f[i] = colorMusic[i] * SMOOTH_FREQ + colorMusic_f[i] * (1 - SMOOTH_FREQ);    
          if (colorMusic_f[i] > ((float)colorMusic_aver[i] * MAX_COEF_FREQ)) 
          {
            thisBright[i] = 255;
            colorMusicFlash[i] = true;
            running_flag[i] = true;
          } else colorMusicFlash[i] = false;
          if (thisBright[i] >= 0) thisBright[i] -= SMOOTH_STEP;
          if (thisBright[i] < EMPTY_BRIGHT) 
          {
            thisBright[i] = EMPTY_BRIGHT;
            running_flag[i] = false;
          }
        }
        animation();
      }
      if (this_mode == 5) 
      {
        if ((long)millis() - strobe_timer > STROBE_PERIOD) 
        {
          strobe_timer = millis();
          strobeUp_flag = true;
          strobeDwn_flag = false;
        }
        if ((long)millis() - strobe_timer > light_time) 
        {
          strobeDwn_flag = true;
        }
        if (strobeUp_flag) 
        {                    
          if (strobe_bright < 255)              
            strobe_bright += STROBE_SMOOTH;     
          if (strobe_bright > 255) 
          {            
            strobe_bright = 255;                
            strobeUp_flag = false;              
          }
        }

        if (strobeDwn_flag) 
        {                   
          if (strobe_bright > 0)                
            strobe_bright -= STROBE_SMOOTH;     
          if (strobe_bright < 0) 
          {              
            strobeDwn_flag = false;
            strobe_bright = 0;                  
          }
        }
        animation();
      }
      if (this_mode == 6) animation();

      if (!IRLremote.receiving())    
        FastLED.show();         
      if (this_mode != 7)      
        FastLED.clear();        
      main_timer = millis();    
    }
  }
}

void animation() 
{
  switch (this_mode) 
  {
    case 0:
      count = 0;
      for (int i = (MAX_CH - 1); i > ((MAX_CH - 1) - Rlenght); i--) 
      {
        leds[i] = ColorFromPalette(myPal, (count * index));   
        count++;
      }
      count = 0;
      for (int i = (MAX_CH); i < (MAX_CH + Llenght); i++ ) 
      {
        leds[i] = ColorFromPalette(myPal, (count * index));  
        count++;
      }
      if (EMPTY_BRIGHT > 0) 
      {
        CHSV this_dark = CHSV(EMPTY_COLOR, 255, EMPTY_BRIGHT);
        for (int i = ((MAX_CH - 1) - Rlenght); i > 0; i--)
          leds[i] = this_dark;
        for (int i = MAX_CH + Llenght; i < NUM_LEDS; i++)
          leds[i] = this_dark;
      }
      break;
    case 1:
      if (millis() - rainbow_timer > 30) 
      {
        rainbow_timer = millis();
        hue = floor((float)hue + RAINBOW_STEP);
      }
      count = 0;
      for (int i = (MAX_CH - 1); i > ((MAX_CH - 1) - Rlenght); i--) 
      {
        leds[i] = ColorFromPalette(RainbowColors_p, (count * index) / 2 - hue);  
        count++;
      }
      count = 0;
      for (int i = (MAX_CH); i < (MAX_CH + Llenght); i++ ) 
      {
        leds[i] = ColorFromPalette(RainbowColors_p, (count * index) / 2 - hue); 
        count++;
      }
      if (EMPTY_BRIGHT > 0) {
        CHSV this_dark = CHSV(EMPTY_COLOR, 255, EMPTY_BRIGHT);
        for (int i = ((MAX_CH - 1) - Rlenght); i > 0; i--)
          leds[i] = this_dark;
        for (int i = MAX_CH + Llenght; i < NUM_LEDS; i++)
          leds[i] = this_dark;
      }
      break;
    case 2:
      for (int i = 0; i < NUM_LEDS; i++) 
      {
        if (i < STRIPE)          leds[i] = CHSV(HIGH_COLOR, 255, thisBright[2]);
        else if (i < STRIPE * 2) leds[i] = CHSV(MID_COLOR, 255, thisBright[1]);
        else if (i < STRIPE * 3) leds[i] = CHSV(LOW_COLOR, 255, thisBright[0]);
        else if (i < STRIPE * 4) leds[i] = CHSV(MID_COLOR, 255, thisBright[1]);
        else if (i < STRIPE * 5) leds[i] = CHSV(HIGH_COLOR, 255, thisBright[2]);
      }
      break;
    case 3:
      for (int i = 0; i < NUM_LEDS; i++) 
      {
        if (i < NUM_LEDS / 3)          leds[i] = CHSV(HIGH_COLOR, 255, thisBright[2]);
        else if (i < NUM_LEDS * 2 / 3) leds[i] = CHSV(MID_COLOR, 255, thisBright[1]);
        else if (i < NUM_LEDS)         leds[i] = CHSV(LOW_COLOR, 255, thisBright[0]);
      }
      break;
    case 4:
      switch (freq_strobe_mode) 
      {
        case 0:
          if (colorMusicFlash[2]) HIGHS();
          else if (colorMusicFlash[1]) MIDS();
          else if (colorMusicFlash[0]) LOWS();
          else SILENCE();
          break;
        case 1:
          if (colorMusicFlash[2]) HIGHS();
          else SILENCE();
          break;
        case 2:
          if (colorMusicFlash[1]) MIDS();
          else SILENCE();
          break;
        case 3:
          if (colorMusicFlash[0]) LOWS();
          else SILENCE();
          break;
      }
      break;
    case 5:
      if (strobe_bright > 0)
        for (int i = 0; i < NUM_LEDS; i++) leds[i] = CHSV(STROBE_COLOR, STROBE_SAT, strobe_bright);
      else
        for (int i = 0; i < NUM_LEDS; i++) leds[i] = CHSV(EMPTY_COLOR, 255, EMPTY_BRIGHT);
      break;
    case 6:
      switch (light_mode) {
        case 0: for (int i = 0; i < NUM_LEDS; i++) leds[i] = CHSV(LIGHT_COLOR, LIGHT_SAT, 255);
          break;
        case 1:
          if (millis() - color_timer > COLOR_SPEED) 
          {
            color_timer = millis();
            if (++this_color > 255) this_color = 0;
          }
          for (int i = 0; i < NUM_LEDS; i++) leds[i] = CHSV(this_color, LIGHT_SAT, 255);
          break;
        case 2:
          if (millis() - rainbow_timer > 30) 
          {
            rainbow_timer = millis();
            this_color += RAINBOW_PERIOD;
            if (this_color > 255) this_color = 0;
            if (this_color < 0) this_color = 255;
          }
          rainbow_steps = this_color;
          for (int i = 0; i < NUM_LEDS; i++) 
          {
            leds[i] = CHSV((int)floor(rainbow_steps), 255, 255);
            rainbow_steps += RAINBOW_STEP_2;
            if (rainbow_steps > 255) rainbow_steps = 0;
            if (rainbow_steps < 0) rainbow_steps = 255;
          }
          break;
      }
      break;
    case 7:
      switch (freq_strobe_mode) 
      {
        case 0:
          if (running_flag[2]) leds[NUM_LEDS / 2] = CHSV(HIGH_COLOR, 255, thisBright[2]);
          else if (running_flag[1]) leds[NUM_LEDS / 2] = CHSV(MID_COLOR, 255, thisBright[1]);
          else if (running_flag[0]) leds[NUM_LEDS / 2] = CHSV(LOW_COLOR, 255, thisBright[0]);
          else leds[NUM_LEDS / 2] = CHSV(EMPTY_COLOR, 255, EMPTY_BRIGHT);
          break;
        case 1:
          if (running_flag[2]) leds[NUM_LEDS / 2] = CHSV(HIGH_COLOR, 255, thisBright[2]);
          else leds[NUM_LEDS / 2] = CHSV(EMPTY_COLOR, 255, EMPTY_BRIGHT);
          break;
        case 2:
          if (running_flag[1]) leds[NUM_LEDS / 2] = CHSV(MID_COLOR, 255, thisBright[1]);
          else leds[NUM_LEDS / 2] = CHSV(EMPTY_COLOR, 255, EMPTY_BRIGHT);
          break;
        case 3:
          if (running_flag[0]) leds[NUM_LEDS / 2] = CHSV(LOW_COLOR, 255, thisBright[0]);
          else leds[NUM_LEDS / 2] = CHSV(EMPTY_COLOR, 255, EMPTY_BRIGHT);
          break;
      }
      leds[(NUM_LEDS / 2) - 1] = leds[NUM_LEDS / 2];
      if (millis() - running_timer > RUNNING_SPEED) 
      {
        running_timer = millis();
        for (byte i = 0; i < NUM_LEDS / 2 - 1; i++) 
        {
          leds[i] = leds[i + 1];
          leds[NUM_LEDS - i - 1] = leds[i];
        }
      }
      break;
    case 8:
      byte HUEindex = HUE_START;
      for (byte i = 0; i < NUM_LEDS / 2; i++) 
      {
        byte this_bright = map(freq_f[(int)floor((NUM_LEDS / 2 - i) / freq_to_stripe)], 0, freq_max_f, 0, 255);
        this_bright = constrain(this_bright, 0, 255);
        leds[i] = CHSV(HUEindex, 255, this_bright);
        leds[NUM_LEDS - i - 1] = leds[i];
        HUEindex += HUE_STEP;
        if (HUEindex > 255) HUEindex = 0;
      }
      break;
  }
}

void HIGHS() 
{
  for (int i = 0; i < NUM_LEDS; i++) leds[i] = CHSV(HIGH_COLOR, 255, thisBright[2]);
}
void MIDS() 
{
  for (int i = 0; i < NUM_LEDS; i++) leds[i] = CHSV(MID_COLOR, 255, thisBright[1]);
}
void LOWS() 
{
  for (int i = 0; i < NUM_LEDS; i++) leds[i] = CHSV(LOW_COLOR, 255, thisBright[0]);
}
void SILENCE() 
{
  for (int i = 0; i < NUM_LEDS; i++) leds[i] = CHSV(EMPTY_COLOR, 255, EMPTY_BRIGHT);
}


int smartIncr(int value, int incr_step, int mininmum, int maximum) 
{
  int val_buf = value + incr_step;
  val_buf = constrain(val_buf, mininmum, maximum);
  return val_buf;
}

float smartIncrFloat(float value, float incr_step, float mininmum, float maximum) 
{
  float val_buf = value + incr_step;
  val_buf = constrain(val_buf, mininmum, maximum);
  return val_buf;
}

void remoteTick() 
{
  if (IRLremote.available())  
  {
    auto data = IRLremote.read();
    IRdata = data.command;
    ir_flag = true;
  }
  if (ir_flag) 
  { 
    eeprom_timer = millis();
    eeprom_flag = true;
    switch (IRdata) 
    {
      case BUTT_1: this_mode = 0;
        break;
      case BUTT_2: this_mode = 1;
        break;
      case BUTT_3: this_mode = 2;
        break;
      case BUTT_4: this_mode = 3;
        break;
      case BUTT_5: this_mode = 4;
        break;
      case BUTT_6: this_mode = 5;
        break;
      case BUTT_7: this_mode = 6;
        break;
      case BUTT_8: this_mode = 7;
        break;
      case BUTT_9: this_mode = 8;
        break;
      case BUTT_0: fullLowPass();
        break;
      case BUTT_STAR: ONstate = !ONstate; FastLED.clear(); FastLED.show(); updateEEPROM();
        break;
      case BUTT_HASH:
        switch (this_mode) 
        {
          case 4:
          case 7: if (++freq_strobe_mode > 3) freq_strobe_mode = 0;
            break;
          case 6: if (++light_mode > 2) light_mode = 0;
            break;
        }
        break;
      case BUTT_OK: settings_mode = !settings_mode; digitalWrite(13, settings_mode);
        break;
      case BUTT_UP:
        if (settings_mode) 
        {
          EMPTY_BRIGHT = smartIncr(EMPTY_BRIGHT, 5, 0, 255);
        } else 
        {
          switch (this_mode) 
          {
            case 0:
              break;
            case 1: RAINBOW_STEP = smartIncrFloat(RAINBOW_STEP, 0.5, 0.5, 20);
              break;
            case 2:
            case 3:
            case 4: MAX_COEF_FREQ = smartIncrFloat(MAX_COEF_FREQ, 0.1, 0, 5);
              break;
            case 5: STROBE_PERIOD = smartIncr(STROBE_PERIOD, 20, 1, 1000);
              break;
            case 6:
              switch (light_mode) 
              {
                case 0: LIGHT_SAT = smartIncr(LIGHT_SAT, 20, 0, 255);
                  break;
                case 1: LIGHT_SAT = smartIncr(LIGHT_SAT, 20, 0, 255);
                  break;
                case 2: RAINBOW_STEP_2 = smartIncrFloat(RAINBOW_STEP_2, 0.5, 0.5, 10);
                  break;
              }
              break;
            case 7: MAX_COEF_FREQ = smartIncrFloat(MAX_COEF_FREQ, 0.1, 0.0, 10);
              break;
            case 8: HUE_START = smartIncr(HUE_START, 10, 0, 255);
              break;
          }
        }
        break;
      case BUTT_DOWN:
        if (settings_mode) 
        {
          EMPTY_BRIGHT = smartIncr(EMPTY_BRIGHT, -5, 0, 255);
        } else 
        {
          switch (this_mode) 
          {
            case 0:
              break;
            case 1: RAINBOW_STEP = smartIncrFloat(RAINBOW_STEP, -0.5, 0.5, 20);
              break;
            case 2:
            case 3:
            case 4: MAX_COEF_FREQ = smartIncrFloat(MAX_COEF_FREQ, -0.1, 0, 5);
              break;
            case 5: STROBE_PERIOD = smartIncr(STROBE_PERIOD, -20, 1, 1000);
              break;
            case 6:
              switch (light_mode) 
              {
                case 0: LIGHT_SAT = smartIncr(LIGHT_SAT, -20, 0, 255);
                  break;
                case 1: LIGHT_SAT = smartIncr(LIGHT_SAT, -20, 0, 255);
                  break;
                case 2: RAINBOW_STEP_2 = smartIncrFloat(RAINBOW_STEP_2, -0.5, 0.5, 10);
                  break;
              }
              break;
            case 7: MAX_COEF_FREQ = smartIncrFloat(MAX_COEF_FREQ, -0.1, 0.0, 10);
              break;
            case 8: HUE_START = smartIncr(HUE_START, -10, 0, 255);
              break;
          }
        }
        break;
      case BUTT_LEFT:
        if (settings_mode) 
        {
          BRIGHTNESS = smartIncr(BRIGHTNESS, -20, 0, 255);
          FastLED.setBrightness(BRIGHTNESS);
        } else 
        {
          switch (this_mode) 
          {
            case 0:
            case 1: SMOOTH = smartIncrFloat(SMOOTH, -0.05, 0.05, 1);
              break;
            case 2:
            case 3:
            case 4: SMOOTH_FREQ = smartIncrFloat(SMOOTH_FREQ, -0.05, 0.05, 1);
              break;
            case 5: STROBE_SMOOTH = smartIncr(STROBE_SMOOTH, -20, 0, 255);
              break;
            case 6:
              switch (light_mode) 
              {
                case 0: LIGHT_COLOR = smartIncr(LIGHT_COLOR, -10, 0, 255);
                  break;
                case 1: COLOR_SPEED = smartIncr(COLOR_SPEED, -10, 0, 255);
                  break;
                case 2: RAINBOW_PERIOD = smartIncr(RAINBOW_PERIOD, -1, -20, 20);
                  break;
              }
              break;
            case 7: RUNNING_SPEED = smartIncr(RUNNING_SPEED, -10, 1, 255);
              break;
            case 8: HUE_STEP = smartIncr(HUE_STEP, -1, 1, 255);
              break;
          }
        }
        break;
      case BUTT_RIGHT:
        if (settings_mode) 
        {
          BRIGHTNESS = smartIncr(BRIGHTNESS, 20, 0, 255);
          FastLED.setBrightness(BRIGHTNESS);
        } else 
        {
          switch (this_mode) 
          {
            case 0:
            case 1: SMOOTH = smartIncrFloat(SMOOTH, 0.05, 0.05, 1);
              break;
            case 2:
            case 3:
            case 4: SMOOTH_FREQ = smartIncrFloat(SMOOTH_FREQ, 0.05, 0.05, 1);
              break;
            case 5: STROBE_SMOOTH = smartIncr(STROBE_SMOOTH, 20, 0, 255);
              break;
            case 6:
              switch (light_mode) 
              {
                case 0: LIGHT_COLOR = smartIncr(LIGHT_COLOR, 10, 0, 255);
                  break;
                case 1: COLOR_SPEED = smartIncr(COLOR_SPEED, 10, 0, 255);
                  break;
                case 2: RAINBOW_PERIOD = smartIncr(RAINBOW_PERIOD, 1, -20, 20);
                  break;
              }
              break;
            case 7: RUNNING_SPEED = smartIncr(RUNNING_SPEED, 10, 1, 255);
              break;
            case 8: HUE_STEP = smartIncr(HUE_STEP, 1, 1, 255);
              break;
          }
        }
        break;
      default: eeprom_flag = false;  
        break;
    }
    ir_flag = false;
  }
}

void autoLowPass() 
{
  delay(10);                                
  int thisMax = 0;                          
  int thisLevel;
  for (byte i = 0; i < 200; i++) 
  {
    thisLevel = analogRead(SOUND_R);       
    if (thisLevel > thisMax)                
      thisMax = thisLevel;                  
    delay(4);                               
  }
  LOW_PASS = thisMax + LOW_PASS_ADD;       


  thisMax = 0;
  for (byte i = 0; i < 100; i++) 
  {          
    analyzeAudio();                         
    for (byte j = 2; j < 32; j++) 
    {      
      thisLevel = fht_log_out[j];
      if (thisLevel > thisMax)             
        thisMax = thisLevel;                
    }
    delay(4);                               
  }
  SPEKTR_LOW_PASS = thisMax + LOW_PASS_FREQ_ADD;  
  if (EEPROM_LOW_PASS && !AUTO_LOW_PASS) 
  {
    EEPROM.updateInt(70, LOW_PASS);
    EEPROM.updateInt(72, SPEKTR_LOW_PASS);
  }
}

void analyzeAudio() 
{
  for (int i = 0 ; i < FHT_N ; i++) 
  {
    int sample = analogRead(SOUND_R_FREQ);
    fht_input[i] = sample; // put real data into bins
  }
  fht_window();  // window the data for better frequency response
  fht_reorder(); // reorder the data before doing the fht
  fht_run();     // process the data in the fht
  fht_mag_log(); // take the output of the fht
}

void buttonTick() 
{
  butt1.tick();  
  if (butt1.isSingle())                             
    if (++this_mode >= MODE_AMOUNT) this_mode = 0;   

  if (butt1.isHolded()) 
  {    
    fullLowPass();
  }
}
void fullLowPass() 
{
  digitalWrite(13, HIGH);   
  FastLED.setBrightness(0); 
  FastLED.clear();          
  FastLED.show();           
  delay(500);               
  autoLowPass();           
  delay(500);              
  FastLED.setBrightness(BRIGHTNESS);  
  digitalWrite(13, LOW);    
}
void updateEEPROM() 
{
  EEPROM.updateByte(1, this_mode);
  EEPROM.updateByte(2, freq_strobe_mode);
  EEPROM.updateByte(3, light_mode);
  EEPROM.updateInt(4, RAINBOW_STEP);
  EEPROM.updateFloat(8, MAX_COEF_FREQ);
  EEPROM.updateInt(12, STROBE_PERIOD);
  EEPROM.updateInt(16, LIGHT_SAT);
  EEPROM.updateFloat(20, RAINBOW_STEP_2);
  EEPROM.updateInt(24, HUE_START);
  EEPROM.updateFloat(28, SMOOTH);
  EEPROM.updateFloat(32, SMOOTH_FREQ);
  EEPROM.updateInt(36, STROBE_SMOOTH);
  EEPROM.updateInt(40, LIGHT_COLOR);
  EEPROM.updateInt(44, COLOR_SPEED);
  EEPROM.updateInt(48, RAINBOW_PERIOD);
  EEPROM.updateInt(52, RUNNING_SPEED);
  EEPROM.updateInt(56, HUE_STEP);
  EEPROM.updateInt(60, EMPTY_BRIGHT);
  if (KEEP_STATE) EEPROM.updateByte(64, ONstate);
}
void readEEPROM() 
{
  this_mode = EEPROM.readByte(1);
  freq_strobe_mode = EEPROM.readByte(2);
  light_mode = EEPROM.readByte(3);
  RAINBOW_STEP = EEPROM.readInt(4);
  MAX_COEF_FREQ = EEPROM.readFloat(8);
  STROBE_PERIOD = EEPROM.readInt(12);
  LIGHT_SAT = EEPROM.readInt(16);
  RAINBOW_STEP_2 = EEPROM.readFloat(20);
  HUE_START = EEPROM.readInt(24);
  SMOOTH = EEPROM.readFloat(28);
  SMOOTH_FREQ = EEPROM.readFloat(32);
  STROBE_SMOOTH = EEPROM.readInt(36);
  LIGHT_COLOR = EEPROM.readInt(40);
  COLOR_SPEED = EEPROM.readInt(44);
  RAINBOW_PERIOD = EEPROM.readInt(48);
  RUNNING_SPEED = EEPROM.readInt(52);
  HUE_STEP = EEPROM.readInt(56);
  EMPTY_BRIGHT = EEPROM.readInt(60);
  if (KEEP_STATE) ONstate = EEPROM.readByte(64);
}
void eepromTick() 
{
  if (eeprom_flag)
    if (millis() - eeprom_timer > 30000) 
    { 
      eeprom_flag = false;
      eeprom_timer = millis();
      updateEEPROM();
    }
}
