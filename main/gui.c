#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include "font/lv_font.h"
#include "gui.h"

static lv_obj_t *chain_label;
static lv_obj_t *status_label;

void gui_init(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    // Chain label (top)
    chain_label = lv_label_create(scr);
    lv_label_set_text(chain_label, "Patch Bay");
    lv_obj_set_style_text_color(chain_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(chain_label, &lv_font_montserrat_12, 0);
    lv_obj_align(chain_label, LV_ALIGN_TOP_MID, 0, 10);

    // Status label (bottom)
    status_label = lv_label_create(scr);
    lv_label_set_text(status_label, "");
    lv_obj_set_style_text_color(status_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_10, 0);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -10);
}

void gui_update_chain(const uint8_t *patch, uint8_t len)
{
    char buf[64] = "Chain: ";
    if (len == 0)
    {
        strcat(buf, "Bypass");
    }
    else
    {
        for (int i = 0; i < len; i++)
        {
            char tmp[4];
            snprintf(tmp, sizeof(tmp), "%d", patch[i]);
            strcat(buf, tmp);
            if (i < len - 1)
                strcat(buf, "->");
        }
    }
    lv_label_set_text(chain_label, buf);
}

void gui_set_status(const char *status)
{
    lv_label_set_text(status_label, status);
}