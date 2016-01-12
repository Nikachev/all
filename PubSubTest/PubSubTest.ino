#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

#include <MsTimer2.h>

#include <avr/wdt.h>

#include <WiznetWatchdog.h>

#include <IO_boards.h>
#include <IO_mqttShell.h>

byte mac[] = { 0xBE, 0xE0, 0x0E, 0xE0, 0xE7, 0x15 };
byte mqttServer[] = { 10, 1, 10, 17 };

EthernetClient ethClient;

void onTopicUpdate(char * topic, byte * payload, unsigned int length);
PubSubClient mqttClient(mqttServer, 1883, onTopicUpdate, ethClient);

char mqttClientName[] = "TestDevice";
const PROGMEM char DEVICE_TS[] = "TestDevice/state";
const PROGMEM char DEVICE_TC[] = "TestDevice/commands";
WiznetWatchdog wiznetWatchdog(A0, mac, mqttClient, mqttClientName, DEVICE_TS, DEVICE_TC);

IO::Modules modules(2, 3, 3, 4, 5, 6, 7);
IO::Modules& IO::IExt::_modules = modules;
PubSubClient & IO::MqttShell::_mqttClient = mqttClient;


IO::ExtOut out1(0, 0);
const PROGMEM char OUT_PREFIX[] = "tm/test/";
IO::OutMqttShell out1Shell(out1, OUT_PREFIX);

IO::ExtIn in1(0, 0, INVERT);
IO::DebouncedIn in1d(in1, 100);
const PROGMEM char IN1_PREFIX[] = "tm/in1/";
IO::InMqttShell in1Shell(in1d, IN1_PREFIX);

IO::ExtIn in2(0, 7);
const PROGMEM char IN2_PREFIX[] = "tm/in2/";
IO::InMqttShell in2Shell(in2, IN2_PREFIX);

IO::ExtIn in3(1, 0);
const PROGMEM char IN3_PREFIX[] = "tm/in3/";
IO::InMqttShell in3Shell(in3, IN3_PREFIX);

IO::ExtIn in4(1, 7);
const PROGMEM char IN4_PREFIX[] = "tm/in4/";
IO::InMqttShell in4Shell(in4, IN4_PREFIX);


void timer_ev()
{
	interrupts();
	wiznetWatchdog.onTimerEvent();
}

void setup()
{
	MsTimer2::set(3, timer_ev);
	MsTimer2::start();
}

void onTopicUpdate(char * topic, byte * payload, unsigned int length)
{
	char message[MQTT_MAX_PACKET_SIZE + 1];
	memcpy(message, payload, length);
	message[length] = '\0';

	wiznetWatchdog.onTopicUpdate(topic, message);

	out1Shell.onTopicUpdate(topic, message);

	in1Shell.onTopicUpdate(topic, message);
	in2Shell.onTopicUpdate(topic, message);
	in3Shell.onTopicUpdate(topic, message);
	in4Shell.onTopicUpdate(topic, message);
}

void mqttReconnect()
{
	wdt_reset();
	if (mqttClient.loop()) return;
	wiznetWatchdog.onMQTTReconnect();

	out1Shell.onMQTTReconnect();

	in1Shell.onMQTTReconnect();
	in2Shell.onMQTTReconnect();
	in3Shell.onMQTTReconnect();
	in4Shell.onMQTTReconnect();
}

void loop()
{
	mqttReconnect();
	wiznetWatchdog.loop();

	out1Shell.loop();

	in1Shell.loop();
	in2Shell.loop();
	in3Shell.loop();
	in4Shell.loop();

	modules.refresh();
}