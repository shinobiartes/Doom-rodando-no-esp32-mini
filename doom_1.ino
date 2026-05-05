#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#define TFT_CS    5
#define TFT_DC    7
#define TFT_RST   6
#define SCK_PIN   2
#define SDA_PIN   3
#define BUZZER    4   
#define BTN_TIRO  1   
#define BTN_PULO  8   

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

const int MAP_W = 16;
const int MAP_H = 16;
const byte worldMap[16][16] = {
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,1,1,1,1,0,1,1,1,1,1,1,1,0,1},
  {1,0,1,0,0,0,0,0,0,0,0,0,0,1,0,1},
  {1,0,1,0,1,1,1,0,1,1,1,1,0,1,0,1},
  {1,0,0,0,1,0,0,0,0,0,0,1,0,0,0,1},
  {1,0,1,0,1,0,1,1,1,1,0,1,1,1,0,1},
  {1,0,1,0,0,0,1,0,0,1,0,0,0,0,0,1},
  {1,0,1,1,1,0,1,0,0,1,1,1,1,1,0,1},
  {1,0,0,0,1,0,0,0,0,0,0,0,0,1,0,1},
  {1,1,1,0,1,1,1,1,1,1,1,1,0,1,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1},
  {1,0,1,1,1,1,1,1,0,1,1,1,1,1,0,1},
  {1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

float pX = 2.0, pY = 2.0, pDirX = -1.0, pDirY = 0.0, planeX = 0.0, planeY = 0.66;
int vOffset = 0, vida = 100;
float npcX = 10.0, npcY = 10.0;
bool npcVivo = true;

// Variáveis para o balanço da arma
float bobAngle = 0;
int bobX = 0, bobY = 0;

void telaAbertura(String msg) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(3); tft.setTextColor(ST77XX_RED);
  tft.setCursor(45, 30); tft.print("DOOM");
  tft.setTextSize(1); tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(40, 70); tft.print(msg);
  tft.setCursor(25, 100); tft.print("Aperte para Iniciar");
  while(digitalRead(10) && digitalRead(11) && digitalRead(12) && digitalRead(13) && digitalRead(BTN_TIRO) && digitalRead(BTN_PULO)) delay(10);
  vida = 100; pX = 2.0; pY = 2.0; npcVivo = true; npcX = 10.0; npcY = 10.0;
  tft.fillScreen(ST77XX_BLACK);
}

void desenharHUD() {
  tft.drawRect(108, 5, 42, 10, ST77XX_WHITE);
  uint16_t corV = (vida > 40) ? ST77XX_GREEN : ST77XX_RED;
  tft.fillRect(109, 6, (vida * 40) / 100, 8, corV);
  tft.fillRect(109 + (vida * 40 / 100), 6, 40 - (vida * 40 / 100), 8, ST77XX_BLACK);
}

void desenharArma(bool atirando, bool movendo) {
  // Se estiver movendo, calcula o deslocamento senoidal
  if (movendo) {
    bobAngle += 0.4;
    bobX = sin(bobAngle) * 5;      // Balanço lateral
    bobY = abs(cos(bobAngle)) * 4; // Balanço vertical
  } else {
    bobX = 0; bobY = 0; bobAngle = 0;
  }

  int centroX = 80 + bobX;
  int baseY = 128 + vOffset - bobY;

  if(atirando) {
    tft.fillTriangle(centroX-6, baseY-55, centroX+6, baseY-55, centroX, baseY-75, ST77XX_WHITE);
    baseY += 5; // Recuo forte
  }
  tft.fillRect(centroX - 5, baseY - 40, 10, 40, 0x2104); 
}

void setup() {
  SPI.begin(SCK_PIN, -1, SDA_PIN, TFT_CS); 
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(3);
  pinMode(10, INPUT_PULLUP); pinMode(11, INPUT_PULLUP); 
  pinMode(12, INPUT_PULLUP); pinMode(13, INPUT_PULLUP);
  pinMode(BTN_TIRO, INPUT_PULLUP); pinMode(BTN_PULO, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  telaAbertura("ESP32-S3 EDITION");
}

void loop() {
  if (vida <= 0) { telaAbertura("GAME OVER"); return; }
  
  bool movendo = false;

  // IA NPC
  if (npcVivo) {
    float dx = pX - npcX, dy = pY - npcY;
    float dist = sqrt(dx*dx + dy*dy);
    if(dist > 0.4) { npcX += (dx/dist)*0.035; npcY += (dy/dist)*0.035; }
    else { vida -= 2; tone(BUZZER, 50, 10); }
  }

  // Pulo e Tiro
  if (digitalRead(BTN_PULO) == LOW && vOffset == 0) { vOffset = 15; tone(BUZZER, 200, 30); }
  else if (vOffset > 0) vOffset -= 2;

  bool atirando = (digitalRead(BTN_TIRO) == LOW);
  if (atirando) {
    tone(BUZZER, random(500, 800), 10);
    float dx = npcX - pX, dy = npcY - pY;
    float dist = sqrt(dx*dx + dy*dy);
    if (((dx/dist)*pDirX + (dy/dist)*pDirY) > 0.92 && dist < 5.0) { npcVivo = false; tone(BUZZER, 1000, 80); }
  }

  // Renderização 3D
  for(int x = 0; x < 160; x += 4) {
    float cameraX = 2 * x / (float)160 - 1;
    float rDX = pDirX + planeX * cameraX, rDY = pDirY + planeY * cameraX;
    int mX = int(pX), mY = int(pY);
    float dDX = abs(1/rDX), dDY = abs(1/rDY), sDX, sDY, pWD;
    int stX, stY, side; bool hit = false;
    if (rDX < 0) { stX = -1; sDX = (pX - mX) * dDX; } else { stX = 1; sDX = (mX + 1.0 - pX) * dDX; }
    if (rDY < 0) { stY = -1; sDY = (pY - mY) * dDY; } else { stY = 1; sDY = (mY + 1.0 - pY) * dDY; }
    while (!hit) {
      if (sDX < sDY) { sDX += dDX; mX += stX; side = 0; } else { sDY += dDY; mY += stY; side = 1; }
      if (worldMap[mX][mY] > 0) hit = true;
    }
    pWD = (side == 0) ? (sDX - dDX) : (sDY - dDY);
    int lH = (int)(128 / pWD);
    int dS = -lH / 2 + 64 + vOffset, dE = lH / 2 + 64 + vOffset;
    if(dS < 0) dS = 0; if(dE >= 128) dE = 127;
    tft.fillRect(x, 0, 4, dS, 0x0000); 
    tft.fillRect(x, dS, 4, dE - dS, (side == 1) ? 0x52AA : 0x8410); 
    tft.fillRect(x, dE, 4, 128 - dE, 0x3186); 
  }

  // NPC Sprite
  if(npcVivo) {
    float sX = npcX - pX, sY = npcY - pY;
    float invDet = 1.0 / (planeX * pDirY - pDirX * planeY);
    float trX = invDet * (pDirY * sX - pDirX * sY), trY = invDet * (-planeY * sX + planeX * sY);
    if(trY > 0.2) {
      int sScnX = int((160/2)*(1+trX/trY)), sH = abs(int(128/trY));
      if(sScnX > 0 && sScnX < 160) tft.fillRect(sScnX - sH/4, 64 - sH/4 + vOffset, sH/2, sH/2, ST77XX_RED);
    }
  }

  // Movimentação
  float mS = 0.12, rS = 0.08;
  if (digitalRead(10) == LOW) { // Frente
    movendo = true;
    if(worldMap[int(pX+pDirX*mS)][int(pY)] == 0) pX += pDirX*mS;
    if(worldMap[int(pX)][int(pY+pDirY*mS)] == 0) pY += pDirY*mS;
  }
  if (digitalRead(11) == LOW) { // Tras
    movendo = true;
    if(worldMap[int(pX-pDirX*mS)][int(pY)] == 0) pX -= pDirX*mS;
    if(worldMap[int(pX)][int(pY-pDirY*mS)] == 0) pY -= pDirY*mS;
  }
  if (digitalRead(12) == LOW) { // Giro Esq
    movendo = true;
    float oX = pDirX; pDirX = pDirX*cos(rS)-pDirY*sin(rS); pDirY = oX*sin(rS)+pDirY*cos(rS);
    float oPX = planeX; planeX = planeX*cos(rS)-planeY*sin(rS); planeY = oPX*sin(rS)+planeY*cos(rS);
  }
  if (digitalRead(13) == LOW) { // Giro Dir
    movendo = true;
    float oX = pDirX; pDirX = pDirX*cos(-rS)-pDirY*sin(-rS); pDirY = oX*sin(-rS)+pDirY*cos(-rS);
    float oPX = planeX; planeX = planeX*cos(-rS)-planeY*sin(-rS); planeY = oPX*sin(-rS)+planeY*cos(-rS);
  }

  desenharArma(atirando, movendo);
  desenharHUD();
}