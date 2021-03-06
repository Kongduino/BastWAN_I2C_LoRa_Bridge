#define CBC 0
#define CTR 1
#define ECB 2
#define REG_OCP 0x0B
#define REG_PA_CONFIG 0x09
#define REG_LNA 0x0c
#define REG_OP_MODE 0x01
#define REG_MODEM_CONFIG_1 0x1d
#define REG_MODEM_CONFIG_2 0x1e
#define REG_MODEM_CONFIG_3 0x26
#define REG_PA_DAC 0x4D
#define PA_DAC_HIGH 0x87

#define RFM_TCXO (40u)
#define RFM_SWITCH (41u)

uint8_t myMode = ECB;
bool needEncryption = false;
uint8_t SecretKey[33] = "YELLOW SUBMARINEENIRAMBUS WOLLEY";
uint8_t encBuf[128], hexBuf[256];
uint8_t randomStock[256];
uint8_t randomIndex = 0;
char incomingBuff[256] = {0};
char msg[256] = {0};
uint8_t msgLen = 0;
char currentPacket[256] = {0};
uint8_t packetSize = 0;
bool deletePacket = false;

void hexDump(uint8_t *, uint16_t);
uint16_t encryptECB(uint8_t*);
void decryptECB(uint8_t*, uint8_t);
void array2hex(uint8_t *, size_t, uint8_t *, uint8_t);
void hex2array(uint8_t *, uint8_t *, size_t);
void sendPacket(char *);
void setPWD(char *);
void stockUpRandom();

void writeRegister(uint8_t reg, uint8_t value) {
  LoRa.writeRegister(reg, value);
}
uint8_t readRegister(uint8_t reg) {
  return LoRa.readRegister(reg);
}

void hex2array(uint8_t *src, uint8_t *dst, size_t sLen) {
  size_t i, n = 0;
  for (i = 0; i < sLen; i += 2) {
    uint8_t x, c;
    c = src[i];
    if (c != '-') {
      if (c > 0x39) c -= 55;
      else c -= 0x30;
      x = c << 4;
      c = src[i + 1];
      if (c > 0x39) c -= 55;
      else c -= 0x30;
      dst[n++] = (x + c);
    }
  }
}

void array2hex(uint8_t *inBuf, size_t sLen, uint8_t *outBuf, uint8_t dashFreq = 0) {
  size_t i, len, n = 0;
  const char * hex = "0123456789ABCDEF";
  for (i = 0; i < sLen; ++i) {
    outBuf[n++] = hex[(inBuf[i] >> 4) & 0xF];
    outBuf[n++] = hex[inBuf[i] & 0xF];
    if (dashFreq > 0 && i != sLen - 1) {
      if ((i + 1) % dashFreq == 0) outBuf[n++] = '-';
    }
  }
  outBuf[n++] = 0;
}

void decryptECB(uint8_t* myBuf, uint8_t olen) {
  SerialUSB.println(" . Decrypting:");
  hexDump(myBuf, olen);
  SerialUSB.println("  - Dehexing myBuf to encBuf:");
  hex2array(myBuf, encBuf, olen);
  uint8_t len = olen / 2;
  hexDump(encBuf, len);
  SerialUSB.println("  - Decrypting encBuf:");
  struct AES_ctx ctx;
  AES_init_ctx(&ctx, SecretKey);
  uint8_t rounds = len / 16, steps = 0;
  for (uint8_t ix = 0; ix < rounds; ix++) {
    //void AES_ECB_encrypt(const struct AES_ctx* ctx, uint8_t* buf);
    AES_ECB_decrypt(&ctx, (uint8_t*)encBuf + steps);
    steps += 16;
    // encrypts in place, 16 bytes at a time
  }
  hexDump(encBuf, len);
}

uint16_t encryptECB(uint8_t* myBuf) {
  uint8_t len = 0;
  uint16_t olen;
  struct AES_ctx ctx;
  // first ascertain length
  while (myBuf[len] > 31) len += 1;
  // prepare the buffer
  myBuf[len++] = 0;
  olen = len;
  if (olen != 16) {
    if (olen % 16 > 0) {
      if (olen < 16) olen = 16;
      else olen += 16 - (olen % 16);
    }
  }
  memset(encBuf, (olen - len), olen);
  memcpy(encBuf, myBuf, len);
  AES_init_ctx(&ctx, (const uint8_t*)SecretKey);
  uint8_t rounds = olen / 16, steps = 0;
  for (uint8_t ix = 0; ix < rounds; ix++) {
    AES_ECB_encrypt(&ctx, encBuf + steps);
    // void AES_ECB_decrypt(&ctx, encBuf + steps);
    steps += 16;
    // encrypts in place, 16 bytes at a time
  }
  array2hex(encBuf, olen, hexBuf);
  SerialUSB.println("encBuf:");
  hexDump(encBuf, olen);
  SerialUSB.println("hexBuf:");
  hexDump(hexBuf, olen * 2);
  return (olen * 2);
}


void setPWD(char *buff) {
  // buff can be 32 or 64 bytes:
  // 32 bytes = plain text
  // 64 bytes = hex-encoded
  uint8_t len = strlen(buff), i;
  for (i = 0; i < len; i++) {
    if (buff[i] < 32) {
      buff[i] = 0;
      i = len + 1;
    }
  }
  len = strlen(buff);
  SerialUSB.print("setPWD: ");
  SerialUSB.println(buff);
  SerialUSB.print("len: ");
  SerialUSB.println(len);
  hexDump((uint8_t *)buff, len);
  if (len == 32) {
    // copy to the SecretKey buffer
    memcpy(SecretKey, buff, 32);
    needEncryption = true;
    hexDump((uint8_t *)SecretKey, 32);
    return;
  }
  if (len == 64) {
    // copy to the SecretKey buffer
    hex2array((uint8_t *)buff, SecretKey, 64);
    needEncryption = true;
    hexDump((uint8_t *)SecretKey, 32);
    return;
  }
}

void sendPacket(char *buff) {
  hexDump((uint8_t*)buff, strlen(buff));
  if (needEncryption) {
    uint16_t olen = encryptECB((uint8_t*)buff);
    // encBuff = encrypted buffer
    // hexBuff = encBuf, hex encoded
    // olen = len(hexBuf)
  }
  SerialUSB.print("Sending packet...");
  // Now send a packet
  digitalWrite(LED_BUILTIN, 1);
  //digitalWrite(PIN_PA28, LOW);
  LoRa.idle();
  digitalWrite(RFM_SWITCH, 0);
  LoRa.writeRegister(REG_LNA, 00); // TURN OFF LNA FOR TRANSMIT
  LoRa.beginPacket();
  if (needEncryption) {
    LoRa.print((char*)hexBuf);
  } else {
    LoRa.print(buff);
  }
  LoRa.endPacket();
  SerialUSB.println(" done!");
  delay(500);
  digitalWrite(LED_BUILTIN, 0);
  digitalWrite(RFM_SWITCH, 1);
  // digitalWrite(PIN_PA28, HIGH);
  LoRa.receive();
  LoRa.writeRegister(REG_LNA, 0x23); // TURN ON LNA FOR RECEIVE
  if (needEncryption) {
    SerialUSB.println("Reversing...");
    SerialUSB.println(" . Zeroing buffers...");
    memset(encBuf, 0, 128);
    memset(hexBuf, 0, 256);
    SerialUSB.println(" . Encrypting source again with key:");
    hexDump((uint8_t *)SecretKey, 32);
    uint16_t olen = encryptECB((uint8_t*)buff);
    SerialUSB.println(" . Decrypting...");
    decryptECB((uint8_t*)hexBuf, olen);
    SerialUSB.println((char*)encBuf);
  }
}

void stockUpRandom() {
  fillRandom(randomStock, 256);
  randomIndex = 0;
  hexDump(randomStock, 256);
}
