#ifndef __IO_MQTT_SHELL_H__
#define __IO_MQTT_SHELL_H__

#include <IO_boards.h>
#include <PubSubClient.h>

namespace IO {

	class MqttShell {
	public:
		MqttShell(const char * prefix);
		void loop();

		static PubSubClient & _mqttClient;

	protected:
		void sendState(bool state);
		void subscribe();

		const char * _prefix;
		unsigned char _prefixLen;
	};

	class OutMqttShell : public MqttShell {
	public:
		OutMqttShell(IOut& out, const char * prefix);
		void onMQTTReconnect();
		void onTopicUpdate(char * topic, char * message);

	protected:
		IOut& _out;
	};

	class InMqttShell : public MqttShell {
	public:
		InMqttShell(IIn& in, const char * prefix);
		void loop();
		void onMQTTReconnect();
		void onTopicUpdate(char * topic, char * message);
	protected:
		IIn& _in;
	};

}

#endif