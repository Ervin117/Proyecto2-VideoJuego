#include <Bluepad32.h>
#include <esp_pm.h>

// Instancia del control
ControllerPtr myController = nullptr;

// Configuración de UART2 
// Pin 16 es RX, Pin 17 es TX
HardwareSerial MySerial(2);

// Control de tiempos
uint32_t lastSend = 0;
const int SEND_INTERVAL = 20; // Enviar cada 20ms

// Zona muerta para los Joysticks
const int DEADZONE = 200;

// Estado anterior para enviar solo si hay cambios
int lastState[6] = {-1, -1, -1, -1, -1, -1};

// ================================================================
// CALLBACKS DE CONEXIÓN
// ================================================================

void onConnectedController(ControllerPtr ctl) {
    if (myController != nullptr) return;

    Serial.println("¡Control conectado!");
    
    // Feedback: Vibrar y poner LED en azul
    ctl->setRumble(0xc0, 0x40);
    ctl->setColorLED(255, 0, 0); 
    
    myController = ctl;
}

void onDisconnectedController(ControllerPtr ctl) {
    if (myController == ctl) {
        Serial.println("Control desconectado.");
        myController = nullptr;
    }
}

// ================================================================
// PROCESAMIENTO DE DATOS
// ================================================================

int applyDeadzone(int value) {
    if (abs(value) < DEADZONE) return 0;
    return value;
}

void processGamepad(ControllerPtr ctl) {
    // Si no hay datos nuevos o no ha pasado el intervalo, salimos
    if (millis() - lastSend < SEND_INTERVAL) return;
    lastSend = millis();

    // Obtener estado de botones
    uint16_t buttons = ctl->buttons();

    // Obtener ejes y aplicar zona muerta
    int lx = applyDeadzone(ctl->axisX());
    int ly = applyDeadzone(ctl->axisY());

    int current[6];

    // Mapeo binario (0 o 1)
    current[0] = (ly < -DEADZONE) ? 1 : 0;      // Arriba
    current[1] = (lx > DEADZONE)  ? 1 : 0;      // Derecha
    current[2] = (ly > DEADZONE)  ? 1 : 0;      // Abajo
    current[3] = (lx < -DEADZONE) ? 1 : 0;      // Izquierda
    
    // Máscaras de bits para Bluepad32:
    // Cuadrado = 0x0004 | R1 = 0x0020
    current[4] = (buttons & 0x0004) ? 1 : 0;    // Cuadrado
    current[5] = (buttons & 0x0020) ? 1 : 0;    // R1

    // Verificar si algo cambió respecto al estado anterior
    bool changed = false;
    for (int i = 0; i < 6; i++) {
        if (current[i] != lastState[i]) {
            changed = true;
            break;
        }
    }

    if (changed) {
        // Formato: Arriba,Derecha,Abajo,Izquierda,Cuadrado,R1
        String data = String(current[0]) + "," + 
                      String(current[1]) + "," + 
                      String(current[2]) + "," + 
                      String(current[3]) + "," + 
                      String(current[4]) + "," + 
                      String(current[5]);

        // Enviar por UART2 al otro micro
        MySerial.println(data);
        MySerial.flush();

        // Debug por USB
        Serial.print("Enviado: ");
        Serial.println(data);
        Serial.flush();

        // Actualizar último estado
        for (int i = 0; i < 6; i++) {
            lastState[i] = current[i];
        }
    }
}

// ================================================================
// CONFIGURACIÓN INICIAL
// ================================================================

void setup() {

    setCpuFrequencyMhz(240); 

    BP32.setup(&onConnectedController, &onDisconnectedController);

    // Monitor Serial USB
    Serial.begin(9600);

    // UART2: Baudios, Config, RX, TX
    MySerial.begin(9600, SERIAL_8N1, 16, 17);

    Serial.println("Buscando controles...");

    // Inicializar Bluepad32
    BP32.setup(&onConnectedController, &onDisconnectedController);
    
    // Desactivar dispositivos virtuales (ratones/teclados virtuales)
    BP32.enableVirtualDevice(false);

    // NOTA: Si un control no conecta, descomenta la siguiente línea una vez,
    // sube el código, ejecútalo, y luego vuelve a comentarla.

    //BP32.forgetBluetoothKeys();
}

// ================================================================
// BUCLE PRINCIPAL
// ================================================================

void loop() {
    // Actualiza el stack de Bluetooth. Devuelve true si hay datos nuevos.
    bool dataUpdated = BP32.update();

    if (myController && myController->isConnected() && myController->hasData()) {
        if (myController->isGamepad()) {
            processGamepad(myController);
        }
    }

    // Pequeña pausa para estabilidad del RTOS (Sistema operativo del ESP32)
    vTaskDelay(5);
}