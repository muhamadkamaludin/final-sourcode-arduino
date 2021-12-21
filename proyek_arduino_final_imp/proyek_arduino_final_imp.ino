#include <SPI.h>                                      // Library komunikasi SPI
#include <Ethernet.h>                                 // Library Ethernet
#include <ArduinoJson.h>                              // Library Arduino JSON
#include <MFRC522.h>                                  // Library RFID
#include <Servo.h>                                    // Library Servo

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

EthernetClient client;          // Membuat instance baru dari library Ethernet 

int    HTTP_PORT   = 80;
String HTTP_METHOD = "GET";
char   HOST_NAME[] = "192.168.43.10";                 // Alamat IP Laptop
String PATH_NAME   = "/proyek_delameta/data-api.php"; // URL website

String getData;

// Variabel yang akan dikirim ke website
String lokasi  = "Tegal";
String golongan;;
//String golongan = "";
// Variabel yang digunakan oleh RFID
#define SS_PIN 8
#define RST_PIN 9

// Variabel masukan (input)
int infrared_back  = A1;    // Mendeteksi bagian belakang mobil
int infrared_front   = A0;  // Mendeteksi bagian depan mobil
int infrared_pass   = A2;   // Mendeteksi apakah kendaraan sudah lewat palang atau belum

// Variabel keluaran (output)
int buzzer    = 5;
int pinServo  = 6;
int ledRed    = 4;
int ledYellow = 3;
int ledGreen  = 2; 

// Variabel millis()
unsigned long millis_akhir = 0;

// Variabel tambahan
int kunci = 0;
int pass_awal = 0;
int intervalBuzzer = 100;
int countBuzzer = 0;

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Membuat instance baru dari library MFRC522 
Servo myservo;                  // Membuat instance baru dari library Servo

void setup() {
  Serial.begin(115200);

  // Mendeklarasikan pin Arduino sebagai input/output
  pinMode(infrared_back, INPUT);
  pinMode(infrared_front, INPUT);
  pinMode(infrared_pass, INPUT);
  pinMode(buzzer,OUTPUT);
  pinMode(ledRed, OUTPUT);
  pinMode(ledYellow, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  myservo.attach(pinServo);           // Mendeklarasikan pin servo sebagai output
  myservo.write(0);                   // Mengatur posisi awal servo 
  while(!Serial);
  SPI.begin();          // Initiate  SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522

  //START IP DHCP
  Serial.println("Konfigurasi DHCP, Silahkan Tunggu!");
  if(Ethernet.begin(mac) == 0){
    Serial.println("DHCP Gagal!");
    if(Ethernet.hardwareStatus() == EthernetNoHardware){
      Serial.println("Ethernet Tidak tereteksi :(");
    } 
    else if(Ethernet.linkStatus() == LinkOFF){
      Serial.println("Hubungkan kabel Ethernet!");
    }
    while (true){delay(1);}
  }  
  //End DHCP
   
  delay(5000); 
  Serial.print("IP address: ");
  Serial.println(Ethernet.localIP());  
  client.connect(HOST_NAME, HTTP_PORT);   // Menghubungkan ke alamat IP
  Serial.println("Siap Digunakan!");
}

void loop() {

  // Memanggil millis sebagai pengganti delay()
  unsigned long millis_awal = millis();
  // Baca data Infrared
  int bacaInfraredBack   = map(digitalRead(infrared_back), 1, 0, 0, 1);
  int bacaInfraredFront  = map(digitalRead(infrared_front), 1, 0, 0, 1);
  int bacaInfraredPass   = map(digitalRead(infrared_pass), 1, 0, 0, 1);
  int pass_akhir = bacaInfraredPass;
  
  if(kunci == 0){                       // Mengaktifkan langkah 1
    // LED awal
    kondisiLED(0, 1, 0);
    cek_golongan(bacaInfraredFront, bacaInfraredBack);
  }
//  Serial.println("Kunci :" + String(kunci));
//  Serial.println("Gol :" + String(golongan));
//  Serial.println("IR :" + String(bacaInfraredPass)); 
//  delay(2000);
  if(kunci == 1){                      // Start if kunci, Mengaktifkan langkah 2
    
    if(!mfrc522.PICC_IsNewCardPresent()){return;}
    if(!mfrc522.PICC_ReadCardSerial()){return;}
  
    // Membaca dan menampilkan UID tag pada Serial Monitor
    Serial.println("UID tag :");
    // variabel RFID
    String uidString;
    byte letter;
    for (byte i = 0; i < mfrc522.uid.size; i++){
       uidString.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "" : ""));
       uidString.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    Serial.print("Message : ");
    uidString.toUpperCase();
    Serial.println(uidString);
    digitalWrite(buzzer, HIGH);
    delay(100);
    digitalWrite(buzzer, LOW);
  //  Serial.print("Nama tol :" + String(lokasi));
  //  Serial.print("Nomor golongan :" + String(golongan));
    
    // Mem-POST-kan data ke website
    client.connect(HOST_NAME, HTTP_PORT);
    client.println(HTTP_METHOD + " " + PATH_NAME + 
                   "?rfid=" + String(uidString) + 
                   "&lokasi=" + String(lokasi) +
                   "&golongan=" + String(golongan)+
                   " HTTP/1.1");
    client.println("Host: " + String(HOST_NAME));
    client.println("Connection: close");
    client.println(); 
    // end HTTP header
    
    while(client.connected()) {         // Start client.connected()
      if(client.available()){           // Start client.avaliable()
        char endOfHeaders[] = "\r\n\r\n";
        client.find(endOfHeaders);
        getData = client.readString();
        getData.trim();
        Serial.println(getData);
          
          
        // meng-GET-kan data dari JSON
        const size_t capacity = JSON_OBJECT_SIZE(8) + 160; //cari dulu nilainya pakai Arduino Json 5 Asisten
        DynamicJsonDocument doc(capacity);
        // StaticJsonDocument<192> doc;
        DeserializationError error = deserializeJson(doc, getData);
        //const size_t capacity = JSON_OBJECT_SIZE(18) + 410;

        const char* id_rfid         = doc["id_rfid"]; // "2"
  //      const char* nama_lengkap  = doc["nama_lengkap"]; // "Benedetto"
        const char* saldo           = doc["saldo"]; // "4433000"
  //      const char* nomor_rfid    = doc["nomor_rfid"]; // "1ACE980"
        const char* nama_tol        = doc["nama_tol"]; // "Tegal"
  //      const char* id_pembayaran = doc["id_pembayaran"]; // "5058"
        const char* no_transaksi    = doc["no_transaksi"]; // "INVTOL 2 812 10 130204"
        const char* ambilStatus         = doc["status"]; // "Berhasil"
        const char* ambilKeterangan      = doc["keterangan"]; // "Transaksi berhasil"
        const char* tanggal         = doc["tanggal"]; // "2021-12-17 13:02:04"
        const char* tarif           = doc["tarif"]; // "2021-12-17 13:02:04"
        
  //      Serial.print("ID rfid     : ");Serial.println(String(id_rfid));
  ////      Serial.print("Nama lengkap: ");Serial.println(nama_lengkap);
  //      Serial.print("Saldo       : ");Serial.println(String(saldo));
  ////      Serial.print("Nomor rfid  : ");Serial.println(nomor_rfid);
  //      Serial.print("Nama tol    : ");Serial.println(String(nama_tol));
  ////      Serial.print("ID pembayaran: ");Serial.println(id_pembayaran);
  //      Serial.print("No transaksi : ");Serial.println(String(no_transaksi));
//        Serial.print("Status       : ");Serial.println(pstatus);
//        Serial.print("Keterangan   : ");Serial.println(keterangan);
  //      Serial.print("Tanggal      : ");Serial.println(String(tanggal));
  //      Serial.print("Tarif        : ");Serial.println(String(tarif));
  
  //      LOGIKA
        if(String(ambilStatus) == "Berhasil"){
          Serial.println("PALANG TERBUKA");
//          Serial.println("SALDO CUKUP");
//          Serial.print("Nama gerbang tol  : "); Serial.println(nama_tol);
//          Serial.print("Tanggal transaksi : "); Serial.println(tanggal);
//          Serial.print("Nomor transaksi   : "); Serial.println(no_transaksi);
//          Serial.print("Golongan          : "); Serial.println(golongan);
//          Serial.print("Tarif tol         : ");Serial.println(tarif);
//          Serial.print("Nomor RFID        : ");Serial.println(uidString);
//          Serial.print("Sisa Saldo        : ");Serial.println(saldo);
          myservo.write(90);
          kondisiLED(0, 0, 1);
          buzzer_berhasil();
          kunci = 2;
          Serial.print(kunci);
        }
        else if(String(ambilStatus) == "Saldo"){
          myservo.write(0);
          kondisiLED(1, 0, 0);
          buzzer_gagal();
          kondisiLED(0, 1, 0);
          Serial.println("Saldo Anda Tidak Cukup");
          //POST TO SERIAL
          Serial.print("Nomor RFID        : ");Serial.println(uidString);
          Serial.print("Sisa Saldo        : ");Serial.println(saldo);
        }
        else if(String(ambilStatus) == "Tol"){   
          myservo.write(0);
          kondisiLED(1, 0, 0);
          buzzer_gagal();
          kondisiLED(0, 1, 0);
          Serial.println("Tol Tidak Terdaftar!");
        }
        else if(String(ambilStatus) == "Gagal"){   
          myservo.write(0);
          kondisiLED(1, 0, 0);
          buzzer_gagal();
          kondisiLED(0, 1, 0);
          Serial.println("RFID Tidak terdaftar!");
          
        } 
        else{
          Serial.print("Semua perintah gagal.");
//          Serial.print("Status :"); Serial.print(ambilStatus);
        }   // End Else if
      }     // End client.avaliable() 
    }       // End client.connected()
  }         // End if kunci

  if(kunci == 2 && pass_akhir == 0 && pass_awal == 1){
      delay(500);
      myservo.write(0);
      buzzer_berhasil();
      Serial.println("PALANG TERTUTUP");
      kondisiLED(0, 1, 0);
      delay(1000);
      kunci = 0;
    }
    pass_awal = pass_akhir;
}
// Fungsi untuk mengecek golongan kendaraan
void cek_golongan(int ir_front, int ir_back){
  if(ir_front == 1 and ir_back == 1){
    golongan = "golongan_2";
    kunci = 1;
  }
  else if(ir_front == 1 and ir_back == 0){
    golongan = "golongan_1";
    kunci = 1;
  }  
}

void buzzer_berhasil(){
  digitalWrite(buzzer,HIGH);
  delay(100);
  digitalWrite(buzzer,LOW);
  delay(100);
  digitalWrite(buzzer,HIGH);
  delay(100);
  digitalWrite(buzzer,LOW);
  delay(100);
}

void buzzer_gagal(){
  digitalWrite(buzzer,HIGH);
  delay(1000);
  digitalWrite(buzzer,LOW);
  delay(1000);
}

void kondisiLED(int led1, int led2, int led3){
  digitalWrite(ledRed, led1);
  digitalWrite(ledYellow, led2);
  digitalWrite(ledGreen, led3);
}
