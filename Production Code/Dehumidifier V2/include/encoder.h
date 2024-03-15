#include <SimpleRotary.h>
#include <Arduino.h>
#include "pins.h"

SimpleRotary rotary(EN_A,EN_B,EN_BTN);

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
      for(long int i = 0; i<10000; i++){
        digitalWrite(BUZZER, HIGH); 
      }
      digitalWrite(BUZZER, LOW);
      
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