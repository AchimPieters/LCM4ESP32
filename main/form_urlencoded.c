#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "form_urlencoded.h"

static inline bool is_hex(int c) {
    c = toupper((unsigned char)c);
    return ('0' <= c && c <= '9') || ('A' <= c && c <= 'F');
}

static inline int hex_value(int c) {
    c = toupper((unsigned char)c);
    if ('0' <= c && c <= '9') {
        return c - '0';
    }
    return c - 'A' + 10;
}

char *url_unescape(const char *buffer, size_t size) {
    size_t len = 0;

    int i = 0, j;
    while (i < size) {
        len++;
        if (buffer[i] == '%') {
            i += 3;
        } else {
            i++;
        }
    }

    char *result = malloc(len + 1);
    if (!result) {
        return NULL;
    }
    i = j = 0;
    while (i < size) {
        if (buffer[i] == '+') {
            result[j++] = ' ';
            i++;
        } else if (buffer[i] != '%') {
            result[j++] = buffer[i++];
        } else {
            if (i + 2 < size && is_hex(buffer[i + 1]) && is_hex(buffer[i + 2])) {
                result[j++] = hex_value(buffer[i + 1]) * 16 + hex_value(buffer[i + 2]);
                i += 3;
            } else {
                result[j++] = buffer[i++];
            }
        }
    }
    result[j] = 0;
    return result;
}


form_param_t *form_params_parse(const char *s) {
    form_param_t *params = NULL;

    int i = 0;
    while (1) {
        int pos = i;
        while (s[i] && s[i] != '=' && s[i] != '&') i++;
        if (i == pos) {
            i++;
            continue;
        }

        form_param_t *param = malloc(sizeof(form_param_t));
        if (!param) {
            form_params_free(params);
            return NULL;
        }
        param->name = url_unescape(s + pos, i - pos);
        if (!param->name) {
            free(param);
            form_params_free(params);
            return NULL;
        }
        param->value = NULL;
        param->next = params;
        params = param;

        if (s[i] == '=') {
            i++;
            pos = i;
            while (s[i] && s[i] != '&') i++;
            if (i != pos) {
                param->value = url_unescape(s + pos, i - pos);
                if (!param->value) {
                    free(param->name);
                    free(param);
                    form_params_free(params);
                    return NULL;
                }
            }
        }

        if (!s[i])
            break;
    }

    return params;
}


form_param_t *form_params_find(form_param_t *params, const char *name) {
    while (params) {
        if (!strcmp(params->name, name))
            return params;
        params = params->next;
    }

    return NULL;
}


void form_params_free(form_param_t *params) {
    while (params) {
        form_param_t *next = params->next;
        if (params->name)
            free(params->name);
        if (params->value)
            free(params->value);
        free(params);

        params = next;
    }
}


