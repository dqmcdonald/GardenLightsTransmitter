/* Transmitter for the Garden Lights
  Communication is by NRF24L01. Designed for a Arduino Pro Mini
  Using: https://www.sparkfun.com/products/705 for communication
  Based on:
  nrf24_reliable_datagram_client.pde
  -*- mode: C++ -*-
  Connections:

    Pin A0 -> Pot for channel 1
    Pin A1 -> Pot for channel 2
    Pin  4 -> Channel 1 push button (input)
    Pin  5 -> Channel 2 push button (input)
    Pin  6 -> Bicolor LED Green
    Pin  7 -> Bicolor LED Orange
    Pin  8 -> NRF24L01 CE
    Pin 10 -> NRF24L01 CSN (Chip select in)
    Pin 11 -> NRF24L01 SDI (SPI MOSI)
    Pin 12 -> NRF24L01 SDO (SPI MOSI)
    Pin 13 -> NRF24L01 SCK (SPI Clock)

  Commands are expected as 15 byte strings. There are three sections:

  CODE CHNL VALU

  CODE is the operation to be performend. Values are:
    MODE - button is pressed to change the channel mode (turn off, turn to pulsing etc)
  POTS - a potentiometer value has changed

  CHNL is the channel, an integer value

  VALU is the value being sent - an integer value. Currently this is only used for POTS messages

*/
#include <RHReliableDatagram.h>
#include <RH_NRF24.h>
#include <SPI.h>
#include <Bounce2.h>

#define TRANSMITTER_ADDRESS 1
#define RECEIVER_ADDRESS 2

#define CHANNEL_ONE_POT_PIN A0
#define CHANNEL_TWO_POT_PIN A1

#define CHANNEL_ONE_BUTTON_PIN 4
#define CHANNEL_TWO_BUTTON_PIN 5


#define GREEN_LED_PIN 6
#define ORANGE_LED_PIN 7

// Singleton instance of the radio driver
RH_NRF24 driver;

const char* MODE_TEXT = "MODE ";
const char* POTS_TEXT = "POTS ";
const int CHANNEL_OFFSET = 5;
const int VALUE_OFFSET = 10;


void sendCommand( const char* command, bool flash_led = false );

int channel_one_pot_value;
int channel_two_pot_value;

// Instantiate a Bounce object :
Bounce channel_one_button_debouncer = Bounce();
Bounce channel_two_button_debouncer = Bounce();

const int COMMAND_BUF_LEN = 16;
char command_buf[COMMAND_BUF_LEN];

const int POTS_EPS = 4;

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, TRANSMITTER_ADDRESS);

void setup()
{
  Serial.begin(9600);

  Serial.println("Setting up Garden Lights Transmitter" );
  if (!manager.init())
  {
    Serial.println("init of NRF24L01 failed");
    while (true) {
      delay(1000);
    }
  }

  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
  if (!driver.setRF(RH_NRF24::DataRate250kbps, RH_NRF24::TransmitPower0dBm))
  {
    Serial.println("setRF failed");
    while (true)
      delay(1000);
  }


  // Setup the button with an internal pull-up :
  pinMode(CHANNEL_ONE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(CHANNEL_TWO_BUTTON_PIN, INPUT_PULLUP);

  // After setting up the button, setup the Bounce instance :
  channel_one_button_debouncer.attach(CHANNEL_ONE_BUTTON_PIN);
  channel_one_button_debouncer.interval(10);
  channel_two_button_debouncer.attach(CHANNEL_TWO_BUTTON_PIN);
  channel_two_button_debouncer.interval(10);

  pinMode(CHANNEL_ONE_POT_PIN, INPUT );
  pinMode(CHANNEL_TWO_POT_PIN, INPUT );

  channel_one_pot_value = analogRead( CHANNEL_ONE_POT_PIN );
  channel_two_pot_value = analogRead( CHANNEL_TWO_POT_PIN );

  pinMode(GREEN_LED_PIN, OUTPUT );
  pinMode(ORANGE_LED_PIN, OUTPUT );

  digitalWrite(GREEN_LED_PIN, LOW );
  digitalWrite(ORANGE_LED_PIN, HIGH );

  delay(1000);

  digitalWrite(GREEN_LED_PIN, HIGH );
  digitalWrite(ORANGE_LED_PIN, LOW );

  delay(1000);

  digitalWrite(GREEN_LED_PIN, LOW );
  digitalWrite(ORANGE_LED_PIN, LOW );

  delay(1000);


  Serial.println("Garden Lights Transmitter setup complete" );
}

// Dont put this on the stack:
uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];

void loop()
{

  int val = 0;

  // Check for button press and send message
  channel_one_button_debouncer.update();
  channel_two_button_debouncer.update();

  // Call code if Bounce fell (transition from HIGH to LOW) :
  if ( channel_one_button_debouncer.fell() ) {
    sendButton(1);
  }

  if ( channel_two_button_debouncer.fell() ) {
    sendButton(2);
  }

  val = analogRead( CHANNEL_ONE_POT_PIN );
  if ( abs(val - channel_one_pot_value) > POTS_EPS ) {
    channel_one_pot_value = val;
    sendPots(1, channel_one_pot_value );
  }
  val = analogRead( CHANNEL_TWO_POT_PIN );
  if ( abs(val - channel_two_pot_value) > POTS_EPS ) {
    channel_two_pot_value = val;
    sendPots(2, channel_two_pot_value );
  }
}


void sendButton( const int& channel ) {
  /* Construct a command that sends a MODE change command: */
  snprintf( command_buf, 6, "%5s", MODE_TEXT );
  snprintf( command_buf + CHANNEL_OFFSET, 6, "%5d", channel );
  sendCommand( command_buf, true );
}

void sendPots( const int& channel, const int& value ) {
  /* Send a POTS command */
  snprintf( command_buf, 6, "%5s", POTS_TEXT );
  snprintf( command_buf + CHANNEL_OFFSET, 6, "%5d", channel );
  snprintf( command_buf + VALUE_OFFSET, 6, "%5d", value );
  sendCommand( command_buf );
}

void sendCommand( const char* command, bool flash_led = false ) {

  Serial.print("Sending command: ");
  Serial.println(command);

  if (manager.sendtoWait(command, COMMAND_BUF_LEN, RECEIVER_ADDRESS))
  {
    if ( flash_led ) {
      digitalWrite(GREEN_LED_PIN, HIGH );
      digitalWrite(ORANGE_LED_PIN, LOW );
      delay(200);
    }
    Serial.print("Sent send OK: ");
    Serial.println(command);
  } else {
    if ( flash_led ) {
      digitalWrite(GREEN_LED_PIN, LOW );
      digitalWrite(ORANGE_LED_PIN, HIGH );
      delay(200);
    }

    Serial.println("sendtoWait failed");

  }

  if ( flash_led ) {
    digitalWrite(GREEN_LED_PIN, LOW );
    digitalWrite(ORANGE_LED_PIN, LOW );
  }
}
