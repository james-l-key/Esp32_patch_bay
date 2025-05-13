#ifndef BUTTONS_H
#define BUTTONS_H

void buttons_init(void);
void buttons_task(void *pvParameters);
void buttons_load_patch(void);
uint8_t buttons_get_patch(uint8_t *patch);

#endif