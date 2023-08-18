#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Wire.h> 
#include<Servo.h>
#include <WiFiClientSecure.h>
#include <WiFiUDP.h>
#include <WakeOnLan.h>


Servo brava; 
int angle = 0; 
unsigned long onTime = 0;
unsigned long offTime = 0;
unsigned long startMillis;
unsigned long currentMillis;
unsigned long elapsedSeconds;
unsigned long elapsedMinutes;
unsigned long elapsedHours;

//Fontana nivo vode
const int waterSensorPin = A0;


// Fontane, vreme rada
unsigned long fontanaStartTime = 0;
unsigned long fontana2StartTime = 0;
bool fontanaAktivna = false;
bool fontana2Aktivna = false;



// Provera rada fontana da li su vec ukljucene
bool fontanaUpaljena = false;
bool fontana2Upaljena = false;


// Provera rada svetala da li su vec ukljuceni
bool zakljucana = true;
bool lightOn = false;
bool lightOnG1 = false;
bool lightOnG2 = false;
bool lightOnG3 = false;
bool lightOnS1 = false;
bool lightOnS2 = false;
bool lightOnS3 = false;
bool lightOnS4 = false;
bool manualControl = false;
const unsigned long duration = 60000; // trajanje tajmera u milisekundama (1 minut)
unsigned long startTime = 0; // vreme kada su releji upaljeni

float napon = 0.0;
int sifra = 0;
int RelayPinFontana2 = D0;// define output pin for relay 
int RelayPinFontana = D1;// define output pin for relay 
int RelayPinKugle = D7;// define output pin for relay //nekadasnji D7
int RelayPinErco = D6;// define output pin for relay 
int RelayPinGaraza1 = D5;// define output pin for relay 
int RelayPinGaraza2 = D4;// define output pin for relay 
int RelayPinGaraza3 = D2;// define output pin for relay 
int RelayPinTerasa = D8;// define output pin for relay
int RelayPinReflektor = D9;// define output pin for relay
//const int buzzer = D3; //buzzer to arduino pin D3

int RelayPins[] = {RelayPinKugle, RelayPinErco, RelayPinGaraza1, RelayPinGaraza2, RelayPinTerasa, RelayPinReflektor, RelayPinGaraza3};
const int NumRelays = sizeof(RelayPins)/sizeof(RelayPins[0]);

unsigned long previousMillis = 0;
const long INTERVAL = 6000;  
char ssid[] = "Laki";         // your network SSID (name)
char password[] = "inteljezakon"; // your network password
String chat_id = "5569752868";
#define TELEGRAM_BOT_TOKEN "6245273161:AAFgrRig0thxkqJ01D0ecSrQtXSi73tArRg"
// This is the Wifi client that supports HTTPS
WiFiClientSecure client;
UniversalTelegramBot bot(TELEGRAM_BOT_TOKEN, client);

bool in_password_state = false;

int delayBetweenChecks = 1000;
unsigned long lastTimeChecked;   //last time messages' scan has been done

unsigned long lightTimerExpires;
boolean lightTimerActive = false;
boolean lightTimerActiveg = false;
boolean lightTimerActivevg = false;
//
struct targetDevice {
  byte mac[6]; //The targets MAC address
  String deviceName;
};

targetDevice devices[] ={
  {{ 0xFC, 0xAA, 0x14, 0x1F, 0xCC, 0x10 }, "Anci"}, //BC-5F-F4-FF-FF-FF
  {{ 0x88, 0x88, 0x88, 0x88, 0x87, 0x88 }, "Laki"} //04-D9-F5-FF-FF-FF
};

// Change to match how many devices are in the above array.
int numDevices = 2;

//------- ---------------------- ------

WiFiUDP UDP;
/**
 * This will brodcast the Magic packet on your entire network range.
 */
IPAddress computer_ip(255,255,255,255); 
//

void setup() {
  Serial.begin(115200);
  angle = 0; 
  startMillis = millis();
  //brava.attach(D10); Vrata od garaze
  pinMode(RelayPinFontana2, OUTPUT);
  pinMode(RelayPinFontana, OUTPUT);
  pinMode(RelayPinKugle, OUTPUT);
   // Set RelayPin as an output pin
  pinMode(RelayPinErco, OUTPUT);

   pinMode(RelayPinGaraza1, OUTPUT);
   pinMode(RelayPinGaraza2, OUTPUT);
   pinMode(RelayPinTerasa, OUTPUT);
   pinMode(RelayPinReflektor, OUTPUT);
   pinMode(RelayPinGaraza3, OUTPUT);
   //pinMode(buzzer, OUTPUT); // Set buzzer - pin D3 as an output
   
  digitalWrite(RelayPinFontana2, HIGH); // turn the relay ON (low is ON if relay is LOW trigger. change it to HIGH if you have got HIGH trigger relay)
  digitalWrite(RelayPinFontana, HIGH); // turn the relay ON (low is ON if relay is LOW trigger. change it to HIGH if you have got HIGH trigger relay)
  digitalWrite(RelayPinKugle, HIGH); // turn the relay ON (low is ON if relay is LOW trigger. change it to HIGH if you have got HIGH trigger relay)
  digitalWrite(RelayPinErco, HIGH); // turn the relay ON (low is ON if relay is LOW trigger. change it to HIGH if you have got HIGH trigger relay)
  digitalWrite(RelayPinGaraza1, HIGH); // turn the relay ON (low is ON if relay is LOW trigger. change it to HIGH if you have got HIGH trigger relay)
  digitalWrite(RelayPinGaraza2, HIGH); // turn the relay ON (low is ON if relay is LOW trigger. change it to HIGH if you have got HIGH trigger relay)
  digitalWrite(RelayPinTerasa, HIGH); // turn the relay ON (low is ON if relay is LOW trigger. change it to HIGH if you have got HIGH trigger relay)
  digitalWrite(RelayPinReflektor, HIGH); // turn the relay ON (low is ON if relay is LOW trigger. change it to HIGH if you have got HIGH trigger relay)
  digitalWrite(RelayPinGaraza3, HIGH); // turn the relay ON (low is ON if relay is LOW trigger. change it to HIGH if you have got HIGH trigger relay)

  // Set WiFi to station mode and disconnect from an AP if it was Previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Only required on 2.5 Beta
  client.setInsecure();

  
  // longPoll keeps the request to Telegram open for the given amount of seconds if there are no messages
  // This hugely improves response time of the bot, but is only really suitable for projects
  // where the the initial interaction comes from Telegram as the requests will block the loop for
  // the length of the long poll
  bot.longPoll = 60;
  bot.sendMessage(chat_id, "Sistem je upravo startovan 😁!");
}

void(* resetFunc) (void) = 0;

String getElapsedTimeString(unsigned long startTime, bool isActive) {
  if (isActive) {
    unsigned long elapsedTime = millis() - startTime;
    unsigned long elapsedHours = elapsedTime / 3600000;
    unsigned long elapsedMinutes = (elapsedTime % 3600000) / 60000;
    unsigned long elapsedSeconds = (elapsedTime % 60000) / 1000;

    return String(elapsedHours) + "s " + String(elapsedMinutes) + "m " + String(elapsedSeconds) + "s";
  } else {
    return "N/A";
  }
}

void checkRelays()
{
  bool error = false;
  for(int i = 0; i < NumRelays; i++)
  {
    digitalWrite(RelayPins[i], LOW);
    delay(500);
    if(digitalRead(RelayPins[i]) != LOW)
    {
      // Relej nije uključen
      error = true;
      break;
    }
    digitalWrite(RelayPins[i], HIGH);
    delay(500);
    if(digitalRead(RelayPins[i]) != HIGH)
    {
      // Relej nije isključen
      error = true;
      break;
    }
  }
  if(error)
  {
    // Javi korisniku da postoji problem s relejima
    bot.sendChatAction(chat_id, "typing");
    bot.sendMessage(chat_id, "Došlo je do problema sa relejima! 🛠");
  }
  else
  {
    // Svi releji rade ispravno
    bot.sendChatAction(chat_id, "typing");
    bot.sendMessage(chat_id, "Svi releji rade ispravno! ✅");
  }
}

  void flashRelays() {
    const int numFlashes = 10; // Broj treptaja
    const int flashDelay = 300; // Vrijeme u milisekundama između svakog treptaja

    for (int i = 0; i < numFlashes; i++) {
      digitalWrite(RelayPinKugle, HIGH);
      digitalWrite(RelayPinErco, HIGH);
      digitalWrite(RelayPinTerasa, HIGH);
      digitalWrite(RelayPinReflektor, HIGH);
      delay(100);
      delay(flashDelay);
      digitalWrite(RelayPinKugle, LOW);
      digitalWrite(RelayPinErco, LOW);
      digitalWrite(RelayPinTerasa, LOW);
      digitalWrite(RelayPinReflektor, LOW);
      delay(100);
      delay(flashDelay);
    }
      bot.sendChatAction(chat_id, "typing");
      bot.sendMessage(chat_id, "Uspesno izvrseno ✅");
  }

void handleNewMessages(int numNewMessages) {

  for (int i = 0; i < numNewMessages; i++) {

    // If the type is a "callback_query", a inline keyboard button was pressed
    if (bot.messages[i].type ==  F("callback_query")) {
      String text = bot.messages[i].text;
      String chat_id = String(bot.messages[i].chat_id);
      Serial.print("Call back button pressed with text: ");
      Serial.println(text);

      if (text == F("RELAYKUGLEON")) 
      {
        if(lightOnS1)
        {
          bot.sendMessage(chat_id, "Svetlo je već upaljeno");
        }
        else
        {
          lightOnS1 = true;
          lightOn = lightOnS1 && lightOnS2 && lightOnS3 && lightOnS4;
          digitalWrite(RelayPinKugle, LOW); 
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Svetlo upaljeno ✅");
        }
      } 
      else if (text == F("RELAYKUGLEOFF")) 
      {
        if (!lightOnS1) 
      {
          //pošaljite poruku da je svetlo već ugašeno
          bot.sendMessage(chat_id, "Svetlo je već ugaseno");
      }
      else
      {
        lightOnS1 = false;
        lightOn = lightOnS1 && lightOnS2 && lightOnS3 && lightOnS4;
        digitalWrite(RelayPinKugle, HIGH); 
        bot.sendChatAction(chat_id, "typing");
        bot.sendMessage(chat_id, "Svetlo ugaseno ❌");
      }
      }
      else if (text == F("RELAYREFLEKTORON")) 
      {
        if(lightOnS2)
        {
          bot.sendMessage(chat_id, "Svetlo je već upaljeno");
        }
        else
        {
          lightOnS2 = true;
          lightOn = lightOnS1 && lightOnS2 && lightOnS3 && lightOnS4;
          digitalWrite(RelayPinReflektor, LOW); 
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Svetlo upaljeno ✅");
        }
      } 
      else if (text == F("RELAYREFLEKTOROFF")) 
      {
        if (!lightOnS2) 
        {
            //pošaljite poruku da je svetlo već ugašeno
          bot.sendMessage(chat_id, "Svetlo je već ugaseno");
        }
        else
        {
          lightOnS2 = false;
          lightOn = lightOnS1 && lightOnS2 && lightOnS3 && lightOnS4;
          digitalWrite(RelayPinReflektor, HIGH); 
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Svetlo ugaseno ❌");
        }
      } 
      else if (text == F("RELAYERCOON")) 
      {
        if(lightOnS3)
        {
          bot.sendMessage(chat_id, "Svetlo je već upaljeno");
        }
        else
        {
          lightOnS3 = true;
          lightOn = lightOnS1 && lightOnS2 && lightOnS3 && lightOnS4;
          digitalWrite(RelayPinErco, LOW); 
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Svetlo upaljeno ✅");
        }
      } 
      else if (text == F("RELAYERCOOFF")) 
      {
        if (!lightOnS3) 
        {
            //pošaljite poruku da je svetlo već ugašeno
          bot.sendMessage(chat_id, "Svetlo je već ugaseno");
        }
        else
        {
          lightOnS3 = false;
          lightOn = lightOnS1 && lightOnS2 && lightOnS3 && lightOnS4;
          digitalWrite(RelayPinErco, HIGH); 
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Svetlo ugaseno ❌");
        }
      } 
      else if (text == F("RELAYTERASAON")) 
      {
        if(lightOnS4)
        {
          bot.sendMessage(chat_id, "Svetlo je već upaljeno");
        }
        else
        {
          lightOnS4 = true;
          lightOn = lightOnS1 && lightOnS2 && lightOnS3 && lightOnS4;
          digitalWrite(RelayPinTerasa, LOW); 
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Svetlo upaljeno ✅");
        }
      } 
      else if (text == F("RELAYTERASAOFF")) 
      {
        if (!lightOnS4) 
        {
            //pošaljite poruku da je svetlo već ugašeno
          bot.sendMessage(chat_id, "Svetlo je već ugaseno");
        }
        else
        {
          lightOnS4 = false;
          lightOn = lightOnS1 && lightOnS2 && lightOnS3 && lightOnS4;
          digitalWrite(RelayPinTerasa, HIGH);
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Svetlo ugaseno ❌"); 
        }
      } 
      else if (text == F("RELAYGARAZA1ON"))
      {
        if(lightOnG1)
        {
          bot.sendMessage(chat_id, "Svetlo je već upaljeno");
        }
        else
        {
          digitalWrite(RelayPinGaraza1, LOW);
          lightOnG1 = true;
          lightOn = lightOnG1 && lightOnG2 && lightOnG3;
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Svetlo upaljeno ✅");
        }
      }
      else if (text == F("RELAYGARAZA1OFF"))
      {
        if (!lightOnG1) 
        {
            //pošaljite poruku da je svetlo već ugašeno
            bot.sendMessage(chat_id, "Svetlo je već ugaseno");
        }
        else
        {
          lightOnG1 = false;
          lightOn = lightOnG1 && lightOnG2 && lightOnG3;
          digitalWrite(RelayPinGaraza1, HIGH);
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Svetlo ugaseno ❌");
        }
      }
      
      else if (text == F("RELAYGARAZA2ON"))
      {
        if(lightOnG2)
        {
          bot.sendMessage(chat_id, "Svetlo je već upaljeno");
        }
        else
        {
          lightOnG2 = true;
          lightOn = lightOnG1 && lightOnG2 && lightOnG3;
          digitalWrite(RelayPinGaraza2, LOW);
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Svetlo upaljeno ✅");
        }
      }
      else if (text == F("RELAYGARAZA2OFF"))
      {
        if (!lightOnG2) 
        {
            //pošaljite poruku da je svetlo već ugašeno
            bot.sendMessage(chat_id, "Svetlo je već ugaseno");
        }
        else
        {
          lightOnG2 = false;
          lightOn = lightOnG1 && lightOnG2 && lightOnG3;
          digitalWrite(RelayPinGaraza2, HIGH);
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Svetlo ugaseno ❌");
        }
      }
      else if (text == F("RELAYGARAZA3ON"))
      {
        if(lightOnG3)
        {
          bot.sendMessage(chat_id, "Svetlo je već upaljeno");
        }
        else
        {
          lightOnG3 = true;
          lightOn = lightOnG1 && lightOnG2 && lightOnG3;
          digitalWrite(RelayPinGaraza3, LOW);
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Svetlo upaljeno ✅");
        }
      }
      else if (text == F("RELAYGARAZA3OFF"))
      {
          if (!lightOnG3) 
        {
            //pošaljite poruku da je svetlo već ugašeno
            bot.sendMessage(chat_id, "Svetlo je već ugaseno");
        }
        else
        {
          lightOnG3 = false;
          lightOn = lightOnG1 && lightOnG2 && lightOnG3;
          digitalWrite(RelayPinGaraza3, HIGH);
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Svetlo ugaseno ❌");
        }
      }
      else if (text == F("SVENG"))
      {
        if (lightOn) 
        {
        //pošaljite poruku da je svetlo već upaljeno
          bot.sendMessage(chat_id, "Svetlo je već upaljeno");
        }
        else
        {
          lightOnG1 = true;
          lightOnG2 = true;
          lightOnG3 = true;
          lightOn = true;
          digitalWrite(RelayPinGaraza1, LOW);
          digitalWrite(RelayPinGaraza2, LOW);
          digitalWrite(RelayPinGaraza3, LOW);
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Svetlo upaljeno ✅");
        }
      }
      else if (text == F("STANIG"))
      {
         if (!lightOnG1 && !lightOnG2 && !lightOnG3 && !lightOn) 
      {
          //pošaljite poruku da je svetlo već ugašeno
          bot.sendMessage(chat_id, "Svetlo je već ugaseno");
      }
        else
        {
          lightOnG1 = false;
          lightOnG2 = false;
          lightOnG3 = false;
          lightOn = false;
          digitalWrite(RelayPinGaraza1, HIGH);
          digitalWrite(RelayPinGaraza2, HIGH);
          digitalWrite(RelayPinGaraza3, HIGH);
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Svetlo ugaseno ❌");
        }
      }
      else if (text == F("SVEN"))
      {
        if (lightOn) 
        {
        //pošaljite poruku da je svetlo već upaljeno
          bot.sendMessage(chat_id, "Svetlo je već upaljeno");
        }
        else
        {
          lightOnS1 = true;
          lightOnS2 = true;
          lightOnS3 = true;
          lightOnS4 = true;
          lightOn = true;
          digitalWrite(RelayPinErco, LOW);
          digitalWrite(RelayPinKugle, LOW);
          digitalWrite(RelayPinReflektor, LOW);
          digitalWrite(RelayPinTerasa, LOW);
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Svetlo upaljeno ✅");
        }
      }
      else if (text == F("RELAYFONTANAON")) 
      {
        if (!fontanaUpaljena) {
          digitalWrite(RelayPinFontana, LOW);
          fontanaUpaljena = true;
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Fontana upaljena ✅");
        } else {
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Fontana je već upaljena ❗");
        }
      }
      else if (text == F("RELAYFONTANA2ON")) 
      {
        if (!fontana2Upaljena) {
          digitalWrite(RelayPinFontana2, LOW);
          fontana2Upaljena = true;
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Fontana 2 upaljena ✅");
        } else {
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Fontana 2 je već upaljena ❗");
        }
    }

      else if (text == F("RELAYFONTANAOFF")) 
      {
        if (fontanaUpaljena) {
          digitalWrite(RelayPinFontana, HIGH);
          fontanaUpaljena = false;
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Fontana ugašena ❌");
        } else {
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Fontana je već ugašena ❗");
        }
    }

    else if (text == F("RELAYFONTANA2OFF")) 
    {
      if (fontana2Upaljena) {
        digitalWrite(RelayPinFontana2, HIGH);
        fontana2Upaljena = false;
        bot.sendChatAction(chat_id, "typing");
        bot.sendMessage(chat_id, "Fontana 2 ugašena ❌");
      } else {
        bot.sendChatAction(chat_id, "typing");
        bot.sendMessage(chat_id, "Fontana 2 je već ugašena ❗");
      }
    }
      else if (text == F("RELAYFONTANAALLON")) {
      if (!fontanaUpaljena && !fontana2Upaljena) {
        digitalWrite(RelayPinFontana, LOW);
        digitalWrite(RelayPinFontana2, LOW);
        fontanaUpaljena = true;
        fontana2Upaljena = true;
        bot.sendChatAction(chat_id, "typing");
        bot.sendMessage(chat_id, "Fontane upaljene ✅");
      } else {
        bot.sendChatAction(chat_id, "typing");
        bot.sendMessage(chat_id, "Jedna ili obe fontane su već upaljene ❗");
      }
    }
      else if (text == F("RELAYFONTANAALLOFF")) 
          {
      if (fontanaUpaljena || fontana2Upaljena) {
        digitalWrite(RelayPinFontana, HIGH);
        digitalWrite(RelayPinFontana2, HIGH);
        fontanaUpaljena = false;
        fontana2Upaljena = false;
        bot.sendChatAction(chat_id, "typing");
        bot.sendMessage(chat_id, "Fontane ugašene ❌");
      } else {
        bot.sendChatAction(chat_id, "typing");
        bot.sendMessage(chat_id, "Sve fontane su već ugašene ❗");
      }
    }
      else if (text.startsWith("STIME1")) {
        //text.replace("STIME1", "");
        int timeRequested = text.toInt();
        
        digitalWrite(RelayPinErco, LOW);
        digitalWrite(RelayPinKugle, LOW);
        digitalWrite(RelayPinReflektor, LOW);
        digitalWrite(RelayPinTerasa, LOW);
        bot.sendChatAction(chat_id, "typing");
        bot.sendMessage(chat_id, "Svetlo upaljeno ✅");
        lightTimerActive = true;
        lightTimerExpires = millis() + (1000 * 60);
      }
      else if (text.startsWith("STIME5")) {
        //text.replace("STIME5", "");
        int timeRequested = text.toInt();
        
        digitalWrite(RelayPinErco, LOW);
        digitalWrite(RelayPinKugle, LOW);
        digitalWrite(RelayPinReflektor, LOW);
        digitalWrite(RelayPinTerasa, LOW);
        bot.sendChatAction(chat_id, "typing");
        bot.sendMessage(chat_id, "Svetlo upaljeno ✅");
        lightTimerActive = true;
        lightTimerExpires = millis() + (1000 * 60 * 5);
      }
      else if (text.startsWith("STIME1H")) {
        //text.replace("STIME1H", "");
        int timeRequested = text.toInt();
        
        digitalWrite(RelayPinErco, LOW);
        digitalWrite(RelayPinKugle, LOW);
        digitalWrite(RelayPinReflektor, LOW);
        digitalWrite(RelayPinTerasa, LOW);
        bot.sendChatAction(chat_id, "typing");
        bot.sendMessage(chat_id, "Svetlo upaljeno ✅");
        lightTimerActive = true;
        lightTimerExpires = millis() + (1000 * 60 * 60);
      }
      else if (text.startsWith("GTIME1")) {
        //text.replace("GTIME1", "");
        int timeRequested = text.toInt();
        
        digitalWrite(RelayPinGaraza1, LOW);
        digitalWrite(RelayPinGaraza2, LOW);
        digitalWrite(RelayPinGaraza3, LOW);
        bot.sendChatAction(chat_id, "typing");
        bot.sendMessage(chat_id, "Svetlo upaljeno ✅");
        lightTimerActiveg = true;
        lightTimerExpires = millis() + (1000 * 60);
      }
      else if (text.startsWith("GTIME5")) {
        //text.replace("GTIME5", "");
        int timeRequested = text.toInt();
        
        digitalWrite(RelayPinGaraza1, LOW);
        digitalWrite(RelayPinGaraza2, LOW);
        digitalWrite(RelayPinGaraza3, LOW);
        bot.sendChatAction(chat_id, "typing");
        bot.sendMessage(chat_id, "Svetlo upaljeno ✅");
        lightTimerActiveg = true;
        lightTimerExpires = millis() + (1000 * 60 * 5);
      }
      else if (text.startsWith("GTIME1H")) {
        //text.replace("GTIME1H", "");
        int timeRequested = text.toInt();
        
        digitalWrite(RelayPinGaraza1, LOW);
        digitalWrite(RelayPinGaraza2, LOW);
        digitalWrite(RelayPinGaraza3, LOW);
        bot.sendChatAction(chat_id, "typing");
        bot.sendMessage(chat_id, "Svetlo upaljeno ✅");
        lightTimerActiveg = true;
        lightTimerExpires = millis() + (1000 * 60 * 60);
      }
      else if (text.startsWith("GVTIME5")) {
        //text.replace("GVTIME5", "");
        int timeRequested = text.toInt();
        for(angle = 180; angle>=1; angle-= 20)    
          {                                  
            brava.write(angle);                 
            delay(15); 
          }
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Vrata otkljucana 🔓");
        lightTimerActivevg = true;
        lightTimerExpires = millis() + (1000 * 60 * 5);
      }
      else if (text.startsWith("GVTIME1H")) {
        //text.replace("GVTIME1H", "");
        int timeRequested = text.toInt();
        for(angle = 180; angle>=1; angle-= 20)    
          {                                  
            brava.write(angle);                 
            delay(15); 
          }
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Vrata otkljucana 🔓");
        lightTimerActivevg = true;
        lightTimerExpires = millis() + (1000 * 60 * 60);
      }
      else if (text == F("STANI"))
      {
        if (!lightOnS1 && !lightOnS2 && !lightOnS3 && !lightOnS4 && !lightOn) 
        {
          //pošaljite poruku da je svetlo već ugašeno
          bot.sendMessage(chat_id, "Svetlo je već ugaseno");
        }
        else
        {
          lightOnS1 = false;
          lightOnS2 = false;
          lightOnS3 = false;
          lightOnS4 = false;
          lightOn = false;
          digitalWrite(RelayPinErco, HIGH);
          digitalWrite(RelayPinKugle, HIGH);
          digitalWrite(RelayPinReflektor, HIGH);
          digitalWrite(RelayPinTerasa, HIGH);
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Svetlo ugaseno ❌");
        }
      }
      else if (text == F("TEST"))
      {
        int ukupno = 0;
        for(int i = 0; i < 5; i++)
        {
          digitalWrite(RelayPinKugle, LOW);
          digitalWrite(RelayPinErco, LOW);
          digitalWrite(RelayPinGaraza1, LOW);
          digitalWrite(RelayPinGaraza2, LOW);
          digitalWrite(RelayPinTerasa, LOW);
          digitalWrite(RelayPinReflektor, LOW);
          digitalWrite(RelayPinGaraza3, LOW);
          delay(500);
          digitalWrite(RelayPinKugle, HIGH);
          digitalWrite(RelayPinErco, HIGH);
          digitalWrite(RelayPinGaraza1, HIGH);
          digitalWrite(RelayPinGaraza2, HIGH);
          digitalWrite(RelayPinTerasa, HIGH);
          digitalWrite(RelayPinReflektor, HIGH);
          digitalWrite(RelayPinGaraza3, HIGH);
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Uspesan test broj " + String(i+1) + " ✅");
          ukupno = i+1;
        }
        bot.sendChatAction(chat_id, "typing");
        bot.sendMessage(chat_id, "Svih " + String(ukupno) + " testova je proslo! ✅");
      }
      else if (text == F("CHECK"))
      {
        checkRelays();
      }
      else if (text == F("FLASH"))
      {
        flashRelays();
      }
      else if (text == F("RESET"))
      {
        bot.sendChatAction(chat_id, "typing");
        bot.sendMessage(chat_id, "Sistem će biti resetovan! ↺");
        //resetFunc();
        delay(1000); // sačekajte jednu sekundu pre nego što ponovo pokrenete sistem
      }
      else if (text == F("OTVORI"))
      {
        // provjeri je li vrata zaključana
        if (zakljucana)
        {
          for(angle = 180; angle>=1; angle-= 20)    
          {                                  
            brava.write(angle);                 
            delay(15); 
          }
          // postavi varijablu za zaključavanje na false
          zakljucana = false;
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Vrata otkljucana 🔓");
        }
        else
        {
          // poruka ako su vrata već otključana
          bot.sendChatAction(chat_id, "typing");
          bot.sendMessage(chat_id, "Vrata su već otključana! 🚪🔓");
        }
      }
      else if (text == F("ZATVORI"))
      {
        if (!zakljucana)
      {
        for(angle = 1; angle<=180; angle+=20)    
        {                                  
          brava.write(angle);                 
          delay(15); 
        }
        // postavi varijablu za zaključavanje na true
        zakljucana = true;
        bot.sendChatAction(chat_id, "typing");
        bot.sendMessage(chat_id, "Vrata zaključana 🔒");
      }
      else
      {
        // poruka ako su vrata već zaključana
        bot.sendChatAction(chat_id, "typing");
        bot.sendMessage(chat_id, "Vrata su već zaključana! 🚪🔒");
      }
      }
      else if (text == F("STANJE"))
      {
        int sensorValue = analogRead(waterSensorPin);
        float waterLevelPercent = map(sensorValue, 0, 1023, 0 , 100);
        
        bot.sendChatAction(chat_id, "typing"); 
        String statusKugle1 = digitalRead(RelayPinKugle) == LOW ? "ON ✅" : "OFF ❌";
        String statusErco2 = digitalRead(RelayPinErco) == LOW ? "ON ✅" : "OFF ❌";
        String statusGaraza1 = digitalRead(RelayPinGaraza1) == LOW ? "ON ✅" : "OFF ❌";
        String statusGaraza2 = digitalRead(RelayPinGaraza2) == LOW ? "ON ✅" : "OFF ❌";
        String statusTerasa = digitalRead(RelayPinTerasa) == LOW ? "ON ✅" : "OFF ❌";
        String statusReflektor = digitalRead(RelayPinReflektor) == LOW ? "ON ✅" : "OFF ❌";
        String statusGaraza3 = digitalRead(RelayPinGaraza3) == LOW ? "ON ✅" : "OFF ❌";
        String statusFontana= digitalRead(RelayPinFontana) == LOW ? "ON ✅" : "OFF ❌";
        String statusFontana2= digitalRead(RelayPinFontana2) == LOW ? "ON ✅" : "OFF ❌";
        String statusGaraze = digitalRead(angle) == LOW ? "Zakljucana 🔒" : "Otkljucana 🔓";
        String poruka = "Stanje:\nKugle: " + statusKugle1 + "\nDrugi deo: " + statusErco2 + "\nGaraza 1: " + statusGaraza1 + "\nGaraza 2: " + statusGaraza2 + 
        "\nTerasa: " + statusTerasa + "\nReflektor: " + statusReflektor + "\nGaraza 3: " + statusGaraza3 + "\nFontana: " + statusFontana + 
        " (" + getElapsedTimeString(fontanaStartTime, fontanaAktivna) + ")\nFontana2: " + statusFontana2 + 
        " (" + getElapsedTimeString(fontana2StartTime, fontana2Aktivna) + ") \nNivo vode Fontana 2: " + String(waterLevelPercent) + " %" + "\nStatus garaze: " + statusGaraze + 
        "\n🕑Vreme rada sistema: " + elapsedHours + ":" + elapsedMinutes % 60 + ":" + elapsedSeconds % 60 + " sekundi";
        bot.sendMessage(chat_id, poruka);
      }
      else if (text.startsWith("WOL")) {
        text.replace("WOL", "");
        int index = text.toInt();
        Serial.print("Sending WOL to: ");
        Serial.println(devices[index].deviceName);
        WakeOnLan::sendWOL(computer_ip, UDP, devices[index].mac, sizeof devices[index].mac);
      }
      else if (text == F("STATUS"))
      {
        int freeMem = ESP.getFreeHeap();
        napon = analogRead(A0) * (3.3 / 1023.0);
        String poruka = "💻SSID: " + WiFi.SSID() + "\n" +
        "📶Signalna snaga: " + WiFi.RSSI() + " dBm\n" +
        "ℹIP adresa: " + WiFi.localIP().toString() + "\n" +
        "ℹMAC adresa: " + WiFi.macAddress() + "\n" +
        "🏠Naziv uređaja: " + WiFi.hostname() + "\n" +
        "⚡Napon: " + String(napon) + " V\n" +
        "🐏Slobodna memorija: " + String(freeMem) + " bajtova\n" +
        "🕑Vreme rada: "+ elapsedHours + ":" + elapsedMinutes % 60 + ":" + elapsedSeconds % 60 + " sekundi";

        int numClients = WiFi.softAPgetStationNum();
        poruka += "\n📡Broj povezanih klijenata: " + String(numClients) + "";

          // Dodaje informacije o verziji firmware-a
          poruka += "\n🌐Verzija firmware-a: " + String(ESP.getSdkVersion()) + "\n";

          // Dodaje informacije o fleš memoriji
          uint32_t flashSize = ESP.getFlashChipSize() / 1024;
          uint32_t flashFree = (ESP.getFlashChipSize() - ESP.getFlashChipRealSize()) / 1024;
          poruka += "💾Veličina fleš memorije: " + String(flashSize) + " KB\n" +
              "💾Preostalo na fleš memoriji: " + String(flashFree) + " KB\n";



        bot.sendChatAction(chat_id, "typing");
        bot.sendMessage(chat_id, poruka);
      }

    } 
    else
    {
      String chat_id = String(bot.messages[i].chat_id);
      String text = bot.messages[i].text;
      Serial.println("Ispis poruke "+ text); //prikazuje poruku koju proisnik unese
      Serial.println("Chatid je "+ chat_id); //prikazuje chat id bota
      if (text == F("/spolja") && sifra==333) {

        String keyboardJson = R"([
    [
      { "text" : "LED", "callback_data" : "DUMMY" },
      { "text" : "UTICNICA", "callback_data" : "DUMMY" }
    ],
  [
    { "text" : "✅ON", "callback_data" : "RELAYKUGLEON"},
    { "text" : "❌OFF", "callback_data" : "RELAYKUGLEOFF"  },
    { "text" : "✅ON", "callback_data" : "RELAYREFLEKTORON" },
    { "text" : "❌OFF", "callback_data" : "RELAYREFLEKTOROFF" }
  ],
  [
      { "text" : "KUGLE", "callback_data" : "DUMMY" },
      { "text" : "REFLEKTOR", "callback_data" : "DUMMY" }
  ],
  [
    { "text" : "✅ON", "callback_data" : "RELAYERCOON" },
    { "text" : "❌OFF", "callback_data" : "RELAYERCOOFF" },
    { "text" : "✅ON", "callback_data" : "RELAYTERASAON" },
    { "text" : "❌OFF", "callback_data" : "RELAYTERASAOFF" }
  ],
  [
    { "text" : "✅ALL ON", "callback_data" : "SVEN" },
    { "text" : "❌ALL OFF", "callback_data" : "STANI" }
  ],
  [
    { "text" : "🕑1 Mins", "callback_data" : "STIME1" },
    { "text" : "🕑5 Mins", "callback_data" : "STIME5" },
    { "text" : "🕑1 Hours", "callback_data" : "STIME1H" }
  ]
])";
bot.sendMessageWithInlineKeyboard(chat_id, "💡Spoljasnja rasveta", "", keyboardJson);
      }

      else if (text == F("/garaza") && sifra==333) {
        String keyboardJson = R"([
  [
    { "text" : "✅ON", "callback_data" : "RELAYGARAZA1ON" },
    { "text" : "❌OFF", "callback_data" : "RELAYGARAZA1OFF" },
    { "text" : "✅ON", "callback_data" : "RELAYGARAZA2ON" },
    { "text" : "❌OFF", "callback_data" : "RELAYGARAZA2OFF" }
  ],
  [
    { "text" : "✅ON", "callback_data" : "RELAYGARAZA3ON" },
    { "text" : "❌OFF", "callback_data" : "RELAYGARAZA3OFF" }
  ],
  [
    { "text" : "✅ALL ON", "callback_data" : "SVENG" },
    { "text" : "❌ALL OFF", "callback_data" : "STANIG" }
  ],
  [
    { "text" : "🔓OTVORI", "callback_data" : "OTVORI" },
    { "text" : "🔒ZATVORI", "callback_data" : "ZATVORI" }
  ],
  [
    { "text" : "🕑1 Mins", "callback_data" : "GTIME1" },
    { "text" : "🕑5 Mins", "callback_data" : "GTIME5" },
    { "text" : "🕑1 Hours", "callback_data" : "GTIME1H" }
  ],
  [
    { "text" : "🕑🚪🔓5 Mins", "callback_data" : "GVTIME5" },
    { "text" : "🕑🚪🔓1 Hours", "callback_data" : "GVTIME1H" }
  ]
])";

bot.sendMessageWithInlineKeyboard(chat_id, "💡Garaza", "", keyboardJson);
      }

      else if (text == F("/pc") && sifra==333) {

        String keyboardJson = "[";
        for(int i = 0; i< numDevices; i++)
        {
          keyboardJson += "[{ \"text\" : \"" + devices[i].deviceName + "\", \"callback_data\" : \"WOL" + String(i) + "\" }]";
          if(i + 1 < numDevices){
            keyboardJson += ",";
          }
        }
        keyboardJson += "]";
        bot.sendMessageWithInlineKeyboard(chat_id, "Racunari:", "", keyboardJson);
      }


     else if (text == F("/fontana") && sifra == 333) {
        String keyboardJson = R"([      
      [
            { "text" : "FONTANA 1", "callback_data" : "DUMMY" }
      ],
     [
    { "text" : "✅FONTANA 1 ON", "callback_data" : "RELAYFONTANAON" },
    { "text" : "❌FONTANA 1 OFF", "callback_data" : "RELAYFONTANAOFF" }
    ],
    [
            { "text" : "FONTANA 2", "callback_data" : "DUMMY" }
    ],
    [
      { "text" : "✅FONTANA 2 ON", "callback_data" : "RELAYFONTANA2ON" },
      { "text" : "❌FONTANA 2 OFF", "callback_data" : "RELAYFONTANA2OFF" }
    ],
    [
            { "text" : "FONTANA ALL", "callback_data" : "DUMMY" }
    ],
    [
      { "text" : "✅FONTANA ALL ON", "callback_data" : "RELAYFONTANAALLON" },
      { "text" : "❌FONTANA ALL OFF", "callback_data" : "RELAYFONTANAALLOFF" }
    ] 
])";
bot.sendMessageWithInlineKeyboard(chat_id, "⛲ Fontana", "", keyboardJson);
     }

      else if (text == F("/info") && sifra == 333) {
        String keyboardJson = R"([       
  [
    { "text" : "💁STANJE", "callback_data" : "STANJE" }
  ],
  [
    { "text" : "🧪TEST", "callback_data" : "TEST" },
    { "text" : "✓CHECK", "callback_data" : "CHECK" },
    { "text" : "↺RESET", "callback_data" : "RESET" }
  ],
  [
    { "text" : "⚡FLASH", "callback_data" : "FLASH" },
    { "text" : "ℹSTATUS", "callback_data" : "STATUS" }
  ],
  [
    { "text" : "10 Mins", "callback_data" : "TIME10" },
    { "text" : "20 Mins", "callback_data" : "TIME20" },
    { "text" : "30 Mins", "callback_data" : "TIME30" }
  ]
])";
bot.sendMessageWithInlineKeyboard(chat_id, "💁 Info", "", keyboardJson);
      }   
      else if((text != F("/333")))
      {
        bot.sendMessage(chat_id, "Morate se logovati /start");
      }
      // When a user first uses a bot they will send a "/start" command
      // So this is a good place to let the users know what commands are available
      if (text == F("/start")) {
  // Postavite stanje šifre na "true" i zatražite od korisnika šifru
  String chat_id = bot.messages[0].chat_id;
  String text = bot.messages[0].text;
  in_password_state = true;
  bot.sendMessage(chat_id, "Unesite sifru");
} else if (in_password_state) {
  // Proverite da li je šifra ispravna
  if (text == F("/333")) {
    String chat_id = bot.messages[0].chat_id;
    sifra=333;
    bot.sendMessage(chat_id, "Uspesno logovanje!\nKomande koje mozete koristiti su:\n/garaza, \n/fontana , \n/spolja, \n/info, \n/pc");

    // Podesite stanje šifre na "false" kako bi se omogućilo korišćenje komandi
    in_password_state = false;
  } else {
    String chat_id = bot.messages[0].chat_id;
    bot.sendMessage(chat_id, "Neispravna sifra!");
  }
} else  {
  // Ova poruka se prikazuje samo ako niste u stanju šifre i ako unesete pogrešnu komandu
  String chat_id = bot.messages[0].chat_id;
  if (text == F("/garaza") || text == F("/spolja") || text == F("/info") || text == F("/pc") || text == F("/fontana")){
    // kod koji treba izvršiti ako je bilo koji od uslova ispunjen
  }
  else 
  {
    bot.sendMessage(chat_id, "Uneli ste neispravnu komandu. ⛔");
  }
}
}
}
}
void loop() {

  int sensorValue = analogRead(waterSensorPin);
  float waterLevelPercent = map(sensorValue, 0, 1023, 0 , 100);

  int signalStrength = WiFi.RSSI();

  currentMillis = millis();  // trenutno vrijeme
  elapsedSeconds = (currentMillis - startMillis) / 1000;  // izračunaj proteklo vrijeme u sekundama
  elapsedMinutes = elapsedSeconds / 60;  // izračunaj proteklo vrijeme u minutama
  elapsedHours = elapsedMinutes / 60;  // izračunaj proteklo vrijeme u satima
  if (millis() > lastTimeChecked + delayBetweenChecks)  {

    // getUpdates returns 1 if there is a new message from Telegram
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    if (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
    }

     lastTimeChecked = millis();

    if (lightTimerActive && millis() > lightTimerExpires) {
      lightTimerActive = false;
        digitalWrite(RelayPinErco, HIGH);
        digitalWrite(RelayPinKugle, HIGH);
        digitalWrite(RelayPinReflektor, HIGH);
        digitalWrite(RelayPinTerasa, HIGH);
        bot.sendChatAction(chat_id, "typing");
        bot.sendMessage(chat_id, "Svetlo ugaseno ❌");
    }
    else if (lightTimerActiveg && millis() > lightTimerExpires)
    {
      lightTimerActiveg = false;
      digitalWrite(RelayPinGaraza1, HIGH);
        digitalWrite(RelayPinGaraza2, HIGH);
        digitalWrite(RelayPinGaraza3, HIGH);
        bot.sendChatAction(chat_id, "typing");
        bot.sendMessage(chat_id, "Svetlo ugaseno ❌");
    }
    else if (lightTimerActivevg && millis() > lightTimerExpires)
    {
      lightTimerActivevg = false;
     for(angle = 1; angle<=180; angle+=20)    
        {                                  
          brava.write(angle);                 
          delay(15); 
        }
        bot.sendChatAction(chat_id, "typing");
        bot.sendMessage(chat_id, "Vrata zaključana 🔒");
    }
    else if (signalStrength < -72) 
    {
      bot.sendChatAction(chat_id, "typing");
      bot.sendMessage(chat_id, "Nestabilan internet signal! Jačina signala: " + String(signalStrength) + " dBm");
    }
    else if (waterLevelPercent < 5)
    {
      bot.sendChatAction(chat_id, "typing");
      digitalWrite(RelayPinFontana2, HIGH);
      digitalWrite(RelayPinFontana, HIGH);
      fontana2Upaljena = false;
      fontanaUpaljena = false;
      bot.sendMessage(chat_id, "Fontana ugasena, dopunite vodu! ❌");
      bot.sendMessage(chat_id, "\nNivo vode je ispod minimuma: " + String(waterLevelPercent) + " %");
    }
    else if (waterLevelPercent < 10)
    {
      bot.sendChatAction(chat_id, "typing");
      bot.sendMessage(chat_id, "Dopunite vodu u fontanu! \nNivo vode je kritican: " + String(waterLevelPercent) + " %");
    }

    // Pracenje vremena rada za prvu fontanu
  else if (digitalRead(RelayPinFontana) == LOW) {
    if (!fontanaAktivna) {
      fontanaAktivna = true;
      fontanaStartTime = millis();
    }
  } else {
    if (fontanaAktivna) {
      fontanaAktivna = false;
      fontanaStartTime = 0;
    }
  }

  // Pracenje vremena rada za drugu fontanu
  if (digitalRead(RelayPinFontana2) == LOW) {
    if (!fontana2Aktivna) {
      fontana2Aktivna = true;
      fontana2StartTime = millis();
    }
  } else {
    if (fontana2Aktivna) {
      fontana2Aktivna = false;
      fontana2StartTime = 0;
    }
  }
  
}
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("Signalna snaga: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  Serial.print("IP adresa: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC adresa: ");
  Serial.println(WiFi.macAddress());
  
  // prikupljanje drugih informacija
  Serial.print("Naziv uređaja: ");
  Serial.println(WiFi.hostname());
  Serial.print("Vreme rada: ");
  Serial.print(millis() / 1000);
  Serial.println(" sekundi");
}
