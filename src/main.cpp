#include <Arduino.h>
#include <AutoOTA.h>
#include <Ticker.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DGO_VKbot.h>

#define ONE_WIRE_BUS 12                  //Пин датчика температуры

OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

// НАСТРОЙКИ
#define WIFI_SSID "Dnlvrn"
#define WIFI_PASS "2155791975"
#define VK_TOKEN "vk1.a.S9yn4Jfxbps_iLkBwY5qCPZ0yZGiv0MkCCIvgbuzoKeEKiQJ3kNLPxyDbtd1LL6-LHTSCe6NuQy06e0OYBffAcndyVz8Udtnl6OI-z6ZOWn1sThQ8DAuQEnZvHAQ9jNXLFCT1YTZZgf957DCsNOOWYslZ5S8UHiO0XCYhODFsenYpKELuXgo5hvA6SvWgvN7PzKepBqTKFJxUHCmxSjZlw"
#define GROUP_ID "-241019082"
#define YOUR_USER_ID 353090963 


#define zik D1                                 // Пин зажигание
#define starter D2                             // Стартер
#define obogrev D8                             // печка
#define gabarit D4                             // габариты

#define kpp_sens D5                          //контроль КПП
//#define ds18b20_sens D6                      // DS18b20
#define tormoz D7                            // контроль Ручник
#define flash D3                             //прерывание сигнал с катушки

uint16_t command = 0;                        // Команда
int peer_id;                                 //идентификатор собеседника
Ticker timerCounter;                         // Таймер для прерываний
int8_t count = 10;



DGO_VKbot bot;                              // Создаем экземпляр бота

AutoOTA ota("1.9", "Srvrn1/auto_start");    //текущая версия==============================

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
    
    default:
      bot.sendMessage("Неизвестная команда", peer_id);
      break;
    }
  }
  
}


void VKbot_begin(){
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
}

void timer1() {
  count--;
  Serial.println(count);

}


void setup() {
  Serial.begin(74880);

  pinMode(zik, OUTPUT);
  pinMode(starter, OUTPUT);
  pinMode(obogrev, OUTPUT);
  pinMode(gabarit , OUTPUT);

  digitalWrite(zik, LOW);          
  digitalWrite(starter, LOW);
  digitalWrite(obogrev, LOW);
  digitalWrite(gabarit, LOW);
  
  pinMode(kpp_sens, INPUT_PULLUP);
  //pinMode(ds18b20_sens, INPUT);
  pinMode(tormoz, INPUT_PULLUP);
  pinMode(flash, INPUT_PULLUP);

  WiFi_connect();
  VKbot_begin();

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

  timerCounter.attach(1.0, timer1);  // Запуск прерывания по таймеру 1 раз в секунду
}

void loop() {
 if(count <= 0){
    count = 120;
    Serial.println("hello");
    bot.sendMessage("hello", YOUR_USER_ID);
  }

  bot.tick();

  if(WiFi.status() != WL_CONNECTED) WiFi_connect();
}