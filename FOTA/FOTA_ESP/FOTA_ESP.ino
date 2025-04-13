/*--------------------------------------------------------- Include Dependencies ---------------------------------------------------------------------*/
#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#elif __has_include(<WiFiNINA.h>)
#include <WiFiNINA.h>
#elif __has_include(<WiFi101.h>)
#include <WiFi101.h>
#elif __has_include(<WiFiS3.h>)
#include <WiFiS3.h>
#endif
#include <PubSubClient.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>// Provide the token generation process info.
#include <addons/SDHelper.h>  // Provide the SD card interfaces setting and mounting
#include <LittleFS.h>  // Include this instead of <SPIFFS.h>
#define SPIFFS LittleFS
/*----------------------------------------------------------------------------------------------------------------------------------------------------*/


/*--------------------------------------------------------- Special Setup Variable -------------------------------------------------------------------*/
/* 1. Define the WiFi credentials */
#define WIFI_SSID "OrangeDSL-Mohamed"
#define WIFI_PASSWORD "0822129799manar##"

/* 2. Define the API Key */
#define API_KEY "AIzaSyCDI5rFVdGg1fl4EFtihzFQ3x2t22Dlahc"

/* 3. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "mm4890448@gmail.com"
#define USER_PASSWORD "123456"

/* 4. Define the Firebase storage bucket ID e.g bucket-name.appspot.com */
#define STORAGE_BUCKET_ID "databaseforsport.appspot.com"

const char* mqtt_server = "test.mosquitto.org";
/*----------------------------------------------------------------------------------------------------------------------------------------------------*/


/*--------------------------------------------------------- macro functions ---------------------------------------------------------------------*/
// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
WiFiMulti multi;
#endif

//for mqtt
WiFiClient espClient;
PubSubClient client(espClient);
/*----------------------------------------------------------------------------------------------------------------------------------------------------*/


/*-------------------------------------------------------- Preprocessor Directives -------------------------------------------------------------------*/
#define BAUD_RATE                    115200

#define NACK_CODE                    0xAB
#define ACK_CODE                     0xCD

#define TIME_OUT_VALUE               2000

#define CBL_GO_TO_ADDR_CMD           0x14
#define CBL_FLASH_ERASE_CMD          0x15
#define CBL_MEM_WRITE_CMD            0x16

#define BL_HOST_BUFFER_RX_LENGTH     71
#define DATA_SIZE                    64

#define MSG_BUFFER_SIZE	             (50)
/*----------------------------------------------------------------------------------------------------------------------------------------------------*/


/*------------------------------------------------------------ Global Variables ----------------------------------------------------------------------*/

bool taskCompleted = false;     //for firebase

//for mqtt 
unsigned long lastMsg = 0;

char msg[MSG_BUFFER_SIZE];
int value = 0;

uint32_t flashAddress = 0x8008000;  // STM32 Flash start address

char packet[BL_HOST_BUFFER_RX_LENGTH];
char buf[3];
unsigned long start_time;
char ACK_BIT ;
uint8_t ack_packet[2] = {0};           // Assuming 2 bytes: 1 for ACK/NACK, 1 for message length

/*----------------------------------------------------------------------------------------------------------------------------------------------------*/


/*---------------------------------------------------------- Functions Definition --------------------------------------------------------------------*/
// The Firebase Storage download callback function

// The Firebase Storage download callback function
void fcsDownloadCallback(FCS_DownloadStatusInfo info)
{
    if (info.status == firebase_fcs_download_status_init)
    {
        Serial.printf("Downloading file %s (%d) to %s\n", info.remoteFileName.c_str(), info.fileSize, info.localFileName.c_str());
    }
    else if (info.status == firebase_fcs_download_status_download)
    {
        Serial.printf("Downloaded %d%s, Elapsed time %d ms\n", (int)info.progress, "%", info.elapsedTime);
    }
    else if (info.status == firebase_fcs_download_status_complete)
    {
        Serial.println("Download completed\n");
    }
    else if (info.status == firebase_fcs_download_status_error)
    {
        Serial.printf("Download failed, %s\n", info.errorMsg.c_str());
    }
}

//reconnect to mqtt server
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("FOTA_SEND");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

//call back function from mqtt
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  if (String(topic) == "FOTA_SEND") {
    client.publish("FOTA_RECEIVE", message.c_str());
  }

  if(message.equals("50")){
    Serial.println("download code");
    client.publish("FOTA_RECEIVE", "download code");
    BL_memory_write();
  }else if(message.equals("100")){
    Serial.println("enter jump to application");
    client.publish("FOTA_RECEIVE", "enter jump to application");
    BL_jump_to_address(flashAddress);
  }else if(message.equals("20")){
    Serial.println("erase flash from sector two");
    client.publish("FOTA_RECEIVE", "erase flash from sector two");
    BL_erase_from_sector_two(flashAddress);
  }else if(message.equals("10")){
    Serial.println("flash mass erase");
    client.publish("FOTA_RECEIVE", "flash mass erase");
    BL_mass_erase();
  }
  
}

/*************************write data to mamory***************************/
void BL_memory_write(){

    // Open file in read mode
    uint32_t flashAddresss = 0x8008000;  // STM32 Flash start address
    File file = SPIFFS.open("/FOTA_TEST_F.txt", "r");  
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }

    while (file.available()) {

        //Prepare Packet Header
        packet[0] = 71;                                     // Packet size (71 bytes)
        packet[1] = 0x16;                                   // Command
        *((uint32_t *)(packet + 2)) = flashAddresss;
        packet[6] = DATA_SIZE;  // Data size (64 bytes)

        //local variables 
        char counter = 0;
        char var = 0xff;


         while (counter < 64){
			memcpy(buf, &var, 2);   
			file.readBytes((char*)buf, 2);
			char HexData = strtol((const char*)buf, NULL, 16);
			packet[7 + counter] = HexData;
            Serial.printf("packet[%i] = 0x%X \n",counter+7,packet[7 + counter]);
			counter++;
        }

        counter = 0;
		Serial1.write(packet, sizeof(packet));
        Serial1.flush();  // Ensures all data is sent before waiting for ACK
        Serial.println("Waiting for ACK...");

        ACK_BIT = ACK_receive();
        if(1 == ACK_BIT){
            flashAddresss += DATA_SIZE;
            delay(20);
        }
    }
    //Serial.println("File transfer complete!");
    file.close();    
}

/*************************recieve acknowledge ***************************/
uint8_t ACK_receive(){
	start_time = millis();
    while (Serial.available() < 1) 
    {     
		if (millis() - start_time > TIME_OUT_VALUE) 
        { 
			Serial.println("Timeout waiting for ACK packet!");
			return 0;
		}
	}
    Serial.readBytes(&ack_packet[0], 1);  // Read the first byte (ACK or NACK)
	// Check ACK/NACK
    switch (ack_packet[0])
    {
        case ACK_CODE:
        Serial.println("Received ACK");       
        while (Serial.available() < 1) 
        {     
            if (millis() - start_time > TIME_OUT_VALUE) 
            { 
                Serial.println("Timeout waiting for Message Length!");
                return 0;
            }
        }
        Serial.readBytes(&ack_packet[1], 1);      // Read the second byte (message length)
        break;

    case NACK_CODE:
        Serial.println("Received NACK!");
        return 3;     // Return from the hole function
        break;

    default:
        Serial.println("Error in receiving ACK/NACK from BootLoader!");
        return 2;     // Return from the hole function
    }
    return 1;
    
}

/*****************************jump to app *******************************/

void BL_jump_to_address(uint32_t address){

    packet[0] = 6; // Command
    packet[1] = 0x14;
    *((uint32_t *)(packet + 2)) = address;

    Serial.println("Sending command packet:");
	for (size_t i = 0; i < 6; i++) 
    {
		Serial.printf("0x%02X ", packet[i]);
	}
	Serial.println();

	// Sending the entire packet over Serial2
	Serial1.write(packet, 6);
    Serial1.flush();  // Ensures all data is sent before waiting for ACK

    Serial.println("Waiting for ACK...");
    ACK_BIT = ACK_receive();
}

/*****************************erese from sector two *******************************/
void BL_erase_from_sector_two(uint32_t address){
    packet[0] = 4; // Command
    packet[1] = 0x15;
    packet[2] = 0x02;
    packet[3] = 0x09;

    Serial.println("Sending command packet:");
	for (size_t i = 0; i < 4; i++) 
    {
		Serial.printf("0x%02X ", packet[i]);
	}
	Serial.println();

	// Sending the entire packet over Serial2
	Serial1.write(packet, 4);
    Serial1.flush();  // Ensures all data is sent before waiting for ACK

    Serial.println("Waiting for ACK...");
    ACK_BIT = ACK_receive();
}


/*****************************erese from sector two *******************************/

void BL_mass_erase(void){

    packet[0] = 4; // Command
    packet[1] = 0x15;
    packet[2] = 0xff;
    packet[3] = 0x09;

    Serial.println("Sending command packet:");
	for (size_t i = 0; i < 4; i++) 
    {
		Serial.printf("0x%02X ", packet[i]);
	}
	Serial.println();

	// Sending the entire packet over Serial2
	Serial1.write(packet, 4);
    Serial1.flush();  // Ensures all data is sent before waiting for ACK

    Serial.println("Waiting for ACK...");
    ACK_BIT = ACK_receive();
}

void clearUARTBuffer() {
    while (Serial.available()) {
        Serial.read(); // Discard any received bytes
    }
}


void setup()
{

    Serial.begin(BAUD_RATE);  // Corrected Serial.begin()
    Serial1.begin(BAUD_RATE); 
    // Initialize SPIFFS
    clearUARTBuffer();
    if (!SPIFFS.begin()) {
        Serial.println("SPIFFS Mount Failed! Formatting...");
        SPIFFS.format();
        if (!SPIFFS.begin()) {
            Serial.println("SPIFFS Mount Failed Again! Exiting...");
            return;
        }
    }
 #if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    multi.addAP(WIFI_SSID, WIFI_PASSWORD);
    multi.run();
 #else
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 #endif

    Serial.print("Connecting to Wi-Fi");
    unsigned long ms = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
 #if defined(ARDUINO_RASPBERRY_PI_PICO_W)
        if (millis() - ms > 10000)
            break;
 #endif
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

    /* Assign the api key (required) */
    config.api_key = API_KEY;

    /* Assign the user sign in credentials */
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;

    // The WiFi credentials are required for Pico W
    // due to it does not have reconnect feature.
 #if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    config.wifi.clearAP();
    config.wifi.addAP(WIFI_SSID, WIFI_PASSWORD);
 #endif

    /* Assign the callback function for the long running token generation task */
    config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

    // Comment or pass false value when WiFi reconnection will control by your code or third party library e.g. WiFiManager
    Firebase.reconnectNetwork(true);

    // Since v4.4.x, BearSSL engine was used, the SSL buffer need to be set.
    // Large data transmission may require larger RX buffer, otherwise connection issue or data read time out can be occurred.
    fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);

    /* Assign download buffer size in byte */
    // Data to be downloaded will read as multiple chunks with this size, to compromise between speed and memory used for buffering.
    // The memory from external SRAM/PSRAM will not use in the TCP client internal rx buffer.
    config.fcs.download_buffer_size = 2048;

    Firebase.begin(&config, &auth);

    // if use SD card, mount it.
    //SD_Card_Mounting(); // See src/addons/SDHelper.h

    //mqtt conection 
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
}



void loop()
{

    // Firebase.ready() should be called repeatedly to handle authentication tasks.
    if (Firebase.ready() && !taskCompleted)
    {
        taskCompleted = true;

        Serial.println("\nDownload file...\n");

        // The file systems for flash and SD/SDMMC can be changed in FirebaseFS.h.
        if (!Firebase.Storage.download(&fbdo, STORAGE_BUCKET_ID /* Firebase Storage bucket id */, "FOTA_TEST_F.txt" /* path of remote file stored in the bucket */, "/FOTA_TEST_F.txt" /* path to local file */, mem_storage_type_flash /* memory storage type, mem_storage_type_flash and mem_storage_type_sd */, fcsDownloadCallback /* callback function */))
            Serial.println(fbdo.errorReason());
    }


    if (!client.connected()) {
        reconnect();
    }
    client.loop();
    

}
