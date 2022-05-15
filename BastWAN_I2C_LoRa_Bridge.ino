#include <SPI.h>
#include <LoRa.h>
#include <LoRandom.h>
#include "aes.c"
#include "helper.h"
#include "Commands.h"
#include <Wire.h>

#define RFM_TCXO (40u)

void setup() {
  time_t timeout = millis();
  while (!Serial) {
    // on nRF52840, Serial is not available right away.
    // make the MCU wait a little
    if ((millis() - timeout) < 5000) {
      delay(100);
    } else {
      break;
    }
  }
  uint8_t x = 5;
  while (x > 0) {
    Serial.print(" ");
    Serial.print(x--);
    Serial.print(" ");
    delay(500);
  } // Just for show
  Serial.println("0!");
  SerialUSB.println("SerialUSB started.");
  Wire.begin(0x18);
  delay(1000);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  SerialUSB.println("Wire started.");
  SerialUSB.println("\n\nLoRa Bridge by BastWAN, RAKwireless and Kongduino");
  Serial.println("------------------------------------------------------");
  pinMode(RFM_TCXO, OUTPUT);
  pinMode(RFM_SWITCH, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  //pinMode(PIN_PA28, OUTPUT);
  //digitalWrite(RFM_TCXO, LOW);
  //digitalWrite(PIN_PA28, HIGH);
  LoRa.setPins(SS, RFM_RST, RFM_DIO0);
  if (!LoRa.begin(868.125E6)) {
    SerialUSB.println("Starting LoRa failed!\nNow that's disappointing...");
    while (1);
  }
  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(125e3);
  LoRa.setCodingRate4(5);
  LoRa.setPreambleLength(8);
  LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);
  digitalWrite(RFM_SWITCH, HIGH);
  LoRa.writeRegister(REG_LNA, 0x23); // TURN ON LNA FOR RECEIVE
  if (!needEncryption) memset(SecretKey, 0, 32);
  digitalWrite(RFM_SWITCH, LOW);
  digitalWrite(RFM_TCXO, HIGH);
  LoRa.receive();
  cmdCount = sizeof(cmds) / sizeof(myCommand);
  handleHelp("");
}

void loop() {
  int incoming = LoRa.parsePacket();
  if (incoming) {
    SerialUSB.print("Received packet:\n");
    memset(currentPacket, 0, 256);
    packetSize = 0;
    while (LoRa.available()) {
      char c = (char)LoRa.read();
      delay(10);
      currentPacket[packetSize++] = c;
    }
    if (needEncryption) {
      Serial.println("Decrypting [" + String(packetSize) + "]...");
      hexDump((uint8_t *)currentPacket, packetSize);
      SerialUSB.println("With key:");
      hexDump((uint8_t *)SecretKey, 32);
      decryptECB((uint8_t*)currentPacket, packetSize);
      hexDump((uint8_t *)encBuf, packetSize / 2);
    } else {
      SerialUSB.print(currentPacket);
      SerialUSB.print("RSSI: ");
      SerialUSB.println(LoRa.packetRssi());
    }
    // digitalWrite(RFM_SWITCH, LOW);
    // digitalWrite(RFM_TCXO, HIGH);
    // LoRa.receive();
  }
  if (SerialUSB.available()) {
    memset(incomingBuff, 0, 256);
    int ix = 0;
    while (SerialUSB.available()) {
      char c = SerialUSB.read();
      delay(10);
      incomingBuff[ix++] = c;
    }
    SerialUSB.println("Incoming from USB:");
    SerialUSB.println(incomingBuff);
    handleCommands(incomingBuff);
  }
}

void receiveEvent(int bytes) {
  if (!Wire.available()) return;
  uint16_t ix = 0;
  memset(incomingBuff, 0, 256);
  while (ix < bytes) incomingBuff[ix++] = Wire.read();
  SerialUSB.println("Incoming from Wire:");
  SerialUSB.println(incomingBuff);
  handleCommands(incomingBuff);

  if (strcmp(incomingBuff, "msg?") == 0) {
    // Do we have a packet available?
    msgLen = packetSize;
    memcpy(msg, currentPacket, msgLen);
    deletePacket = true;
    return;
  }
}

void requestEvent() {
  Serial.print("Request from Master. Sending:\n");
  hexDump((uint8_t *)msg, msgLen);
  Wire.write(msg, msgLen);
  msgLen = 0;
  if (deletePacket) {
    memset(currentPacket, 0, 256);
    packetSize = 0;
  }
  memset(msg, 0, 256);
}
