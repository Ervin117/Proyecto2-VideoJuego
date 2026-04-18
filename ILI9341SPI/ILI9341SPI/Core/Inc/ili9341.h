/*
 * ili9341.h
 *
 *  Created on: Aug 20, 2024
 *      Author: Pablo Mazariegos
 */

#ifndef INC_ILI9341_H_
#define INC_ILI9341_H_

#include "lcd_registers.h"
#include "font.h"
#include <stdint.h>
#include "main.h"

void LCD_Init(void);
void LCD_CMD(uint8_t cmd);
void LCD_DATA(uint8_t data);
void SetWindows(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2);
void LCD_Clear(unsigned int c);
void H_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c);
void V_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c);
void Rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c);
void FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int c);
void LCD_Print(char* text, int x, int y, int fontSize, int color, int background);

void LCD_Bitmap(unsigned int x, unsigned int y, unsigned int width, unsigned int height, const uint16_t *bitmap);
void LCD_BitmapTransparent(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *bitmap, uint16_t transparentColor);
void LCD_Sprite(int x, int y, int width, int height, const uint16_t *bitmap,
                int columns, int index, char flip, char offset);

void LCD_DibujarSpriteBuffer(int x, int y, int w, int h, const uint16_t *sprite_map, int frame, const uint16_t *fondo_global, int ancho_fondo_total, uint16_t color_transparente);

void LCD_Bitmap_DMA(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *bitmap);

void LCD_DibujarSpriteUniversal(int x, int y, int w, int h, const uint16_t *sprite_map, int frame, int ancho_ss, const uint16_t *fondo_global, int ancho_fondo_total, uint16_t color_transparente, uint16_t *buffer_dest);

typedef struct {
    const uint16_t *sprite;
    const uint16_t *fondo;
    int posX;
    int ancho;
    int alto;
    int yStop;
} DatosPersonaje;

void EjecutarAnimacion(DatosPersonaje p);


typedef struct {
    int x, y;           // Posición actual
    int oldX;           // Para borrar rastro al moverse
    int direccion;      // 1: derecha, -1: izquierda
    int limite_izq;     // Píxel mínimo (ej. 10)
    int limite_der;     // Píxel máximo (ej. 300)

    // Variables de disparo
    int bx, by;
    int b_activo;
    int dist_recorrida;
    int oldBX, oldBY;
    uint32_t ultimo_disparo;
} NaveEnemiga;

// Luego el prototipo de la función:
void ProcesarEnemigo(NaveEnemiga *e, int frame_enemigo, const uint16_t *spr_en, const uint16_t *spr_be,
                     const uint16_t *fondo, uint16_t colorT, int offset_ms);
#endif /* INC_ILI9341_H_ */
