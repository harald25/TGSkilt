#define USE_OCTOWS2811
#include<OctoWS2811.h>
#include<FastLED.h>
#include <SLIPEncodedSerial.h>
#include <OSCMessage.h>

String readString;

// FastLED provides these pre-conigured incandescent color profiles:
//     Candle, Tungsten40W, Tungsten100W, Halogen, CarbonArc,
//     HighNoonSun, DirectSunlight, OvercastSky, ClearBlueSky,
// FastLED provides these pre-configured gaseous-light color profiles:
//     WarmFluorescent, StandardFluorescent, CoolWhiteFluorescent,
//     FullSpectrumFluorescent, GrowLightFluorescent, BlackLightFluorescent,
//     MercuryVapor, SodiumVapor, MetalHalide, HighPressureSodium,
// FastLED also provides an "Uncorrected temperature" profile
//    UncorrectedTemperature;

#define TEMPERATURE UncorrectedTemperature


DEFINE_GRADIENT_PALETTE( pink_gp ) {
   0,     0,   0,   0,    //Black
   25,  255, 255, 255,    //White
  127,  255,  0,  255,    //Pink
  230,  128,  0,  128,   //Purple
  255,    0,  0,    0};    //Black

DEFINE_GRADIENT_PALETTE( heatmap_gp ) {
  0,     0,  0,  0,   //black
85,   255,  0,  0,   //red
170,   255,255,  0,   //bright yellow
255,     0,  0,  0 };  //black

DEFINE_GRADIENT_PALETTE( blink_purple_gp ) {
  0,     0,  0,  0,     //black
100,   255,  0,  255,   //Purple
255,     0,  0,  0};   //Black

DEFINE_GRADIENT_PALETTE( blink_orange_gp ) {
  0,     0,  0,  0,     //black
50,   255,  100,  0,   //orange
100,     0,  0,  0,     //black
150,   255,  100,  0,   //orange
200,     0,  0,  0,   //orange
255,     255,  100,  0};   //black

DEFINE_GRADIENT_PALETTE( bg_orange_gp ) {
  0,     0,  0,  0,     //black
50,   255,  100,  0,   //orange
100,     0,  0,  0,     //black
150,   255,  100,  0,   //orange
200,     0,  0,  0,   //orange
255,     255,  100,  0};   //black

DEFINE_GRADIENT_PALETTE( blink_bluetones_gp ) {
  0,     0,  0,  0,     //black
42,   33,  180,  226,   //Dark blue
85,     0,  0,  0,   //black
127,   109,  197,  224, //Light blue
170,     0,  0,  0,   //black
212,   33,  180,  226,  //Dark blue
255,     0,  0,  0};   //black



CRGBPalette16 heatcolorPalette = heatmap_gp;
CRGBPalette16 pinkPalette = pink_gp;
CRGBPalette16 blinkPalettePurple = blink_purple_gp;
CRGBPalette16 blinkPaletteOrange = blink_orange_gp;
CRGBPalette16 blinkPaletteBluetones = blink_bluetones_gp;
CRGBPalette16 bgPaletteOrange = bg_orange_gp;
CRGBPalette16 rainbowPalette = RainbowColors_p;

CRGBPalette16 * paletteArray[] = { &blinkPalettePurple, &blinkPaletteOrange, &blinkPaletteBluetones,&heatcolorPalette,&pinkPalette,&bgPaletteOrange, &rainbowPalette};

#define NUM_LEDS_PER_STRIP 73
#define NUM_STRIPS 4
#define BRIGHTNESS 250
CRGB leds[NUM_STRIPS * NUM_LEDS_PER_STRIP];

SLIPEncodedSerial SLIPSerial(Serial1);
enum pattern { NONE, SKILTLYS};
enum mode { BLOMST, VU_METER, STREAM };
enum direction { FORWARD, REVERSE };

uint8_t ledarray[2][NUM_LEDS_PER_STRIP];



class LedsMultitask
{
  public:

    pattern active_pattern1;
    mode active_mode;
    direction dir;
    bool EndOfPattern;

    unsigned long interval1;
    unsigned long interval2;
    unsigned long lastUpdate1;
    unsigned long lastUpdate2;

    uint8_t hue1;
    uint8_t value1;
    uint8_t saturation1;
    uint8_t hue2;
    uint8_t value2;
    uint8_t saturation2;

    int delta;
    int index1;
    int increment_by1;
    int index2;
    int increment_by2;

    int total_steps1;
    int total_steps2;

    uint8_t NumberOfStrips;
    uint8_t LedsPerStrip;
    uint16_t pixels;
    boolean UpdatePattern;
    boolean Flash;
    boolean Dim;
    uint8_t PixelDistance;
    uint8_t PixelWidth;
    uint8_t TailLength;

    uint32_t BlinkRandomTime;
    uint32_t NextBlink_LEDi[NUM_LEDS_PER_STRIP * NUM_STRIPS];
    CRGBPalette16 * ActivePalette1;
    CRGBPalette16 * ActivePalette2;

    int x_offset;
    int y_offset;
    uint8_t active_arm;
    uint8_t active_cell;
    boolean make_random;
    uint8_t fadetoblack_speed;
    uint8_t triggered1;
    uint8_t triggered2;


    LedsMultitask(mode displaymode)
    {
        pixels = NUM_LEDS_PER_STRIP * NUM_STRIPS;
        active_mode = displaymode;
    }

    // Update the pattern
    void Update1()
    {

      if ((millis() - lastUpdate1) > interval1) // time to update
      {

        lastUpdate1 = millis();
        switch (active_pattern1)
        {
          case SKILTLYS:
            SkiltLysUpdate();
            break;
          default:
            break;
        }
      }
    }


    void ResetAllLEDs () {
      for (int i = 0; i < NUM_STRIPS*NUM_LEDS_PER_STRIP; i++) {
        leds[i] = CRGB::Black;
      }
      FastLED.show();
    }

    // Increment the Index and reset at the end
    void Increment(int *index, int *total_steps, int *increment_by, direction dir)
    {

      if (dir == FORWARD)
      {
        *index = *index + *increment_by;
        if (*index >= *total_steps)
        {
          *index = 0;
        }
      }

      if (dir == REVERSE)
      {

        *index = *index - *increment_by;
        if (*index <= 0)
        {
          *index = *total_steps;
        }
      }
    }

    void SkiltLys()
    {
      Serial.println("Skiltlys");
      active_pattern1 = SKILTLYS;
      total_steps1 = 255;
      index1 = 0;
      index2 = 0;
      interval1 = 40;
      increment_by1 = 1;
      hue1 = 10;
      hue2 = 150;
      dir = FORWARD;
      value1 = 255;
      //ActivePalette1 = &bgPaletteOrange;
      //ActivePalette2 = &blinkPaletteBluetones;
    }

    void SkiltLysUpdate()
    {
      /*
      Serial.println("Skiltlys update");
      for (int i = 0; i < NUM_STRIPS; i++)
      {
        for (int x = 0; x < NUM_LEDS_PER_STRIP; x++)
        {

          if (ledarray[0][x+(i*NUM_LEDS_PER_STRIP)] == 0) {
            //leds[x+(i*NUM_LEDS_PER_STRIP)] = ColorFromPalette(bgPaletteOrange,(index1 + x));
            leds[x+(i*NUM_LEDS_PER_STRIP)] = CRGB::Orange;

          }
          if (ledarray[0][x+(i*NUM_LEDS_PER_STRIP)] == 1) {
            //leds[x+(i*NUM_LEDS_PER_STRIP)] = ColorFromPalette(blinkPaletteBluetones,(index1 + x));
            leds[x+(i*NUM_LEDS_PER_STRIP)] = CRGB::Blue;
          }
        }
      }
      Increment(&index1, &total_steps1, &increment_by1, FORWARD);
      FastLED.show();
      */


      Serial.println("Skiltlys update");
      for (int i = 0; i < NUM_STRIPS; i++)
      {
        for (int x = 0; x < NUM_LEDS_PER_STRIP; x++)
        {

          if (ledarray[0][x+(i*NUM_LEDS_PER_STRIP)] == 0) {
            leds[x+(i*NUM_LEDS_PER_STRIP)] = ColorFromPalette(bgPaletteOrange,(index1 + x));

          }
          if (ledarray[0][x+(i*NUM_LEDS_PER_STRIP)] == 1) {
            leds[x+(i*NUM_LEDS_PER_STRIP)] = ColorFromPalette(blinkPaletteBluetones,(index1 + x));
          }
        }
      }
      Increment(&index1, &total_steps1, &increment_by1, FORWARD);
      FastLED.show();
    }




    void updateInterval(unsigned long interval)
    {
      interval1 = interval;
    }

    void updatevalue1(uint8_t value)
    {
      value1 = value;
    }
    void updatevalue2(uint8_t value)
    {
      value2 = value;
    }

    void updatehue1(uint8_t hue)
    {
      hue1 = hue;
    }
    void updatehue2(uint8_t hue)
    {
      hue2 = hue;
    }

    void updatesaturation1(uint8_t sat)
    {
      saturation1 = sat;
    }
    void updatesaturation2(uint8_t sat)
    {
      saturation2 = sat;
    }

    void updatePatternFunction()
    {
      UpdatePattern = 1;
    }

    void updateColor1(uint8_t x, uint8_t y)
    {
      saturation1 = x;
      hue1 = y;
    }
    void updateColor2(uint8_t x, uint8_t y)
    {
      saturation2 = x;
      hue2 = y;
    }
    void changeactive_mode(int x)
    {
      switch (x)
      {
        case 1:
          active_mode = BLOMST;
          break;
        case 2:
          active_mode = VU_METER;
          break;
        case 3:
          active_mode = STREAM;
          break;
        default:
          break;
      }
    }

    void changeDirection(uint8_t x)
    {
      switch (x)
      {
        case 1:
          dir = FORWARD;
          break;
        case 2:
          dir = REVERSE;
          break;
        default:
          break;
      }
    }

    void updatePixelDistance(uint8_t x)
    {
      PixelDistance = x;
    }

    void updatePixelWidth(uint8_t x)
    {
      PixelWidth = x;
    }

    void updateTailLength(uint8_t x)
    {
      TailLength = x;
    }

    void updateBlinkRandomTime(uint32_t x)
    {
      BlinkRandomTime = x;
    }

    void updateOrigin(int x, int y) {
      x_offset = x;
      y_offset = y;
    }

    void SetActivePalette1(int x) {
      ActivePalette1 = paletteArray[x];
    }
    void SetActivePalette2(int x) {
      ActivePalette2 = paletteArray[x];
    }

};

LedsMultitask Ledstrip(BLOMST);

void setup() {
  Serial.begin(9600);         //Teensy <=> Computer
  SLIPSerial.begin(115200);   //Teensy <=> ESP
  delay(1000);
  Serial.println("Started");

  FastLED.addLeds<OCTOWS2811>(leds, NUM_LEDS_PER_STRIP).setCorrection( TypicalSMD5050 );
  FastLED.setTemperature( TEMPERATURE );
  LEDS.setBrightness(BRIGHTNESS);

  LedArrayMaker();
  //Ledstrip.ResetAllLEDs();
  Ledstrip.SkiltLys();
}

void loop() {
  OSCMsgReceive();
  Ledstrip.Update1();
}

void OSCMsgReceive()
{

  OSCMessage msgIN;
  int size;

  static unsigned long microsTime = 0;
  while (!SLIPSerial.endofPacket())
  {
    if ((size = SLIPSerial.available()) > 0)
    {
      microsTime = micros();
      while (size--)  //this needs a byte limit
      {
        msgIN.fill(SLIPSerial.read());
      }
    }
    if ((micros() - microsTime) > 10000) break; //Timeout for no eoP()
  }


  if(!msgIN.hasError())
  {
    //Serial.println("Routing OSC message");
    msgIN.route("/Mode",changeLEDMode);
    msgIN.route("/Fader",funcValue);
  }

}

void changeLEDMode(OSCMessage &msg, int addrOffset ){

  //Serial.println("Changing LED mode");

  //PROGRAMS


  //PARAMETERS
  if(msg.fullMatch("/Mode/direction"))
  {
    float value = msg.getFloat(0);
    Ledstrip.changeDirection((int)value);
  }

  //DISPLAY MODES
  if(msg.fullMatch("/Mode/displaymode/blomst"))
  {
    Ledstrip.changeactive_mode(1);
  }
  if(msg.fullMatch("/Mode/displaymode/vu_meter"))
  {
    Ledstrip.changeactive_mode(2);
  }
  if(msg.fullMatch("/Mode/displaymode/stream"))
  {
    Ledstrip.changeactive_mode(3);
  }
}

void funcValue(OSCMessage &msg, int addrOffset ){

  //OSCMessage msgOUT("/Fader");
  Ledstrip.updatePatternFunction();

  // Origin
  if (msg.fullMatch("/Fader/origin"))
  {
    float x_offset = msg.getFloat(0);
    float y_offset = msg.getFloat(1);
    Ledstrip.updateOrigin((int)x_offset,(int)y_offset);
  }

  // INTERVAL
  if (msg.fullMatch("/Fader/interval"))
  {
    float value = msg.getFloat(0);
    Ledstrip.updateInterval((int)value);
  }

  // Value (brightness)
  if (msg.fullMatch("/Fader/value1"))
  {
    float value = msg.getFloat(0);
    Ledstrip.updatevalue1((int)value);
  }
  if (msg.fullMatch("/Fader/value2"))
  {
    float value = msg.getFloat(0);
    Ledstrip.updatevalue2((int)value);
  }

  // HUE
  if (msg.fullMatch("/Fader/hue1"))
  {
    float value = msg.getFloat(0);
    Ledstrip.updatehue1((int)value);
  }
  if (msg.fullMatch("/Fader/hue2"))
  {
    float value = msg.getFloat(0);
    Ledstrip.updatehue2((int)value);
  }

  // SATURATION
  if (msg.fullMatch("/Fader/saturation1"))
  {
    float value = msg.getFloat(0);
    Ledstrip.updatesaturation1((int)value);
  }
  if (msg.fullMatch("/Fader/saturation2"))
  {
    float value = msg.getFloat(0);
    Ledstrip.updatesaturation2((int)value);
  }

  // COLOR (will replace hue and saturation)
  if (msg.fullMatch("/Fader/color1"))
  {
    float x = msg.getFloat(0);
    float y = msg.getFloat(1);
    Ledstrip.updateColor1((uint8_t)x, (uint8_t)y);
  }
  if (msg.fullMatch("/Fader/color2"))
  {
    float x = msg.getFloat(0);
    float y = msg.getFloat(1);
    Ledstrip.updateColor2((uint8_t)x, (uint8_t)y);
  }

  if (msg.fullMatch("/Fader/palette1"))
  {
    int x = (int)msg.getFloat(0);
    if (x != -1) {
      Ledstrip.SetActivePalette1(x);
    }
  }

  if (msg.fullMatch("/Fader/palette2"))
  {
    int x = (int)msg.getFloat(0);
    if (x != -1) {
      Ledstrip.SetActivePalette2(x);
    }
  }
}

void LedArrayMaker()
{
  for (int x = 0; x < NUM_LEDS_PER_STRIP; x++)
  {
    ledarray[0][x] = 0;
    ledarray[0][x+(NUM_LEDS_PER_STRIP*2)] = 0;
  }
  for (int x = 0; x < NUM_LEDS_PER_STRIP; x++)
  {
    ledarray[0][x+NUM_LEDS_PER_STRIP] = 1;
    ledarray[0][x+(NUM_LEDS_PER_STRIP*3)] = 1;
  }
}
