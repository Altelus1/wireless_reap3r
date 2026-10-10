
//#define USER_SETUP_INFO "My_User_Setup"

//#define ILI9163_DRIVER

#define ST7735_DRIVER
#define TFT_RGB_ORDER TFT_RGB

#define TFT_WIDTH   128
#define TFT_HEIGHT  128

#define TFT_MOSI    36  // aka SDA
#define TFT_SCLK    37  // aka SCL or SCK
#define TFT_CS      42  // Chip select control pin
#define TFT_DC      38  // Data Command control pin. aka A0
#define TFT_RST     40   // Reset pin (could connect to Arduino RESET pin)
#define TFT_MISO    3  // "For some reason DMA requires that a MISO pin is defined" - https://github.com/Bodmer/TFT_eSPI/discussions/2233

#define ST7735_GREENTAB3
#define LOAD_GLCD     // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2    // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4    // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6    // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7    // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:-.
#define LOAD_FONT8    // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF    // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

#define SMOOTH_FONT

#define USE_HSPI_PORT
#define SPI_FREQUENCY         27000000
#define SPI_READ_FREQUENCY    20000000
#define SPI_TOUCH_FREQUENCY   2500000

/*#define ILI9163_DRIVER

#define TFT_SDA_READ // This option is for ESP32 ONLY, tested with ST7789 and GC9A01 display only

// #define TFT_RGB_ORDER TFT_RGB // Colour order Red-Green-Blue
#define TFT_RGB_ORDER TFT_BGR // Colour order Blue-Green-Red

// For ST7789, ST7735, ILI9163 and GC9A01 ONLY, define the pixel width and height in portrait orientation
#define TFT_WIDTH 128 // ST7789 240 x 240 and 240 x 320
#define TFT_HEIGHT 128 // ST7789 240 x 320

// If colours are inverted (white shows as black) then uncomment one of the next
// 2 lines try both options, one of the options should correct the inversion.

#define TFT_INVERSION_ON
// #define TFT_INVERSION_OFF

// ###### EDIT THE PIN NUMBERS IN THE LINES FOLLOWING TO SUIT YOUR ESP32 SETUP ######

#define TFT_MISO -1
#define TFT_MOSI 4 // In some display driver board, it might be written as "SDA" and so on.
#define TFT_SCLK 5
#define TFT_CS 1 // Chip select control pin
#define TFT_DC 3 // Data Command control pin
#define TFT_RST 2 // Reset pin (could connect to Arduino RESET pin)

#define LOAD_GLCD // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2 // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4 // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6 // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7 // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:-.
#define LOAD_FONT8 // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts
#define SMOOTH_FONT

#define USE_HSPI_PORT
#define SPI_FREQUENCY 40000000
#define SPI_TOUCH_FREQUENCY 2500000
*/
