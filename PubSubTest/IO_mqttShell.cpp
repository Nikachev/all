#include <IO_mqttShell.h>
#include <avr/pgmspace.h>
#include <PubSubClient.h>


namespace IO {
	const PROGMEM char STR_STATE[] = "state";
	const PROGMEM char STR_COMMANDS[] = "commands";
}


using namespace IO;


MqttShell::MqttShell(const char * prefix) : _prefix(prefix) {

	_prefixLen = strlen_P(_prefix);
}

void MqttShell::loop() {
	// empty
}

void MqttShell::sendState(bool state) {
	char buf[96];
	strcpy_P(buf, _prefix);
	strcpy_P(buf + _prefixLen, STR_STATE);
	state ? _mqttClient.publish(buf, "ON") : _mqttClient.publish(buf, "OFF");
}

void MqttShell::subscribe() {
	char buf[96];
	strcpy_P(buf, _prefix);
	strcpy_P(buf + _prefixLen, STR_COMMANDS);
	_mqttClient.subscribe(buf);
}


OutMqttShell::OutMqttShell(IOut& out, const char * prefix) :
	MqttShell(prefix), _out(out)
{
	_out.setState(false);
}

void OutMqttShell::onMQTTReconnect() {
	subscribe();
	sendState(_out.getState());
}

void OutMqttShell::onTopicUpdate(char * topic, char * message) {
	if (!strncmp_P(topic, _prefix, _prefixLen)) {
		if (!strcmp_P(topic + _prefixLen, STR_COMMANDS)) {
			if (!strcmp(message, "ON")) {
				_out.setState(true);
			}
			else if (!strcmp(message, "OFF")) {
				_out.setState(false);
			}
			sendState(_out.getState());
		}
	}
}


InMqttShell::InMqttShell(IIn& in, const char * prefix) :
	MqttShell(prefix), _in(in)
{
	// empty
}

void InMqttShell::loop() {
	if (_in.isChanged()) sendState(_in.getState());
}

void InMqttShell::onMQTTReconnect() {
	subscribe();
	sendState(_in.getState());
}

void InMqttShell::onTopicUpdate(char * topic, char * message) {
	if (!strncmp_P(topic, _prefix, _prefixLen)) {
		if (!strcmp_P(topic + _prefixLen, STR_COMMANDS)) {
			sendState(_in.getState());
		}
	}
}