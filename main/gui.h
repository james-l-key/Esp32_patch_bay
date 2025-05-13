#ifndef GUI_H
#define GUI_H

void gui_init(void);
void gui_update_chain(const uint8_t *patch, uint8_t len);
void gui_set_status(const char *status);

#endif