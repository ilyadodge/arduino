/*   Данный скетч делает следующее: передатчик (TX) отправляет массив
 *   данных, который генерируется согласно показаниям с кнопки и с 
 *   двух потенциомтеров. Приёмник (RX) получает массив, и записывает
 *   данные на реле, сервомашинку и генерирует ШИМ сигнал на транзистор.
    by AlexGyver 2016
*/

#include <SPI.h>          // библиотека для работы с шиной SPI
#include "nRF24L01.h"     // библиотека радиомодуля
#include "RF24.h"         // ещё библиотека радиомодуля
#include <OneWire.h>

OneWire  ds(3);  // on pin 3 (a 4.7K resistor is necessary)
RF24 radio(9, 10); // "создать" модуль на пинах 9 и 10 Для Уно
//RF24 radio(9,53); // для Меги
byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"}; //возможные номера труб
unsigned long timing; // Переменная для хранения точки отсчета

void setup() {
  Serial.begin(9600); //открываем порт для связи с ПК
  radio.begin(); //активировать модуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0, 15);    //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);     //размер пакета, в байтах
  radio.openWritingPipe(address[0]);   //мы - труба 0, открываем канал для передачи данных
  radio.setChannel(0x60);  //выбираем канал (в котором нет шумов!)
  radio.setPALevel (RF24_PA_MAX); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_250KBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  //должна быть одинакова на приёмнике и передатчике!
  //при самой низкой скорости имеем самую высокую чувствительность и дальность!!
  
}
float testdata0 = 0;
int k = 0;
void loop(void) {
 
if (millis() - timing > 10000){ // Вместо 10000 подставить нужное  значение паузы (пауза между циклами проверки данных)
  timing = millis(); 
 
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  
  
  if ( !ds.search(addr)) {  
    ds.reset_search();
    delay(250);
    return;
  }
  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return;
  }
  Serial.println();
  ds.reset();
  ds.select(addr);
  ds.write(0x44);             
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE); 
         
for ( i = 0; i < 9; i++) {           
  data[i] = ds.read();
 (data[i], HEX);  
 }

  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; 
    if (data[7] == 0x10) {
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    if (cfg == 0x00) raw = raw & ~7; 
    else if (cfg == 0x20) raw = raw & ~3;
    else if (cfg == 0x40) raw = raw & ~1; 
  }
  float data1[2]; 
  data1[0] = float(raw) / 16.0;
  

    //  Serial.print("testdata0 "); Serial.println(testdata0); //проверка для отладки чему равны переменные используемые для работы условий отправки
    //  Serial.print("data1[0] "); Serial.println(data1[0]);   //проверка для отладки

  if (testdata0 != data1[0]) {
    testdata0 = data1[0];
    //Serial.print("testdata1 "); Serial.println(testdata0); // проверка постоянной отправки данных при условии что данные при каждом цикле изменяються
    radio.powerUp(); //начать работу
    radio.stopListening();  //не слушаем радиоэфир, мы передатчик
    radio.write(&data1, sizeof(data1)); // передаем показания наприемник
    radio.powerDown();
    k =0 ; 
   } else {
      //Serial.print("testdata2 "); Serial.println(testdata0); // проверка работы условия при котором данные не отпрравляються по причине того что они равны предыдущим
      k++;
      //Serial.print("k = "); Serial.println(k); 
      delay (5000); // задержка дальнейшего выполняни при срабатывании условия (по хорошему вместо задержки должен стоять блок отправки в спячку)
        if (k >= 10) {
          testdata0 = data1[0];
          //Serial.print("testdata3 "); Serial.println(testdata0); //проверка работы условия по отправки через N циклов не изменяющихся данных
          radio.powerUp(); //начать работу
          radio.stopListening();  //не слушаем радиоэфир, мы передатчик
          radio.write(&data1, sizeof(data1)); // передаем показания наприемник
          radio.powerDown(); 
          k = 0;
          delay(5000); // задержка дальнейшего выполняни при срабатывании условия (по хорошему вместо задержки должен стоять блок отправки в спячку)
        }
   }
 }
}