#pragma once

#include "stm32l476xx.h"

// Stałe opisujące klawisze
#define IR_CODE_ONOFF      0x45
#define IR_CODE_MENU       0x47
#define IR_CODE_TEST       0x44
#define IR_CODE_PLUS       0x40
#define IR_CODE_BACK       0x43
#define IR_CODE_REWIND     0x07
#define IR_CODE_PLAY       0x15
#define IR_CODE_FORWARD    0x09
#define IR_CODE_0          0x16
#define IR_CODE_MINUS      0x19
#define IR_CODE_CANCEL     0x0D
#define IR_CODE_1          0x0C
#define IR_CODE_2          0x18
#define IR_CODE_3          0x5E
#define IR_CODE_4          0x08
#define IR_CODE_5          0x1C
#define IR_CODE_6          0x5A
#define IR_CODE_7          0x42
#define IR_CODE_8          0x52
#define IR_CODE_9          0x4A

// Procedura obsługi przerwania
void ir_tim_interrupt(void);

// Inicjalizacja modułu
void ir_init(void);

// Funkcja odczytująca dane
// return - kod klawisza lub -1
int ir_read(void);
