#include <stdio.h>
#include <string.h>
#include <cmath>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/timer.h"
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

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
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
//#define STOP_BITS 2 // for real system

#define USE_MODBUS
#define USE_MQTT
#define USE_SSD1306

#define SSID "Mgzz-57"
#define PASSWD "557949122aA"

static const char *topic = "magnus/pico/listener/LED";

void messageArrived(MQTT::MessageData &md);
std::map<std::string, std::string> json_parser(MQTT::Message &message);
void handle_message(std::map<std::string, std::string> payloadMap);
void publishMessage(MQTT::Client<IPStack, Countdown> &client, std::string topic, std::string payload);
std::string mapToString(std::map<std::string, std::string> &map);

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
           // i++;

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

void publishMessage(MQTT::Client<IPStack, Countdown> &client, std::string topic, std::string payload) {
    MQTT::Message message;
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void *) payload.c_str();
    message.payloadlen = payload.length();
    int rc = client.publish(topic.c_str(), message);
    printf("publishMessage rc=%d\n", rc);
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

int main() {

    const uint led1 = 22;
    const uint led2 = 21;
    const uint led3 = 20;

    const uint button1 = 9;
    const uint button2 = 8;
    const uint button3 = 7;

    // Initialize LEDs
    for (auto pin : {led1, led2, led3}) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT);
    }

    // Initialize buttons
    for (auto pin : {button1, button2, button3}) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
    }

    // Initialize chosen serial port
    stdio_init_all();
    printf("Boot\n");


/*
    // Init OLED
#ifdef USE_SSD1306
    // I2C is "open drain",
    // pull ups to keep signal high when no data is being sent
    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(14, GPIO_FUNC_I2C); // the display has external pull-ups
    gpio_set_function(15, GPIO_FUNC_I2C); // the display has external pull-ups
    ssd1306 display(i2c1);
    display.fill(0);
    display.text("Hello", 0, 0);
#endif
*/

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

    // We subscribe QoS2. Messages sent with lower QoS will be delivered using the QoS they were sent with
    rc = client.subscribe(topic, MQTT::QOS0, messageArrived);
    if (rc != 0) {
        printf("rc from MQTT subscribe is %d\n", rc);
        exit(-1);
    }
    printf("MQTT subscribed\n");

    //auto mqtt_send = make_timeout_time_ms(2000);
    //int mqtt_qos = 0;
    //int msg_count = 0;

    // Publish message to verify connection
    publishMessage(client, "magnus/pico/listener/verifyconnection", "{\"topic\":\"magnus/pico/listener/verifyconnection\",\"msg\":\"Magnus pico connected\"}");
    printf("Publishing message: Magnus pico connected\n To topic: magnus/pico/listener/verifyconnection\n");
#endif

/*
#ifdef USE_MODBUS
    auto uart{std::make_shared<PicoUart>(UART_NR, UART_TX_PIN, UART_RX_PIN, BAUD_RATE, STOP_BITS)};
    auto rtu_client{std::make_shared<ModbusClient>(uart)};
    ModbusRegister rh(rtu_client, 241, 256);
    auto modbus_poll = make_timeout_time_ms(3000);
    ModbusRegister produal(rtu_client, 1, 0);
    produal.write(100);
    sleep_ms((100));
    produal.write(100);
#endif
*/

    std::map <std::string, std::string> buttonMsgMap;
    buttonMsgMap["topic"] = "magnus/pico/listener/button";
    buttonMsgMap["msg"] = "My unique message";

    // Main loop
    while (true) {

        if (!client.isConnected()) {
            printf("Not connected...\n");
            rc = client.connect(data);
            if (rc != 0) {
                printf("rc from MQTT connect is %d\n", rc);
            }
        }

        int counter = 0;
        for (auto pin : {button1, button2, button3}) {
            if (!gpio_get(pin)) {
                while (counter <= 10000){
                    if (gpio_get(pin)) {
                        counter++;
                    }
                }
                publishMessage(client, buttonMsgMap["topic"], mapToString(buttonMsgMap));
            }
        }


        cyw43_arch_poll(); // obsolete? - see below
        client.yield(100); // socket that client uses calls cyw43_arch_poll()

    }

    return 0;
}




/*
    while (true) {
#ifdef USE_MODBUS
        if (time_reached(modbus_poll)) {
            gpio_put(led_pin, !gpio_get(led_pin)); // toggle  led
            modbus_poll = delayed_by_ms(modbus_poll, 3000);
            printf("RH=%5.1f%%\n", rh.read() / 10.0);
        }
#endif
#ifdef USE_MQTT
        if (time_reached(mqtt_send)) {
            mqtt_send = delayed_by_ms(mqtt_send, 2000);
            if (!client.isConnected()) {
                printf("Not connected...\n");
                rc = client.connect(data);
                if (rc != 0) {
                    printf("rc from MQTT connect is %d\n", rc);
                }

            }
            char buf[100];
            int rc = 0;
            MQTT::Message message;
            message.retained = false;
            message.dup = false;
            message.payload = (void *) buf;
            switch (mqtt_qos) {
                case 0:
                    // Send and receive QoS 0 message
                    sprintf(buf, "Msg nr: %d QoS 0 message", ++msg_count);
                    printf("%s\n", buf);
                    message.qos = MQTT::QOS0;
                    message.payloadlen = strlen(buf) + 1;
                    rc = client.publish(topic, message);
                    printf("Publish rc=%d\n", rc);
                    ++mqtt_qos;
                    break;
                case 1:
                    // Send and receive QoS 1 message
                    sprintf(buf, "Msg nr: %d QoS 1 message", ++msg_count);
                    printf("%s\n", buf);
                    message.qos = MQTT::QOS1;
                    message.payloadlen = strlen(buf) + 1;
                    rc = client.publish(topic, message);
                    printf("Publish rc=%d\n", rc);
                    ++mqtt_qos;
                    break;
#if MQTTCLIENT_QOS2
                    case 2:
                        // Send and receive QoS 2 message
                        sprintf(buf, "Msg nr: %d QoS 2 message", ++msg_count);
                        printf("%s\n", buf);
                        message.qos = MQTT::QOS2;
                        message.payloadlen = strlen(buf) + 1;
                        rc = client.publish(topic, message);
                        printf("Publish rc=%d\n", rc);
                        ++mqtt_qos;
                        break;
#endif
                default:
                    mqtt_qos = 0;
                    break;
            }
        }

        cyw43_arch_poll(); // obsolete? - see below
        client.yield(100); // socket that client uses calls cyw43_arch_poll()
#endif
    }
*/