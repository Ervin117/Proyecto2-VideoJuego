#include <Bluepad32.h>

ControllerPtr myControllers[BP32_MAX_GAMEPADS];

// Control de frecuencia de envío (IMPORTANTE)
uint32_t lastSend = 0;
const int SEND_INTERVAL = 20; // ms → 50 Hz

void onConnectedController(ControllerPtr ctl) {
  Serial.printf("CALLBACK: Controller connected, idx=%d model=%s\n",
                ctl->index(), ctl->getModelName().c_str());
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == nullptr) {
      myControllers[i] = ctl;
      break;
    }
  }
}

void onDisconnectedController(ControllerPtr ctl) {
  Serial.println("CALLBACK: Controller disconnected");
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == ctl) {
      myControllers[i] = nullptr;
      break;
    }
  }
}

void processGamepad(ControllerPtr ctl) {
  // Limitar frecuencia de envío
  if (millis() - lastSend < SEND_INTERVAL) return;
  lastSend = millis();

  uint16_t buttons = ctl->buttons();

  // Joysticks
  int lx = ctl->axisX();
  int ly = ctl->axisY();
  int rx = ctl->axisRX();
  int ry = ctl->axisRY();

  // Botones (1 o 0)
  int square = (buttons & 0x0004) ? 1 : 0;
  int r1     = (buttons & 0x0020) ? 1 : 0;  // 

  // Envío CSV puro por UART
  Serial.printf("%d,%d,%d,%d,%d,%d\n", lx, ly, rx, ry, square, r1);
}

void processControllers() {
  for (auto myController : myControllers) {
    if (myController && myController->isConnected() && myController->hasData()) {
      if (myController->isGamepad())
        processGamepad(myController);
      else
        Serial.println("Unsupported controller");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("START");

  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.enableVirtualDevice(false);
  BP32.forgetBluetoothKeys();

  //Serial.println("Setup done.");
}

void loop() {
  bool dataUpdated = BP32.update();
  if (dataUpdated)
    processControllers();

  vTaskDelay(1);
}