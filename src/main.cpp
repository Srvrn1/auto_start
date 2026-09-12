#include <Arduino.h>
#include <AutoOTA.h>
#include <Ticker.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DGO_VKbot.h>

#define ONE_WIRE_BUS 14

OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

// НАСТРОЙКИ
#define WIFI_SSID "Dnlvrn"
#define WIFI_PASS "2155791975"
#define VK_TOKEN "vk1.a.S9yn4Jfxbps_iLkBwY5qCPZ0yZGiv0MkCCIvgbuzoKeEKiQJ3kNLPxyDbtd1LL6-LHTSCe6NuQy06e0OYBffAcndyVz8Udtnl6OI-z6ZOWn1sThQ8DAuQEnZvHAQ9jNXLFCT1YTZZgf957DCsNOOWYslZ5S8UHiO0XCYhODFsenYpKELuXgo5hvA6SvWgvN7PzKepBqTKFJxUHCmxSjZlw"
#define GROUP_ID "-241019082"
#define YOUR_USER_ID 353090963 


#define PIN1 D1                             // Пин зажигание
#define PIN2 D2                             // Стартер
#define PIN3 D3                             // печка
#define PIN4 D4                             // габариты
#define zig_sens D5                         //оптопара на зажигание
#define KPP_sens D0                          // Передача КПП

uint16_t command = 0;                        // Команда
bool ledState = 0;                           // Состояние светодиодов
int peer_id;
Ticker timerCounter;                         // Таймер для прерываний
int8_t count = 0;

DGO_VKbot bot;                              // Создаем экземпляр бота
bool flag = false;                          //запрос на температуру

AutoOTA ota("1.8", "Srvrn1/auto_start");    //текущая версия==============================

void WiFi_connect(){
  int8_t i=20;
  Serial.println("Подключение к WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED && i) {
    delay(500);
    Serial.print(".");
    i--;
  }
  Serial.println();
  Serial.println("WiFi подключен!");
  Serial.print("IP адрес: ");
  Serial.println(WiFi.localIP());
}

void timer1() {
  count++;
  if (count >= 60) {
    count = 0;
  }
}

void onNewMessage(VkUpdate& update) {         // Обработчик новых сообщений
  if(update.type == VK_MESSAGE_NEW){
    String text = update.message.text;
    text.toLowerCase();
    text.trim();
    command = text.toInt();

    peer_id = update.message.peer_id;
    
    Serial.print("Получено сообщение: ");
    Serial.println(text);


    switch (command){                           //обработка команд

    case 2155:
      Serial.println("Перезагрузка...");
      bot.sendMessage("reset", peer_id);
      ESP.restart();
      break;
    
    case 5:
      sensors.requestTemperatures();             // Send the command to get temperatures
      bot.sendMessage("получение данных", peer_id);
      delay(500);
      bot.sendMessage(String(sensors.getTempCByIndex(0)), peer_id);
      break;

    case 10:                                    //выключение
      digitalWrite(PIN1, HIGH);                 //инверсное реле
      bot.sendMessage("PIN1 выключен", peer_id);
      Serial.println("PIN1 выключен");
      break;
    
    case 11:                                     //включение
      digitalWrite(PIN1, LOW);                   //инверсное реле
      bot.sendMessage("PIN1 включен", peer_id);
      Serial.println("PIN1 включен");
      break;

    case 12:                                      //проверка состояния
      ledState = digitalRead(PIN1);
      bot.sendMessage("PIN1: "+ String(!ledState), peer_id);
      Serial.println("Статус");
      break;
    
    case 20:                                     //выключение
      digitalWrite(PIN2, LOW);
      bot.sendMessage("PIN2 выключен", peer_id);
      Serial.println("PIN2 выключен");
      break;
    
    case 21:                                      //включение
      digitalWrite(PIN2, HIGH);
      bot.sendMessage("PIN2 включен", peer_id);
      Serial.println("PIN2 включен");
      break;

    case 22:                                      //проверка состояния
      ledState = digitalRead(PIN2);
      bot.sendMessage("PIN2: "+ String(ledState), peer_id);
      Serial.println("Статус");
      break;

    case 30:                                     //выключение
      digitalWrite(PIN3, LOW);
      bot.sendMessage("PIN3 выключен", peer_id);
      Serial.println("PIN3 выключен");
      break;
    
    case 31:                                      //включение
      digitalWrite(PIN3, HIGH);
      bot.sendMessage("PIN3 включен", peer_id);
      Serial.println("PIN3 включен");
      break;

    case 32:                                      //проверка состояния
      ledState = digitalRead(PIN3);
      bot.sendMessage("PIN3: "+ String(ledState), peer_id);
      Serial.println("Статус");
      break;

    case 40:                                     //выключение
      digitalWrite(PIN4, HIGH);                  //инверсное реле
      bot.sendMessage("PIN4 выключен", peer_id);
      Serial.println("PIN4 выключен");
      break;
    
    case 41:                                      //включение
      digitalWrite(PIN4, LOW);                    //инверсное реле
      bot.sendMessage("PIN4 включен", peer_id);
      Serial.println("PIN4 включен");
      break;

    case 42:                                      //проверка состояния
      ledState = digitalRead(PIN4);
      bot.sendMessage("PIN4: "+ String(!ledState), peer_id);
      Serial.println("Статус");
      break;

    default:
      bot.sendMessage("Неизвестная команда", peer_id);
      break;
    }
  }
  
}

void setup() {
  Serial.begin(74880);
  
  // Настройка пина LED
  pinMode(KPP_sens, INPUT_PULLUP);            //

  pinMode(PIN1, OUTPUT);
  pinMode(PIN2, OUTPUT);
  pinMode(PIN3, OUTPUT);
  pinMode(PIN4, OUTPUT);

  digitalWrite(PIN1, LOW);          
  digitalWrite(PIN2, LOW);
  digitalWrite(PIN3, LOW);
  digitalWrite(PIN4, LOW);
  
  WiFi_connect();

  Serial.println("текущая версия:  " + ota.version());
  Serial.println("Проверка обновлений...");                  //проверка обновлений
  String ver, notes;
  if (ota.checkUpdate(&ver, &notes)) {
    Serial.println("Обновление доступно!");
    Serial.print("Версия: ");
    Serial.println(ver);
    Serial.print("Описание: ");
    Serial.println(notes);
    ota.updateNow();
  }
  else {
    Serial.println("Обновлений нет!");
  }

  // Настраиваем бота
  bot.setToken(VK_TOKEN);
  bot.setGroupId(GROUP_ID);
  bot.attach(onNewMessage);

  // Рекомендуемые настройки Long Poll для отзывчивого loop()
  bot.setModeLongPoll();
  bot.setNonBlockingLongPoll(true);
  bot.setLongPollWaitSeconds(1);
  bot.setLongPollTimeoutMs(3000);
  
  // Запускаем бота
  Serial.println("Запуск VK бота...");
  if (bot.begin()) {
    Serial.println("Бот успешно запущен!");
    
    // Синхронизируем время
    bot.setTimezone(3); // UTC+3
    bot.syncTime();

    // Отправляем уведомление о готовности
    delay(500);
    bot.sendMessage("Бот готов к управлению: "+ ota.version(), YOUR_USER_ID);
  } else {
    Serial.println("Ошибка запуска бота!");
  }
  
  timerCounter.attach(1.0, timer1);  // Запуск прерывания по таймеру 1 раз в секунду
  sensors.begin();
}

void loop() {
  bot.tick();

  if (flag) {                               //
    flag = false;
  }

  if(WiFi.status() != WL_CONNECTED) WiFi_connect();
}