#include <stdio.h>
#include <string.h>

void handleHelp(char *);
void sendPing(char*);
void retrievePacket(char*);
void retrievePacketSize(char*);

void evalCmd(char *, char *);
void handleCommands(char *);

char deviceName[32] = {};
uint32_t pingCounter = 0;

int cmdCount = 0;
struct myCommand {
  void (*ptr)(char *); // Function pointer
  char name[12];
  char help[48];
};

myCommand cmds[] = {
  {handleHelp, "help", "Shows this help."},
  {sendPing, "ping", "Sends a preformatted PING."},
  {retrievePacketSize, "any", "Retrieves the pending packet's size, if any."},
  {retrievePacket, "msg", "Retrieves the pending packet."},
};

void evalCmd(char *str, char *fullString) {
  char strq[12];
  for (int i = 0; i < cmdCount; i++) {
    sprintf(strq, "%s?", cmds[i].name);
    if (strcmp(str, cmds[i].name) == 0 || strcmp(strq, str) == 0) {
      cmds[i].ptr(fullString);
      return;
    }
  }
}

void handleCommands(char *str1) {
  char kwd[32];
  int i = sscanf(str1, "/%s", kwd);
  if (i > 0) evalCmd(kwd, str1);
  else handleHelp("");
}

void retrievePacket(char *param) {
  // Do we have a packet available?
  msgLen = packetSize;
  memcpy(msg, currentPacket, msgLen);
  deletePacket = true;
  return;
}

void retrievePacketSize(char *param) {
  // Do we have a packet available?
  Serial.println("Requested packet size");
  msgLen = 3;
  msg[0] = packetSize;
  msg[1] = (uint8_t)(LoRa.packetSnr() + 100);
  msg[2] = (uint8_t)(LoRa.packetRssi() * -1);
  deletePacket = false;
  return;
}

void handleHelp(char *param) {
  sprintf(msg, "Available commands: %d\n", cmdCount);
  Serial.print(msg);
  for (int i = 0; i < cmdCount; i++) {
    sprintf(msg, " . /%s: %s", cmds[i].name, cmds[i].help);
    Serial.println(msg);
  }
}

void sendPing(char* param) {
  char UUID[4];
  if (randomIndex > 252) stockUpRandom();
  sprintf(
    msg,
    "{\"from\":\"%s\",\"UUID\":\"%02x%02x%02x%02x\", \"cmd\":\"ping\", \"count\":%d}",
    deviceName, UUID[randomIndex], UUID[randomIndex + 1], UUID[randomIndex + 2], UUID[randomIndex + 3], pingCounter++);
  randomIndex += 4;
  sendPacket(msg);
}
