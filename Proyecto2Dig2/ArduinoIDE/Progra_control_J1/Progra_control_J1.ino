#include <Bluepad32.h>

// Solo un control
ControllerPtr myController = nullptr;

// UART2
HardwareSerial MySerial(2);

// Control de frecuencia
uint32_t lastSend = 0;
const int SEND_INTERVAL = 20;

// Deadzone
const int DEADZONE = 200;

// Estado anterior
int lastState[6] = {-1, -1, -1, -1, -1, -1};

// ==============================
// CALLBACKS
// ==============================

void onConnectedController(ControllerPtr ctl) {
  if (myController != nullptr) return;

  Serial.println("Control conectado");
  myController = ctl;
}

void onDisconnectedController(ControllerPtr ctl) {
  if (myController == ctl) {
    Serial.println("Control desconectado");
    myController = nullptr;
  }
}

// ==============================
// FUNCIONES
// ==============================

int applyDeadzone(int value) {
  if (abs(value) < DEADZONE)
    return 0;
  return value;
}

void processGamepad(ControllerPtr ctl) {
  if (!ctl || !ctl->isConnected())
    return;

  if (millis() - lastSend < SEND_INTERVAL)
    return;

  lastSend = millis();

  uint16_t buttons = ctl->buttons();

  int lx = applyDeadzone(ctl->axisX());
  int ly = applyDeadzone(ctl->axisY());

  int current[6];

  current[0] = (ly < -DEADZONE) ? 1 : 0; // arriba
  current[1] = (lx > DEADZONE)  ? 1 : 0; // derecha
  current[2] = (ly > DEADZONE) ? 1 : 0; // abajo
  current[3] = (lx < -DEADZONE)  ? 1 : 0; // izquierda
  current[4] = (buttons & 0x0004) ? 1 : 0; // cuadrado
  current[5] = (buttons & 0x0020) ? 1 : 0; // R1

  bool changed = false;

  for (int i = 0; i < 6; i++) {
    if (current[i] != lastState[i]) {
      changed = true;
      break;
    }
  }

  if (changed) {
    // UART2 (al otro micro)
    MySerial.printf("%d,%d,%d,%d,%d,%d\n",
                    current[0], current[1], current[2],
                    current[3], current[4], current[5]);

    // Monitor serial (USB)
    Serial.printf("%d,%d,%d,%d,%d,%d\n",
                  current[0], current[1], current[2],
                  current[3], current[4], current[5]);

    for (int i = 0; i < 6; i++) {
      lastState[i] = current[i];
    }
  }
}

// ==============================
// SETUP
// ==============================

void setup() {
  Serial.begin(115200);

  // UART2 → RX=16, TX=17
  MySerial.begin(115200, SERIAL_8N1, 16, 17);

  Serial.println("START");

  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.enableVirtualDevice(false);

  // Úsalo SOLO si quieres forzar emparejamiento:
  BP32.forgetBluetoothKeys();
}

// ==============================
// LOOP
// ==============================

void loop() {
  bool dataUpdated = BP32.update();

  if (dataUpdated && myController && myController->hasData()) {
    if (myController->isGamepad()) {
      processGamepad(myController);
    }
  }

  vTaskDelay(1);
}