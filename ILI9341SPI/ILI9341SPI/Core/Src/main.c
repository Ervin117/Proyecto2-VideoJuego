/* USER CODE BEGIN Header */
/*
 * Este código es la pantalla del videojuego proyecto 2 de electronica digital 2
 * Para este código se implementaron varios conceptos aprendidos en clase
 */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
//Se incluyen todas las librerias que se van a utilizar.
#include "fatfs_sd.h"
#include "ili9341.h"
#include "bitmaps.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
FATFS fs;
FATFS *pfs;
FIL fil;
FRESULT fres;
DWORD fre_clust;
uint32_t totolSpace, freeSpace;
char buffer[100];

// Implementación de estructura para las variables de interrupción provenientes del controls de PS4
typedef struct {
    int arriba;
    int derecha;
    int abajo;
    int izquierda;
    int cuadrado;
    int r1;
} PlayStationBuffer;

// Estructura para colocar los datos necesarios para mover las naves de los jugadores
typedef struct {
    int posX, posY, oldX, oldY;
    int ancho, alto;
    int frame_actual;
    int old_frame;             //Reseteo del sprite
    const uint16_t *sprite;
    uint16_t *buffer;
    int blaster_activo;
    float blasterX, blasterY;
    int oldBX, oldBY;
    float dist_blaster;
    int frame_blaster;
    int vivo;
    uint16_t *b_buffer;
} Jugador;



/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart5;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */

//--- Variables para selección de personajes ---------
int cinematica = 0;								// Variable para llevar el registro de que parte del videojuego esta ejecutando en el ciclo de la logica
int colocar_fondo = 1;
// Variables para reiniciar y llamar a los fondos en cada parte del codigo necesario.
int fondo = 0;
int fondo1 = 0;
int fondo2 = 0;
int fondo3 = 0;
int fondo4 = 0;
int fondo5 = 0;
int fondo6 = 0;
int balas_por_sobrevivir = 40; // Cantidad de disparos que deben pasar
int balas_lanzadas = 0;

// Definición de variables para la logica del boss del juego (NIVEL 3)
int bossHP = 10;
int bossX = 110, bossY = 10, bossOldX;
int bossDir = 1;
uint16_t b_mezcla_boss[50 * 50];
//----------------------------------------------------

// ------ Variables para cinematica ------------------
uint16_t sprite_buffer[64 * 64]; 				// Las librerias utilizan este buffer
// ---------------------------------------------------

//-------- Variables exclusivas del blaster jugadores ---------
int anchoB = 7, altoB = 14;						//Dimensiones del blaster
//-------------------------------------------------------------

// ================= variables para JUGADORES==========================
//=====================================================================
// Variables generales para ambos jugadores
Jugador j[2]; 									// Definimos la estructura
uint16_t colorTrans = 0xffe0; 					// Definimos el color transparente
int velocidad = 5;								// Para controlar la velocidad del movimiento
//-------------- Variables exclusivas del X-wing J1 ---------
// para la seleccion de personaje
int character = 0;
int char_final = 0;
int personaje_seleccionado_J1 = 0;
int nivel_actual = 1;
int nivel_confirmado = 0;
// para jugabilidad y borrado de rastro cuando se mueve
uint16_t sprite_buffer_J1[64 * 64];				// Buffer para poder borrar el fondo del sprite, debe ser mas grande que una columna del sprite
uint16_t b_buffer_J1[28*14];
//-------------------------------------------------

//----------- Variables exclusivas del halcon milenario J2 ---------
// Para seleccion personaje
 uint8_t char_final_J2 = 0;
uint8_t character_J2 = 0;
uint8_t personaje_seleccionado_J2 = 0;
//---------------------------
uint16_t sprite_buffer_J2[64 * 64];				// Buffer para poder borrar el fondo del sprite, debe ser mas grande del una columna del sprite
uint16_t b_buffer_J2[28*14];
//===================================================================

// ================= variables para ENEMIGOS==========================
//=====================================================================
volatile uint32_t tiempo_spawn = 0; // Contador de tiempo real
uint32_t limite_spawn = 2000;       // Cada 2 segundos (2000 ms)

uint16_t buffer_mezcla[35 * 35];				// Buffer para nave enemiga
NaveEnemiga enemigos[3];						// Definicion de la estructura (libreria)
//-----------------------------------------------------
// Bandera para saber si el DMA terminó de enviar
volatile uint8_t dma_libre = 1;

// MELMAN: esto lo tengo por los fondos que tenia en la ram pero ya no los tengo los comente y solo aparece uno
// Variables necesarias para desplegar los fondos guardados en la RAM
//extern const uint16_t halcon[];
extern const uint16_t xwing[];
extern const uint16_t stars[];
//extern const uint16_t borrador[];
extern const uint16_t choose[];

//------ para UART 1---------- jugador J1
PlayStationBuffer control;
uint8_t rx_data;              // Byte recibido actualmente
char rx_buffer[50];           // Buffer para guardar la cadena "0,0,0,0,0,0"
int rx_index = 0;             // Índice del buffer
int values[6];                // Array para guardar los valores recibidos
uint8_t data_ready = 0;       // Bandera

//------ para UART 3---------- jugador J1
PlayStationBuffer control_J2;
uint8_t rx_data_J2;              // Byte recibido actualmente
char rx_buffer_J2[50];           // Buffer para guardar la cadena "0,0,0,0,0,0"
int rx_index_J2 = 0;             // Índice del buffer
int values_J2[6];                // Array para guardar los valores recibidos
uint8_t data_ready_J2 = 0;       // Bandera


//---- para UART en general -----
uint8_t rxData = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_UART5_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

//********************************FUNCIONES VARIAS *********************************//
//=================================Función corregir colores de los archivo .bin==========================
void FixColorEndianness(uint16_t *buffer, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = (buffer[i] << 8) | (buffer[i] >> 8);
    }
}

//=================================Función verificar coliciones =========================
int colision(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2)
{
	if (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2)
		{
		uint16_t buffer_explo[32 * 26];
		//Dibujar la explisión
			 for (int frame = 0; frame < 4; frame++) {
			        //Dibujo de la explosión con traparencia del fondo.
				 	LCD_DibujarSpriteUniversal(x2, y2, 32, 26, explo, frame, 128, stars, 320, colorTrans, buffer_explo);
			        HAL_Delay(30);
			 }
		//Limpiar la explosión
			 for (int r = 0; r < 26; r++) {
					 static uint16_t line_buffer[32];
					 for (int c = 0; c < 32; c++) {
						 uint32_t idx = (uint32_t)(y2 + r) * 320 + (x2 + c);
						 line_buffer[c] = (stars[idx] << 8) | (stars[idx] >> 8);
					 }
					 LCD_Bitmap(x2, y2 + r, 32, 1, line_buffer);
				 }
			 return 1;
		}
	return 0;
}

//================================= Función para dibujar los fondo desde la SD =======================================
void fondos(char* nombre, uint16_t x, uint16_t y, uint16_t w, uint16_t h ){
	FIL fil;
	UINT bytesRead;
	uint16_t fila_buffer[320];

	// 1. Deseleccionar LCD antes de hablar con la SD
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);   // LCD OFF
	HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);   // SD ON
	HAL_Delay(1);

	if (f_open(&fil, nombre, FA_READ) == FR_OK) {
		for (int i = 0; i < h; i++) {
			// f_read leerá los datos de la SD
			if (f_read(&fil, fila_buffer, w * 2, &bytesRead) == FR_OK /*&& bytesRead > 0*/) {

				// 2. Ahora vamos a hablar con el LCD: Deseleccionar SD, Seleccionar LCD
				HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET); // SD OFF
				HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET); // LCD ON

				// Corregir colores
				FixColorEndianness(fila_buffer, w);

				// Dibujar fila
				LCD_Bitmap(x, y + i, w, 1, (uint16_t*)fila_buffer);

				// 3. Volver a habilitar SD para la siguiente lectura
				HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET); // LCD OFF
				HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET); // SD ON
			}
		}
		f_close(&fil);
		HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET); // Todo OFF al final
	} else {

	}
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_TIM2_Init();
  MX_FATFS_Init();
  MX_UART5_Init();
  /* USER CODE BEGIN 2 */
	LCD_Init();
	LCD_Clear(0x00);

	 HAL_TIM_Base_Start_IT(&htim2); // Inicia el Timer 2 con Interrupciones

	 HAL_UART_Receive_IT(&huart2, &rxData, 1); // Disparar interrupción cuando reciba un byte
	 HAL_UART_Receive_IT(&huart1, &rx_data, 1); // Disparar interrupción cuando reciba un byte
	 HAL_UART_Receive_IT(&huart3, &rxData, 1); // Disparar interrupción cuando reciba un byte

	 // ==============================================================================
	 //========== Inicializar los personajes de sprites para J1 y J2==================
	 // ==============================================================================
	 // --- Jugador 1 (X-Wing) ---
	 j[0].ancho = 50;  j[0].alto = 42;			// Tamaño del sprite
	 j[0].posX = 80;   j[0].posY = 180;			// Posicion inicial
	 j[0].sprite = xwing_realista;  			// Sprite fijo J1
	 j[0].buffer = sprite_buffer_J1;
	 j[0].b_buffer = b_buffer_J1;
	 j[0].vivo = 1;

	 // --- Jugador 2 (Halcón) ---
	 j[1].ancho = 60;  j[1].alto = 38;			// Tamaño del sprite
	 j[1].posX = 240;  j[1].posY = 180;			// Posicion inicial
	 j[1].sprite = halcon_sprite;   			// Sprite fijo J2
	 j[1].buffer = sprite_buffer_J2;
	 j[1].b_buffer = b_buffer_J2;
	 j[1].vivo = 1;

	 // ==============================================================================
	 //========== Inicializar los enemigos uno por uno ===============================
	 // ==============================================================================
	 // Enemigo 1
	 enemigos[0].x = 40;						// Posición en x inicial de la nave
	 enemigos[0].y = 30;						// Posición en y inicial de la nave
	 enemigos[0].direccion = 1;					// Cada nave puede ir a derecha o izquierda inicialmente
	 enemigos[0].limite_izq = 10;				// Limite de la nave lado izquierdo
	 enemigos[0].limite_der = 100;				// Limite de la nave lado derecho

	 // Enemigo 2
	 enemigos[1].x = 200;
	 enemigos[1].y = 50;
	 enemigos[1].direccion = -1;
	 enemigos[1].limite_izq = 150;
	 enemigos[1].limite_der = 300;

	 // Enemigo 2
	 enemigos[2].x = 100;
	 enemigos[2].y = 10;
	 enemigos[2].direccion = 1;
	 enemigos[2].limite_izq = 100;
	 enemigos[2].limite_der = 200;


	 // ==============================================================================
	 //================================= Montar la SD  ===============================
	 // ==============================================================================
	 fres = f_mount(&fs, "", 1);
	 if (fres == FR_OK) {
		 HAL_Delay(100);
	 } else {
	 }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

	while (1) {
		// ==================================================================================================
		// ==================================INICIO=========================================================
		// =================================================================================================

		if (cinematica ==0){
			if (fondo2 == 0) {
				HAL_UART_Transmit(&huart5, (uint8_t*)"3", 1, 10); // Se llama a la música de inicio
				fondos("lucasfilm.bin", 0, 0, 320, 240); // Se llama el fondo de inicio
				HAL_Delay(100);
				fondos("inicio.bin", 0, 0, 320, 240);
				fondo2 = 1;
			}

			// Si cualquier jugador presiona CUADRADO, saltamos a la selección
			if (control.cuadrado || control_J2.cuadrado) {
				cinematica = 1;
				fondo2 = 0;
				HAL_UART_Transmit(&huart5, (uint8_t*)"0", 1, 10);
			}
		}


		// ==================================================================================================
		// ==================================SELECCIÓN PERSONAJES===========================================
		// ==================================================================================================
		if (cinematica == 1){
			if (fondo == 0) {
				fondos("choose.bin", 0, 0, 320, 240); // Se llama al fondo de los porsonajes
				HAL_UART_Transmit(&huart5, (uint8_t*)"1", 1, 10); // Envio de comando para el musica.
				fondo = 1;
			}
		// Lógica seleccion de personaje jugador 1
		if(personaje_seleccionado_J1 == 0){
			if(control.izquierda){
				//Borrar cuadrado anterior
				Rect(84, 51, 74, 174, 0x0000); Rect(84, 52, 74, 174, 0x0000);
				Rect(84, 53, 74, 174, 0x0000); Rect(85, 51, 74, 174, 0x0000); Rect(86, 51, 74, 174, 0x0000);
				//Cuadrado de selección
				Rect(5, 51, 74, 174, 0xf9c0); Rect(5, 52, 74, 174, 0xf9c0);
				Rect(5, 53, 74, 174, 0xf9c0); Rect(6, 51, 74, 174, 0xf9c0); Rect(7, 51, 74, 174, 0xf9c0);
				character = 4;
			}
			else if (control.derecha){
				//Borrar cuadrado anterior
				Rect(5, 51, 74, 174, 0x0000); Rect(5, 52, 74, 174, 0x0000);
				Rect(5, 53, 74, 174, 0x0000); Rect(6, 51, 74, 174, 0x0000); Rect(7, 51, 74, 174, 0x0000);
				//cuadrado de selección
				Rect(84, 51, 74, 174, 0xf9c0); Rect(84, 52, 74, 174, 0xf9c0);
				Rect(84, 53, 74, 174, 0xf9c0); Rect(85, 51, 74, 174, 0xf9c0); Rect(86, 51, 74, 174, 0xf9c0);
				character = 2;
			}else if (control.cuadrado){
				char_final = character;
				personaje_seleccionado_J1 = 1;
			}
		}
		// Lógica seleccion de personaje jugador 2
		if (personaje_seleccionado_J2 == 0){
			if(control_J2.derecha){
				//Borrar cuadrado anterior
				Rect(162, 51, 74, 174, 0x0000); Rect(162, 52, 74, 174, 0x0000);
				Rect(162, 53, 74, 174, 0x0000); Rect(163, 51, 74, 174, 0x0000); Rect(164, 51, 74, 174, 0x0000);
				//Cuadrado de selección
				Rect(238, 51, 74, 174, 0x0210); Rect(238, 52, 74, 174, 0x0210);
				Rect(238, 53, 74, 174, 0x0210); Rect(239, 51, 74, 174, 0x0210); Rect(240, 51, 74, 174, 0x0210);
				character_J2 = 3;
			}
			else if (control_J2.izquierda){
				//Borrar cuadrado anterior
				Rect(238, 51, 74, 174, 0x0000); Rect(238, 52, 74, 174, 0x0000);
				Rect(238, 53, 74, 174, 0x0000); Rect(239, 51, 74, 174, 0x0000); Rect(240, 51, 74, 174, 0x0000);
				//cuadrado de selección
				Rect(162, 51, 74, 174, 0x0210); Rect(162, 52, 74, 174, 0x0210);
				Rect(162, 53, 74, 174, 0x0210); Rect(163, 51, 74, 174, 0x0210); Rect(164, 51, 74, 174, 0x0210);
				character_J2 = 1;
			}else if (control_J2.cuadrado){
				char_final_J2 = character_J2;
				personaje_seleccionado_J2 = 1;
			}
		}


		if (personaje_seleccionado_J1 && personaje_seleccionado_J2){
			cinematica = 2;
			HAL_UART_Transmit(&huart5, (uint8_t*)"0", 1, 10); //Se envia para apagar la musica en cada sección no necesaria
															  //Para cada cinematica o nivel
		}


			 // ==================================================================================================
			 // ==================================CINEMÁTICA PERSONAJES===========================================
			 // ==================================================================================================
		}else if (cinematica == 2){
			DatosPersonaje p;
			HAL_UART_Transmit(&huart5, (uint8_t*)"3", 1, 10); // Se llama a la musica de cinematica
		    // Asignación de valores
		    if (char_final == 2) { p.sprite = luke; p.fondo = borrador;  p.posX = 180; p.ancho = 14; p.alto = 30; p.yStop = 115; } // Cambiar p.fondo = xwing o halcon
		    else if (char_final == 4) { p.sprite = piloto; p.fondo = borrador;  p.posX = 180; p.ancho = 14; p.alto = 30; p.yStop = 115; }

		    fondos("xwing.bin",0,0,320,240);	 // Se llama al fondo correspondiente del jugador 1
		    EjecutarAnimacion(p);                // Llamar a la lógica modular
		    cinematica = 3;
	}
	else if (cinematica == 3){
			DatosPersonaje p;

			// Asignación de valores
			if (char_final_J2 == 1)      { p.sprite = chewbacca; p.fondo = borrador; p.posX = 252; p.ancho = 16; p.alto = 36; p.yStop = 100; }
			else if (char_final_J2 == 3) { p.sprite = han_solo;  p.fondo = borrador; p.posX = 252; p.ancho = 14; p.alto = 32; p.yStop = 100; }

			fondos("halcon.bin",0,0,320,240);	// Se llama el fondo correpondiente del jugador 2
			EjecutarAnimacion(p);                // Llamar a la lógica modular

			cinematica = 4;
			HAL_UART_Transmit(&huart5, (uint8_t*)"0", 1, 10);
		}

		// ==================================================================================================
		// ================================== SELECCIÓN NIVELES ===========================================
		// ==================================================================================================
	else if (cinematica == 4){
		if (fondo3 == 0) {
			fondos("levels.bin", 0, 0, 320, 240); // Fondo de la selección de niveles
			HAL_UART_Transmit(&huart5, (uint8_t*)"1", 1, 10); // Se llama a la musica de selección
			fondo3 = 1;
		}

        // --- Moviento del cuadro a la derecha -----------
        if ((control.derecha || control_J2.derecha) && nivel_actual < 4) {
            // 1. Borrar cuadro actual
            int oldX = (nivel_actual == 1) ? 5 : (nivel_actual == 2) ? 84 : (nivel_actual == 3) ? 163 : 242;
            Rect(oldX, 51, 74, 174, 0x0000); Rect(oldX, 52, 74, 174, 0x0000);
            Rect(oldX, 53, 74, 174, 0x0000); Rect(oldX+1, 51, 74, 174, 0x0000); Rect(oldX+2, 51, 74, 174, 0x0000);

            nivel_actual++; // Avanzar nivel

            // 2. Dibujar cuadro en nueva posición
            int newX = (nivel_actual == 1) ? 5 : (nivel_actual == 2) ? 84 : (nivel_actual == 3) ? 163 : 242;
            Rect(newX, 51, 74, 174, 0x07E0); Rect(newX, 52, 74, 174, 0x07E0);
            Rect(newX, 53, 74, 174, 0x07E0); Rect(newX+1, 51, 74, 174, 0x07E0); Rect(newX+2, 51, 74, 174, 0x07E0);

            HAL_Delay(200);
        }

        // --- Movimiento del cuadro a la izquierda ---
        else if ((control.izquierda || control_J2.izquierda) && nivel_actual > 1) {
            // 1. Borrar cuadro actual
            int oldX = (nivel_actual == 1) ? 5 : (nivel_actual == 2) ? 84 : (nivel_actual == 3) ? 163 : 242;
            Rect(oldX, 51, 74, 174, 0x0000); Rect(oldX, 52, 74, 174, 0x0000);
            Rect(oldX, 53, 74, 174, 0x0000); Rect(oldX+1, 51, 74, 174, 0x0000); Rect(oldX+2, 51, 74, 174, 0x0000);

            nivel_actual--; // Retroceder nivel

            // 2. Dibujar cuadro en nueva posición
            int newX = (nivel_actual == 1) ? 5 : (nivel_actual == 2) ? 84 : (nivel_actual == 3) ? 163 : 242;
            Rect(newX, 51, 74, 174, 0x07E0); Rect(newX, 52, 74, 174, 0x07E0);
            Rect(newX, 53, 74, 174, 0x07E0); Rect(newX+1, 51, 74, 174, 0x07E0); Rect(newX+2, 51, 74, 174, 0x07E0);

            HAL_Delay(200);
        }
        if (control.cuadrado || control_J2.cuadrado) {
			cinematica = 4 + nivel_actual;  // Depende de la posición del cuadro se selecciona el nivel.
			HAL_UART_Transmit(&huart5, (uint8_t*)"0", 1, 10);
        }

	}

		// ==================================================================================================
		// ==================================== JUEGO PRINCIPAL / NIVELES =============================================
		// ==================================================================================================


		//===========================================================================================
		// ====================================	NIVEL 1 =============================================
		//===========================================================================================
	else if (cinematica == 5){
		if (fondo4 == 0) {
			fondos("nivel3.bin", 0, 0, 320, 240); // fondo del nivel
			fondo4 = 1;
			HAL_UART_Transmit(&huart5, (uint8_t*)"4", 1, 10);
			// Reiniciar los jugadores y los enemigos
			for (int i = 0; i < 2; i++) {
				j[i].vivo = 1;
				j[i].blaster_activo = 0;
				j[i].posX = (i == 0) ? 80 : 240;
				j[i].posY = 180;
			}

			for (int e = 0; e < 3; e++) {
				enemigos[e].b_activo = 0;
				enemigos[e].dist_recorrida = 0;
				enemigos[e].ultimo_disparo = HAL_GetTick(); // Resetear reloj de disparos
			}

			// Posiciones iniciales de enemigos
			enemigos[0].x = 10;  enemigos[0].y = 30;  enemigos[0].direccion = 1;
			enemigos[1].x = 290; enemigos[1].y = 70;  enemigos[1].direccion = -1;
			enemigos[2].y = -100;
			j[0].vivo = 1; j[1].vivo = 1;
			j[0].posX = 80; j[0].posY = 180;
			j[1].posX = 240; j[1].posY = 180;
			j[0].blaster_activo = 0; j[1].blaster_activo = 0;

			// Posición de los enemigos
			enemigos[0].x = 10;  enemigos[0].y = 30; enemigos[0].direccion = 1;
			enemigos[1].x = 290; enemigos[1].y = 70; enemigos[1].direccion = -1;
			enemigos[2].y = -100;
			for(int i=0; i<3; i++) { enemigos[i].limite_izq = 10; enemigos[i].limite_der = 290; }
		}

		// ================= FASE 1: JUGADORES =================
		 for (int i = 0; i < 2; i++) {
				if (!j[i].vivo) continue;

				j[i].oldX = j[i].posX;
				j[i].oldY = j[i].posY;
				j[i].oldBX = (int)j[i].blasterX;
				j[i].oldBY = (int)j[i].blasterY;

				PlayStationBuffer *ctrl = (i == 0) ? &control : &control_J2;

				// 1. ASIGNACIÓN DE FRAME
				j[i].frame_actual = 0;

				// 2. DETECCIÓN DE BOTONES
				if (ctrl->derecha) {
					j[i].posX += velocidad;
					j[i].frame_actual = 2; // Frame de inclinación derecha
				}
				else if (ctrl->izquierda) {
					j[i].posX -= velocidad;
					j[i].frame_actual = 1; // Frame de inclinación izquierda
				}

				if (ctrl->arriba) j[i].posY -= velocidad;
				if (ctrl->abajo)  j[i].posY += velocidad;

				// 3. LÍMITES DE PANTALLA
				if (j[i].posX < 0) j[i].posX = 0;
				if (j[i].posX > (320 - j[i].ancho)) j[i].posX = 320 - j[i].ancho;
				if (j[i].posY < 120) j[i].posY = 120; // Ajustado para que no suban mucho
				if (j[i].posY > (240 - j[i].alto)) j[i].posY = 240 - j[i].alto;

				// 4. BORRADO DE RASTRO
				if (j[i].oldX != j[i].posX || j[i].oldY != j[i].posY || j[i].frame_actual != j[i].old_frame) {
					while(!dma_libre);
					for (int r = 0; r < j[i].alto; r++) {
						for (int c = 0; c < j[i].ancho; c++) {
							uint32_t idx = (uint32_t)(j[i].oldY + r) * 320 + (j[i].oldX + c);
							j[i].buffer[r * j[i].ancho + c] = (stars[idx] << 8) | (stars[idx] >> 8);
						}
					}
					LCD_Bitmap_DMA(j[i].oldX, j[i].oldY, j[i].ancho, j[i].alto, j[i].buffer);
					j[i].old_frame = j[i].frame_actual;
				}

			// Logica para generar los blaster de los jugadores
			if (ctrl->cuadrado && !j[i].blaster_activo) {
				j[i].blaster_activo = 1;
				j[i].blasterX = j[i].posX + (j[i].ancho/2) - 3;
				j[i].blasterY = j[i].posY - 12;
			}

			if (j[i].blaster_activo) {
				j[i].oldBX = (int)j[i].blasterX; j[i].oldBY = (int)j[i].blasterY;
				j[i].blasterY -= 8;

				// Borrar rastro blaster
				while(!dma_libre);
				for (int r = 0; r < altoB; r++) {
					for (int c = 0; c < anchoB; c++) {
						int py = j[i].oldBY + r; int px = j[i].oldBX + c;
						if(py < 0) py = 0; if(py > 239) py = 239;
						uint32_t idx = (uint32_t)py * 320 + px;
						j[i].b_buffer[r * anchoB + c] = (stars[idx] << 8) | (stars[idx] >> 8);
					}
				}
				LCD_Bitmap_DMA(j[i].oldBX, j[i].oldBY, anchoB, altoB, j[i].b_buffer);
				if (j[i].blasterY < 15) j[i].blaster_activo = 0;
			}
		}

		// ================= FASE 2: CONSTRUCCIÓN DE ENEMIGOS =================
		for (int i = 0; i < 2; i++) {
			if (enemigos[i].y < 0) continue;

			// CONTROL DE DISPARO
			if (!enemigos[i].b_activo) {
				int ex = enemigos[i].x;
				// Disparos en los extremos y a la mitad de la pantalla
				if (ex <= 12 || ex >= 288 || (ex > 148 && ex < 155)) {
					enemigos[i].bx = enemigos[i].x + 4;
					enemigos[i].by = enemigos[i].y + 24;
					enemigos[i].b_activo = 1;
					enemigos[i].dist_recorrida = 0;
				}
			}
			// La función ProcesarEnemigo mueve el enemigo a 2px
			ProcesarEnemigo(&enemigos[i], i, enemigo, blaster_verde, stars, colorTrans, 0);
		}

		// ================= FASE 3: COLISIONES Y MUERTE LIMPIA =================
		for (int i = 0; i < 2; i++) {
			if (!j[i].vivo) continue;
			for (int e = 0; e < 2; e++) {
				if (enemigos[e].y < 0) continue;

				// Jugador elimina a Enemigo
				if (j[i].blaster_activo && colision((int)j[i].blasterX, (int)j[i].blasterY, anchoB, altoB, enemigos[e].x, enemigos[e].y, 16, 24)) {
					j[i].blaster_activo = 0;
					enemigos[e].y = -100; // Eliminar
				}
				// Enemigo elimina a Jugador
				if (enemigos[e].b_activo && colision(enemigos[e].bx, enemigos[e].by, 8, 23, j[i].posX, j[i].posY, j[i].ancho, j[i].alto)) {
					// Borrado de jugadores.
					while(!dma_libre);
					for (int r = 0; r < j[i].alto; r++) {
						for (int c = 0; c < j[i].ancho; c++) {
							uint32_t idx = (uint32_t)(j[i].posY + r) * 320 + (j[i].posX + c);
							j[i].buffer[r * j[i].ancho + c] = (stars[idx] << 8) | (stars[idx] >> 8);
						}
					}
					LCD_Bitmap_DMA(j[i].posX, j[i].posY, j[i].ancho, j[i].alto, j[i].buffer);
					j[i].vivo = 0; // El jugador se borra si es eliminado para no volverlo a generarse.
				}
			}
		}

		// ================= FASE 4: DIBUJADO =================
		for (int i = 0; i < 2; i++) {
			if (!j[i].vivo) continue;
			LCD_DibujarSpriteUniversal(j[i].posX, j[i].posY, j[i].ancho, j[i].alto, j[i].sprite, j[i].frame_actual, (j[i].ancho * 3), stars, 320, colorTrans, j[i].buffer);
			if (j[i].blaster_activo) {
				LCD_DibujarSpriteUniversal((int)j[i].blasterX, (int)j[i].blasterY, anchoB, altoB, blaster, j[i].frame_blaster, 28, stars, 320, colorTrans, j[i].b_buffer);
			}
		}

		// ================= FASE 5: TRANSICIONES =================
		if (enemigos[0].y < 0 && enemigos[1].y < 0) {
			HAL_Delay(500); LCD_Clear(0x0000);
			HAL_UART_Transmit(&huart5, (uint8_t*)"0", 1, 10);
			LCD_Print("VICTORY", 120, 110, 2, 0x07E0, 0x0000); // Se indica que el jugador ya gano.
			HAL_Delay(2000);
			cinematica = 6; fondo1 = 0; fondo4 = 0; // Pasar al nivel 2
		}

		if (j[0].vivo == 0 && j[1].vivo == 0) {
			HAL_Delay(500); LCD_Clear(0x0000);
			HAL_UART_Transmit(&huart5, (uint8_t*)"0", 1, 10);
			LCD_Print("GAME OVER", 110, 110, 2, 0xF800, 0x0000); // se indica que le jugador perdio
			HAL_Delay(2000);
			cinematica = 4; // Regresar a selección de niveles luego de perder
			fondo = 0; fondo1 = 0; fondo2 = 0; fondo3 = 0; fondo4 = 0; fondo5 = 0; fondo6 = 0;
		}
		HAL_Delay(15); // Un poco más lento para que sea más fácil reaccionar a la pantalla.
	}




		//===========================================================================================
		// ====================================	NIVEL 2 =============================================
		//===========================================================================================
	else if (cinematica == 6) {
		if (fondo1 == 0) {
			fondos("nivel3.bin", 0, 0, 320, 240);
			HAL_UART_Transmit(&huart5, (uint8_t*)"4", 1, 10);
			fondo1 = 1;
			//inicialización de lo enemigos y de los jugadores
			enemigos[0].x = 40;
			enemigos[0].y = 30;
			enemigos[0].direccion = 1;
			enemigos[0].b_activo = 0; // Limpiar balas viejas

			// Enemigo 2
			enemigos[1].x = 200;
			enemigos[1].y = 50;
			enemigos[1].direccion = -1;
			enemigos[1].b_activo = 0;

			// Enemigo 3
			enemigos[2].x = 100;
			enemigos[2].y = 10;
			enemigos[2].direccion = 1;
			enemigos[2].b_activo = 0;

			// También es bueno resetear el estado de los jugadores por si uno murió en el nivel anterior
			j[0].vivo = 1;
			j[1].vivo = 1;
	}

		// ================= FASE 1: MOVIMIENTO, ANIMACIÓN Y DISPARO =================
		    for (int i = 0; i < 2; i++) {
		        if (!j[i].vivo) continue;

		        // Guardamos posiciones anteriores
		        j[i].oldX = j[i].posX;
		        j[i].oldY = j[i].posY;
		        j[i].oldBX = (int)j[i].blasterX;
		        j[i].oldBY = (int)j[i].blasterY;

		        PlayStationBuffer *ctrl = (i == 0) ? &control : &control_J2;

		        // --- 1. Movimiento de los jugadores---
		        j[i].frame_actual = 0;
		        if (ctrl->derecha)      { j[i].posX += velocidad; j[i].frame_actual = 2; }
		        else if (ctrl->izquierda) { j[i].posX -= velocidad; j[i].frame_actual = 1; }

		        if (ctrl->arriba) j[i].posY -= velocidad;
		        if (ctrl->abajo)  j[i].posY += velocidad;

		        // Límites de pantalla
		        if (j[i].posX < 0) j[i].posX = 0;
		        if (j[i].posX > (320 - j[i].ancho)) j[i].posX = 320 - j[i].ancho;
		        if (j[i].posY < 120) j[i].posY = 120;
		        if (j[i].posY > (240 - j[i].alto)) j[i].posY = 240 - j[i].alto;

		        // --- 2. BORRADO DE RASTRO DE LA NAVE ---
		        if (j[i].oldX != j[i].posX || j[i].oldY != j[i].posY || j[i].frame_actual != j[i].old_frame) {
		            while(!dma_libre);
		            for (int r = 0; r < j[i].alto; r++) {
		                for (int c = 0; c < j[i].ancho; c++) {
		                    uint32_t idx = (uint32_t)(j[i].oldY + r) * 320 + (j[i].oldX + c);
		                    j[i].buffer[r * j[i].ancho + c] = (stars[idx] << 8) | (stars[idx] >> 8);
		                }
		            }
		            LCD_Bitmap_DMA(j[i].oldX, j[i].oldY, j[i].ancho, j[i].alto, j[i].buffer);
		            j[i].old_frame = j[i].frame_actual;
		        }

		        // --- 3. LÓGICA DE DISPARO ---
		        if (ctrl->cuadrado && !j[i].blaster_activo) {
		            j[i].blaster_activo = 1;
		            j[i].blasterX = j[i].posX + (j[i].ancho / 2) - 3;
		            j[i].blasterY = j[i].posY - 12;
		            j[i].dist_blaster = 0;
		        }

		        if (j[i].blaster_activo) {
		            // Borrar rastro de la bala.
		            while(!dma_libre);
		            for (int r = 0; r < altoB; r++) {
		                for (int c = 0; c < anchoB; c++) {
		                    int py = j[i].oldBY + r;
		                    if(py < 0) py = 0; if(py > 239) py = 239;
		                    uint32_t idx = (uint32_t)py * 320 + (j[i].oldBX + c);
		                    j[i].b_buffer[r * anchoB + c] = (stars[idx] << 8) | (stars[idx] >> 8);
		                }
		            }
		            LCD_Bitmap_DMA(j[i].oldBX, j[i].oldBY, anchoB, altoB, j[i].b_buffer);

		            // Actualizar posición y frame de animación del láser
		            j[i].blasterY -= 8;
		            j[i].dist_blaster += 8;
		            int fb = (int)(j[i].dist_blaster / 20);
		            j[i].frame_blaster = (fb < 4) ? fb : 3;

		            // Desactivar si sale de pantalla o recorre su distancia máxima
		            if (j[i].blasterY < 10 || j[i].dist_blaster > 150) {
		                j[i].blaster_activo = 0;
		            }
		        }
		    }
	    // ================= FASE 2: PROCESAR ENEMIGOS =================
	    // Esto mueve naves y sus disparos verdes
	    for (int i = 0; i < 3; i++) {
	        ProcesarEnemigo(&enemigos[i], i, enemigo, blaster_verde, stars, colorTrans, i * 300);
	    }

	    // ================= FASE 3: COLISIONES CRÍTICAS =================
	    for (int i = 0; i < 2; i++) {
	        if (!j[i].vivo) continue;

	        for (int e = 0; e < 3; e++) {
	            // ---- JUGADOR GOLPEA A ENEMIGO ----
	            if (j[i].blaster_activo) {
	                if (colision((int)j[i].blasterX, (int)j[i].blasterY, anchoB, altoB, enemigos[e].x, enemigos[e].y, 16, 24)) {

	                    // 1. Borrar rastro del blaster
	                    while(!dma_libre);
	                    for (int r = 0; r < altoB; r++) {
	                        for (int c = 0; c < anchoB; c++) {
	                            uint32_t idx = (uint32_t)(j[i].oldBY + r) * 320 + (j[i].oldBX + c);
	                            j[i].b_buffer[r * anchoB + c] = (stars[idx] << 8) | (stars[idx] >> 8);
	                        }
	                    }
	                    LCD_Bitmap_DMA(j[i].oldBX, j[i].oldBY, anchoB, altoB, j[i].b_buffer);

	                    // 2. Borrar rastro del blaster del enemigo
	                    if (enemigos[e].b_activo) {
	                        while(!dma_libre);
	                        // Parametros de la bala del enemigo.
	                        for (int r = 0; r < 23; r++) {
	                            for (int c = 0; c < 8; c++) {
	                                uint32_t idx = (uint32_t)(enemigos[e].by + r) * 320 + (enemigos[e].bx + c);
	                                buffer_mezcla[r * 8 + c] = (stars[idx] << 8) | (stars[idx] >> 8);
	                            }
	                        }
	                        LCD_Bitmap_DMA(enemigos[e].bx, enemigos[e].by, 8, 23, buffer_mezcla);
	                    }

	                    j[i].blaster_activo = 0;
	                    enemigos[e].b_activo = 0;
	                    enemigos[e].y = -50;
	                    enemigos[e].x = rand() % 280;
	                }
	            }

	            // ---- BLASTER VERDE GOLPEA JUGADOR ----
	            if (enemigos[e].b_activo) {
	                if (colision(enemigos[e].bx, enemigos[e].by, 8, 23, j[i].posX, j[i].posY, j[i].ancho, j[i].alto)) {

	                    //Borrar bala verde en punto de impacto
	                    while(!dma_libre);
	                    for (int r = 0; r < 23; r++) {
	                        for (int c = 0; c < 8; c++) {
	                            uint32_t idx = (uint32_t)(enemigos[e].by + r) * 320 + (enemigos[e].bx + c);
	                            buffer_mezcla[r * 8 + c] = (stars[idx] << 8) | (stars[idx] >> 8);
	                        }
	                    }
	                    LCD_Bitmap_DMA(enemigos[e].bx, enemigos[e].by, 8, 23, buffer_mezcla);

	                    //Borrar cuerpo del Jugador
	                    while(!dma_libre);
	                    for (int r = 0; r < j[i].alto; r++) {
	                        for (int c = 0; c < j[i].ancho; c++) {
	                            uint32_t idx = (uint32_t)(j[i].posY + r) * 320 + (j[i].posX + c);
	                            j[i].buffer[r * j[i].ancho + c] = (stars[idx] << 8) | (stars[idx] >> 8);
	                        }
	                    }
	                    LCD_Bitmap_DMA(j[i].posX, j[i].posY, j[i].ancho, j[i].alto, j[i].buffer);

	                    j[i].vivo = 0;
	                    j[i].blaster_activo = 0;
	                    enemigos[e].b_activo = 0;

	                }
	            }
	        }
	    }

	    // ================= FASE 4: BORRADO DE RASTRO MOVIMIENTO =================
	    for (int i = 0; i < 2; i++) {
	        if (!j[i].vivo) continue;

	        // Borrar nave si se movió
	        if (j[i].oldX != j[i].posX || j[i].oldY != j[i].posY) {
	            while(!dma_libre);
	            for (int r = 0; r < j[i].alto; r++) {
	                for (int c = 0; c < j[i].ancho; c++) {
	                    uint32_t idx = (uint32_t)(j[i].oldY + r) * 320 + (j[i].oldX + c);
	                    j[i].buffer[r * j[i].ancho + c] = (stars[idx] << 8) | (stars[idx] >> 8);
	                }
	            }
	            LCD_Bitmap_DMA(j[i].oldX, j[i].oldY, j[i].ancho, j[i].alto, j[i].buffer);
	        }

	        // Borrar blaster en vuelo
	        if (j[i].blaster_activo) {
	            while(!dma_libre);
	            for (int r = 0; r < altoB; r++) {
	                for (int c = 0; c < anchoB; c++) {
	                    uint32_t idx = (uint32_t)(j[i].oldBY + r) * 320 + (j[i].oldBX + c);
	                    j[i].b_buffer[r * anchoB + c] = (stars[idx] << 8) | (stars[idx] >> 8);
	                }
	            }
	            LCD_Bitmap_DMA(j[i].oldBX, j[i].oldBY, anchoB, altoB, j[i].b_buffer);
	        }
	    }

	    // ================= FASE 5: DIBUJADO =================
	    for (int i = 0; i < 2; i++) {
	        if (!j[i].vivo) continue;

	        LCD_DibujarSpriteUniversal(j[i].posX, j[i].posY, j[i].ancho, j[i].alto,
	                                   j[i].sprite, j[i].frame_actual, (j[i].ancho * 3),
	                                   stars, 320, colorTrans, j[i].buffer);

	        if (j[i].blaster_activo) {
	        	// dibujar blaster con fondo del nivvel
	            LCD_DibujarSpriteUniversal((int)j[i].blasterX, (int)j[i].blasterY,
	                                       anchoB, altoB, blaster, j[i].frame_blaster,
	                                       28, stars, 320, colorTrans, j[i].b_buffer);
	        }
	    }
	    HAL_Delay(20);

	    // ================= FASE 6: VERIFICACIÓN DE VICTORIA O DERROTA =================

		// --- Comprobar si ambos jugadores han muerto ---
		if (j[0].vivo == 0 && j[1].vivo == 0) {
			HAL_Delay(100); // Pausa dramática
			LCD_Clear(0x0000);
			HAL_UART_Transmit(&huart5, (uint8_t*)"0", 1, 10);
			LCD_Print("GAME OVER", 100, 110, 2, 0xF800, 0x0000); // Imprimir si se pierde
			HAL_Delay(200);
			cinematica = 4; //Regresar el menu de selección
			// Reseteo de todas las variables de fondos, para inicializarlas nuevamente.
			fondo = 0; fondo1 = 0; fondo2 = 0; fondo3 = 0; fondo4 = 0; fondo5 = 0; fondo6 = 0;
			//Resetear las naves para el próximo intento
			j[0].vivo = 1; j[1].vivo = 1;
			j[0].posX = 80;  j[0].posY = 180;
			j[1].posX = 240; j[1].posY = 180;
		}

		//Comprobar si todos los enemigos fueron eliminados
		int enemigos_vivos = 0;
		for (int e = 0; e < 3; e++) {
			if (enemigos[e].y >= 0) {
				enemigos_vivos++;
			}
		}

		if (enemigos_vivos == 0) {
			HAL_Delay(100);
			LCD_Clear(0x0000);
			HAL_UART_Transmit(&huart5, (uint8_t*)"0", 1, 10);
			LCD_Print("VICTORY!", 80, 110, 2, 0x07E0, 0x0000);//Se imprime cuando se gana.
			HAL_Delay(200);
			cinematica = 7;
		}
	}







		//===========================================================================================
		// ====================================	NIVEL 3 =============================================
		//===========================================================================================
	else if (cinematica == 7){
			if (fondo5 == 0) {
			fondos("nivel3.bin", 0, 0, 320, 240); //Se imprime el fondo del nivel.
			HAL_UART_Transmit(&huart5, (uint8_t*)"7", 1, 10);
			            fondo5 = 1;
			            bossHP = 15; bossX = 135; bossY = 20; bossDir = 1;
			            for(int i=0; i<2; i++) {
			                j[i].vivo = 1; j[i].blaster_activo = 0;
			                j[i].posX = (i == 0) ? 80 : 240; j[i].posY = 180;
			                j[i].frame_actual = 0; j[i].old_frame = 0;
			            }
			            for(int e=0; e<3; e++) enemigos[e].b_activo = 0;
			        }

			        // ================= FASE 1: JUGADORES =================
			        for (int i = 0; i < 2; i++) {
			            if (!j[i].vivo) continue;
			            j[i].oldX = j[i].posX; j[i].oldY = j[i].posY;
			            j[i].oldBX = (int)j[i].blasterX; j[i].oldBY = (int)j[i].blasterY;
			            PlayStationBuffer *ctrl = (i == 0) ? &control : &control_J2;

			            j[i].frame_actual = 0;
			            if (ctrl->derecha)      { j[i].posX += velocidad; j[i].frame_actual = 2; }
			            else if (ctrl->izquierda) { j[i].posX -= velocidad; j[i].frame_actual = 1; }
			            if (ctrl->arriba) j[i].posY -= velocidad;
			            if (ctrl->abajo)  j[i].posY += velocidad;
			            // Logica para la creación de las naves y posicionamiento.
			            if (j[i].posX < 0) j[i].posX = 0;
			            if (j[i].posX > (320 - j[i].ancho)) j[i].posX = 320 - j[i].ancho;
			            if (j[i].posY < 115) j[i].posY = 115;
			            if (j[i].posY > 200) j[i].posY = 200;

			            // Borrado Nave
			            if (j[i].oldX != j[i].posX || j[i].oldY != j[i].posY || j[i].frame_actual != j[i].old_frame) {
			                while(!dma_libre);
			                for (int r = 0; r < j[i].alto; r++) {
			                    for (int c = 0; c < j[i].ancho; c++) {
			                        uint32_t idx = (uint32_t)(j[i].oldY + r) * 320 + (j[i].oldX + c);
			                        j[i].buffer[r * j[i].ancho + c] = (stars[idx] << 8) | (stars[idx] >> 8);
			                    }
			                }
			                LCD_Bitmap_DMA(j[i].oldX, j[i].oldY, j[i].ancho, j[i].alto, j[i].buffer);
			                j[i].old_frame = j[i].frame_actual;
			            }

			            // Disparo
			            if (ctrl->cuadrado && !j[i].blaster_activo) {
			                j[i].blaster_activo = 1;
			                j[i].blasterX = j[i].posX + (j[i].ancho / 2) - 4;
			                j[i].blasterY = j[i].posY - 12;
			                j[i].dist_blaster = 0;
			            }

			            if (j[i].blaster_activo) {
			                j[i].oldBX = (int)j[i].blasterX; j[i].oldBY = (int)j[i].blasterY;
			                j[i].blasterY -= 8;
			                j[i].dist_blaster += 8;

			                while(!dma_libre);
			                for (int r = 0; r < altoB; r++) {
			                    for (int c = 0; c < anchoB; c++) {
			                        int py = j[i].oldBY + r;
			                        if(py < 0) py = 0; if(py > 239) py = 239;
			                        uint32_t idx = (uint32_t)py * 320 + (j[i].oldBX + c);
			                        j[i].b_buffer[r * anchoB + c] = (stars[idx] << 8) | (stars[idx] >> 8);
			                    }
			                }
			                LCD_Bitmap_DMA(j[i].oldBX, j[i].oldBY, anchoB, altoB, j[i].b_buffer);

			                if (colision((int)j[i].blasterX, (int)j[i].blasterY, anchoB, altoB, bossX, bossY, 50, 50)) {
			                    bossHP--; j[i].blaster_activo = 0;
			                    HAL_UART_Transmit(&huart5, (uint8_t*)"2", 1, 10);
			                }
			                if (j[i].blasterY < 10 || j[i].dist_blaster > 180) j[i].blaster_activo = 0;
			            }
			        }

			        // ================= FASE 2: LÓGICA DEL BOSS =================
			        bossOldX = bossX;
			        int bossV = 3;
			        bossX += (bossDir * bossV);
			        if (bossX > 265 || bossX < 5) bossDir *= -1;

			        // Borrado de rastro del Boss
			        if (bossOldX != bossX) {
			            // Si va a la derecha, borra la franja izquierda. Si va a la izquierda, borra la derecha.
			            int bx_limpiar = (bossDir == 1) ? bossOldX : bossOldX + 50 - bossV;
			            while(!dma_libre);
			            for (int r = 0; r < 50; r++) {
			                for (int c = 0; c < bossV; c++) {
			                    uint32_t idx = (uint32_t)(bossY + r) * 320 + (bx_limpiar + c);
			                    b_mezcla_boss[r * bossV + c] = (stars[idx] << 8) | (stars[idx] >> 8);
			                }
			            }
			            LCD_Bitmap_DMA(bx_limpiar, bossY, bossV, 50, b_mezcla_boss);
			        }

			        // Disparos lluvia del boss
			        for (int e = 0; e < 3; e++) {
			            if (!enemigos[e].b_activo && rand() % 50 == 0) {
			                enemigos[e].bx = bossX + 10 + (rand() % 30);
			                enemigos[e].by = bossY + 45;
			                enemigos[e].b_activo = 1;
			                enemigos[e].dist_recorrida = 0;
			                HAL_UART_Transmit(&huart5, (uint8_t*)"6", 1, 10);
			            }
			            if (enemigos[e].b_activo) {
			                enemigos[e].oldBX = enemigos[e].bx; enemigos[e].oldBY = enemigos[e].by;
			                enemigos[e].by += 7; enemigos[e].dist_recorrida += 7;

			                while(!dma_libre);
			                for (int r = 0; r < 23; r++) {
			                    for (int c = 0; c < 8; c++) {
			                        int py = enemigos[e].oldBY + r;
			                        if(py > 239) py = 239;
			                        uint32_t idx = (uint32_t)py * 320 + (enemigos[e].oldBX + c);
			                        buffer_mezcla[r * 8 + c] = (stars[idx] << 8) | (stars[idx] >> 8);
			                    }
			                }
			                LCD_Bitmap_DMA(enemigos[e].oldBX, enemigos[e].oldBY, 8, 23, buffer_mezcla);

			                //Borrar rastro si va a morir
			                if (enemigos[e].by > 225) {
			                    // Dibujamos fondo una última vez antes de apagar la bala
			                    LCD_Bitmap_DMA(enemigos[e].bx, enemigos[e].by, 8, 23, buffer_mezcla);
			                    enemigos[e].b_activo = 0;
			                } else {
			                    LCD_DibujarSpriteUniversal(enemigos[e].bx, enemigos[e].by, 8, 23, blaster_verde, (enemigos[e].dist_recorrida/20)%4, 32, stars, 320, colorTrans, buffer_mezcla);
			                }

			                // Colisión Bala vs Jugador
			                for(int i=0; i<2; i++) {
			                    if (j[i].vivo && colision(enemigos[e].bx, enemigos[e].by, 8, 23, j[i].posX, j[i].posY, j[i].ancho, j[i].alto)) {
			                        while(!dma_libre); // Borrar jugador al morir
			                        for (int r = 0; r < j[i].alto; r++) {
			                            for (int c = 0; c < j[i].ancho; c++) {
			                                uint32_t idx = (uint32_t)(j[i].posY + r) * 320 + (j[i].posX + c);
			                                j[i].buffer[r * j[i].ancho + c] = (stars[idx] << 8) | (stars[idx] >> 8);
			                            }
			                        }
			                        LCD_Bitmap_DMA(j[i].posX, j[i].posY, j[i].ancho, j[i].alto, j[i].buffer);
			                        j[i].vivo = 0;
			                    }
			                }
			            }
			        }

			        // ================= FASE 3: DIBUJADO FINAL =================
			        //Dibujar el boss con fondo transparente.
			        LCD_DibujarSpriteUniversal(bossX, bossY, 50, 50, deathS, 0, 50, stars, 320, 0x0000, b_mezcla_boss);

			        for (int i = 0; i < 2; i++) {
			            if (!j[i].vivo) continue;
			            LCD_DibujarSpriteUniversal(j[i].posX, j[i].posY, j[i].ancho, j[i].alto, j[i].sprite, j[i].frame_actual, (j[i].ancho * 3), stars, 320, colorTrans, j[i].buffer);
			            if (j[i].blaster_activo) {
			                LCD_DibujarSpriteUniversal((int)j[i].blasterX, (int)j[i].blasterY, anchoB, altoB, blaster, j[i].frame_blaster, 28, stars, 320, colorTrans, j[i].b_buffer);
			            }
			        }

			        // Victoria y Derrota
			        if (bossHP <= 0) {
			            HAL_Delay(500); LCD_Clear(0x0000);
			            LCD_Print("VICTORY", 120, 110, 2, 0x07E0, 0x0000); // Se imprime al ganar
			            HAL_UART_Transmit(&huart5, (uint8_t*)"0", 1, 10);
			            HAL_Delay(150)
			            cinematica = 8; fondo6 = 0; fondo5 = 0; // se pasa al siguinete nivel
			        }
			        if (j[0].vivo == 0 && j[1].vivo == 0) {
			            HAL_Delay(500); LCD_Clear(0x0000);
			            LCD_Print("GAME OVER", 110, 110, 2, 0xF800, 0x0000); // Se imprime al perder
			            HAL_UART_Transmit(&huart5, (uint8_t*)"0", 1, 10);
			            HAL_Delay(150);
			            cinematica = 4; fondo = 0; fondo5 = 0; fondo3 = 0; // Se regresa a la selección de nivle
			        }
			        HAL_Delay(15);
			    }

		//===========================================================================================
		// ====================================	NIVEL 4 =============================================
		//===========================================================================================
  else if (cinematica == 8) {
		if (fondo6 == 0) {
			fondos("nivel3.bin", 0, 0, 320, 240);
			HAL_UART_Transmit(&huart5, (uint8_t*)"7", 1, 10);
			fondo6 = 1;
			balas_lanzadas = 0;
			balas_por_sobrevivir = 40; //Cantidad de balas por sobrevivir.
			for(int i=0; i<2; i++) {
				j[i].vivo = 1; j[i].blaster_activo = 0;
				j[i].posX = (i == 0) ? 80 : 240;
				j[i].posY = 40;
				j[i].frame_actual = 0;
				j[i].old_frame = 0;
			}
			for(int e=0; e<3; e++) {
				enemigos[e].y = -100;
				enemigos[e].b_activo = 0;
			}
		}

		// ================= FASE 1: JUGADORES =================
		for (int i = 0; i < 2; i++) {
			if (!j[i].vivo) continue;
			j[i].oldX = j[i].posX; j[i].oldY = j[i].posY;
			PlayStationBuffer *ctrl = (i == 0) ? &control : &control_J2;

			j[i].frame_actual = 0;
			if (ctrl->derecha)      { j[i].posX += velocidad; j[i].frame_actual = 2; }
			else if (ctrl->izquierda) { j[i].posX -= velocidad; j[i].frame_actual = 1; }

			if (ctrl->arriba) j[i].posY -= velocidad;
			if (ctrl->abajo)  j[i].posY += velocidad;

			// Limites para los jugadores
			if (j[i].posX < 0) j[i].posX = 0;
			if (j[i].posX > (320 - j[i].ancho)) j[i].posX = 320 - j[i].ancho;
			if (j[i].posY < 10) j[i].posY = 10;
			if (j[i].posY > 100) j[i].posY = 100; //No se pasan de la mitad

			// Borrado de rastro
			if (j[i].oldX != j[i].posX || j[i].oldY != j[i].posY || j[i].frame_actual != j[i].old_frame) {
				while(!dma_libre);
				for (int r = 0; r < j[i].alto; r++) {
					for (int c = 0; c < j[i].ancho; c++) {
						uint32_t idx = (uint32_t)(j[i].oldY + r) * 320 + (j[i].oldX + c);
						j[i].buffer[r * j[i].ancho + c] = (stars[idx] << 8) | (stars[idx] >> 8);
					}
				}
				LCD_Bitmap_DMA(j[i].oldX, j[i].oldY, j[i].ancho, j[i].alto, j[i].buffer);
				j[i].old_frame = j[i].frame_actual;
			}
		}

		// ================= FASE 2: LLUVIA INVERSA =================
		for (int e = 0; e < 3; e++) {
			if (!enemigos[e].b_activo && balas_lanzadas < balas_por_sobrevivir) {
				if (rand() % 40 == 0) {
					enemigos[e].bx = rand() % 300;
					enemigos[e].by = 240;
					enemigos[e].b_activo = 1;
					enemigos[e].dist_recorrida = 0;
					balas_lanzadas++;
				}
			}

			if (enemigos[e].b_activo) {
				enemigos[e].oldBX = enemigos[e].bx;
				enemigos[e].oldBY = enemigos[e].by;

				enemigos[e].by -= 7;
				enemigos[e].dist_recorrida += 7;

				//Borrado de rastro del blaster
				while(!dma_libre);
				for (int r = 0; r < 23; r++) {
					for (int c = 0; c < 8; c++) {
						int py = enemigos[e].oldBY + r;
						if (py < 0) py = 0; if (py > 239) py = 239;
						uint32_t idx = (uint32_t)py * 320 + (enemigos[e].oldBX + c);
						buffer_mezcla[r * 8 + c] = (stars[idx] << 8) | (stars[idx] >> 8);
					}
				}
				LCD_Bitmap_DMA(enemigos[e].oldBX, enemigos[e].oldBY, 8, 23, buffer_mezcla);

				//Dibujar frame animado
				int frame_b = (enemigos[e].dist_recorrida / 15) % 4;
				LCD_DibujarSpriteUniversal(enemigos[e].bx, enemigos[e].by, 8, 23,
										   blaster_verde, frame_b, 32, stars, 320,
										   colorTrans, buffer_mezcla);

				//Desactivar si sale por arriba
				if (enemigos[e].by < -23) enemigos[e].b_activo = 0;

				//Colisión
				for (int i = 0; i < 2; i++) {
					if (j[i].vivo && colision(enemigos[e].bx, enemigos[e].by, 8, 23, j[i].posX, j[i].posY, j[i].ancho, j[i].alto)) {
						// Borrar jugador al morir
						while(!dma_libre);
						for (int r = 0; r < j[i].alto; r++) {
							for (int c = 0; c < j[i].ancho; c++) {
								uint32_t idx = (uint32_t)(j[i].posY + r) * 320 + (j[i].posX + c);
								j[i].buffer[r * j[i].ancho + c] = (stars[idx] << 8) | (stars[idx] >> 8);
							}
						}
						LCD_Bitmap_DMA(j[i].posX, j[i].posY, j[i].ancho, j[i].alto, j[i].buffer);
						j[i].vivo = 0;
					}
				}
			}
		}

		// ================= FASE 3: DIBUJADO JUGADORES =================
		for (int i = 0; i < 2; i++) {
			if (j[i].vivo) {
				LCD_DibujarSpriteUniversal(j[i].posX, j[i].posY, j[i].ancho, j[i].alto,
										   j[i].sprite, j[i].frame_actual, (j[i].ancho * 3),
										   stars, 320, colorTrans, j[i].buffer);
			}
		}

		// ================= FASE 4: TRANSICIONES =================
		if (balas_lanzadas >= balas_por_sobrevivir && !enemigos[0].b_activo && !enemigos[1].b_activo && !enemigos[2].b_activo) {
			HAL_Delay(500); LCD_Clear(0x0000);
			HAL_UART_Transmit(&huart5, (uint8_t*)"0", 1, 10);
			fondos("win.bin", 0, 0, 320, 240); // Imprimir fondo del ganador
			HAL_Delay(150);
			cinematica = 0; fondo = 0; fondo6 = 0; fondo3 = 0; fondo1 = 0; fondo2 = 0; fondo4 = 0; fondo5 = 0; // Se resetea todo para volver a jugar
			//Reseteo de las selección de jugadores.
			personaje_seleccionado_J1 = 0;
			personaje_seleccionado_J2 = 0;
		}

		if (j[0].vivo == 0 && j[1].vivo == 0) {
			HAL_Delay(500); LCD_Clear(0x0000);
			HAL_UART_Transmit(&huart5, (uint8_t*)"0", 1, 10);
			LCD_Print("GAME OVER", 110, 110, 2, 0xF800, 0x0000); // Se imprime al perder
			HAL_Delay(150);
			cinematica = 4; fondo = 0; fondo6 = 0; fondo3 = 0; fondo1 = 0; fondo2 = 0; fondo4 = 0; fondo5 = 0; // Se regresa al menu de selección de nivel
		}
		HAL_Delay(20);
	}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 8399;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 99;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief UART5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART5_Init(void)
{

  /* USER CODE BEGIN UART5_Init 0 */

  /* USER CODE END UART5_Init 0 */

  /* USER CODE BEGIN UART5_Init 1 */

  /* USER CODE END UART5_Init 1 */
  huart5.Instance = UART5;
  huart5.Init.BaudRate = 115200;
  huart5.Init.WordLength = UART_WORDLENGTH_8B;
  huart5.Init.StopBits = UART_STOPBITS_1;
  huart5.Init.Parity = UART_PARITY_NONE;
  huart5.Init.Mode = UART_MODE_TX_RX;
  huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart5.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART5_Init 2 */

  /* USER CODE END UART5_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 9600;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin|SD_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_RESET_Pin */
  GPIO_InitStruct.Pin = LCD_RESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(LCD_RESET_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_DC_Pin */
  GPIO_InitStruct.Pin = LCD_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(LCD_DC_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_CS_Pin SD_CS_Pin */
  GPIO_InitStruct.Pin = LCD_CS_Pin|SD_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI1) {
        dma_libre = 1;
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    // --- JUGADOR 1 (USART1) ---
    if (huart->Instance == USART1) {
        if (rx_data == '\n' || rx_data == '\r') {
            rx_buffer[rx_index] = '\0';
            if (sscanf(rx_buffer, "%d,%d,%d,%d,%d,%d", // Intentamos leer 6 enteros. sscanf devuelve cuántos leyó con éxito.
                       &control.arriba, &control.derecha, &control.abajo,
                       &control.izquierda, &control.cuadrado, &control.r1) == 6) {
                data_ready = 1;
            }
            rx_index = 0;
        } else {
            if (rx_index < 49) {
                rx_buffer[rx_index++] = rx_data;
            }
        }
        HAL_UART_Receive_IT(&huart1, &rx_data, 1);
    }

    // --- JUGADOR 2 (USART3) ---
    else if (huart->Instance == USART3) {
        if (rx_data_J2 == '\n' || rx_data_J2 == '\r') { // Usar variable J2
            rx_buffer_J2[rx_index_J2] = '\0';
            if (sscanf(rx_buffer_J2, "%d,%d,%d,%d,%d,%d",
                       &control_J2.arriba, &control_J2.derecha, &control_J2.abajo,
                       &control_J2.izquierda, &control_J2.cuadrado, &control_J2.r1) == 6) {
                data_ready_J2 = 1;
            }
            rx_index_J2 = 0;
        } else {
            if (rx_index_J2 < 49) {
                rx_buffer_J2[rx_index_J2++] = rx_data_J2;
            }
        }
        HAL_UART_Receive_IT(&huart3, &rx_data_J2, 1);
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        tiempo_spawn += 10; // Sumamos 10ms cada vez que entra aquí
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
