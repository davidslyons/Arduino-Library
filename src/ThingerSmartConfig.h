// The MIT License (MIT)
//
// Copyright (c) 2015 THINGER LTD
// Author: alvarolb@gmail.com (Alvaro Luis Bustamante)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef THINGERSMARTCONFIG_H
#define THINGERSMARTCONFIG_H

#include "ThingerClient.h"

#define WIFI_CONNECTION_TIMEOUT_MS 15000
#define IP_ADDRESS_TIMEOUT_MS 30000
#define SMART_CONFIG_WAIT_MS 60000

class ThingerSmartConfig : public ThingerClient {

public:
    ThingerSmartConfig(const char* user, const char* device, const char* device_credential, bool use_led=true) :
            ThingerClient(client_, user, device, device_credential), use_led_(use_led)
    {
        if(use_led_){
            pinMode(BUILTIN_LED, OUTPUT);
        }
    }

    ~ThingerSmartConfig(){

    }

protected:

    virtual bool network_connected(){
        return WiFi.status() == WL_CONNECTED && !(WiFi.localIP() == INADDR_NONE);
    }

    virtual bool connect_network(){
        // turn off led
        if(use_led_) {
            digitalWrite(BUILTIN_LED, HIGH);
        }

        // try to connect to the last known Wifi Network
        if(WiFi.SSID()!=NULL){
            #ifdef _DEBUG_
                Serial.print(F("[NETWORK] Trying to connect to the last known network: "));
                Serial.println(WiFi.SSID());
            #endif
            long wifi_timeout = millis();
            WiFi.begin(WiFi.SSID().c_str(), WiFi.psk().c_str());
            while(WiFi.status() != WL_CONNECTED && (millis() - wifi_timeout < WIFI_CONNECTION_TIMEOUT_MS)) {
                yield();
            }
            #ifdef _DEBUG_
                if(WiFi.status() != WL_CONNECTED){
                    Serial.print(F("[NETWORK] Cannot connect to network: "));
                    Serial.println(WiFi.SSID());
                }
            #endif
        }

        // if not success connection then start the SmartConfig
        if(WiFi.status() != WL_CONNECTED){
            long wifi_timeout = millis();
            #ifdef _DEBUG_
                Serial.println(F("[NETWORK] Waiting Smart Config... "));
            #endif
            WiFi.stopSmartConfig();
            WiFi.beginSmartConfig();
            while(!WiFi.smartConfigDone()) {
                // stop smart config process on timeout
                if(millis() - wifi_timeout > SMART_CONFIG_WAIT_MS){
                    return false;
                }
                // blink led if required, otherwise yield to do network tasks
                if(use_led_){
                    digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED));
                    delay(500);
                }else{
                    yield();
                }
            }
            #ifdef _DEBUG_
                Serial.println(F("[NETWORK] Smart Config Process Completed!"));
            #endif
        }

        // ensure that the the ESP8266 get connected to the network
        long wifi_timeout = millis();
        while(WiFi.status() != WL_CONNECTED) {
            if(millis() - wifi_timeout > WIFI_CONNECTION_TIMEOUT_MS){
                #ifdef _DEBUG_
                    Serial.println(F("[NETWORK] Cannot connect to WiFi! Check the credentials!"));
                #endif
                // clear wifi configuration
                WiFi.disconnect();
                return false;
            }
            yield();
        }

        #ifdef _DEBUG_
        Serial.println(F("[NETWORK] Connected to WiFi!"));
        #endif
        // 10 small blinks to notify network connection
        if(use_led_) {
            for (int i = 0; i < 10; i++) {
                digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED));
                delay(100);
            }
        }

        #ifdef _DEBUG_
            Serial.println(F("[NETWORK] Getting IP Address..."));
        #endif
        // wait for an ip address
        wifi_timeout = millis();
        while (WiFi.localIP() == INADDR_NONE) {
            if(millis() - wifi_timeout > IP_ADDRESS_TIMEOUT_MS) return false;
            yield();
        }
        #ifdef _DEBUG_
            Serial.print(F("[NETWORK] Got IP Address: "));
            Serial.println(WiFi.localIP());
        #endif
        return true;
    }

private:
    bool use_led_;
    WiFiClient client_;
};

#endif