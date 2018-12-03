#include "secrets.h"

#include <SPI.h>
#include <WiFi101.h>

WiFiClient client;           // instance of the WIFi client library
int status = WL_IDLE_STATUS; // WiFi status
int keyIndex = 0;

char ssid[] = SECRET_SSID;       // your network SSID (name)
char pass[] = SECRET_PASS;       // your network password (use for WPA, or use as key for WEP)
char server[] = "128.122.6.178"; //hosted web server

const int connectButton = 0;     // the pushbutton for connecting/disconnecting
const int sendInterval = 100;    // minimum time between messages to the server
const int debounceInterval = 15; // used to smooth out pushbutton readings
int prevButtonState = 0;         // previous state of the pushbutton
long lastTimeSent = 0;           // timestamp of the last server message

long interval = 1000;
long lastTime = 0;

int switchMain;
int heightAdjust;
int collisionDetect;
int alarmOn;
int homeSet;

int dirPin = 2;
int stepPin = 3;
int ms1 = 4;
int ms2 = 5;
bool goingUp = true;
bool stickTouched = false;
bool stickWasTouched = false;

void setup()
{
    pinMode(dirPin, OUTPUT);
    pinMode(stepPin, OUTPUT);
    pinMode(ms1, OUTPUT);
    pinMode(ms2, OUTPUT);

    pinMode(A1, INPUT);
    pinMode(A2, INPUT);

    //Initialize serial and wait for port to open:
    Serial.begin(9600);
    while (!Serial)
        ; // wait for serial port to connect.

    // Print WiFi MAC address:
    printMacAddress();

    while (status != WL_CONNECTED)
    {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid);
        // Connect to WPA/WPA2 network:
        status = WiFi.begin(ssid, pass);

        // wait 3 seconds for connection:
        delay(3000);
    }

    printWifiStatus();

    Serial.println("\nStarting connection to server...");
    // if you get a connection, report back via serial:

    httpRequest();
}

void loop()
{
    while (client.available())
    {
        String httpIn = client.readStringUntil('}');
        int firstCurly = httpIn.indexOf('{');
        String json = httpIn.substring(firstCurly + 1);
        Serial.println(json);

        if (json.length() > 0)
        {
            int firstVal = json.indexOf(',', 0);
            int secondVal = json.indexOf(',', firstVal + 1);
            int thirdVal = json.indexOf(',', secondVal + 1);
            int fourthVal = json.indexOf(',', thirdVal + 1);
            int fifthVal = json.length();

            String val1 = json.substring(firstVal - 1, firstVal);
            String val2 = json.substring(secondVal - 1, secondVal);
            String val3 = json.substring(thirdVal - 1, thirdVal);
            String val4 = json.substring(fourthVal - 1, fourthVal);
            String val5 = json.substring(fifthVal - 1, fifthVal);

            switchMain = val1.toInt();
            heightAdjust = val2.toInt();
            collisionDetect = val3.toInt();
            alarmOn = val4.toInt();
            homeSet = val5.toInt();
            Serial.println("value updated!");
        }
    }

    if (heightAdjust == 1)
    {
        goingUp = false;
        motorStep(100, goingUp);

        getRequest(2);
    }

    int stick1 = analogRead(A1);
    int stick2 = analogRead(A2);

    if (stick1 > 850 || stick2 > 850)
    {
        Serial.println("Stick was touched!");
        getRequest(3);
        stickTouched = true;
        while (stick1 > 850 || stick2 > 850)
        {
            stick1 = analogRead(A1);
            stick2 = analogRead(A2);
        }

        Serial.println("Stick is okay.");
        getRequest(4);
        // stickWasTouched = stickTouched;
    }

    if (homeSet == 1)
    {
        goingUp = true;
        motorStep(3500, goingUp);

        Serial.println("Stick has been raised to the top");
        getRequest(5);
    }

    if (millis() - lastTime > interval)
    {
        Serial.print("Main Switch is ");
        Serial.print(switchMain);
        Serial.print("Height Adjust is ");
        Serial.print(heightAdjust);
        Serial.print(", Collision Detect is ");
        Serial.print(collisionDetect);
        Serial.print(", Home Set is ");
        Serial.println(homeSet);

        httpRequest();
    }
}

void motorStep(int stepNo, bool direction)
{
    for (int i = 0; i < stepNo; i++)
    {
        if (direction == true)
        {
            digitalWrite(dirPin, HIGH);
        }
        else
        {
            digitalWrite(dirPin, LOW);
        }

        digitalWrite(stepPin, HIGH);
        delay(1);

        digitalWrite(stepPin, LOW);
        delay(1);
    }

    Serial.println("motor moved");
}

void getRequest(int whichValue)
{
    client.stop();

    if (client.connect(server, 3000))
    {
        // Serial.println("connected to server");
        // Make a HTTP request :
        if (whichValue == 1)
        {
            client.println("POST /switchedMain HTTP/1.1");
        }
        if (whichValue == 2)
        {
            client.println("POST /heightAdjusted HTTP/1.1");
            heightAdjust = 0;
        }
        if (whichValue == 3)
        {
            client.println("POST /collided HTTP/1.1");
        }
        if (whichValue == 4)
        {
            client.println("POST /uncollided HTTP/1.1");
        }
        if (whichValue == 5)
        {
            client.println("POST /homeSetted HTTP/1.1");
            homeSet = 0;
        }
        client.print("Host: ");
        client.println(server);
        client.println("Connection: close");
        client.println();
    }

    lastTime = millis();
}

void httpRequest()
{
    client.stop();

    if (client.connect(server, 3000))
    {
        Serial.println("connected to server");
        // Make a HTTP request :
        client.println("GET /url HTTP/1.1");
        client.print("Host: ");
        client.println(server);
        client.println("Connection: close");
        client.println();
    }

    lastTime = millis();
}

void printMacAddress()
{
    // the MAC address of your WiFi shield
    byte mac[6];

    // print your MAC address:
    WiFi.macAddress(mac);
    Serial.print("MAC: ");
    Serial.print(mac[5], HEX);
    Serial.print(":");
    Serial.print(mac[4], HEX);
    Serial.print(":");
    Serial.print(mac[3], HEX);
    Serial.print(":");
    Serial.print(mac[2], HEX);
    Serial.print(":");
    Serial.print(mac[1], HEX);
    Serial.print(":");
    Serial.println(mac[0], HEX);
}

void printWifiStatus()
{
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}
