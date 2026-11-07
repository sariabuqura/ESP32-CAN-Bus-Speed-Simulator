
#include <SPI.h>
#include <Wire.h>
#include <mcp2515.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

#define CAN_DATA_ID 0x036
#define CAN_ACK_ID  0x037

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

struct can_frame canMsg;
MCP2515 mcp2515(5);

void setup()
{
  Serial.begin(115200);

  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS))
  {
    Serial.println("OLED initialization failed!");
    while (true);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.display();

  SPI.begin(18, 19, 23, 5);

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  Serial.println("CAN Receiver Ready");

  display.setCursor(5, 15);
  display.setTextSize(1);
  display.println("CAN RECEIVER");
  display.setCursor(5, 35);
  display.println("Waiting for CAN...");
  display.display();
}

void loop()
{
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK)
  {
    if ((canMsg.can_id & CAN_SFF_MASK) == CAN_DATA_ID &&
        canMsg.can_dlc >= 2)
    {
      // Reconstruct received percentage
      uint16_t percentage =
        ((uint16_t)canMsg.data[0] << 8) |
        canMsg.data[1];

      percentage = constrain(percentage, 0, 100);

      // Serial output
      Serial.print("Received Percentage: ");
      Serial.print(percentage);
      Serial.println("%");

      // OLED display
      display.clearDisplay();

      display.setTextColor(SSD1306_WHITE);

      display.setTextSize(1);
      display.setCursor(5, 3);
      display.println("CAN POTENTIOMETER");

      display.drawLine(0, 16, 127, 16, SSD1306_WHITE);

      display.setTextSize(3);
      display.setCursor(30, 23);
      display.print(percentage);
      display.print("%");

      display.setTextSize(1);
      display.setCursor(5, 52);
      display.print("Level: ");

      display.drawRect(48, 51, 75, 10, SSD1306_WHITE);

      display.fillRect(
        50, 53,
        (percentage * 71) / 100,
        6,
        SSD1306_WHITE
      );

      display.display();

      // Send ACK
      canMsg.can_id = CAN_ACK_ID;
      canMsg.can_dlc = 0;

      if (mcp2515.sendMessage(&canMsg) == MCP2515::ERROR_OK)
      {
        Serial.println("ACK sent");
      }
      else
      {
        Serial.println("ACK transmission failed");
      }
    }
  }
}
