

/*
 * Instituto TecnolÃ³gico de Costa Rica
 * Computer Engineering
 * Taller de ProgramaciÃ³n
 * 
 * Código Servidor
 * Implementación del servidor NodeMCU
 * Proyecto 2, semestre 1
 * 2019
 * 
 * Profesor: Milton Villegas Lemus
 * Autor: Santiago Gamboa RamÃ­rez
 * 
 * Restricciónes: Biblioteca ESP8266WiFi instalada
 */
#include <ESP8266WiFi.h>

//Cantidad maxima de clientes es 1
#define MAX_SRV_CLIENTS 1
//Puerto por el que escucha el servidor
#define PORT 7070

/*
 * ssid: Nombre de la Red a la que se va a conectar el Arduino
 * password: Contraseña de la red
 * 
 * Este servidor no funciona correctamente en las redes del TEC,
 * se recomienda crear un hotspot con el celular
 */
const char* ssid = "RCCofGet";
const char* password = "50495049";


// servidor con el puerto y variable con la maxima cantidad de 

WiFiServer server(PORT);
WiFiClient serverClients[MAX_SRV_CLIENTS];

/*
 * Intervalo de tiempo que se espera para comprobar que haya un nuevo mensaje
 */
unsigned long previousMillis = 0, temp = 0;
const long interval = 100;

/*
 * Pin donde está conectado el sensor de luz
 * Señal digital, lee 1 si hay luz y 0 si no hay.
 */
#define ldr D7
/**
 * Variables para manejar las luces con el registro de corrimiento.
 * Utilizan una función propia de Arduino llamada shiftOut.
 * shiftOut(ab,clk,LSBFIRST,data), la función recibe 2 pines, el orden de los bits 
 * y un dato de 8 bits.
 * El registro de corrimiento tiene 8 salidas, desde QA a QH. Nosotros usamos 6 de las 8 salidas
 * Ejemplos al enviar data: 
 * data = B00000000 -> todas encendidas
 * data = B11111111 -> todas apagadas
 * data = B00001111 -> depende de LSBFIRST o MSBFIRST la mitad encendida y la otra mitad apagada
 */
#define ab  D6 
#define clk D8
/*
 * Variables para controlar los motores.
 * EnA y EnB son los que habilitan las salidas del driver.
 * EnA = 0 o EnB = 0 -> free run (No importa que haya en las entradas el motor no recibe potencia)
 * EnA = 0 -> Controla la potencia (Para regular la velocidad utilizar analogWrite(EnA,valor), 
 * con valor [0-1023])
 * EnB = 0 -> Controla la dirección, poner en 0 para avanzar directo.
 * In1 e In2 son inputs de driver, controlan el giro del motor de potencia
 * In1 = 0 ∧ In2 = 1 -> Moverse hacia adelante
 * In1 = 1 ∧ In2 = 0 -> Moverse en reversa
 * In3 e In4 son inputs de driver, controlan la dirección del carro
 * In3 = 0 ∧ In4 = 1 -> Gira hacia la izquierda
 * In3 = 1 ∧ In4 = 0 -> Gira hacia la derecha
 */
#define EnA D4 // 
#define In1 D3 // D4 en HIGH : retroceder
#define In2 D2 // D3 en HIGH : avanzar
#define In3 D1 // 
#define EnB D0 // 
#define In4 D5 // 0 para ir hacia adelante



/**
 * Variables
 */
// #AGREGAR VARIABLES NECESARIAS 

/**
 * Función de configuración.
 * Se ejecuta la primera vez que el módulo se enciende.
 * Si no puede conectarse a la red especificada entra en un ciclo infinito 
 * hasta ser reestablecido y volver a llamar a la función de setup.
 * La velocidad de comunicación serial es de 115200 baudios, tenga presente
 * el valor para el monitor serial.
 */
void setup() {
  Serial.begin(115200);
  pinMode(In1,OUTPUT);
  pinMode(In2,OUTPUT);
  pinMode(In3,OUTPUT);
  pinMode(In4,OUTPUT);
  pinMode(EnA,OUTPUT);
  pinMode(EnB,OUTPUT);
  pinMode(clk,OUTPUT);
  pinMode(ab,OUTPUT);
  
  pinMode(ldr,INPUT);

  // ip estática para el servidor
  IPAddress ip(192,168,43,200);
  IPAddress gateway(192,168,43,1);
  IPAddress subnet(255,255,255,0);

  WiFi.config(ip, gateway, subnet);

  // Modo para conectarse a la red
  WiFi.mode(WIFI_STA);
  // Intenta conectar a la red
  WiFi.begin(ssid, password);
  
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) delay(500);
  if (i == 21) {
    Serial.print("\nCould not connect to: "); Serial.println(ssid);
    while (1) delay(500);
  } else {
    Serial.print("\nConnection Succeeded to: "); Serial.println(ssid);
    Serial.println(".....\nWaiting for a client at");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Port: ");
    Serial.print(PORT);
  }
  server.begin();
  server.setNoDelay(true);




}

/*
 * Función principal que llama a las otras funciones y recibe los mensajes del cliente
 * Esta función comprueba que haya un nuevo mensaje y llama a la función de procesar
 * para interpretar el mensaje recibido.
 */
void loop() {
  
  unsigned long currentMillis = millis();
  uint8_t i;
  //check if there are any new clients
  if (server.hasClient()) {
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {
      //find free/disconnected spot
      if (!serverClients[i] || !serverClients[i].connected()) {
        if (serverClients[i]) serverClients[i].stop();
        serverClients[i] = server.available();
        continue;
      }
    }
    //no free/disconnected spot so reject
    WiFiClient serverClient = server.available();
    serverClient.stop();
  }

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {
      // El cliente existe y está conectado
      if (serverClients[i] && serverClients[i].connected()) {
        // El cliente tiene un nuevo mensaje
        if(serverClients[i].available()){
          // Leemos el cliente hasta el caracter '\r'
          String mensaje = serverClients[i].readStringUntil('\r');
          // Eliminamos el mensaje leído.
          serverClients[i].flush();
          
          // Preparamos la respuesta para el cliente
          String respuesta; 
          procesar(mensaje, &respuesta);
          Serial.println(mensaje);
          // Escribimos la respuesta al cliente.
          serverClients[i].println(respuesta);
        }  
        serverClients[i].stop();
      }
    }
  }
}

/*
 * Función para dividir los comandos en pares llave, valor
 * para ser interpretados y ejecutados por el Carro
 * Un mensaje puede tener una lista de comandos separados por ;
 * Se analiza cada comando por separado.
 * Esta función es semejante a string.split(char) de python
 * 
 */
void procesar(String input, String * output){
  //Buscamos el delimitador ;
  Serial.println("Checking input....... ");
  int comienzo = 0, delComa, del2puntos;
  bool result = false;
  delComa = input.indexOf(';',comienzo);
  
  while(delComa>0){
    String comando = input.substring(comienzo, delComa);
    Serial.print("Processing comando: ");
    Serial.println(comando);
    del2puntos = comando.indexOf(':');
    /*
    * Si el comando tiene ':', es decir tiene un valor
    * se llama a la función exe 
    */
    if(del2puntos>0){
        String llave = comando.substring(0,del2puntos);
        String valor = comando.substring(del2puntos+1);

        Serial.print("(llave, valor) = ");
        Serial.print(llave);
        Serial.println(valor);
        //Una vez separado en llave valor 
        *output = implementar(llave,valor); 
    }
    else if(comando == "sense"){
      *output = getSense();         
    }
    /**
     * ## AGREGAR COMPARACIONES PARA COMANDOS SIN VALOR
     * EJEM: else if (comando == CIRCLE) {
     *  
     * } 
     */
    else{
      Serial.print("Comando no reconocido. Solo presenta llave");
      *output = "Undefined key value: " + comando+";";
    }
    comienzo = delComa+1;
    delComa = input.indexOf(';',comienzo);
  }
}

String implementar(String llave, String valor){
  /**
   * La variable result puede cambiar para beneficio del desarrollador
   * Si desea obtener más información al ejecutar un comando.
   */
  String result="ok;";
  Serial.print("Comparing llave: ");
  Serial.println(llave);
  int Valor = valor.toInt();
  if(llave == "pwm"){
    Serial.print("Move....: ");
    Serial.println(valor);
    //# AGREGAR PARA CÓDIGO PARA MOVER EL CARRO HACIA DELANTE Y ATRAS
    if(valor.toInt()>0 and valor.toInt()<=1023){
      Serial.println("Valor positivo para PWM");
      //In1 = 0 ∧ In2 = 1
      digitalWrite(In1, LOW);
      digitalWrite(In2, HIGH);
      digitalWrite(In3, HIGH);
      digitalWrite(In4, LOW);
      analogWrite(EnA,Valor);
      analogWrite(EnB,Valor);
      delay (2500);
      digitalWrite(In1, LOW);
      digitalWrite(In2, LOW);
      digitalWrite(In3, LOW);
      digitalWrite(In4, LOW);
      
    }else{
    Serial.println("Valor negativo para PWM");
      digitalWrite(In1, HIGH);
      digitalWrite(In2, LOW);
      digitalWrite(In3, LOW);
      digitalWrite(In4, HIGH);
      analogWrite(EnA,-Valor);
      analogWrite(EnB,-Valor);
      delay (2500);
      digitalWrite(In1, LOW);
      digitalWrite(In2, LOW);
      digitalWrite(In3, LOW);
      digitalWrite(In4, LOW);
    }
      
    
  }
 
  else if(llave == "dir"){
    switch (valor.toInt()){
      case 1:
        Serial.println("Girando derecha");
      digitalWrite(In1, LOW);
      digitalWrite(In2, HIGH);
      digitalWrite(In3, LOW);
      digitalWrite(In4, HIGH);
      delay (800);
      digitalWrite(In1, LOW);
      digitalWrite(In2, LOW);
      digitalWrite(In3, LOW);
      digitalWrite(In4, LOW);
        break;
      case -1:
        Serial.println("Girando izquierda");
      digitalWrite(In1, HIGH);
      digitalWrite(In2, LOW);
      digitalWrite(In3, HIGH);
      digitalWrite(In4, LOW);
      delay (800);
      digitalWrite(In1, LOW);
      digitalWrite(In2, LOW);
      digitalWrite(In3, LOW);
      digitalWrite(In4, LOW);
        break;
       default:
        Serial.println("directo");
      digitalWrite(In1, LOW);
      digitalWrite(In2, HIGH);
      digitalWrite(In3, HIGH);
      digitalWrite(In4, LOW);
      analogWrite(EnA,1023);
      analogWrite(EnB,1023);
      delay (2000);
      digitalWrite(In1, LOW);
      digitalWrite(In2, LOW);
      digitalWrite(In3, LOW);
      digitalWrite(In4, LOW);
        break;
    }
  }
  else if(llave[0] == 'l'){
    Serial.println("Cambiando Luces");
    Serial.print("valor luz: ");
    Serial.println(valor);
    int vsensor = digitalRead(ldr);
    byte data = B11111111;
    //Recomendación utilizar operadores lógico de bit a bit (bitwise operators)
    switch (llave[1]){
      case 'f':
        Serial.println("Luces frontales");
        data = B11110011;
        //# B11101111 enciende los dos leds de la derecha rojos. B11011111 también
        //# B10111111 enciende el led rojo que está en el centro/derecha
        //# B01111111 enciende el led blanco del centro/derecha
        //# B11110111 enciende el led rojo que está full izquierda
        //# B11111011 enciende el led blanco del centro/izquierda
        //# (Combinación de los anteriores)B11110011 enciende los dos leds que están más a la izquierda (rojo y el blanco del centro/izquierda)
        break;
      case 'b':
        Serial.println("Luces traseras");
        data = B11001111;
        //# AGREGAR CÓDIGO PARA ENCENDER O APAGAR LUCES TRASERAS
        break;
      case 'l':
        Serial.println("Luces izquierda");
        data = B10111111;
        
        //# AGREGAR CÓDIGO PARA ENCENDER O APAGAR DIRECCIONAL IZQUIERDA
        break;
      case 'r':
        Serial.println("Luces derechas");
        data = B01111111;
        //# AGREGAR PARA CÓDIGO PARA ENCENDER O APAGAR DIRECCIONAL DERECHA
        break;
      case 'o':
        Serial.println("Luces apagadas");
        data = B11111111;
        //# AGREGAR PARA CÓDIGO PARA ENCENDER O APAGAR TODAS
        break;
      case 'a':
        Serial.println("Luces encendidas");
        data = B00000011;
        //# AGREGAR PARA CÓDIGO PARA ENCENDER O APAGAR TODAS
        break;
      case 'x':
        Serial.println("Luces encendidas según sensor");
        if(vsensor == 0){ 
          data = B00000011;
        }else if(vsensor == 1){
          data = B11111111; 
        }
        
        
        break;
      /**
       * # AGREGAR CASOS CON EL FORMATO l[caracter]:valor;
       * SI SE DESEAN manejar otras salidas del registro de corrimiento
       */
      default:
        Serial.println("Ninguna de las anteriores");
        
        break;
    }
    //data VARIABLE QUE DEFINE CUALES LUCES SE ENCIENDEN Y CUALES SE APAGAN
    shiftOut(ab, clk, LSBFIRST, data);
  }
  else if (llave == "circle"){
    Serial.println("Círculo");
    Serial.println(valor);
    switch(valor.toInt()){
      case 1:
      digitalWrite(In1, LOW);
      digitalWrite(In2, HIGH);
      digitalWrite(In3, LOW);
      digitalWrite(In4, HIGH);
      analogWrite(EnA,1023);
      analogWrite(EnB,1023);
      delay (2000);
      digitalWrite(In1, LOW);
      digitalWrite(In2, LOW);
      digitalWrite(In3, LOW);
      digitalWrite(In4, LOW);
      break;
      case -1:
      digitalWrite(In1, HIGH);
      digitalWrite(In2, LOW);
      digitalWrite(In3, HIGH);
      digitalWrite(In4, LOW);
      analogWrite(EnA,1023);
      analogWrite(EnB,1023);
      delay (2000);
      digitalWrite(In1, LOW);
      digitalWrite(In2, LOW);
      digitalWrite(In3, LOW);
      digitalWrite(In4, LOW);
      break;  
      }
      
      
    }
    else if (llave == "infinite"){
    Serial.println("Infinito u Ocho");
    Serial.println(valor);
      digitalWrite(In1, LOW);
      digitalWrite(In2, HIGH);
      digitalWrite(In3, LOW);
      digitalWrite(In4, HIGH);
      analogWrite(EnA,900);
      analogWrite(EnB,1023);
      delay (2000);
      digitalWrite(In1, HIGH);
      digitalWrite(In2, LOW);
      digitalWrite(In3, HIGH);
      digitalWrite(In4, LOW);
      analogWrite(EnA,1023);
      analogWrite(EnB,950);
      delay (2000);
      digitalWrite(In1, LOW);
      digitalWrite(In2, LOW);
      digitalWrite(In3, LOW);
      digitalWrite(In4, LOW);  
  }
  else if (llave == "zigzag"){
    Serial.println("ZigZag");
    Serial.println(valor);
      digitalWrite(In1, LOW);
      digitalWrite(In2, HIGH);
      digitalWrite(In3, LOW);
      digitalWrite(In4, HIGH);
      analogWrite(EnA,900);
      analogWrite(EnB,1023);
      delay (600);
      digitalWrite(In1, HIGH);
      digitalWrite(In2, LOW);
      digitalWrite(In3, HIGH);
      digitalWrite(In4, LOW);
      analogWrite(EnA,1023);
      analogWrite(EnB,950);
      delay (600);
      digitalWrite(In1, LOW);
      digitalWrite(In2, HIGH);
      digitalWrite(In3, LOW);
      digitalWrite(In4, HIGH);
      analogWrite(EnA,900);
      analogWrite(EnB,1023);
      delay (600);
      digitalWrite(In1, HIGH);
      digitalWrite(In2, LOW);
      digitalWrite(In3, HIGH);
      digitalWrite(In4, LOW);
      analogWrite(EnA,1023);
      analogWrite(EnB,950);
      delay (600);
      digitalWrite(In1, LOW);
      digitalWrite(In2, LOW);
      digitalWrite(In3, LOW);
      digitalWrite(In4, LOW);
  }
  else if(llave == "especial"){
      Serial.println("Especial");
      Serial.println("¿Pasé el semestre?");
      digitalWrite(In1, LOW);
      digitalWrite(In2, HIGH);
      digitalWrite(In3, HIGH);
      digitalWrite(In4, LOW);
      analogWrite(EnA,1023);
      analogWrite(EnB,1023);
      delay (900);
      digitalWrite(In1, HIGH);
      digitalWrite(In2, LOW);
      digitalWrite(In3, LOW);
      digitalWrite(In4, HIGH);
      analogWrite(EnA,1023);
      analogWrite(EnB,1023);
      delay (900);
      digitalWrite(In1, LOW);
      digitalWrite(In2, HIGH);
      digitalWrite(In3, HIGH);
      digitalWrite(In4, LOW);
      analogWrite(EnA,1023);
      analogWrite(EnB,1023);
      delay (900);
      digitalWrite(In1, HIGH);
      digitalWrite(In2, LOW);
      digitalWrite(In3, LOW);
      digitalWrite(In4, HIGH);
      analogWrite(EnA,1023);
      analogWrite(EnB,1023);
      delay (900);
      digitalWrite(In1, LOW);
      digitalWrite(In2, HIGH);
      digitalWrite(In3, LOW);
      digitalWrite(In4, HIGH);
      analogWrite(EnA,900);
      analogWrite(EnB,1023);
      delay (900);
      digitalWrite(In1, HIGH);
      digitalWrite(In2, LOW);
      digitalWrite(In3, HIGH);
      digitalWrite(In4, LOW);
      analogWrite(EnA,1023);
      analogWrite(EnB,950);
      delay (900);
      digitalWrite(In1, LOW);
      digitalWrite(In2, LOW);
      digitalWrite(In3, LOW);
      digitalWrite(In4, LOW);
  }
  
  /**
   * El comando tiene el formato correcto pero no tiene sentido para el servidor
   */
  else{
    result = "Undefined key value: " + llave+";";
    Serial.println(result);
  }
  return result;
}

/**
 * Función para obtener los valores de telemetría del auto
 */
String getSense(){
  //# EDITAR CÓDIGO PARA LEER LOS VALORES DESEADOS
  int batteryLvl = -1;
  int light = -1;

  // EQUIVALENTE A UTILIZAR STR.FORMAT EN PYTHON, %d -> valor decimal
  char sense [16];
  sprintf(sense, "blvl:%d;ldr:%d;", batteryLvl, light);
  Serial.print("Sensing: ");
  Serial.println(sense);
  return sense;
}
