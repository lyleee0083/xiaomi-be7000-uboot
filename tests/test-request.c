#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "../failsafe/request.h"

int main(void)
{
	static const char complete[] = "POST / HTTP/1.1\r\nContent-Length: 16\r\n\r\nconfig_name=demo";
	static const char short_body[] = "POST / HTTP/1.1\r\nContent-Length: 17\r\n\r\nconfig_name=demo";
	static const char duplicate[] = "POST / HTTP/1.1\r\nContent-Length: 0\r\ncontent-length: 0\r\n\r\n";
	static const char zero[] = "POST / HTTP/1.1\r\nContent-Length: 0\r\n\r\n";
	static const char encoded[] = "config_name=config%40board";
	static const char reset[] = "reset_key=";
	char value[128], max_value[9 + 1 + sizeof(value)];
	const char *body, *name;
	size_t size;

	body = request_body(complete, sizeof(complete) - 1, &size);
	assert(body && size == 16);
	name = request_env_value(body, size, value, sizeof(value));
	assert(name && !strcmp(name, "config_name") && !strcmp(value, "demo"));
	assert(!request_body(short_body, sizeof(short_body) - 1, &size));
	assert(!request_body(duplicate, sizeof(duplicate) - 1, &size));
	assert(request_body(zero, sizeof(zero) - 1, &size) && size == 0);

	name = request_env_value(encoded, sizeof(encoded) - 1, value, sizeof(value));
	assert(name && !strcmp(value, "config@board"));
	name = request_env_value(reset, sizeof(reset) - 1, value, sizeof(value));
	assert(name && !*value);
	memcpy(max_value, reset, sizeof(reset) - 1);
	memset(max_value + sizeof(reset) - 1, 'a', sizeof(value));
	assert(request_env_value(max_value, sizeof(max_value) - 1, value, sizeof(value)));
	assert(strlen(value) == sizeof(value) - 1);
	assert(!request_env_value(max_value, sizeof(max_value), value, sizeof(value)));
	assert(!request_env_value("other=demo", 10, value, sizeof(value)));
	puts("Webfailsafe request parser checks passed.");
	return 0;
}
