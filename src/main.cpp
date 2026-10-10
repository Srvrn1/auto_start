#include <Arduino.h>
#include <AutoOTA.h>
#include <Ticker.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DGO_VKbot.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>



#define ONE_WIRE_BUS 12                  //Пин датчика температуры

OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

#define WIFI_SSID "Mega5/4"
#define WIFI_PASS "54400544"

#define WIFI_SSID1 "Dnlvrn"
#define WIFI_PASS1 "2155791975"
uint8_t WIFI_mode = 0;                  //смена точки доступа

#define VK_TOKEN "vk1.a.S9yn4Jfxbps_iLkBwY5qCPZ0yZGiv0MkCCIvgbuzoKeEKiQJ3kNLPxyDbtd1LL6-LHTSCe6NuQy06e0OYBffAcndyVz8Udtnl6OI-z6ZOWn1sThQ8DAuQEnZvHAQ9jNXLFCT1YTZZgf957DCsNOOWYslZ5S8UHiO0XCYhODFsenYpKELuXgo5hvA6SvWgvN7PzKepBqTKFJxUHCmxSjZlw"
#define GROUP_ID "-241019082"
#define YOUR_USER_ID 353090963 

uint8_t VKbot_state = 0;
//----------------------------------- Настройки MQTT
const char* mqttServer   = "m4.wqtt.ru";
const int   mqttPort     = 9478;
const int   mqttPortTLS  = 9479;
String mqttClientId = "srvrn_proba";
const char* mqttUser = "u_5A3C2X";
const char* mqttPass = "HilZPRjD";

const int   Qos = 1;
uint8_t count_connect;
int8_t pubflag;
                              //===========топики
const char* mqtt_topic_counter = "demo/counter";
const char* topic_alertlamp = "demo/alertlamp";
const char* topic_mqtt_OUT = "demo/mqtt_out";
//-------------------------------------------------------------------------------------

#define zik D1                                 // Пин зажигание
#define starter D2                             // Стартер
#define obogrev D8                             // печка
#define gabarit D4                             // габариты

#define kpp_sens D5                          //контроль КПП
//#define ds18b20_sens D6                      // DS18b20
#define tormoz D7                            // контроль Ручник
#define flash D3                             //прерывание сигнал с катушки
//----------------------------------------------------------------------------------------
uint16_t command = 0;                        // Команда
int peer_id;                                 //идентификатор чата
Ticker timerCounter;                         // Таймер для прерываний
int8_t count = 10;
uint16_t count_test = 0;                     //тестовый для публикаций

int8_t mode_mqtt = 0;                        //если mqtt,то 1, если не соединился то режим ВК, 0
int16_t vk_timer;                            //время через котор будет попытка подкл mqtt
//-------------------------------------------------------------------------------
DGO_VKbot bot;                              // Создаем экземпляр бота
WiFiClient espClient;                       //экземпляр WIFI клиента
PubSubClient client(espClient);             // экземпляр mqtt клиента

AutoOTA ota("2.1", "Srvrn1/auto_start");    //текущая версия==============================

void WiFi_connect(){
  volatile int8_t n = 2;
  while (n){                   //пока n,будем выполнять попытки
    n--;
    String SSID_now;
    String Pass_now;

    if(!WIFI_mode){
      SSID_now = WIFI_SSID;
      Pass_now = WIFI_PASS;
    }
    else{
      SSID_now = WIFI_SSID1;
      Pass_now = WIFI_PASS1;
    }

    int8_t i=20;
    Serial.print("Подключение к WiFi...");
    Serial.println(SSID_now);

    WiFi.begin(SSID_now, Pass_now);

    while (WiFi.status() != WL_CONNECTED && i) {
        i--;
      delay(500);
      Serial.print(".");
    }
    if(WiFi.status() != WL_CONNECTED) {
      WIFI_mode = !WIFI_mode;
      Serial.println();
      Serial.println("WiFi Ошибка");
    }
    else{
      n=0;                                  // если подключились попытки не нужны
      Serial.println();
      Serial.println("WiFi подключен!");
      Serial.print("IP адрес: ");
      Serial.println(WiFi.localIP());
    }
  }
}

void onNewMessage(VkUpdate& update) {         // Обработчик новых сообщений ВК
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
    case 1:
      mode_mqtt = 1;
      bot.sendMessage("передача управления", peer_id);
      Serial.println("mode=mqtt");
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
    VKbot_state = 1;
    // Синхронизируем время
    bot.setTimezone(3); // UTC+3
    bot.syncTime();

    // Отправляем уведомление о готовности
    delay(500);
    bot.sendMessage("Бот готов к управлению: "+ ota.version(), YOUR_USER_ID);
  } else {
    Serial.println("Ошибка запуска бота!");
    VKbot_state = 0;
  }
}

void callback(char* topic, byte* payload, unsigned int length) {   // Функция обратного вызова при получении сообщения MQTT
  Serial.print("Сообщение в топике [");
  Serial.print(topic);
  Serial.print("]: ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  if(strcmp(topic, topic_mqtt_OUT) == 0) {                              //переключаем управление на ВК
    mode_mqtt = 0;
    VKbot_begin();
  }
//----------------------------------------------------
 /* String command = "";                                          // Создаем строку из payload
    for (uint8 i = 0; i < length; i++) {
      command += (char)payload[i];
    }
    command.trim();                                           // Удаляем пробелы из начала и конца строки

    if(strcmp(topic, topic_All_control) == 0) zeroPublish();

    else if (strcmp(topic, mqtt_topic_Relay1) == 0){  // Если топик совпадает с топиком реле
      relayState2 = command.toInt();
      controlRelay();
    }*/
  
}

void reconnect_MQTT() {                                           // Функция переподключения к MQTT брокеру

  // Повторяем попытки до успешного подключения
  while (!client.connected()) {
    Serial.print("Попытка подключения к MQTT брокеру...");
    client.setServer(mqttServer, mqttPort);
    client.setCallback(callback);
 
    client.connect(mqttClientId.c_str(), mqttUser, mqttPass);

      // После подключения можно подписаться на топики (если нужно)
      client.subscribe(topic_mqtt_OUT, Qos);
  /*    client.subscribe(mqtt_topic_Relay1, Qos);
      client.subscribe(mqtt_topic_Relay2, Qos);
      client.subscribe(mqtt_topic_Relay3, Qos);
      client.subscribe(mqtt_topic_Relay4, Qos);

      client.subscribe(topic_All_control, Qos);
*/
     if(client.connected()){
      Serial.println("подключено" );
      mode_mqtt=1;                                          //режим mqtt
     }
    else {
      Serial.print("ошибка, rc=");
      Serial.print(client.state());
      Serial.println(" пробуем снова через 5 секунд");
      count_connect++;
      delay(5000);
    }
    if(count_connect == 5){
        count_connect = 0;
        mode_mqtt = 0;                                       //режим ВК
        vk_timer = 300;
        Serial.println("попытки подключения исчерпаны");
        break;
    }
  }
}

void timer1() {
  count_test++;
  if(count_test>1000)count_test=0;
  count--;
  Serial.println(count);
  if(!mode_mqtt) vk_timer--;  // если режим ВК то уменьшаем таймер
}

void publish(){
  //====================================публикуем счетчик
    char msg[8];                              // Преобразуем значение счетчика в строку
    snprintf(msg, sizeof(msg), "%d", count_test);
    
    // Публикуем в MQTT топик
    if (client.publish(mqtt_topic_counter, msg)) {
      Serial.print("Опубликовано: ");
      Serial.print(msg);
      Serial.print(" в топик ");
      Serial.println(mqtt_topic_counter);
    } else {
      Serial.println("Ошибка публикации");
    }
    pubflag = 1;
}

void pinsMode(){
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
}


void setup() {
  Serial.begin(74880);

  pinsMode();
  WiFi_connect();
  reconnect_MQTT();
  if(!mode_mqtt)  VKbot_begin();

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
  
  if (mode_mqtt && !client.connected()) reconnect_MQTT();    //если режим mqtt и соединение разорвано
  if(!mode_mqtt && !VKbot_state) VKbot_begin();

 if(count <= 0){                             //отправка сообщения по счетчику
    count = 60;
    Serial.print("отправка - ");
    Serial.println(mode_mqtt);
    if(mode_mqtt) publish();
    else{
      bot.sendMessage(String(count_test),YOUR_USER_ID );   
    } 
  }

  if(mode_mqtt) client.loop();               //если режим mqtt
  else{
     bot.tick();                             //если режим ВК
     if(vk_timer <= 0) reconnect_MQTT();
  }
  if(WiFi.status() != WL_CONNECTED) WiFi_connect();  //WIFI проверяем палюбому
}