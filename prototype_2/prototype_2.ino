/*The pin numbers are according to the ESP32 board and can be changed depending on the chip.
SCL/clock = pin 18
SDA/data = pin 51
CS = pin 17
DC = pin 16
RESET = pin 23*/

#include <Arduino.h>
#include <U8g2lib.h>
#include <MUIU8g2.h>
#include <SimpleRotary.h>
#include <Adafruit_NeoPixel.h>
#include "DHT.h"
#ifdef __AVR__
#include <avr/power.h>
#endif

#define NEOPIXEL_PIN  19
#define CLOCK         14
#define DATA          13
#define CS            16
#define DC            17
#define RESET         4

#define DHT_PIN       2

#define EN_A          27
#define EN_B          26
#define EN_BTN        5

#define BUZZER        18

#define RELAY_1       21
#define RELAY_2       22

#define DT_A          23
#define SCK_A         32
#define DT_B          25
#define SCK_B         33


Adafruit_NeoPixel strip(3, NEOPIXEL_PIN, NEO_GRB);

SimpleRotary rotary(EN_A,EN_B,EN_BTN);

DHT dht (DHT_PIN, DHT22);

/*This is used to select the LCD display and the pins related to it.*/
U8G2_ST7567_JLX12864_1_4W_SW_SPI u8g2(U8G2_R2, /* clock=*/ CLOCK, /* data=*/ DATA, /* cs=*/ CS, /* dc=*/ DC, /* reset=*/ RESET);

MUIU8G2 mui;
/*
 * All the global variables will come here
 */

  uint8_t gauge_radius = 16; /*Size of the gauge*/
  uint8_t actual_temp = 0;  /*Temperature reading from DHT11 sensor*/
  uint8_t humidity = 0;    /*Humidity reading from DHT11 sensor*/
  uint8_t check = 0;         /*Checks the status of the De-humidifier*/
  uint8_t start_check = 0;   /*for switching start or stop*/
  uint8_t light = 0;
  
  uint8_t minimum_temp_gauge_value = 0;
  uint8_t maximum_temp_gauge_value = 100;
  uint8_t minimum_hum_gauge_value = 0;
  uint8_t maximum_hum_gauge_value = 100;
  uint8_t minimum_temp_list_value = 0;
  uint8_t maximum_temp_list_value = 250;
  uint8_t minimum_minute_value = 0;
  uint8_t maximum_minute_value = 59;
  uint8_t minimum_hour_value = 0;
  uint8_t maximum_hour_value = 48;

  uint8_t custom_temp = minimum_temp_gauge_value;
  uint8_t custom_hour = 0;
  uint8_t custom_minute = 0;
  
  uint8_t actual_minute = 0;
  uint8_t actual_hour = 0;

  uint8_t encoder_cw_delay = 0;
  uint8_t encoder_ccw_delay = 0;

  const char *filaments[] = { "NONE","PLA", "ABS", "NYLON", "PETG", "TPU"};           /*List of all the Filaments*/
  const char *mode[] = {"PREHEAT", "CONTINUOUS"};                                     /*Drying Mode*/
  const char *stat[] = {"Start", "Abort"};                                             /*Status*/
  const char *message[] = {"IDLE", "DRYING", "ABORTED", "DONE"};                      /*Message to show*/
  const char *mode_info[] = {"Dries for a specified amount of time.", "Continuously dries throughout the",  "printing process."};
  
  uint8_t temp[] = {20, 80, 120, 130, 140, 160};
  uint8_t hour[] = {0, 0, 0, 0, 0, 1};
  uint8_t minute[] = {0, 30, 20, 35, 40, 20};
  
  uint16_t mode_idx = 0;
  uint16_t filament_idx = 0;
  
  unsigned long prev_time = 0;
  unsigned long current_time = 0;
  unsigned long new_hour = 0;
  unsigned long new_minute = 0;
  unsigned long time_interval = 0;

 /*
  * All the custom functions would come here.
  */

 /*Basic geometry of the main frame without the values.
  *It corresponds to MUIF_RO("MM", main_menu) in muif_list.*/
  
uint8_t main_menu(mui_t *ui, uint8_t msg){
  
    if ( msg == MUIF_MSG_DRAW ) {
      
        u8g2_uint_t x = mui_get_x(ui);
        u8g2_uint_t y = mui_get_y(ui);
        
        /*Outer Frame*/
        
        u8g2.drawFrame(0, 0, 128, 64);
        u8g2.drawHLine(0, 10, 128);
        u8g2.drawHLine(0, 43, 128);
        u8g2.drawHLine(0, 53, 128);
        u8g2.drawVLine(63, 0, 43);
    
        /*Temperature Gauge*/
        
        u8g2.drawCircle(x, y, gauge_radius, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_UPPER_LEFT);
        u8g2.drawCircle(x, y, gauge_radius+1, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_UPPER_LEFT);
        u8g2.drawDisc(x, y, 1, U8G2_DRAW_ALL);
    
        /*Humidity Gauge*/
        
        u8g2.drawCircle(x+63, y, gauge_radius, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_UPPER_LEFT);
        u8g2.drawCircle(x+63, y, gauge_radius+1, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_UPPER_LEFT);
        u8g2.drawDisc(x+63, y, 1, U8G2_DRAW_ALL); 
    }
   
    return 0;
}

/*Shows the changing values in the main screen.
 *It corresponds to MUIF_RO("SV", stat_val) in muif_list.*/
 uint8_t stat_val(mui_t *ui, uint8_t msg){
  
     if ( msg == MUIF_MSG_DRAW ) {
      
         u8g2.setCursor(13, 8);
         u8g2.print(actual_temp);
      
         u8g2.setCursor(96, 8);
         u8g2.print(humidity);
         
         u8g2.setCursor(38, 8);
         if (filament_idx == 0)
            u8g2.print("--");
         else
            u8g2.print(temp[filament_idx]+5);
  
         u8g2.setCursor(43, 61);
         u8g2.print(filaments[filament_idx]);
  
         u8g2.setCursor(28, 51);
         u8g2.print(mode[mode_idx]);

         u8g2.setCursor(115, 51);
         if (start_check == 1){
            u8g2.print(actual_minute);
         }
         else {
            u8g2.print("mm");
         }
         u8g2.setCursor(105, 51);
         if (start_check == 1){
            u8g2.print(actual_hour);
         }
         else {
            u8g2.print("hh");
         }
        
     }
    
     return 0;
 }

/*Shows the gauge values.
 *It corresponds to MUIF_RO("GV", gauge_value) in muif_list.*/
 uint8_t gauge_value(mui_t *ui, uint8_t msg){
  
     if ( msg == MUIF_MSG_DRAW ) {
         if (filament_idx == 0){
            minimum_temp_gauge_value = 0;
            maximum_temp_gauge_value = 100;
         }
         else{
            minimum_temp_gauge_value = 20;
            maximum_temp_gauge_value = temp[filament_idx] + 5;
         }
         int temp_inc = (maximum_temp_gauge_value - minimum_temp_gauge_value)/5;
         int hum_inc =  (maximum_hum_gauge_value - minimum_hum_gauge_value)/5;
         int x_new = 12*cos(radians(((float(actual_temp) - float(maximum_temp_gauge_value))/(float(minimum_temp_gauge_value) - float(maximum_temp_gauge_value)))*180));
         int y_new = 12*sin(radians(((float(actual_temp) - float(maximum_temp_gauge_value))/(float(minimum_temp_gauge_value) - float(maximum_temp_gauge_value)))*180));
         int x_rh_new = 12*cos(radians(((float(humidity) - float(100))/- float(100))*180));
         int y_rh_new = 12*sin(radians(((float(humidity) - float(100))/- float(100))*180));
         
         u8g2_uint_t x = mui_get_x(ui);
         u8g2_uint_t y = mui_get_y(ui);
         u8g2.setFont(u8g2_font_tiny5_tf);

         u8g2.drawLine(x-2,y, x - 2 + x_new ,y - y_new);
         u8g2.setCursor(x+17,y+2);
         u8g2.print(maximum_temp_gauge_value);
         u8g2.setCursor(x+14,y-9);
         u8g2.print(minimum_temp_gauge_value+(4*temp_inc));
         u8g2.setCursor(x+3,y-17);
         u8g2.print(minimum_temp_gauge_value+(3*temp_inc));
         u8g2.setCursor(x-13,y-17);
         u8g2.print(minimum_temp_gauge_value+(2*temp_inc));
         u8g2.setCursor(x-24,y-9);
         u8g2.print(minimum_temp_gauge_value+(1*temp_inc));
         u8g2.setCursor(x-28,y+2);
         u8g2.print(minimum_temp_gauge_value+(0*temp_inc));

         u8g2.drawLine(x+64-3,y, x + 64-3 + x_rh_new,y - y_rh_new);
         u8g2.setCursor(x+80,y+2);
         u8g2.print(minimum_hum_gauge_value+(5*hum_inc));
         u8g2.setCursor(x+77,y-9);
         u8g2.print(minimum_hum_gauge_value+(4*hum_inc));
         u8g2.setCursor(x+66,y-17);
         u8g2.print(minimum_hum_gauge_value+(3*hum_inc));
         u8g2.setCursor(x+50,y-17);
         u8g2.print(minimum_hum_gauge_value+(2*hum_inc));
         u8g2.setCursor(x+39,y-9);
         u8g2.print(minimum_hum_gauge_value+(1*hum_inc));
         u8g2.setCursor(x+38,y+2);
         u8g2.print(minimum_hum_gauge_value+(0*hum_inc));
        }
        return 0;
     }
 
/*Changes the background colour.
 *It corresponds to MUIF_RO("BG", colour) in muif_list.*/
 uint8_t colour(mui_t *ui, uint8_t msg){
  
     if ( msg == MUIF_MSG_DRAW ) {
         switch (check){
            case 1:
              strip.setPixelColor(0, strip.Color(255, 0, 0));
              //strip.setPixelColor(1, strip.Color(5, 5, 5));
              //strip.setPixelColor(2, strip.Color(0, 0, 0));
              strip.show();
              u8g2.setCursor(86,61);
              u8g2.print(message[check]);
              break;
           case 2:
              strip.setPixelColor(0, strip.Color(0, 255, 0));
              //strip.setPixelColor(1, strip.Color(5, 5, 5));
              //strip.setPixelColor(2, strip.Color(0, 0, 0));
              strip.show();
              u8g2.setCursor(86,61);
              u8g2.print(message[check]);
              break;
           case 3:
              strip.setPixelColor(0, strip.Color(0, 0, 255));
              //strip.setPixelColor(1, strip.Color(5, 5, 5));
              //strip.setPixelColor(2, strip.Color(0, 0, 0));
              strip.show();
              u8g2.setCursor(86,61);
              u8g2.print(message[check]);
              break;
           default:
              strip.setPixelColor(0, strip.Color(255, 255, 255));
              strip.setPixelColor(1, strip.Color(5, 5, 5));
             strip.setPixelColor(2, strip.Color(0, 0, 0));
             strip.show();
             u8g2.setCursor(86,61);
             u8g2.print(message[check]);
        }
  }
  else if ( msg == MUIF_MSG_CURSOR_SELECT ) {
    mui.gotoForm(2,0);
     // return mui_GotoFormAutoCursorPosition(ui, 2);
  }
  else{ 
      strip.setPixelColor(0, strip.Color(255, 255, 255));
      strip.setPixelColor(1, strip.Color(5, 5, 5));
      strip.setPixelColor(2, strip.Color(0, 0, 0));
      strip.show();
  }
  return 0;
}

/*Basic geometry of the general frame without the values.
  *It corresponds to MUIF_RO("GM", general_menu) in muif_list.*/
  
 uint8_t general_menu(mui_t *ui, uint8_t msg){
    if ( msg == MUIF_MSG_DRAW ) {
      u8g2.drawFrame(0, 0, 128, 64);
      u8g2.drawHLine(0, 13, 128);
    }
    return 0;
 }

 /*Start/Abort Button
  *It corresponds to MUIF_GOTO("PR", process) in muif_list.*/
 u8g2_uint_t fg(mui_t *ui){
    u8g2_uint_t flags = 0;
    if ( mui_IsCursorFocus(ui) ){
      flags |= U8G2_BTN_INV;
      if ( ui->is_mud )
      {
        flags = 0;        // undo INV
      }      
    }
    return flags;
 }
  
 uint8_t process(mui_t *ui, uint8_t msg){
    int index = start_check ;
    if ( msg == MUIF_MSG_DRAW ) {
    mui_u8g2_draw_button_utf(ui, fg(ui), 0, u8g2.getDisplayWidth(), 1, stat[index]);
    }
    else if (msg == MUIF_MSG_CURSOR_SELECT){
      if (start_check == 0) {
        check = 1;
        start_check = 1;
        prev_time = millis();
      }
      else if (start_check == 1){
        check = 2;
        start_check = 0;
        actual_minute = 0;
        actual_hour = 0;
      }
      
      return mui_GotoFormAutoCursorPosition(ui, ui->arg);
    }
    return 0;
 }

 /*To show information about the mode.
  *It corresponds to MUIF_RO("MI", mode_inf) in muif_list.*/
uint8_t mode_inf(mui_t *ui, uint8_t msg) {
  if ( msg == MUIF_MSG_DRAW ) {
    u8g2_uint_t x = mui_get_x(ui);
    u8g2_uint_t y = mui_get_y(ui);
    if (mode_idx == 0){
      u8g2.setCursor(x, y);
      u8g2.print(mode_info[mode_idx]);
    }
    else{
      int index = mode_idx + 1;
      u8g2.setCursor(x, y-2);
      u8g2.print(mode_info[mode_idx]);
      u8g2.setCursor(x, y+5);
      u8g2.print(mode_info[index]);
    }
    
  }
  return 0;
}
 
 /* Number of modes. 
  *It corresponds to MUIF_U8G2_U16_LIST("MD", ....) in muif_list.*/
  uint16_t mode_get_cnt(void *data) {
    return sizeof(mode)/sizeof(*mode);
  }

 /* Mode corresponding to the index 
  *It corresponds to MUIF_U8G2_U16_LIST("MD", ....) in muif_list.*/
  const char *mode_get_str(void *data, uint16_t index) {
    return mode[index];
  }


 /* Total number of filaments. 
  *It corresponds to MUIF_RO("BG", main_menu) in muif_list.*/
  uint16_t filament_name_list_get_cnt(void *data) {
    return sizeof(filaments)/sizeof(*filaments);   
  }

  /* Filament corresponding to particular id 
   *It corresponds to MUIF_RO("BG", main_menu) in muif_list.*/
  const char *filament_name_list_get_str(void *data, uint16_t index) {
    return filaments[index];
  }

/*Can set the temperature of the filament.
 *It corresponds to MUIF_BUTTON("TL", temp_list) in muif_list.*/
uint8_t temp_list(mui_t *ui, uint8_t msg){
  
  custom_temp = temp[filament_idx];
  
  if (msg == MUIF_MSG_DRAW){
     // mui_u8g2_draw_button_utf(ui, mui_u8g2_get_fi_flags(ui), 0, 2, 0, u8x8_u8toa(custom_temp, 3));
     mui_u8g2_draw_button_utf(ui, fg(ui), 0, 2, 0, u8x8_u8toa(custom_temp, 3));
    }
    else if ((msg == MUIF_MSG_CURSOR_SELECT)){
      ui->is_mud = !ui->is_mud;
    }
    else if (msg == MUIF_MSG_EVENT_NEXT){
      if ( ui->is_mud )
        {
          if ( custom_temp > maximum_temp_list_value )
            custom_temp = minimum_temp_list_value;
          else
            (custom_temp)++;
            
          temp[filament_idx] = custom_temp;
          return 1; 
        }
    }
    else if (msg == MUIF_MSG_EVENT_PREV){
      if ( ui->is_mud )
        {
          if ( custom_temp <= minimum_temp_list_value )
            custom_temp = maximum_temp_list_value;
          else
            (custom_temp)--;
            
          temp[filament_idx] = custom_temp;
          return 1;
        }
    }
   return 0;
  }

/*Can set the hour for continuous drying.
 *It corresponds to MUIF_BUTTON("HL", hour_list) in muif_list.*/
uint8_t hour_list(mui_t *ui, uint8_t msg){
  
  custom_minute = hour[filament_idx];
  
  if (msg == MUIF_MSG_DRAW){
      //mui_u8g2_draw_button_utf(ui, mui_u8g2_get_fi_flags(ui), 0, 2, 0, u8x8_u8toa(custom_hour, 2));
      mui_u8g2_draw_button_utf(ui, fg(ui), 0, 2, 0, u8x8_u8toa(custom_hour, 2));
    }
    else if ((msg == MUIF_MSG_CURSOR_SELECT)){
      ui->is_mud = !ui->is_mud;
    }
    else if (msg == MUIF_MSG_EVENT_NEXT){
      if ( ui->is_mud )
        {
          if ( custom_hour > maximum_hour_value )
            custom_hour = minimum_hour_value;
          else
            (custom_hour)++;
          hour[filament_idx] = custom_hour;
          return 1; 
        }
    }
    else if (msg == MUIF_MSG_EVENT_PREV){
      if ( ui->is_mud )
        {
          if ( custom_hour <= minimum_hour_value )
            custom_hour = maximum_hour_value;
          else
            (custom_hour)--;
          hour[filament_idx] = custom_hour;
          return 1;
        }
    }
   return 0;
  }
  
/*Can set the hour for continuous drying.
 *It corresponds to MUIF_BUTTON("HL", temp_list) in muif_list.*/
uint8_t minute_list(mui_t *ui, uint8_t msg){
  
  custom_minute = minute[filament_idx];
  
  if (msg == MUIF_MSG_DRAW){
      //mui_u8g2_draw_button_utf(ui, mui_u8g2_get_fi_flags(ui), 0, 2, 0, u8x8_u8toa(custom_minute, 2));
      mui_u8g2_draw_button_utf(ui, fg(ui), 0, 2, 0, u8x8_u8toa(custom_minute, 2));
    }
    else if ((msg == MUIF_MSG_CURSOR_SELECT)){
      ui->is_mud = !ui->is_mud;
    }
    else if (msg == MUIF_MSG_EVENT_NEXT){
      if ( ui->is_mud )
        {
          if ( custom_minute > maximum_minute_value )
            custom_minute = minimum_minute_value;
          else
            (custom_minute)++;
          minute[filament_idx] = custom_minute;
          return 1; 
        }
    }
    else if (msg == MUIF_MSG_EVENT_PREV){
      if ( ui->is_mud )
        {
          if ( custom_minute <= minimum_minute_value )
            custom_minute = maximum_minute_value;
          else
            (custom_minute)--;
          minute[filament_idx] = custom_minute;
          return 1;
        }
    }
   return 0;
  }



muif_t muif_list[] = {
    MUIF_U8G2_FONT_STYLE(0, u8g2_font_helvR08_tr),        /* regular font */
    MUIF_U8G2_FONT_STYLE(1, u8g2_font_helvB08_tr),        /* bold font */
    MUIF_U8G2_FONT_STYLE(2, u8g2_font_tiny5_tf),          /* tiny font */
    MUIF_U8G2_LABEL(),
  
    /*Mostly Form 1 related*/
    MUIF_RO("MM", main_menu),
    MUIF_RO("SV", stat_val),
    MUIF_RO("GV", gauge_value),
    MUIF_RO("BG", colour),
    MUIF_BUTTON("BT", mui_u8g2_btn_goto_wm_fi),
  
    /*Mostly Form 2 related*/
    MUIF_RO("GM", general_menu),
    MUIF_GOTO(process),
    MUIF_BUTTON("GF",mui_u8g2_goto_form_w1_pi),
    MUIF_RO("GD", mui_u8g2_goto_data),
  
    /*Mostly Form 4 related*/
    MUIF_U8G2_U16_LIST("MD", &mode_idx, NULL, mode_get_str, mode_get_cnt, mui_u8g2_u16_list_line_wa_mud_pi),
    MUIF_RO("MI", mode_inf),
    
    /*Mostly Form 5 related*/
    MUIF_U8G2_U16_LIST("FN", &filament_idx, NULL, filament_name_list_get_str, filament_name_list_get_cnt, mui_u8g2_u16_list_line_wa_mud_pi),
    MUIF_BUTTON("TL", temp_list),
    MUIF_BUTTON("HL", hour_list),
    MUIF_BUTTON("ML", minute_list),
};

fds_t fds_data[] =

  /*
   * Form 1
   */
    MUI_FORM(1)
  
    MUI_STYLE(2)
    MUI_XY("MM", 30, 38)
    MUI_AUX("SV")
    MUI_AUX("BG")
    MUI_XY("GV", 32, 38)
    MUI_LABEL(5,8, "T:")
    MUI_LABEL(25,8, "°C/")
    MUI_LABEL(50,8, "°C")
    MUI_LABEL(82,8, "RH:")
    MUI_LABEL(109,8,"%")
    MUI_LABEL(86,51, "TIME:")
    MUI_LABEL(113,51, ":")
    MUI_LABEL(3,51, "MODE:")
    MUI_LABEL(3,61,"MATERIAL:")
  
    MUI_XYAT("BT", 130, 80, 2, "Next")

  /*
   * Form 2
   */
    MUI_FORM(2)
    
    MUI_AUX("GM")
  
    MUI_STYLE(1)
    MUI_LABEL(48, 11, "MENU")
    
    MUI_STYLE(0)
    MUI_GOTO(5, 24, 1,"")
    MUI_DATA("GD",
        MUI_3 "Settings")
        
    MUI_XYA("GF", 5, 36, 0)
    MUI_XYAT("BT", 64, 62, 1, "BACK")

  /*
   * Form 3
   */
    MUI_FORM(3)

    MUI_AUX("GM")

    MUI_STYLE(1)
    MUI_LABEL(37, 11, "SETTINGS")

    MUI_STYLE(0)
    MUI_DATA("GD",
        MUI_4 "Mode|"
        MUI_5 "Material|")

    MUI_XYA("GF", 5, 24, 0)
    MUI_XYA("GF", 5, 36, 1)

    MUI_XYAT("BT", 64, 62, 2, "BACK")

  /*
   * Form 4
   */
    MUI_FORM(4)

    MUI_AUX("GM")
  
    MUI_STYLE(1)
    MUI_LABEL(48, 11, "MODE")
  
    MUI_STYLE(0)
    MUI_LABEL(5, 24, "SET:")
     
    MUI_XYA("MD", 30, 24, 44)
    MUI_XYAT("BT", 64, 62, 3, "BACK")

    MUI_STYLE(2)
    MUI_XYA("MI", 2, 40, 34)  
  
    
   
  /*
   * Form 5
   */
    MUI_FORM(5)

    MUI_AUX("GM")

    MUI_STYLE(1)
    MUI_LABEL(36, 11, "MATERIAL")

    MUI_STYLE(0)
    MUI_LABEL(5, 24, "NAME :")
    MUI_LABEL(5, 37, "TEMP  :")
    MUI_LABEL(5, 49, "TIME   :")

    MUI_XYA("FN", 45, 24, 79)
    MUI_XYA("TL", 45, 37, 88)
    MUI_XYA("HL", 45, 49, 87)
    MUI_XYA("ML", 65, 49, 86)

    MUI_XYAT("BT", 64, 62, 3, "BACK")

  
;


void setup(void) {
  
    dht.begin ();
    
    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, HIGH);
    pinMode(RELAY_1, OUTPUT);
    pinMode(RELAY_2, OUTPUT);
    strip.begin();
    strip.setBrightness(255);
    strip.setPixelColor(0, strip.Color(255, 255, 255));
    strip.setPixelColor(1, strip.Color(5, 5, 5));
    strip.setPixelColor(2, strip.Color(0, 0, 0));
    strip.show();
    
    u8g2.begin();
    u8g2.enableUTF8Print();
    u8g2.setContrast(150);
    u8g2.clearBuffer(); 
    
    mui.begin(u8g2, fds_data, muif_list, sizeof(muif_list)/sizeof(muif_t));
    mui.gotoForm(/* form_id= */ 1, /* initial_cursor_position= */ 0);

}

uint8_t is_redraw = 1;
uint8_t rotary_event = 0; // 0 = not turning, 1 = CW, 2 = CCW
uint8_t push_event = 0; // 0 = not pushed, 1 = pushed

void detect_events(void) {
  uint8_t tmp;
  
  // 0 = not pushed, 1 = pushed  
  tmp = rotary.push();
  if ( tmp != 0 )         // only assign the push event, never clear the event here
    push_event = tmp;
    
  // 0 = not turning, 1 = CW, 2 = CCW
  tmp = rotary.rotate();
  if ( tmp != 0 )       // only assign the rotation event, never clear the event here
    rotary_event = tmp;    
}

void handle_events(void) {
  // 0 = not pushed, 1 = pushed  
  if ( push_event == 1 ) {
      mui.sendSelect();
      is_redraw = 1;
      push_event = 0;
      for(long int i = 0; i<150000; i++){
        digitalWrite(BUZZER, LOW); 
      }
      digitalWrite(BUZZER, HIGH);
      
  }
  
  // 0 = not turning, 1 = CW, 2 = CCW
  
   if ( rotary_event == 1 ) {
    if (encoder_cw_delay == 4){
      mui.nextField();
      is_redraw = 1;
      rotary_event = 0;
      encoder_cw_delay = 0;
      }
    else {
      encoder_cw_delay ++;      
      }
   }
  
  if ( rotary_event == 2 ) {
    if (encoder_ccw_delay == 4){
      mui.prevField();
      is_redraw = 1;
      rotary_event = 0;
      encoder_ccw_delay = 0;
    } 
    else {
      encoder_ccw_delay ++;      
      }
    }
}
void read_from_dht(void){
  if (!(isnan (actual_temp) || isnan (humidity))) {
    actual_temp = dht.readTemperature();
    humidity = dht.readHumidity();
    is_redraw = 1;
  }
}
void humidity_controller(void){
  if (start_check == 1){
    if (humidity < 8){
      digitalWrite(RELAY_1, LOW);
    }
    else if (humidity > 12){
      digitalWrite(RELAY_1, HIGH);
    }
  }
  else if (start_check == 0){
    digitalWrite(RELAY_1, LOW);
  }
}
void temperature_controller(void){
  if (start_check == 1){
    if (actual_temp < (temp[filament_idx]-2)){
      digitalWrite(RELAY_2, HIGH);
    }
    else if (actual_temp > (temp[filament_idx]+2)){
      digitalWrite(RELAY_2, LOW);
    }
  }
  else if (start_check == 0){
    digitalWrite(RELAY_2, LOW);
  }
}

void check_continuous_clock(void){
  current_time = millis();
  if (current_time - prev_time > 60000){
    prev_time = millis();
    if (actual_minute == 59){
      actual_minute = 0;
      actual_hour ++;
    }
    else {
      actual_minute++;
    }
    is_redraw = 1;
  }
}
/*void check_preset_clock(void){
  current_time = millis();
  if (current_time - prev_time > time_interval){
    
  }
  
  
}*/

void loop() {
  
  /* check whether the menu is active */
  if ( mui.isFormActive() ) {
    read_from_dht();
    humidity_controller();
    temperature_controller();
    if ((start_check == 1) && (mode_idx == 1)){
      check_continuous_clock();
    }
    /*else if ((start_check == 1) && (mode_idx == 0)){
      new_minute = minute[filament_idx] * 60000UL;
      new_hour = hour[filament_idx] * 3600000UL;
      time_interval = new_minute + new_hour;
      check_preset_clock();
    }*/
    /* update the display content, if the redraw flag is set */
    if ( is_redraw ) {
      u8g2.firstPage();
      do {
          detect_events();
          mui.draw();
          detect_events();
      } while( u8g2.nextPage() );
      is_redraw = 0;                    /* clear the redraw flag */
    }

    detect_events();
    handle_events();
      
  } else {
      /* the menu should never become inactive, but if so, then restart the menu system */
      mui.gotoForm(/* form_id= */ 1, /* initial_cursor_position= */ 0);
  }
}
