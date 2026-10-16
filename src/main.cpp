#include <Arduino.h>
#include <AutoOTA.h>
#include <Ticker.h>
//#include <OneWire.h>
//#include <DallasTemperature.h>
#include <DGO_VKbot.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <GyverDS18.h>


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

 //================================================топики
     //публикация                         
const char* mqtt_topic_counter = "demo/counter";
const char* topic_alertlamp = "demo/alertlamp";
const char* topic_Clock = "demo/time";
const char* topic_temper_incar = "demo/t_incar";
const char* topic_handbrake = "demo/hbrake";
const char* topic_kppsens = "demo/kpp";
const char* topoc_dvs_state = "demo/dvs_state";
    //подписки
const char* topic_mqtt_OUT = "demo/mqtt_out";
const char* topic_Clock_set = "demo/time1";
const char* topic_start = "demo/start";
//------------------------------------------------------

#define zik D1                                 // Пин зажигание
#define starter D2                             // Стартер
#define obogrev D8                             // печка
#define gabarit D4                             // габариты

#define kpp_sens D6                           //КПП
//#define ds18b20_sens D5                     // 
#define tormoz D7                            // контроль Ручник
#define flash D3                             //прерывание сигнал с катушки
//------------------------------------------------------
uint8_t count_connect;                       // счетчик кол-ва подключений к mqtt
uint16_t command = 0;                        // Команда
int peer_id;                                 //идентификатор чата ВК
Ticker timerCounter;                         // Таймер для прерываний
//int8_t count = 10;
int8_t flag_minuta = 1;
uint16_t count_test = 0;                     //тестовый для публикаций
int8_t flag_getTemp = 0;                     //что бы не повторять запросы ds18 несколько раз


int8_t mode_mqtt = 0;                        //если mqtt,то 1, если не соединился то режим ВК, 0
int16_t vk_timer;                            //время через котор будет попытка подкл mqtt
volatile unsigned long lastPulseTime = 0;    //для прерывания
volatile unsigned long period = 0;           // период между импульсами в млс
bool dvs_state = false;                     //Запущен ли двигатель
volatile int16_t progrev_count;            //время прогрева
bool progrev_state;

//--------------------------------------------------clock
uint8_t hrs=1;
uint8_t minut=1;
uint8_t sec=1;
//-------------------------------------------------------------------------------
DGO_VKbot bot;                              // Создаем экземпляр бота
WiFiClient espClient;                       //экземпляр WIFI клиента
PubSubClient client(espClient);             // экземпляр mqtt клиента
GyverDS18Single ds(14);                     // пин D5

AutoOTA ota("2.3", "Srvrn1/auto_start");    //текущая версия==============================

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

void onNewMessage(VkUpdate& update) {                              // Обработчик новых сообщений ВК
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

void IRAM_ATTR onPulse() {                             //прерывание для определения оборотов двигателя
  unsigned long now = millis();
  period = now - lastPulseTime;
  lastPulseTime = now;
}

void startengine(){                                       //прогрев двигателя
  attachInterrupt(digitalPinToInterrupt(flash), onPulse, FALLING);                   //начинаем измерять обороты
  if(period<200){                          //проверяем не запущен ли двигатель
      dvs_state = 1;
      client.publish(topoc_dvs_state, "2");     //2 - это статус работы двс
    }

  if(!progrev_state && !digitalRead(kpp_sens) && !digitalRead(tormoz) && !dvs_state){  //Включаем зажигание
    client.publish(topoc_dvs_state, "1");                                              //1 это процесс запуска
    Serial.println("включаем зажигание");
    digitalWrite(zik, HIGH);                           //включаем зажигание
    progrev_count = 4;                                 //засекаем 4 сек
    progrev_state = 1;                                 //статус прогрева включаем
  }
  if(progrev_state && !dvs_state && progrev_count<=0){                                 //Включаем стартер
    progrev_count =3;                                  //время работы стартера 3сек
    Serial.println("включили стартер");
    while (progrev_count && period>200){
      digitalWrite(starter, HIGH);
    }
    digitalWrite(starter, LOW);                       //Выкл стартер
  
    if(period<200){                                  //если двигатель запустился
      dvs_state = 1;
      progrev_count = 1200;                           //Засекаем 20 мин 
      client.publish(topoc_dvs_state, "2");            // 2 это двс работает
      Serial.println("двс запущен");
    }
    else{                                           //если двигатель не запустился
      dvs_state = 0;                                  //
      progrev_state = 0;  
      Serial.println("ошибка запуска ДВС");
      client.publish(topoc_dvs_state, "0");
      digitalWrite(zik, LOW);                         //Выключаем зажигание
    }
  }
  if(dvs_state){
    if(progrev_count <= 0){
      digitalWrite(zik, LOW);                         //Выключаем зажигание
      dvs_state = 0;                                  //
      progrev_state = 0;                              //что бы не попадать в функцию startengine()
    }
  }
}                                               

void callback(char* topic, byte* payload, unsigned int length) {   // Функция щбработки сообщения MQTT
  Serial.print("Сообщение в топике [");
  Serial.print(topic);
  Serial.print("]: ");
  
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if(strcmp(topic, topic_mqtt_OUT) == 0) {                             //переключаем управление на ВК
    mode_mqtt = 0;
    vk_timer = 300;
    VKbot_begin();
  }
  if(strcmp(topic, topic_Clock_set) == 0){                             // Установка времени
    String data = "";
    data += (char)payload[0];
    data += (char)payload[1];
    hrs = data.toInt();
    data="";
    data += (char)payload[3];
    data += (char)payload[4];
    minut =data.toInt();
    data ="";
    data += (char)payload[6];
    data += (char)payload[7];
    sec = data.toInt();

    flag_minuta = 1;    //сразу публикуем
  }
  if(strcmp(topic, topic_start) == 0) startengine();   // переход в функцию запуска двигателя
}

void reconnect_MQTT() {                                           // Функция переподключения к MQTT брокеру
  
  // Повторяем попытки до успешного подключения
  while (!client.connected()) {
    Serial.print("Попытка подключения к MQTT брокеру...");
    client.setServer(mqttServer, mqttPort);
    client.setCallback(callback);
 
    client.connect(mqttClientId.c_str(), mqttUser, mqttPass);

      // ======================================После подключения можно подписаться на топики (если нужно)
      client.subscribe(topic_mqtt_OUT, Qos);
      client.subscribe(topic_Clock_set, Qos);
      client.subscribe(topic_start, Qos);


     if(client.connected()){
      Serial.println("подключено" );
      mode_mqtt = 1;                                       //режим mqtt
      flag_minuta = 1;                                     //сразу отправляем publish()
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
        mode_mqtt = 0;                                        //режим ВК
        vk_timer = 300;             
        Serial.println("попытки подключения исчерпаны");
        break;
    }
  }
}

void timer1() {
  count_test++;
  if(count_test>100)count_test=0;
  if(progrev_state)progrev_count--;                    //если включен прогрев,счетаем таймер

  if(!mode_mqtt) vk_timer--;  // если режим ВК то уменьшаем таймер
  //---------------------
  sec++;             //ход часов
  if(sec == 60) {
    flag_minuta = 1;
    sec = 0;
    minut++;
    if(minut == 60){
      minut = 0;
      hrs++;
      if(hrs == 24)hrs = 0;
    }
  }
//------------------------------
}

void publish(){
    flag_minuta = 0;
    char msg[10];                              // Преобразуем значение счетчика в строку
    
    if(mode_mqtt){           //------------------------------Если режим mqtt 
      snprintf(msg, sizeof(msg), "%d", count_test);     //отправка тестового счетчика
      if (client.publish(mqtt_topic_counter, msg)) {
        Serial.print("Опубликовано: ");
        Serial.print(msg);
        Serial.print(" в топик ");
        Serial.println(mqtt_topic_counter);
      } else {
        Serial.println("Ошибка публикации");
      }
      
      if(minut>9){                                       //отправка времени
        snprintf(msg, sizeof(msg), "%d:%d", hrs, minut);
      }
      else{
        snprintf(msg, sizeof(msg), "%d:0%d", hrs, minut);
      }

      if (client.publish(topic_Clock, msg)) {            //публикуем время
        Serial.print("Опубликовано: ");
        Serial.print(msg);
        Serial.print(" в топик ");
        Serial.println(topic_Clock);
      } else {
        Serial.println("Ошибка публикации");
      }
      float temp_incar;                         //темпер в салоне
      flag_getTemp = 0;                         
      ds.readTemp();                           //read ds18
        
      temp_incar=ds.getTemp();                

      snprintf(msg, sizeof(msg), "%.2f", temp_incar);       //подготавливаем строку обрезаем 2 знака после запятой
      if (client.publish(topic_temper_incar, msg)) {            //публикуем
        Serial.print("Опубликовано: ");
        Serial.print(msg);
        Serial.print(" в топик ");
        Serial.println(topic_temper_incar);
      } else {
        Serial.println("Ошибка публикации");
      }
    }
    else{                    //--------------------------   если режим ВК
      snprintf(msg, sizeof(msg), "%d:%d", hrs, minut);
      bot.sendMessage(String(msg),YOUR_USER_ID ); 
    }
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
  Serial.println();

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
  
  ds.setResolution(10);
  timerCounter.attach(1.0, timer1);  // Запуск прерывания по таймеру 1 раз в секунду
  //attachInterrupt(digitalPinToInterrupt(flash), onPulse, FALLING);
  //detachInterrupt(digitalPinToInterrupt(flash));
}

void loop() {
  
  if (mode_mqtt && !client.connected()) reconnect_MQTT();    //если режим mqtt и соединение разорвано
  if(!mode_mqtt && !VKbot_state) VKbot_begin();

  if(sec > 50 && !flag_getTemp){             //Запрос температуры
    flag_getTemp = 1;
    Serial.println("Запрос температуры");
    if (!ds.requestTemp()) {
      Serial.println("request error");
    }
  }

  if(flag_minuta) publish();                 //отправка сообщения

  if(mode_mqtt) client.loop();               //если режим mqtt

  else{                                      //если режим ВК
     bot.tick();                             
     if(vk_timer <= 0) reconnect_MQTT();
  }
  
  if(progrev_state) startengine();

  if(WiFi.status() != WL_CONNECTED) WiFi_connect();  //WIFI проверяем палюбому
}