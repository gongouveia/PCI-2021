/*
  PCI 2020/2021 - Grupo 2A - António Caramelo, Gonçalo Gouveia, Manuel Mansilha

  Objetivo: - Máquina de lavar roupa com 4 programas diferentes
            - A máquina pode ser ligada/desligada com a tecla Play/Pause do comando
            - O utilizador pode interromper o programa a meio da execução, voltando ao menu inicial
  Refrências:
  - Manual de instruções Kunft KWM3485: https://www.manualpdf.pt/kunft/kwm3485/manual
  - Manual de Instruções Becken BoostWash BWM3640: https://www.manualpdf.pt/becken/boostwash-bwm3640/manual
  - Barra de progresso de programa no LCD (V3) (Programa alterado para se ajustar ao tempo do programa e barra de progressos menor que 16 carateres)
  https://www.carnetdumaker.net/articles/faire-une-barre-de-progression-avec-arduino-et-liquidcrystal/
  - Interfacing LCD1602 with Arduino: https://create.arduino.cc/projecthub/najad/interfacing-lcd1602-with-arduino-764ec4
*/

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%  LEDS, Buzzer e Sensores %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int led_agua = 6; // porta digital a que está ligado este LED representativo da válvula de entrada de água
int led_sabao = 7;  // porta digital a que está ligado o LED representativo da válvula de entrada de sabão
int led_saida = 1;  // porta digital a que está ligado o LED representativo da válvula de saída de água
int led_aquecimento = A4;  // porta analógica a que está ligado o LED representativo da válvula de saída
int buzzer = A3;  // porta digital a que está ligado o buzzer
int LM = A0;  // entrada analógica do arduino a está ligado o sensor de temperatura
int RECV_PIN = A5;  // pino de entrada analógica a que está ligado o sensor de infravermelhos

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%  Bibliotecas e inicializacao do display, recetor IR e motor de passo %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

#include <LiquidCrystal.h>  // configuracao do display
LiquidCrystal lcd(13, 12, 2, 3, 4, 5);  // portas a que está ligado o display

#include <IRremote.h>  // configuracao do recetor IR
IRrecv irrecv(RECV_PIN);  // Inicializa o recetor IR
decode_results results; // Inicializa o descodificador dos valores hexadecimais devolvidos pelo sensor

#include <Stepper.h>  // configuracao do motor
int pin1 = 8; // definir as portas digitais a que está ligado o motor
int pin2 = 9;
int pin3 = 10;
int pin4 = 11;
int StepsPerRevolution = 32; // valor catacterístico
Stepper small_stepper(StepsPerRevolution, pin1, pin3, pin2, pin4);

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%  Características da Máquina %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int velocidade_baixa = 200; // inicializacao das 4 velocidades possíveis de rotação
int velocidade_media = 400;
int velocidade_alta = 800;
int temp_normal = 60.0; // inicialização das temperaturas necessárias
int temp_baixa = 20.0;
int velocidade_centrifugar = 1024;
int passos = 20;  // passos em cada iteração (escolhemos 20 para o movimento ser fluido)
int loadBarSize = 13; // tamanho da barra de progresso
static byte percent = 0;  // byte para apresentar o símbolo de percentagem

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Variáveis para ciclos %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

boolean ligado = false; // para saber se a máquina está ligada ou não
boolean escolheu = false; // para saber se o utilizador escolheu uma opção
unsigned long tempo_inicio = 0; // para guardar o instante em que cada ciclo começa
String numero;  // variavel que guarda o numero selecionado no comando
String imprimir;  // dígitos que serão imrpressos

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Métodos/Funções %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void setup() {

  pinMode(led_agua, OUTPUT);  // definir os LEDs como OUTPUT
  pinMode(led_sabao, OUTPUT);
  pinMode(led_saida, OUTPUT);
  pinMode(led_aquecimento, OUTPUT);
  pinMode(buzzer, OUTPUT);  // definir o buzzer como output
  pinMode(LM, INPUT); // definir o sensor de temperatura como INPUT

  lcd.begin(16, 2); // inicializar o display
  setup_progressbar();  // setup da barra de progresso
  lcd.clear();  // limpar o conteúdo do ecrã
  irrecv.enableIRIn(); // Inicia o recetor IR

}

void loop() {

  while (!ligado) { // enquanto o utilizador não ligar a máquina
    if (irrecv.decode(&results) && results.value == 16761405) { // se clicar na tecla Play/Pause do comando
      ligado = true;  // liga a maquina
      lcd.setCursor(3, 1);
      lcd.print("A ligar...");  // imprime mensagem no display
      delay(2000);
      irrecv.resume();  // aguarda novo sinal IR
    }
    irrecv.resume();
  }

  while (ligado) {
    irrecv.enableIRIn(); // Inicia o recetor IR
    lcd.clear();  // limpa o display
    menuInicial();  // apresenta o menu inicial
    for (int scroll = 1; scroll <= 60; scroll++) {
      if (scroll==60){
        rotinaDesligar("A desligar!"); // desliga a maquina e todos os seus componentes
        ligado = false;
      }
      lcd.scrollDisplayLeft();  // desloca o texto para a esquerda

      if (irrecv.decode(&results)) { // se receber um código
        if (results.value == 16724175) { // tecla 1 do comando
          normal();
        }
        else if (results.value == 16718055) { // tecla 2 do comando
          rapido();
        }
        else if (results.value == 16743045) { // tecla 3 do comando
          delicados();
        }
        else if (results.value == 16716015) { // tecla 4 do comando
          centrifugacao();
        }
        else if (results.value == 16761405) { // tecla 0 do comando
          rotinaDesligar("A desligar!"); // desliga a maquina e todos os seus componentes
          ligado = false;
        }
        irrecv.resume();
        break;
      }
      irrecv.resume();
      delay(750);
    }
  }

}

void menuInicial() {  // método para o menu inicial
  lcd.clear();  // limpa o ecrã
  lcd.setCursor(3, 0);
  lcd.print("1-Normal 3-Delicados Play/Pause-Quit");  // vários programas
  lcd.setCursor(3, 1);
  lcd.print("2-Rapido 4-Centrifugar");
}

void normal() { // programa normal- Duração de 2min para corresponder aos 60min reais

  lcd.clear();
  tempo_inicio = millis();  // guarda o tempo de início do programa
  boolean executar = true;
  irrecv.resume();  // prepara o sensor para novo sinal
  while (executar) { // enquanto não acabar
    if (irrecv.decode(&results)) {
      if (results.value == 16738455) {  // tecla 0 do comando
        executar = false;
        ledDisplay(0, 0, 0);  // LEDS representativos das válvulas apagados
        digitalWrite(led_aquecimento, LOW); // led representativo do aquecimento apagado
        lcd.clear();
        lcd.setCursor(3, 1);
        lcd.print("Aguarde...");  // tempo de espera de 2s para simular a saída de água do tambor e o desligar de todos os componentes
        delay(2000);
        lcd.clear();
        lcd.setCursor(3, 1);
        lcd.print("TERMINADO"); // apresenta a mensagem
        apito();  // aviso sonoro
        break;
      }
    }
    lcd.setCursor(0, 0);
    lcd.print("Normal    0-Sair");  // 1ª linha do display
    if (millis() - tempo_inicio < 10000) { // adiciona água
      rotinaControlo(temp_normal, 0, 0); // método que controla o LED de aquecimento, a velocidade do motor e a direção
      ledDisplay(1, 0, 0);  // controla os LEDs das válvulas (Azul (água), Verde (sabão), Azul (saída))
      desenhoBarraProgresso((millis() - tempo_inicio) / 1200); // apresenta barra de progresso
    }

    else if (millis() - tempo_inicio >= 10000 && millis() - tempo_inicio < 25000) { // envolve roupa na água
      ledDisplay(0, 0, 0);
      rotinaControlo(temp_normal, velocidade_media, -passos);
      desenhoBarraProgresso((millis() - tempo_inicio) / 1200);
    }

    else if (millis() - tempo_inicio >= 25000 && millis() - tempo_inicio < 30000) { // adiciona sabão
      ledDisplay(0, 1, 0);
      rotinaControlo(temp_normal, 0, 0);
      desenhoBarraProgresso((millis() - tempo_inicio) / 1200);
    }

    else if (millis() - tempo_inicio >= 30000 && millis() - tempo_inicio < 45000) { // envolve a roupa na água com sabão

      ledDisplay(0, 0, 0);
      rotinaControlo(temp_normal, velocidade_alta, passos);
      desenhoBarraProgresso((millis() - tempo_inicio) / 1200);
    }

    else if (millis() - tempo_inicio >= 45000 && millis() - tempo_inicio < 55000) { // inverte o sentido da rotação
      ledDisplay(0, 0, 0);
      rotinaControlo(temp_normal, velocidade_alta, -passos);
      desenhoBarraProgresso((millis() - tempo_inicio) / 1200);
    }

    else if (millis() - tempo_inicio >= 55000 && millis() - tempo_inicio < 65000) { // abre válvula de saída
      ledDisplay(0, 0, 1);
      rotinaControlo(temp_normal, 0, passos);
      desenhoBarraProgresso((millis() - tempo_inicio) / 1200);
    }

    else if (millis() - tempo_inicio >= 65000 && millis() - tempo_inicio < 75000) { // adiciona água novamente para retirar  os vestígios de sabão
      rotinaControlo(temp_normal, 0, 0);
      ledDisplay(1, 0, 0);
      desenhoBarraProgresso((millis() - tempo_inicio) / 1200);
    }
    else if (millis() - tempo_inicio >= 75000 && millis() - tempo_inicio < 85000) { // envolve roupa na água
      ledDisplay(0, 0, 0);
      rotinaControlo(temp_normal, velocidade_alta, -passos);
      desenhoBarraProgresso((millis() - tempo_inicio) / 1200);
    }
    else if (millis() - tempo_inicio >= 85000 && millis() - tempo_inicio < 95000) { // inverte o sentido de rotação
      ledDisplay(0, 0, 0);
      rotinaControlo(temp_normal, velocidade_alta, passos);
      desenhoBarraProgresso((millis() - tempo_inicio) / 1200);
    }
    else if (millis() - tempo_inicio >= 95000 && millis() - tempo_inicio < 105000) { // abre válvula de saída (tirar a água)
      ledDisplay(0, 0, 1);
      rotinaControlo(temp_normal, 0, 0);
      desenhoBarraProgresso((millis() - tempo_inicio) / 1200);
    }
    else if (millis() - tempo_inicio >= 105000 && millis() - tempo_inicio <= 120000) { // centrifugação
      ledDisplay(0, 0, 0);
      rotinaControlo(NULL, velocidade_centrifugar, passos);
      desenhoBarraProgresso((millis() - tempo_inicio) / 1200);
    }
    else {  // quando o programa chega ao fim
      executar = false;
      digitalWrite(led_aquecimento,LOW);
      ledDisplay(0, 0, 0);  // apaga os LEDs todos
      lcd.clear();
      lcd.setCursor(3, 1);
      lcd.print("TERMINADO");
      apito();  // sinal sonoro
    }

  }
}

void rapido() { // programa rápido- Duração de 30s para corresponder aos 15min reais
  lcd.clear();  // mesma estrutura que no programa normal
  tempo_inicio = millis();
  boolean executar = true;
  irrecv.resume();
  while (executar) {
    if (irrecv.decode(&results)) {
      if (results.value == 16738455) {
        executar = false;
        ledDisplay(0, 0, 0);  // LEDS representativos das válvulas apagados
        digitalWrite(led_aquecimento, LOW); // led representativo do aquecimento apagado
        lcd.clear();
        lcd.setCursor(3, 1);
        lcd.print("Aguarde...");  // tempo de espera de 2s para simular a saída de água do tambor e o desligar de todos os componentes
        delay(2000);
        lcd.clear();
        lcd.setCursor(3, 1);
        lcd.print("TERMINADO"); // apresenta a mensagem
        apito();  // aviso sonoro
        break;
      }
    }
    lcd.setCursor(0, 0);
    lcd.print("Rapido    0-Sair");
    // Usamos temperatura limite do tipo NULL, pois a lavagem é efetuada a frio, ou seja, a resistência de aquecimento não liga
    if (millis() - tempo_inicio < 4000) { // adiciona água
      rotinaControlo(NULL, 0, 0);
      ledDisplay(1, 0, 0);
      desenhoBarraProgresso((millis() - tempo_inicio) / 300);
    }

    else if (millis() - tempo_inicio >= 4000 && millis() - tempo_inicio < 7000) { // envolve roupa na água
      ledDisplay(0, 0, 0);
      rotinaControlo(NULL, velocidade_media, -passos);
      desenhoBarraProgresso((millis() - tempo_inicio) / 300);
    }

    else if (millis() - tempo_inicio >= 7000 && millis() - tempo_inicio < 9500) { // adiciona sabão
      ledDisplay(0, 1, 0);
      rotinaControlo(NULL, 0, 0);
      desenhoBarraProgresso((millis() - tempo_inicio) / 300);
    }

    else if (millis() - tempo_inicio >= 9500 && millis() - tempo_inicio < 12500) { // envolve a roupa na água com sabão

      ledDisplay(0, 0, 0);
      rotinaControlo(NULL, velocidade_alta, passos);
      desenhoBarraProgresso((millis() - tempo_inicio) / 300);
    }

    else if (millis() - tempo_inicio >= 12500 && millis() - tempo_inicio < 15000) { // inverte o sentido da rotação
      ledDisplay(0, 0, 0);
      rotinaControlo(NULL, velocidade_alta, -passos);
      desenhoBarraProgresso((millis() - tempo_inicio) / 300);
    }

    else if (millis() - tempo_inicio >= 15000 && millis() - tempo_inicio < 17500) { // abre válvula de saída
      ledDisplay(0, 0, 1);
      rotinaControlo(NULL, 0, passos);
      desenhoBarraProgresso((millis() - tempo_inicio) / 300);
    }

    else if (millis() - tempo_inicio >= 17500 && millis() - tempo_inicio < 19000) { // adiciona água novamente para retirar  os vestígios de sabão
      rotinaControlo(NULL, 0, 0);
      ledDisplay(1, 0, 0);
      desenhoBarraProgresso((millis() - tempo_inicio) / 300);
    }
    else if (millis() - tempo_inicio >= 1900 && millis() - tempo_inicio < 21500) { // envolve roupa na água
      ledDisplay(0, 0, 0);
      rotinaControlo(NULL, velocidade_alta, -passos);
      desenhoBarraProgresso((millis() - tempo_inicio) / 300);
    }
    else if (millis() - tempo_inicio >= 21500 && millis() - tempo_inicio < 23000) { // inverte o sentido de rotação
      ledDisplay(0, 0, 0);
      rotinaControlo(NULL, velocidade_alta, passos);
      desenhoBarraProgresso((millis() - tempo_inicio) / 300);
    }
    else if (millis() - tempo_inicio >= 23000 && millis() - tempo_inicio < 25500) { // abre válvula de saída (tirar a água)
      ledDisplay(0, 0, 1);
      rotinaControlo(NULL, 0, 0);
      desenhoBarraProgresso((millis() - tempo_inicio) / 300);
    }
    else if (millis() - tempo_inicio >= 25500 && millis() - tempo_inicio <= 30000) { // centrifugação
      ledDisplay(0, 0, 0);
      rotinaControlo(NULL, velocidade_centrifugar, passos);
      desenhoBarraProgresso((millis() - tempo_inicio) / 300);
    }
    else {  // quando programa acabar
      executar = false;
      digitalWrite(led_aquecimento,LOW);
      ledDisplay(0, 0, 0);  // apaga os LEDs todos
      lcd.clear();
      lcd.setCursor(3, 1);
      lcd.print("TERMINADO");
      apito();  // sinal sonoro
    }

  }
}

void delicados() {  // programa "Delicados"- Duração de 100s para corresponder aos 49min reais
  lcd.clear();  // mesma estrutura que o programa normal
  tempo_inicio = millis();
  boolean executar = true;
  irrecv.resume();
  while (executar) {
    if (irrecv.decode(&results)) {
      if (results.value == 16738455) {
        executar = false;
        ledDisplay(0, 0, 0);  // LEDS representativos das válvulas apagados
        digitalWrite(led_aquecimento, LOW); // led representativo do aquecimento apagado
        lcd.clear();
        lcd.setCursor(3, 1);
        lcd.print("Aguarde...");  // tempo de espera de 2s para simular a saída de água do tambor e o desligar de todos os componentes
        delay(2000);
        lcd.clear();
        lcd.setCursor(3, 1);
        lcd.print("TERMINADO"); // apresenta a mensagem
        apito();  // aviso sonoro
        break;
      }
    }
    lcd.setCursor(0, 0);
    lcd.print("Delicados 0-Sair");
    if (millis() - tempo_inicio < 10000) { // adiciona água
      rotinaControlo(temp_baixa, 0, 0);
      ledDisplay(1, 0, 0);
      desenhoBarraProgresso((millis() - tempo_inicio) / 1200);
    }

    else if (millis() - tempo_inicio >= 10000 && millis() - tempo_inicio < 20000) { // envolve roupa na água
      ledDisplay(0, 0, 0);
      rotinaControlo(temp_baixa, velocidade_baixa, -passos);
      desenhoBarraProgresso((millis() - tempo_inicio) / 1200);
    }

    else if (millis() - tempo_inicio >= 20000 && millis() - tempo_inicio < 25000) { // adiciona sabão
      ledDisplay(0, 1, 0);
      rotinaControlo(temp_baixa, 0, 0);
      desenhoBarraProgresso((millis() - tempo_inicio) / 1200);
    }

    else if (millis() - tempo_inicio >= 25000 && millis() - tempo_inicio < 35000) { // envolve a roupa na água com sabão

      ledDisplay(0, 0, 0);
      rotinaControlo(temp_baixa, velocidade_baixa, passos);
      desenhoBarraProgresso((millis() - tempo_inicio) / 1200);
    }

    else if (millis() - tempo_inicio >= 35000 && millis() - tempo_inicio < 45000) { // inverte o sentido da rotação
      ledDisplay(0, 0, 0);
      rotinaControlo(temp_baixa, velocidade_baixa, -passos);
      desenhoBarraProgresso((millis() - tempo_inicio) / 1200);
    }

    else if (millis() - tempo_inicio >= 45000 && millis() - tempo_inicio < 55000) { // abre válvula de saída
      ledDisplay(0, 0, 1);
      rotinaControlo(temp_baixa, 0, passos);
      desenhoBarraProgresso((millis() - tempo_inicio) / 1200);
    }

    else if (millis() - tempo_inicio >= 55000 && millis() - tempo_inicio < 65000) { // adiciona água novamente para retirar  os vestígios de sabão
      rotinaControlo(temp_normal, 0, 0);
      ledDisplay(1, 0, 0);
      desenhoBarraProgresso((millis() - tempo_inicio) / 1200);
    }
    else if (millis() - tempo_inicio >= 65000 && millis() - tempo_inicio < 72500) { // envolve roupa na água
      ledDisplay(0, 0, 0);
      rotinaControlo(temp_baixa, velocidade_baixa, -passos);
      desenhoBarraProgresso((millis() - tempo_inicio) / 1200);
    }
    else if (millis() - tempo_inicio >= 72500 && millis() - tempo_inicio < 8000) { // inverte o sentido de rotação
      ledDisplay(0, 0, 0);
      rotinaControlo(temp_baixa, velocidade_baixa, passos);
      desenhoBarraProgresso((millis() - tempo_inicio) / 1200);
    }
    else if (millis() - tempo_inicio >= 80000 && millis() - tempo_inicio < 900000) { // abre válvula de saída (tirar a água)
      ledDisplay(0, 0, 1);
      rotinaControlo(temp_baixa, 0, 0);
      desenhoBarraProgresso((millis() - tempo_inicio) / 1200);
    }
    else if (millis() - tempo_inicio >= 90000 && millis() - tempo_inicio <= 100000) { // centrifugação
      ledDisplay(0, 0, 0);
      rotinaControlo(NULL, velocidade_media, passos);
      desenhoBarraProgresso((millis() - tempo_inicio) / 1200);
    }
    else {  // quando programa acabar
      executar = false;
      digitalWrite(led_aquecimento,LOW);
      ledDisplay(0, 0, 0);  // apaga os LEDs todos
      lcd.clear();
      lcd.setCursor(3, 1);
      lcd.print("TERMINADO");
      apito();  // sinal sonoro
    }

  }
}

void centrifugacao() {  // programa de centrifugação- Duração de 24s para corresponder aos 12min reais
  lcd.clear();  // mesma estrutura que os outros programas
  tempo_inicio = millis();
  boolean executar = true;
  irrecv.resume();
  while (executar) {
    if (irrecv.decode(&results)) {
      if (results.value == 16738455) {
        executar = false;
        ledDisplay(0, 0, 0);  // LEDS representativos das válvulas apagados
        digitalWrite(led_aquecimento, LOW); // led representativo do aquecimento apagado
        lcd.clear();
        lcd.setCursor(3, 1);
        lcd.print("Aguarde...");  // tempo de espera de 2s para simular a saída de água do tambor e o desligar de todos os componentes
        delay(2000);
        lcd.clear();
        lcd.setCursor(3, 1);
        lcd.print("TERMINADO"); // apresenta a mensagem
        apito();  // aviso sonoro
        break;
      }
    }
    lcd.setCursor(0, 0);
    lcd.print("Centrif.  0-Sair");
    if (millis() - tempo_inicio < 12000) { // centrifugação a uma dada velocidade
      rotinaControlo(NULL, velocidade_alta, passos);
      ledDisplay(0, 0, 0);
      desenhoBarraProgresso((millis() - tempo_inicio) / 240);
    }

    else if (millis() - tempo_inicio >= 12000 && millis() - tempo_inicio < 24000) { //aumenta a velocidade de rotação
      ledDisplay(0, 0, 0);
      rotinaControlo(temp_baixa, velocidade_centrifugar, passos);
      desenhoBarraProgresso((millis() - tempo_inicio) / 240);
    }
    else {
      executar = false;
      ledDisplay(0, 0, 0);  // apaga os LEDs todos
      lcd.clear();
      lcd.setCursor(3, 1);
      lcd.print("TERMINADO");
      apito();  // sinal sonoro
    }

  }
}

void ledDisplay(boolean agua, boolean sabao, boolean saida) { // método para controlar os LEDs das válvulas
  digitalWrite(led_agua, agua); // Acende/Apaga led da válvula de água
  digitalWrite(led_sabao, sabao); // Acende/Apaga led da válvula de sabão
  digitalWrite(led_saida, saida); // Acende/Apaga led da válvula de saída
}

void termorregulador(float temp_limiar) { // método para controlar temperatura
  if (temp_limiar != NULL) {  // se o valor limite não for nulo
    /*
      // Para o LM35 do António e do Gonçalo
      int val = analogRead(LM);  // valor devolvido pelo sensor
      float mv = (val/1023.0)*5000; // conversao para milivolts
      float celsius = mv/10;  // conversao para graus (LM35: 10mV/ºC)
    */
    // Para o LM335 do Manuel
    int val = analogRead(LM); // valor devolvido pelo LM335
    float mv = (val / 1023.0) * 5000; // conversao para milivolts
    float kelvin = 0.98 * (mv / 10); // conversao para kelvil (fator 0.98 para a temperatura medida ser adequada)
    float celsius = kelvin - 273.15; // conversao para graus celsius

    if (celsius > temp_limiar) { // se a temperatura estiver acima do limite definido
      digitalWrite(led_aquecimento, LOW);
    } else {  // se estiver abaixo
      digitalWrite(led_aquecimento, HIGH);
    }
  }
  else {  // se a temperatura limite for do tipo NULL
    digitalWrite(led_aquecimento, LOW);
  }
}

void apito() {  // método para o sinal sonoro
  tone(buzzer, 1000); // Sinal sonoro de 1kHz
  delay(1000);        // duração de 1s
  noTone(buzzer);     // Pára o sinal
  delay(1000);        // duração de 1s
  tone(buzzer, 1000);
  delay(1000);
  noTone(buzzer);
  delay(1000);
  tone(buzzer, 1000);
  delay(1000);
  noTone(buzzer);
  delay(1000);
}

void rotinaControlo(float tempReg, int veloc, int passos) {  // método para controlar o led de aquecimento e o motor

  termorregulador(tempReg);
  small_stepper.setSpeed(veloc);
  small_stepper.step(-passos);
}

void rotinaDesligar(String LCDdisp) {  // método para desligar a máquina
  ledDisplay(0, 0, 0);  // desliga os LEDs
  digitalWrite(led_aquecimento, LOW);
  lcd.clear();  // limpa o ecrã
  lcd.setCursor(3, 1);
  lcd.print(LCDdisp); // apresenta mensagem ao utilizador
  delay(2000);
  lcd.clear();  // e após 2s apaga
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%  Barra de Progresso %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

byte START_DIV_0_OF_1[8] = {  // bytes necessários para o grafismo da barra
  B01111,
  B11000,
  B10000,
  B10000,
  B10000,
  B10000,
  B11000,
  B01111
};

byte START_DIV_1_OF_1[8] = {
  B01111,
  B11000,
  B10011,
  B10111,
  B10111,
  B10011,
  B11000,
  B01111
};

byte DIV_0_OF_2[8] = {
  B11111,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111
};

byte DIV_1_OF_2[8] = {
  B11111,
  B00000,
  B11000,
  B11000,
  B11000,
  B11000,
  B00000,
  B11111
};

byte DIV_2_OF_2[8] = {
  B11111,
  B00000,
  B11011,
  B11011,
  B11011,
  B11011,
  B00000,
  B11111
};

byte END_DIV_0_OF_1[8] = {
  B11110,
  B00011,
  B00001,
  B00001,
  B00001,
  B00001,
  B00011,
  B11110
};

byte END_DIV_1_OF_1[8] = {
  B11110,
  B00011,
  B11001,
  B11101,
  B11101,
  B11001,
  B00011,
  B11110
};


void setup_progressbar() {  // configuração da barra de progresso
  lcd.createChar(0, START_DIV_0_OF_1);
  lcd.createChar(1, START_DIV_1_OF_1);
  lcd.createChar(2, DIV_0_OF_2);
  lcd.createChar(3, DIV_1_OF_2);
  lcd.createChar(4, DIV_2_OF_2);
  lcd.createChar(5, END_DIV_0_OF_1);
  lcd.createChar(6, END_DIV_1_OF_1);
}

void desenhoBarraProgresso(byte percent) { // imprimir barra de progresso no display
  lcd.setCursor(loadBarSize, 1);
  lcd.print(percent); // byte de percentagem
  lcd.print(F("%"));
  lcd.setCursor(0, 1);  // imprime na linha inferior
  byte nb_columns = map(percent, 0, 100, 0, loadBarSize * 2 - 2);
  for (byte i = 0; i < loadBarSize; ++i) {

    if (i == 0) {
      if (nb_columns > 0) {
        lcd.write(1);
        nb_columns -= 1;

      } else {
        lcd.write((byte) 0);
      }

    } else if (i == loadBarSize - 1) {
      if (nb_columns > 0) {
        lcd.write(6);

      } else {
        lcd.write(5);
      }

    } else {

      if (nb_columns >= 2) {
        lcd.write(4);
        nb_columns -= 2;

      } else if (nb_columns == 1) {
        lcd.write(3);
        nb_columns -= 1;

      } else {
        lcd.write(2);
      }
    }
  }
}
