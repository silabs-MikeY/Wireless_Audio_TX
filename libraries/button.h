#ifndef BUTTON_H_
#define BUTTON_H_

#include <stdbool.h>

bool button__init(void);
void button__deinit(void);
void button__printf(bool add_timestamp, const char *format, ...);
void button__rising_edge(void);
void button__falling_edge(void);

#endif /* BUTTON_H_ */
