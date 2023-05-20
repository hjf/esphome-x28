#include "esphome.h"

#define BIT_TIME (6000 / 3)
#define IDLE (5000)
#define ZERO (2000)

#define INPUT_PIN 22
#define OUTPUT_PIN 23
static void isr();
char isKeyboardCode(uint16_t code);
volatile char available = 0;
uint16_t recbuf = 0;
char curbit = 0;
uint16_t KEYBOARD_CODES[] = {
    0x0000, 0x8013, 0x8025, 0x0036,
    0x8046, 0x0055, 0x0063, 0x8070,
    0x8089, 0x009A, 0x00AC, 0x80BF,
    0x00CF, 0x80DC, 0x80EA, 0x00F9,
    0x00AC, 0x810A, 0x80BF, 0x813C,
    0x00F9, 0x0119, 0x80EA, 0x012F,
    0x8169};

typedef union
{
  struct
  {
    unsigned parity : 1;
    unsigned id : 3;
    unsigned data : 8;
    unsigned checksum : 4;
  };
  uint16_t word;
} MPX_packet;

class MPXController
{
private:
  Switch *switchEstoy;
  Switch *switchActivada;
  bool isActivada = false;
  bool isEstoy = false;
  void changeActivada(bool activada)
  {
    isActivada = activada;
    if (this->switchActivada != nullptr)
      if (isActivada)
        this->switchActivada->turn_on();
      else
        this->switchActivada->turn_off();
  }

  void changeEstoy(bool estoy)
  {
    isEstoy = estoy;
    if (this->switchEstoy != nullptr)
      if (isEstoy)
        this->switchEstoy->turn_on();
      else
        this->switchEstoy->turn_off();
  }

public:
  static MPXController *GetController();
  void setup()
  {
    pinMode(OUTPUT_PIN, OUTPUT);
    digitalWrite(OUTPUT_PIN, LOW);
    pinMode(INPUT_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(INPUT_PIN), isr, CHANGE);
  }
  void tx(uint16_t payload)
  {
    noInterrupts();
    int ncts = IDLE * 5;
    while (ncts)
    {
      if (digitalRead(2) == 1)
        ncts = IDLE * 5;
      else
        ncts--;
    }

    // start bit
    digitalWrite(OUTPUT_PIN, 1);
    delayMicroseconds(BIT_TIME);

    for (int i = 0; i < 16; i++)
    {
      digitalWrite(OUTPUT_PIN, 0);
      if (!(payload & (unsigned int)0x8000))
      {
        delayMicroseconds(BIT_TIME);
        digitalWrite(OUTPUT_PIN, 1);
        delayMicroseconds(2 * BIT_TIME);
      }
      else
      {
        delayMicroseconds(2 * BIT_TIME);
        digitalWrite(OUTPUT_PIN, 1);
        delayMicroseconds(BIT_TIME);
      }
      payload = payload << 1;
    }
    digitalWrite(OUTPUT_PIN, 0);
    delayMicroseconds(1000);
    interrupts();
  }
  void handle()
  {
    if (available)
    {
      available = 0;

      MPX_packet packet;
      packet.word = recbuf;
      available = 0;
      char direction[2] = "<";

      if (isKeyboardCode(packet.word))
        direction[0] = '>';

      ESP_LOGD("custom", "[%010lu]: %s 0x%04X - %1x %2x %3x %2x", millis(), direction, packet.word, packet.parity, packet.id, packet.data, packet.checksum);
      switch (packet.word)
      {
      case 0x49c1:
        changeActivada(true);
        break;
      case 0xc92b:
        changeActivada(false);
        break;
      case 0x4be8:
        changeEstoy(true);
        break;
      case 0xcbae:
        changeEstoy(false);
        break;

      default:
        break;
      }
    }
  }

  void setSwitchEstoy(Switch *estoy)
  {
    this->switchEstoy = estoy;
  }

  void setSwitchActivada(Switch *activada)
  {
    this->switchActivada = activada;
  }
  bool getIsEstoy()
  {
    return this->isEstoy;
  }
  bool getIsActivada()
  {
    return this->isActivada;
  }
  void toggleEstoy()
  {
    ESP_LOGD("custom", "toggling estoy");
    this->tx(0x80DC);
  }

  void activar(bool activada)
  {
    ESP_LOGD("custom", "changing activada to %d", activada);
    if (activada)
    {
      this->tx(KEYBOARD_CODES[1]);
      delay(100);
      this->tx(KEYBOARD_CODES[2]);
      delay(100);
      this->tx(KEYBOARD_CODES[5]);
      delay(100);
      this->tx(KEYBOARD_CODES[4]);
    }
    else
    {
      this->tx(KEYBOARD_CODES[1]);
      delay(100);
      this->tx(KEYBOARD_CODES[2]);
      delay(100);
      this->tx(KEYBOARD_CODES[5]);
      delay(100);
      this->tx(KEYBOARD_CODES[1]);
    }
  }
  void sendPanic()
  {
    ESP_LOGD("custom", "sending panic");

    changeActivada(true);
    delay(500);
    this->tx(0x012F);
  }
};

static MPXController *mpxController;
MPXController *MPXController::GetController()
{
  if (mpxController == nullptr)
  {
    mpxController = new MPXController();
    mpxController->setup();
  }
  return mpxController;
}

static void isr()
{
  static unsigned long prev_micros = 0;
  unsigned long curr_micros = micros();
  unsigned long length = curr_micros - prev_micros;

  if (!available && digitalRead(INPUT_PIN) == 1)
  {
    if (length > IDLE)
    {
      recbuf = 0;
      curbit = 0;
    }
    else
    {
      recbuf = recbuf << 1;
      if (length > ZERO)
        recbuf |= 1;

      if (++curbit == 16)
        available = 1;
    }
  }

  prev_micros = curr_micros;
}

class AlarmaX28 : public Component
{
private:
  char isEstoy = 0;
  char isActivada = 0;
  MPXController *mpxController;

public:
  void setup() override
  {
    mpxController = MPXController::GetController();
  }
  void loop() override
  {
    mpxController->handle();
  }
  void sendPanic()
  {
    mpxController->sendPanic();
  }
};

char isKeyboardCode(uint16_t code)
{

  for (size_t i = 0; i < sizeof(KEYBOARD_CODES) / 2; i++)
  {
    if (KEYBOARD_CODES[i] == code)
      return true;
  }
  return false;
}

class X28Activada : public Component, public Switch
{
private:
  MPXController *mpxController;

public:
  void setup() override
  {
    mpxController = MPXController::GetController();
    mpxController->setSwitchActivada(this);
  }
  void write_state(bool state) override
  {
    if (state != mpxController->getIsActivada())
      mpxController->activar(state);
    publish_state(state);
  }
};

class X28Estoy : public Component, public Switch
{
private:
  MPXController *mpxController;

public:
  void setup() override
  {
    mpxController = MPXController::GetController();
    mpxController->setSwitchEstoy(this);
  }
  void write_state(bool state) override
  {
    if (state != mpxController->getIsEstoy())
      mpxController->toggleEstoy();
    publish_state(state);
  }
};