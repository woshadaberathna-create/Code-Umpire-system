// CONFIGURATION BITS
#pragma config FOSC = HS
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

#include <xc.h>
#define _XTAL_FREQ 8000000

// LCD Hardware Pin Definitions (PORTD)
#define RS PORTDbits.RD2
#define EN PORTDbits.RD3
#define D4 PORTDbits.RD4
#define D5 PORTDbits.RD5
#define D6 PORTDbits.RD6
#define D7 PORTDbits.RD7

// Sensor Inputs (PORTB)
#define SENSOR_GROUND     PORTBbits.RB0  
#define WICKET_BROKEN     PORTBbits.RB1  
#define BOWLER_FOOT_FAULT PORTBbits.RB2  
#define SENSOR_LOW_AIR    PORTBbits.RB3  
#define SENSOR_HIGH_AIR   PORTBbits.RB4  

// Indicators & Alarms (PORTC)
#define RED_LED   PORTCbits.RC0
#define GREEN_LED PORTCbits.RC1
#define BUZZER    PORTCbits.RC2

void Lcd_Port(char a) {
    if(a & 1) D4 = 1; else D4 = 0;
    if(a & 2) D5 = 1; else D5 = 0;
    if(a & 4) D6 = 1; else D6 = 0;
    if(a & 8) D7 = 1; else D7 = 0;
}

void Lcd_Cmd(char a) {
    RS = 0; Lcd_Port(a >> 4); EN = 1; __delay_us(100); EN = 0; __delay_us(100); 
    Lcd_Port(a & 0x0F); EN = 1; __delay_us(100); EN = 0; __delay_ms(2); 
}

void Lcd_Write_Char(char a) {
    RS = 1; Lcd_Port(a >> 4); EN = 1; __delay_us(100); EN = 0; __delay_us(100); 
    Lcd_Port(a & 0x0F); EN = 1; __delay_us(100); EN = 0; __delay_us(100); 
}

void Lcd_Clear() { Lcd_Cmd(0x01); __delay_ms(2); }

void Lcd_Set_Cursor(char r, char c) {
    if(r == 1) Lcd_Cmd(0x80 + c - 1);
    else if(r == 2) Lcd_Cmd(0xC0 + c - 1);
}

void Lcd_Init() {
    __delay_ms(20); RS = 0;
    Lcd_Port(0x03); EN = 1; __delay_us(100); EN = 0; __delay_ms(5);
    Lcd_Port(0x03); EN = 1; __delay_us(100); EN = 0; __delay_us(200);
    Lcd_Port(0x03); EN = 1; __delay_us(100); EN = 0; __delay_us(200);
    Lcd_Port(0x02); EN = 1; __delay_us(100); EN = 0; __delay_ms(2);
    Lcd_Cmd(0x28); Lcd_Cmd(0x0C); Lcd_Cmd(0x06); Lcd_Clear();
}

void Lcd_Write_String(char *a) {
    int i; for(i=0; a[i]!='\0'; i++) Lcd_Write_Char(a[i]);
}

void Sound_Alarm(int duration_ms) {
    for(int i = 0; i < duration_ms; i++) {
        BUZZER = 1;
        __delay_us(500); 
        BUZZER = 0;
        __delay_us(500); 
    }
}

void main(void) {
    TRISD = 0x00; TRISC = 0x00; TRISB = 0xFF;
    PORTD = 0x00; PORTC = 0x00; 

    Lcd_Init();
    Lcd_Set_Cursor(1, 2); Lcd_Write_String("SYSTEM READY");
    Lcd_Set_Cursor(2, 2); Lcd_Write_String("SMART UMPIRE");
    
    GREEN_LED = 1; 
    __delay_ms(2000); Lcd_Clear();

    int state_triggered = 0;
    int bat_grounded_once = 0; 

    while(1) {
        if (SENSOR_GROUND == 1) {
            bat_grounded_once = 1; 
        }
        
        if (SENSOR_GROUND == 0 && SENSOR_LOW_AIR == 0 && SENSOR_HIGH_AIR == 0) {
            bat_grounded_once = 0; 
        }

        if (WICKET_BROKEN == 1) {
            Lcd_Clear();
            int currently_in_zone = (SENSOR_GROUND == 1 || SENSOR_LOW_AIR == 1 || SENSOR_HIGH_AIR == 1);
            int batter_was_given_out = 0; 
            
            if (currently_in_zone == 1 && bat_grounded_once == 1) {
                RED_LED = 0; GREEN_LED = 1; 
                Lcd_Set_Cursor(1, 3); Lcd_Write_String("NOT OUT");
                Lcd_Set_Cursor(2, 1); Lcd_Write_String("BAT SAFE");
                __delay_ms(3000); 
            } else {
                RED_LED = 1; GREEN_LED = 0; 
                batter_was_given_out = 1; 
                
                if (currently_in_zone == 1 && bat_grounded_once == 0) {
                    Lcd_Set_Cursor(1, 7); Lcd_Write_String("OUT!");
                    Lcd_Set_Cursor(2, 3); Lcd_Write_String("NEVER GROUND");
                } else {
                    Lcd_Set_Cursor(1, 1); Lcd_Write_String("MANKAD DETECTED!");
                    Lcd_Set_Cursor(2, 5); Lcd_Write_String("OUT!!");
                }
                Sound_Alarm(3000); 
            }
            
            Lcd_Clear();
            
            // UPGRADED CONTINUOUS TIMER LOGIC
            if (batter_was_given_out == 1) {
                int batter_ready = 0;
                
                // Keep looping until a batter finally shows up!
                while (batter_ready == 0) {
                    int time_left = 120; // Reset to 120s for the next guy
                    
                    while(time_left > 0) {
                        Lcd_Set_Cursor(1, 1); Lcd_Write_String("NEW BATTER TIMER");
                        Lcd_Set_Cursor(2, 1); Lcd_Write_String("TIME LEFT: ");
                        
                        Lcd_Write_Char((time_left / 100) + '0');
                        Lcd_Write_Char(((time_left / 10) % 10) + '0');
                        Lcd_Write_Char((time_left % 10) + '0');
                        Lcd_Write_String("s  ");

                        for(int j = 0; j < 10; j++) {
                            if (SENSOR_GROUND == 1) {
                                batter_ready = 1;
                                break;
                            }
                            __delay_ms(100);
                        }
                        
                        if (batter_ready == 1) break; 
                        time_left--;
                    }

                    Lcd_Clear();
                    if (batter_ready == 1) {
                        GREEN_LED = 1; RED_LED = 0;
                        Lcd_Set_Cursor(1, 3); Lcd_Write_String("BATTER READY");
                        Lcd_Set_Cursor(2, 4); Lcd_Write_String("SAFE!");
                        __delay_ms(2000);
                    } else {
                        // Nobody arrived in time. Trigger out, then restart the loop!
                        GREEN_LED = 0; RED_LED = 1;
                        Lcd_Set_Cursor(1, 3); Lcd_Write_String("TIMED OUT!");
                        Lcd_Set_Cursor(2, 3); Lcd_Write_String("BATTER OUT");
                        Sound_Alarm(3000); 
                        __delay_ms(2000);
                        Lcd_Clear();
                    }
                }
            }
            
            state_triggered = 1;
            while(WICKET_BROKEN == 1); 
        }
        
        else if (BOWLER_FOOT_FAULT == 1) {
            Lcd_Clear();
            RED_LED = 1; GREEN_LED = 0; 
            Lcd_Set_Cursor(1, 5); Lcd_Write_String("NO BALL!");
            Lcd_Set_Cursor(2, 2); Lcd_Write_String("FOOT FAULT");
            Sound_Alarm(1500); 
            __delay_ms(1500); 
            Lcd_Clear();
            state_triggered = 1;
        }

        else {
            if (state_triggered == 1) {
                Lcd_Clear();
                state_triggered = 0; 
            }
            RED_LED = 0; GREEN_LED = 1; 
            Lcd_Set_Cursor(1, 3); Lcd_Write_String("MATCH ACTIVE");
            Lcd_Set_Cursor(2, 1); Lcd_Write_String("AWAITING DELIVERY");
        }
        
        __delay_ms(10); 
    }
}