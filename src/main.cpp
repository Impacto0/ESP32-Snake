/*
  Projekt: https://github.com/Impacto0/ESP32-Snake

  Discord: Doggr#1234 
  Github: https://github.com/Impacto0
*/

#include <Arduino.h>
#include <LedControl.h>

/*
  LedControl wymaga avr/pgmspace.h co jest niedostępne w ESP32.
  
  W LedControl.h wystarczy zmienić:
  #include <avr/pgmspace.h>
  na:
  #include <pgmspace.h>

  Po tej zmianie firmware kompiluje się poprawnie.
*/

/*
  Środek: 0
    X: ~1800
    Y: ~1800

  Góra: 1
    X: ~1800
    Y: 4095

  Dół: 2
    X: ~1800
    Y: 0

  Prawo: 3
    X: 0
    Y: ~1800
  
  Lewo: 4
    X: 4095
    Y: ~1800
*/

#define URX 35
#define URY 34
#define DIN 21
#define CLK 18
#define CS 19

int d = 0; // kierunek

TaskHandle_t Input;
TaskHandle_t Game;

LedControl led = LedControl(DIN, CLK, CS, 1);

byte rows[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // wiersze na wyświetlaczu

struct Cords{ // struct z kordynatami (wiersz i kolumna)
  int row;
  int col;
};

// dane wensza rzecznego
Cords head; // głowa
Cords body[64]; // częsci ciała
int len; // długość

// jabłko
Cords apple;

void inputLoop(void * pvParameters){ // odczytywanie z joystick'a
  // Serial.println("[InputLoop Task] > Start");
  for(;;){
    int x = analogRead(URX);
    int y = analogRead(URY);

    if(x >= 0 && x <= 4095 && y == 4095){
      if(d != 2){
        d = 1;
      }
    }
    else if(x >= 0 && x <= 4095 && y == 0){
      if(d != 1 && d != 0){
        d = 2;
      }
    }
    else if(y >= 0 && y <= 4095 && x == 0){
      if(d != 4){
        d = 3;
      }
    }
    else if(y >= 0 && y <= 4095 && x == 4095){
      if(d != 3){
        d = 4;
      }
    }

    // Serial.println("[InputLoop Task] > Loop");
    delay(100);
  }
}

// Zerowanie danych wyświetlacza
void clearLED(){
  // Serial.println("[LED] > Reset...");
  for(int i = 0; i < 8; i++){
    rows[i] = 0;
  }
  // Serial.println("[LED] > Reset done");
}

// wyświetlenie na wyświetlacz
void render(){
  // zaktualizowanie danych wyświetlacza
  for(int i = 0; i < len; i++){
    rows[body[i].row] |= 128 >> body[i].col;
  }

  rows[apple.row] |= 128 >> apple.col;

  for(int i = 0; i < 8; i++){
    led.setRow(0, i, rows[i]);
  }
}

// losowanie pozycji jabłka
void createApple(){
  // czy udało się wylosować pustą pozycję
  bool success = false;
  int row = 0;
  int col = 0;

  // losowanie do momentu gdy nie będzie wylosowana pusta pozycja
  while(!success){
    success = true; // zakładamy ze udało się wylosować pustą pozycję (obalimy te założenie dalej co ustali czy losować dalej)
    row = (int)random(0, 8);
    col = (int)random(0, 8);

    if(col == head.col && row == head.row){ // sprawdzanie czy pozycja nie jest zajęta przez głowę
      success = false;
      continue;
    }

    for(int i = 0; i < len; i++){ // sprawdzanie czy pozycja nie jest zajęta przez ciało
      if(col == body[i].col && row == body[i].row){
        success = false;
        continue;
      }
    }
  }

  apple.row = row;
  apple.col = col;
}

// koniec gry
void gameOver(){
  delay(200);

  led.clearDisplay(0);

  for(int i = 7; i >= 0; i--){
    led.setRow(0, i, B11111111);
    delay(100);
    led.setRow(0, i, B00000000);
  }

  // reset głowy
  head.col = 4;
  head.row = 5;

  // reset ciała
  body[0].col = 4;
  body[0].row = 5;
  body[1].col = 4;
  body[1].row = 4;
  for(int i = 2; i < 63; i++){
    body[i].col = -1;
    body[i].row = -1;
  }

  // reset długości
  len = 2;

  // reset kierunku
  d = 0;
}

void gameLoop(void * pvParameters){ // logika gry oraz wyświetlanie
  // Serial.println("[GameLoop Task] > Start");
  for(;;){
    clearLED();

    // przemieszczenie głowy
    if(d == 1){
      head.row++; // góra
    }
    else if(d == 2){
      head.row--; // dół
    }
    else if(d == 3){
      head.col--; // prawo
    }
    else if(d == 4){
      head.col++; // lewo
    }
    else{
      render();
      continue; // "pauza" jeśli kierunek jest ustawiony na 0
    }

    // sprawdzanie kolizji z ścianą
    if(head.row == -1 || head.row == 8 || head.col == -1 || head.col == 8){
      gameOver();
      continue;
    }

    // sprawdzanie kolizji z ciałem
    for(int i = 0; i < len; i++){
      if(head.row == body[i].row && head.col == body[i].col){
        gameOver();
        continue;
      }
    }

    // sprawdzanie kolizji z jabłkiem
    if(head.row == apple.row && head.col == apple.col){
      len++;
      createApple();
    }
    else{
      for(int i = 1; i< len; i++){
        body[i - 1].col = body[i].col;
        body[i - 1].row = body[i].row;
      }
    }

    // sprawdzanie konca gry - aka wygranie
    if(len == 64){
      gameOver();
      continue;
    }

    body[len - 1].col = head.col;
    body[len - 1].row = head.row;

    render();

    // Serial.println("[GameLoop Task] > Loop");
    delay(500);
  }

}

void setup() {
  // Serial.begin(115200);

  // uruchomienie i przygotowanie wyświetlacza
  led.shutdown(0, false);
  led.setIntensity(0, 1);
  led.clearDisplay(0);

  xTaskCreatePinnedToCore(inputLoop, "Input", 10000, NULL, 1, &Input, 0);               

  delay(100);

  xTaskCreatePinnedToCore(gameLoop, "Game", 10000, NULL,  1, &Game, 1);

  // pierwsze losowanie pozycji jabłka
  createApple();

  // pierwsze ustawienie długości
  len = 2;

  // pierwsze ustawienie głowy
  head.col = 4;
  head.row = 5;

  // pierwsze ustawienie ciała
  body[0].col = 4;
  body[0].row = 5;
  body[1].col = 4;
  body[1].row = 4;
}

void loop() {
  
}