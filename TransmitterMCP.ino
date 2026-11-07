
#include <SPI.h>
#include <mcp2515.h>

#define POT_PIN 4
#define CAN_CS 5

#define MAX_RETRIES 3
#define CAN_ACK_ID 0x037
#define CAN_DATA_ID 0x036

#define SEND_INTERVAL 500

struct can_frame canMsg;
MCP2515 mcp2515(CAN_CS);

int lastPotValue = -1;
unsigned long lastSendTime = 0;

void setup()
{
  Serial.begin(115200);

  analogReadResolution(12);

  SPI.begin(18, 19, 23, CAN_CS);

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  Serial.println("Potentiometer CAN transmitter started");
}

void loop()
{
  unsigned long currentTime = millis();

  if (currentTime - lastSendTime >= SEND_INTERVAL)
  {
    lastSendTime = currentTime;

    // Read potentiometer
    int potValue = analogRead(POT_PIN);

    // Convert from 0-4095 to 0-100
    int percentage = map(potValue, 0, 4095, 0, 100);
    percentage = constrain(percentage, 0, 100);

    Serial.print("Potentiometer: ");
    Serial.print(potValue);

    Serial.print(" | Sending: ");
    Serial.print(percentage);
    Serial.println("%");

    // Send percentage using the original 2-byte format
    canMsg.can_id = CAN_DATA_ID;
    canMsg.can_dlc = 2;

    canMsg.data[0] = (percentage >> 8) & 0xFF;
    canMsg.data[1] = percentage & 0xFF;

    bool messageSent = false;
    int retries = 0;

    while (!messageSent && retries < MAX_RETRIES)
    {
      if (mcp2515.sendMessage(&canMsg) == MCP2515::ERROR_OK)
      {
        unsigned long startTime = millis();
        bool ackReceived = false;

        while (millis() - startTime < 300)
        {
          struct can_frame receivedMsg;

          if (mcp2515.readMessage(&receivedMsg) == MCP2515::ERROR_OK)
          {
            if ((receivedMsg.can_id & CAN_SFF_MASK) == CAN_ACK_ID)
            {
              ackReceived = true;
              break;
            }
          }
        }

        if (ackReceived)
        {
          messageSent = true;
          Serial.println("ACK received");
        }
        else
        {
          Serial.println("ACK not received, retrying...");
          retries++;
        }
      }
      else
      {
        Serial.println("Error sending message, retrying...");
        retries++;
      }
    }

    if (!messageSent)
    {
      Serial.println("Failed to send message after retries");
    }
  }
}
