//....................HEADER FILES....................//

#include <Arduino.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <MUIU8g2.h>
#include <SimpleRotary.h>
#include <Adafruit_NeoPixel.h>
#include "DHT.h"
#ifdef __AVR__
#include <avr/power.h>
#endif
#include "pins.h"
//....................HEADER FILES....................//


//....................CONSTRUCTORS....................//  
Adafruit_NeoPixel strip(3, NEOPIXEL_PIN, NEO_GRB);
SimpleRotary rotary(EN_A,EN_B,EN_BTN);
DHT dht (DHT_PIN, DHT22);
U8G2_ST7567_JLX12864_1_4W_SW_SPI u8g2(U8G2_R2, /* clock=*/ CLOCK, /* data=*/ DATA, /* cs=*/ CS, /* dc=*/ DC, /* reset=*/ RESET);
MUIU8G2 mui;
//....................CONSTRUCTORS....................//


//....................GLOBAL VARIABLES....................//
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

  const char *filaments[] = { "CUSTOM","PLA", "ABS/ASA", "NYLON/PC", "PETG", "TPU", "PEEK", "ULTEM"};  /*List of all the Filaments*/
  const char *mode[] = {"PREHEAT", "CONTINUOUS"};                                     /*Drying Mode*/
  const char *status[] = {"Start ", "Abort"};                                             /*Status*/
  const char *message[] = {"IDLE", "DRYING", "ABORTED", "DONE"};                      /*Message to show*/
  const char *mode_info[] = {"Dries for a specified amount of time.", "Continuously dries throughout the",  "printing process."};
  
  uint8_t temp[] = {50, 60, 70, 80, 70, 80, 100, 100};
  uint8_t hour[] = {2, 4, 6, 10, 10, 10, 24, 24};
  uint8_t minute[] = {0, 0, 0, 0, 0, 0,0,0};
  
  uint16_t mode_idx = 0;
  uint16_t filament_idx = 0;
  
  unsigned long prev_time = 0;
  unsigned long current_time = 0;
  unsigned long new_hour = 0;
  unsigned long new_minute = 0;
  unsigned long time_interval = 0;


  // Rotary encoder variables
  uint8_t is_redraw = 1;
  uint8_t rotary_event = 0; // 0 = not turning, 1 = CW, 2 = CCW
  uint8_t push_event = 0; // 0 = not pushed, 1 = pushed
  uint8_t debounce_time = 5; // ms

  //Buzzer Settings
  long int buzz_time_push = 4000;
  long int buzz_time_rotate = 600;
 


//....................GLOBAL VARIABLES....................//


//....................FUNCTIONS....................//


//.....Buzzer Functions....//

void pushBuzz(void){ //beep on press
  digitalWrite(BUZZER, HIGH); 
  delayMicroseconds(buzz_time_push);
  digitalWrite(BUZZER, LOW);
}

void rotationBuzz(void){  //beep on rotate

  digitalWrite(BUZZER, HIGH); 
  delayMicroseconds(buzz_time_rotate);
  digitalWrite(BUZZER, LOW); 
}
 

//.....Buzzer Functions....//



//.....Rotary encoder functions....//


void detect_events(void) {  // detect the rotary encoder events of rotation and push
  uint8_t tmp;
  // 0 = not pushed, 1 = pushed  
  tmp = rotary.push();
  if ( tmp != 0 )         // only assign the push event, never clear the event here
  {
    push_event = tmp;
    pushBuzz();

  }
  // 0 = not turning, 1 = CW, 2 = CCW
  tmp = rotary.rotate();
  if ( tmp != 0 )       // only assign the rotation event, never clear the event here
  { 
    rotary_event = tmp;
    rotationBuzz();
  }
}

void handle_events(void) {  // handle the rotary encoder events
  // 0 = not pushed, 1 = pushed  
  if ( push_event == 1 ) {
      mui.sendSelect();
      is_redraw = 1;
      push_event = 0;
  }
  
  // 0 = not turning, 1 = CW, 2 = CCW
  
   if ( rotary_event == 1 ) {
      mui.nextField();
      is_redraw = 1;
      rotary_event = 0;
      }

  
  if ( rotary_event == 2 ) {
      mui.prevField();
      is_redraw = 1;
      rotary_event = 0;
}
}

//.....Rotary encoder functions....//





 /*Basic geometry of the home screen frame
  *It corresponds to MUIF_RO("MM", home_frame) in muif_list.*/
uint8_t home_frame(mui_t *ui, uint8_t msg){
  
    if ( msg == MUIF_MSG_DRAW ) {
      
        /*Outer Frame*/
        
        u8g2.drawFrame(0, 0, 128, 64);
        u8g2.drawHLine(0, 43, 128);
        u8g2.drawHLine(0, 53, 128);
        u8g2.drawVLine(63, 0, 43);
    
    }
   
    return 0;
}

//Display current temperature on the home screen
 uint8_t currrent_temperature_humidity(mui_t *ui, uint8_t msg){
     if ( msg == MUIF_MSG_DRAW ) {
      
         u8g2.setCursor(2, 27);
         u8g2.print(actual_temp);
         u8g2.setCursor(65, 27);
         u8g2.print(humidity);
     }
      return 0;
  }

//Display target temperature & humidity
 uint8_t target_temperature_humidity(mui_t *ui, uint8_t msg){
  
     if ( msg == MUIF_MSG_DRAW ) {
         
         u8g2.setCursor(30, 38);
         if (filament_idx == 0)
            u8g2.print("/--");
         else
         {
            u8g2.print("/");
            u8g2.print(temp[filament_idx]+5);
         }
          u8g2.setCursor(94, 38);
          if (filament_idx == 0)
            u8g2.print("/--");
          else
          {
            u8g2.print("/");
            u8g2.print(5);
          }
     }
      return 0;
  }



/*Shows the changing values in the main screen.
 *It corresponds to MUIF_RO("SV", stat_val) in muif_list.*/
 uint8_t stat_val(mui_t *ui, uint8_t msg){
  
     if ( msg == MUIF_MSG_DRAW ) {
         
  
         u8g2.setCursor(22, 61);
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


 
/*Changes the background colour.
 *It corresponds to MUIF_RO("BG", colour) in muif_list.*/
 uint8_t colour(mui_t *ui, uint8_t msg){
  
     if ( msg == MUIF_MSG_DRAW ) {
         switch (check){
            case 1:
              strip.setPixelColor(0, strip.Color(0, 255, 0));
              strip.show();
              u8g2.setCursor(86,61);
              u8g2.print(message[check]);
              break;
           case 2:
              strip.setPixelColor(0, strip.Color(255, 0, 0));
              strip.show();
              u8g2.setCursor(86,61);
              u8g2.print(message[check]);
              break;
           case 3:
              strip.setPixelColor(0, strip.Color(0, 0, 255));
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
  *It corresponds to MUIF_RO("GM", menu_frame) in muif_list.*/
  
 uint8_t menu_frame(mui_t *ui, uint8_t msg){
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
    mui_u8g2_draw_button_utf(ui, fg(ui), 0, u8g2.getDisplayWidth(), 1, status[index]);
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
  *It corresponds to MUIF_RO("BG", home_frame) in muif_list.*/
  uint16_t filament_name_list_get_cnt(void *data) {
    return sizeof(filaments)/sizeof(*filaments);   
  }

  /* Filament corresponding to particular id 
   *It corresponds to MUIF_RO("BG", home_frame) in muif_list.*/
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
  
  custom_hour = hour[filament_idx];
  
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
    MUIF_U8G2_FONT_STYLE(3, u8g2_font_maniac_te),         /* actual temp/humi font */
    MUIF_U8G2_FONT_STYLE(4, u8g2_font_littlemissloudonbold_te),  /* target temp/humi font */
    MUIF_U8G2_LABEL(),
  
    /*Mostly Form 1 [HOME SCREEN] related*/
    MUIF_RO("MM", home_frame),  //Draw Home frame
    MUIF_RO("SV", stat_val),  //Print Status elements
    MUIF_RO("TH", currrent_temperature_humidity), //Print Current Temperature and Humidity
    MUIF_RO("TV", target_temperature_humidity),  //Print Target Temperature and Humidity

    MUIF_RO("BG", colour),
    MUIF_BUTTON("BT", mui_u8g2_btn_goto_wm_fi),
  
    /*Mostly Form 2 [MENU] related*/
    MUIF_RO("GM", menu_frame),  //Draw Menu frame
    MUIF_GOTO(process),
    MUIF_BUTTON("GF",mui_u8g2_goto_form_w1_pi),  //Buttons for "inverted text cursor"
    MUIF_RO("GD", mui_u8g2_goto_data),  //Create field for the definition of jump targets
  
    /*Mostly Form 4 [MODE] related*/
    MUIF_U8G2_U16_LIST("MD", &mode_idx, NULL, mode_get_str, mode_get_cnt, mui_u8g2_u16_list_line_wa_mud_pi),
    MUIF_RO("MI", mode_inf),
    
    /*Mostly Form 5 [MATERIAL] related*/
    MUIF_U8G2_U16_LIST("FN", &filament_idx, NULL, filament_name_list_get_str, filament_name_list_get_cnt, mui_u8g2_u16_list_line_wa_mud_pi),
    MUIF_BUTTON("TL", temp_list),
    MUIF_BUTTON("HL", hour_list),
    MUIF_BUTTON("ML", minute_list),
};

fds_t fds_data[] =

  /*
   * Form 1 [HOME SCREEN]
   */
    MUI_FORM(1)
    MUI_STYLE(3)  // actual temp/humi font
    MUI_AUX("TH")
    MUI_STYLE(4) // target temp/humi font
    MUI_AUX("TV")
    MUI_STYLE(2) // tiny font
    MUI_XY("MM", 30, 38)
    MUI_LABEL(54,8, "°C")
    MUI_LABEL(112,8,"RH%")
    MUI_AUX("SV")
    MUI_AUX("BG")
    MUI_LABEL(86,51, "TIME:")
    MUI_LABEL(113,51, ":")
    MUI_LABEL(3,51, "MODE:")
    MUI_LABEL(3,61,"MAT:")
    MUI_XYAT("BT", 130, 80, 2, "Next")

  /*
   * Form 2 [MENU]
   */
    MUI_FORM(2)
    
    MUI_AUX("GM")  //Draw Menu frame
    MUI_STYLE(1)
    MUI_LABEL(48, 11, "MENU")
    MUI_STYLE(0)
    MUI_GOTO(5, 24, 1,"")
      MUI_DATA("GD",
      MUI_4 "Set Mode|"
      MUI_5 "Set Material|")
    MUI_XYA("GF", 5, 36, 0)  //Buttons for "inverted text cursor"
    MUI_XYA("GF", 5, 48, 1)  //Buttons for "inverted text cursor"
    MUI_XYAT("BT", 64, 62, 1, "BACK")


  /*
   * Form 4 [MODE]
   */
    MUI_FORM(4)

    MUI_AUX("GM") //Draw Menu frame
  
    MUI_STYLE(1)
    MUI_LABEL(48, 11, "MODE")
  
    MUI_STYLE(0)
    MUI_LABEL(5, 24, "SET:")
     
    MUI_XYA("MD", 30, 24, 44)
    MUI_XYAT("BT", 64, 62, 2, "BACK")

    MUI_STYLE(2)
    MUI_XYA("MI", 2, 40, 34)  
  
    
   
  /*
   * Form 5 [MATERIAL]
   */
    MUI_FORM(5)

    MUI_AUX("GM") //Draw Menu frame

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

    MUI_XYAT("BT", 64, 62, 2, "BACK")

  
;


void setup(void) {
  
    dht.begin ();
    rotary.setDebounceDelay(debounce_time);
    
    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, LOW);
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

// Read the temperature and humidity from the DHT sensor
void read_from_dht(void){
  if (!(isnan (actual_temp) || isnan (humidity))) {
    actual_temp = dht.readTemperature();
    humidity = dht.readHumidity();
    is_redraw = 1;
  }
}
// Control the humidity Bang Bang controller
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

// Control the temperature Bang Bang controller
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

// Check the clock
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
void check_preset_clock(void){
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
  if (((actual_hour * 3600000UL) + (actual_minute* 60000UL)) > time_interval){
    start_check = 0;
    check = 3;
    actual_minute = 0;
    actual_hour = 0;
    is_redraw = 1;
  }
}

// if mode is preheat, then set a countdown timer


void loop() {
  
  /* check whether the menu is active */
  if ( mui.isFormActive() ) {
    read_from_dht();
    humidity_controller();
    temperature_controller();
    if ((start_check == 1) && (mode_idx == 1)){
      check_continuous_clock();
    }
    else if ((start_check == 1) && (mode_idx == 0)){
      new_minute = minute[filament_idx] * 60000UL;
      new_hour = hour[filament_idx] * 3600000UL;
      time_interval = new_minute + new_hour;
      check_preset_clock();
    }
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
