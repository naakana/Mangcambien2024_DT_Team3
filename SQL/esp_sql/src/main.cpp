#include <Arduino.h>
#include <WiFi.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define DHTPIN 4      
#define DHTTYPE DHT11
#define led_pin 5
int id = 1; 
float temperature; // Giá trị nhiệt độ
float humidity;    // Giá trị độ ẩm
DHT dht11_sensor(DHTPIN, DHTTYPE);


const char* ssid = "SITP 2.4Ghz";
const char* password = "kocop@ss2";

void setup() {
    Serial.begin(115200);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Đang kết nối đến WiFi...");
    }
    Serial.println("Đã kết nối đến WiFi");
    dht11_sensor.begin();
    pinMode(led_pin, OUTPUT);
}
void send_data() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;

        // Gửi yêu cầu POST đến server
        http.begin("http://192.168.1.74:5000/receive_data");
        http.addHeader("Content-Type", "application/json"); 

        String payload = "{\"id\":" + String(id) + ", \"temperature\":" + String(temperature) + ", \"humidity\":" + String(humidity) + "}" ;
        Serial.println("Sending payload: " + payload);
        int httpResponseCode = http.POST(payload); 

        if (httpResponseCode > 0) {
            String response = http.getString();  // Nhận phản hồi từ server
            Serial.println("HTTP Response Code: " + String(httpResponseCode));
            Serial.println("Response: " + response);  
        } else {
            Serial.print("Error on sending POST: ");
            Serial.println(httpResponseCode);
        }
        http.end();  // Đóng kết nối
    } else {
        Serial.println("WiFi not connected");
    }
}
void receive_data() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;

        http.begin("http://192.168.1.74:5000/send_data"); 
        http.addHeader("Content-Type", "application/json"); 

        DynamicJsonDocument jsonDoc(256);
        jsonDoc["id"] = id;  // Thêm id vào JSON
        String requestBody;
        serializeJson(jsonDoc, requestBody);

        int httpResponseCode = http.POST(requestBody);  // Gửi yêu cầu POST

        if (httpResponseCode > 0) {
            String response = http.getString();  
            Serial.println("HTTP Response Code: " + String(httpResponseCode));
            Serial.println("Response: " + response);  

            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, response);

            if (error) {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.f_str());
                return;
            }

            JsonArray nodes = doc["nodes"].as<JsonArray>();
            for (JsonObject node : nodes) {
                int led = node["led"];
                if (led == 0) {
                    Serial.println("LED tắt");
                    digitalWrite(led_pin, LOW);
                } else {
                    Serial.println("LED bật");
                    digitalWrite(led_pin, HIGH);
                }
            }
        } else {
            Serial.print("Error on sending POST: ");
            Serial.println(httpResponseCode);
        }
        http.end();
    } else {
        Serial.println("WiFi not connected");
    }
}

void loop() {
    humidity = dht11_sensor.readHumidity();
    temperature = dht11_sensor.readTemperature();
    if (isnan(humidity) || isnan(temperature)) {
        Serial.println("Không thể đọc dữ liệu từ cảm biến DHT11!");
    } else {
        send_data();
    }
    receive_data();
    delay(1000);
}