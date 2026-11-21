#ifndef FAILSAFE_REQUEST_H
#define FAILSAFE_REQUEST_H

/* The caller supplies a NUL-terminated copy of the received bytes. */
static inline int request_header(const char *data, const char *headers_end,
			  const char *name, const char **value,
			  size_t *length)
{
	size_t name_len = strlen(name);
	const char *line = strstr(data, "\r\n");
	int found = 0;

	if (!line)
		return -1;
	line += 2;
	while (line < headers_end) {
		const char *end = strstr(line, "\r\n");
		const char *colon, *next;

		if (!end || end > headers_end)
			return -1;
		next = end + 2;
		colon = memchr(line, ':', end - line);
		if (!colon || colon == line || *line == ' ' || *line == '\t')
			return -1;
		if ((size_t)(colon - line) == name_len &&
		    !strncasecmp(line, name, name_len)) {
			if (found)
				return -1;
			found = 1;
			*value = colon + 1;
			while (*value < end && (**value == ' ' || **value == '\t'))
				(*value)++;
			while (end > *value && (end[-1] == ' ' || end[-1] == '\t'))
				end--;
			*length = end - *value;
		}
		line = next;
	}
	return found;
}

static inline const char *request_headers_end(const char *data, size_t size)
{
	if (memchr(data, '\0', size))
		return NULL;
	return strstr(data, "\r\n\r\n");
}

/* Require a complete, unambiguous body before any state change. */
static inline const char *request_body(const char *data, size_t size, size_t *length)
{
	const char *end = request_headers_end(data, size);
	const char *field;
	size_t n, i, value = 0;

	if (!end ||
	    request_header(data, end, "Transfer-Encoding", &field, &n) != 0 ||
	    request_header(data, end, "Content-Length", &field, &n) != 1 || !n)
		return NULL;
	*length = size - (end + 4 - data);
	for (i = 0; i < n; i++) {
		unsigned int digit = (unsigned char)field[i] - '0';

		if (digit > 9 || value > *length / 10)
			return NULL;
		value = value * 10 + digit;
		if (value > *length)
			return NULL;
	}
	return value == *length ? end + 4 : NULL;
}

static inline int request_hex_digit(unsigned char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	return -1;
}

static inline const char *request_env_value(const char *body, size_t length,
				     char *value, size_t capacity)
{
	const char *eq = memchr(body, '=', length);
	const char *end = body + length;
	const char *name;
	size_t i = 0;

	if (!eq || !capacity)
		return NULL;
	if (eq - body == 9 && !memcmp(body, "reset_key", 9))
		name = "reset_key";
	else if (eq - body == 11 && !memcmp(body, "config_name", 11))
		name = "config_name";
	else
		return NULL;
	for (body = eq + 1; body < end; body++) {
		unsigned char c = *body;

		if (c == '%') {
			int high, low;

			if (end - body < 3)
				return NULL;
			high = request_hex_digit(body[1]);
			low = request_hex_digit(body[2]);
			if (high < 0 || low < 0)
				return NULL;
			c = (high << 4) | low;
			body += 2;
		}
		if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
		      (c >= 'A' && c <= 'Z') || c == '@' || c == '-' ||
		      c == '_' || c == '.' || c == ',' || c == ':'))
			return NULL;
		if (i == capacity - 1)
			return NULL;
		value[i++] = c;
	}
	value[i] = '\0';
	return name;
}

#endif
