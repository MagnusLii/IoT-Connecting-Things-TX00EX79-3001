#include <stdio.h>
#include <string.h>
#include <cmath>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/timer.h"
#include "hardware/adc.h"
#include "uart/PicoUart.h"
#include <map>

#include <cstdlib>
#include <ctime>
#include "IPStack.h"
#include "Countdown.h"
#include "MQTTClient.h"
#include "ModbusClient.h"
#include "ModbusRegister.h"
#include "ssd1306.h"

#if 0
#define UART_NR 0
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#else
#define UART_NR 1
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#endif

#define BAUD_RATE 9600
#define STOP_BITS 1 // for simulator

#define USE_MODBUS
#define USE_MQTT
#define USE_SSD1306

#define SSID "Mgzz-57"
#define PASSWD "557949122aA"

#define ON 1
#define OFF 0

#define LED1 22
#define LED2 21
#define LED3 20

#define LEDS_LIST {LED1, LED2, LED3}

#define BUTTON1 9
#define BUTTON2 8
#define BUTTON3 7
#define ROT_BUTTON 12

#define BUTTONS_LIST {BUTTON1, BUTTON2, BUTTON3, ROT_BUTTON}

static const char *topic = "magnus/pico/listener/LED";

float previousCoreTemp = 25.0; // Init here so it doesn't toggle the leds before the first read.
float high_temp = 30.0;
float low_temp = 25.0;
bool publish_temp = false;

void messageArrived(MQTT::MessageData &md);
std::map<std::string, std::string> json_parser(MQTT::Message &message);
void handle_message(std::map<std::string, std::string> payloadMap);
void publishMessage(MQTT::Client<IPStack, Countdown> &client, std::string topic, std::string payload);
std::string mapToString(std::map<std::string, std::string> &map);
void handle_led(std::string value, int pin);
void read_cpu_temperature();
void handle_temp(std::string value);


int main() {
    printf("Boot\n");

    // Initialize LEDs
    for (auto pin : LEDS_LIST) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT);
    }

    // Initialize buttons
    for (auto pin : BUTTONS_LIST) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
    }

    // Initialize chosen serial port
    stdio_init_all();

    // Init temp related stuff
    adc_init();
    adc_set_temp_sensor_enabled(true);

// Connect to MQTT broker
#ifdef USE_MQTT
    IPStack ipstack(SSID, PASSWD); // example
    auto client = MQTT::Client<IPStack, Countdown>(ipstack);

    int rc = ipstack.connect("18.198.188.151", 21883);
    if (rc != 1) {
        printf("rc from TCP connect is %d\n", rc);
    }

    // Generate a random number between 1 and 100000
    int randomNumber = (rand() % 100000) + 1;

    printf("MQTT connecting\n");
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    std::string clientId = "PicoW-sample" + std::to_string(randomNumber);
    data.clientID.cstring = (char *)clientId.c_str();
    data.username.cstring = (char *) "SmartIoT";
    data.password.cstring = (char *) "SmartIoTMQTT";
    rc = client.connect(data);
    if (rc != 0) {
        printf("rc from MQTT connect is %d\n", rc);
        exit(-1);
    }
    printf("MQTT connected\n");

    rc = client.subscribe(topic, MQTT::QOS0, messageArrived);
    if (rc != 0) {
        printf("rc from MQTT subscribe is %d\n", rc);
        exit(-1);
    }
    printf("MQTT subscribed\n");

    // Publish message to verify connection
    publishMessage(client, "magnus/pico/listener/verifyconnection", "{\"topic\":\"magnus/pico/listener/verifyconnection\",\"msg\":\"Magnus pico connected\"}");
#endif

    // Init repeating timer
    add_repeating_timer_ms(1000, alarm_callback, NULL);

    // Main loop
    while (true) {
        // Handle MQTT disconnection
        if (!client.isConnected()) {
            printf("MQTT disconnected...\nReconnecting...\n");
            rc = client.connect(data);
            if (rc != 0) {
                printf("rc from MQTT connect is %d\n", rc);
            }
        }

        // Handle buttons
        int counter = 0;
        for (auto pin : BUTTONS_LIST) { // All buttons, change to pin 12 if Joseph complains.
            if (!gpio_get(pin)) {
                printf("Button %d pressed\n", pin);
                while (counter <= 10000){
                    if (gpio_get(pin)) {
                        counter++;
                    }
                }
                publish_temp = true;
            }
        }

        // Handle temperature
        if (previousCoreTemp > high_temp) {
            gpio_put(LED3, ON);
        } if (previousCoreTemp < high_temp && previousCoreTemp > low_temp) {
            gpio_put(LED3, OFF);
            gpio_put(LED1, OFF);
        } if (previousCoreTemp < low_temp) {
            gpio_put(LED1, ON);
        }

        // poke the bear
        client.yield(100);

        if (publish_temp) {
            read_cpu_temperature();
            publishMessage(client, "magnus/pico/listener/temp", "{\"topic\":\"magnus/pico/listener/temp\",\"msg\":\"Current Pico temp: " + std::to_string(previousCoreTemp) + "\"}");
            publish_temp = false;
        }
    }

    return 0;
}


void messageArrived(MQTT::MessageData &md) {
    printf("messageArrived()\n");
    MQTT::Message &message = md.message;

    // Print the message
    printf("Message arrived: qos %d, retained %d, dup %d, packetid %d\n",
           message.qos, message.retained, message.dup, message.id);
    printf("Payload %s\n", (char *) message.payload);

    // Parse the message
    std::map<std::string, std::string> payloadMap = json_parser(message);

    // Handle the message
    handle_message(payloadMap);
}


std::map<std::string, std::string> json_parser(MQTT::Message &message) {
    std::string payloadStr((char*)message.payload, message.payloadlen);
    std::map<std::string, std::string> payloadMap;
    std::string key;
    std::string value;

    for (int i = 0; i < (int)payloadStr.length(); i++) {
        if (payloadStr[i] == '"') {
            i++;
            
            // Get key.
            while (payloadStr[i] != '"') {
                key += payloadStr[i];
                i++;
            }
            i++;

            // skip anything between key and value
            while (payloadStr[i] != '"') {
                i++;
            }
            i++;

            // Get value.
            while (payloadStr[i] != '"') {
                value += payloadStr[i];
                i++;
            }

            // Store and clear.
            payloadMap[key] = value;
            key.clear();
            value.clear();
        }
    }

    return payloadMap;
}


// Commands: "ON", "OFF", "TOGG".
/*
void handle_message(std::map<std::string, std::string> payloadMap) {
    printf("handle_message()\n");
    printf("Message: %s\n", mapToString(payloadMap).c_str());
    for (auto const& [key, value] : payloadMap) {
        if (key == "msg"){
            if (value.find("LED1") != std::string::npos) {
                if (value.find("ON") != std::string::npos) {
                    gpio_put(22, 1);
                } else if (value.find("OFF") != std::string::npos) {
                    gpio_put(22, 0);
                } else if (value.find("TOGG") != std::string::npos) {
                    gpio_put(22, !gpio_get(22));
                } else {
                    printf("Unknown message\n");
                }
            } else if (value.find("LED2") != std::string::npos) {
                if (value.find("ON") != std::string::npos) {
                    gpio_put(21, 1);
                } else if (value.find("OFF") != std::string::npos) {
                    gpio_put(21, 0);
                } else if (value.find("TOGG") != std::string::npos) {
                    gpio_put(21, !gpio_get(21));
                } else {
                    printf("Unknown message\n");
                }
            } else if (value.find("LED3") != std::string::npos) {
                if (value.find("ON") != std::string::npos) {
                    gpio_put(20, 1);
                } else if (value.find("OFF") != std::string::npos) {
                    gpio_put(20, 0);
                } else if (value.find("TOGG") != std::string::npos) {
                    gpio_put(20, !gpio_get(20));
                } else {
                    printf("Unknown message\n");
                }
            } else {
                printf("Unknown message\n");
            }
        }
    }
    return;
}
*/


void handle_led(std::string value, int pin) {
    if (value.find("ON") != std::string::npos) {
        gpio_put(pin, ON);
    } else if (value.find("OFF") != std::string::npos) {
        gpio_put(pin, OFF);
    } else if (value.find("TOGG") != std::string::npos) {
        gpio_put(pin, !gpio_get(pin));
    } else {
        printf("handle_led(): Unknown message\n");
    }
}


void handle_temp(std::string value) {
    if (value.find("HIGH") != std::string::npos) {
        high_temp = std::stof(value.substr(value.find(":") + 1));
    } else if (value.find("LOW") != std::string::npos) {
        low_temp = std::stof(value.substr(value.find(":") + 1));
    } else if (value.find("TEMP") != std::string::npos) {
        publish_temp = true;
    } else {
        printf("handle_temp(): Unknown message\n");
    }
}


void handle_message(std::map<std::string, std::string> payloadMap) {
    printf("handle_message() Message: %s\n", mapToString(payloadMap).c_str());

    std::map<std::string, int> ledMap = {
        {"LED1", LED1},
        {"LED2", LED2},
        {"LED3", LED3},
        {"ROT_BUTTON", ROT_BUTTON}
    };

    for (auto const& [key, value] : payloadMap) {
        if (key == "msg"){
            // Check if the message is for a LED.
            for (auto const& [led, pin] : ledMap) {
                if (value.find(led) != std::string::npos) {
                    handle_led(value, pin);
                    return;
                }
            }

            // Check if the message is for the temperature.
            handle_temp(value);
        }
    }
    return;
}


void publishMessage(MQTT::Client<IPStack, Countdown> &client, std::string topic, std::string payload) {
    MQTT::Message message;
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void *) payload.c_str();
    message.payloadlen = payload.length();
    int rc = client.publish(topic.c_str(), message);
    if (rc != 0) {
        printf("publishMessage() failed to publish message\n");
        return;
    }
    printf("publishMessage() published message: %s\n", payload.c_str());
    return;
}


std::string mapToString(std::map<std::string, std::string> &map) {
    std::string josn_str = "{";
    for (auto const& [key, value] : map) {
        josn_str += "\"" + key + "\":\"" + value + "\",";
    }
    josn_str.pop_back();
    josn_str += "}";
    
    printf("mapToString() string: %s\n", josn_str.c_str());

    return josn_str;
}


void read_cpu_temperature() {
    gpio_put(LED2, !gpio_get(LED2)); // Toggle LED2 on every read.
    adc_select_input(4); // Input 4 is the temp sensor.

    const float conversion_factor = 3.3f / (1 << 12); // 3.3V reference / 12bit resolution.

    uint16_t result = adc_read();
    float voltage = result * conversion_factor;

    // Convert the voltage to temperature. https://www.halvorsen.blog/documents/technology/iot/pico/pico_temperature_sensor_builtin.php
    previousCoreTemp = 27 - (voltage - 0.706) / 0.001721;
    
    return;
}


bool alarm_callback(repeating_timer_t *rt) {
    read_cpu_temperature();
    return 1;
}