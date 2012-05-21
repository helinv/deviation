/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Foobar is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "target.h"
#include "pages.h"
#include "gui/gui.h"
#include "mixer.h"
#include "mixer_page.h"

struct Mixer mixer[2];
struct Limit limit;
guiObject_t *graph;
u8 channel;
static char tmpstr[5];

static const char *channel_name[] = {
    "Ch1", "Ch2", "Ch3", "Ch4",
    "Ch5", "Ch6", "Ch7", "Ch8",
    "Ch9", "Ch10", "Ch11", "Ch12",
    "Ch13", "Ch14", "Ch15", "Ch16"
};

#define ENTRIES_PER_PAGE 8

static char input_str[ENTRIES_PER_PAGE][6];
static char switch_str[ENTRIES_PER_PAGE][7];
static enum TemplateType cur_template;
static enum CurveType old_curvetype;

static const char *template_name(enum TemplateType template);
static const char *templatetype_cb(guiObject_t *obj, int value, void *data);
static void templateselect_cb(guiObject_t *obj, void *data);

static void show_none();
static void show_simple();
static void show_dr();
static void show_expo();
static void show_complex();

static const char *inp[] = {
    "THR", "RUD", "ELE", "AIL",
    "", "D/R", "D/R-C1", "D/R-C2"
};

static u8 modifying_template;

void PAGE_MixerInit(int page)
{
    int init_y = 16;
    int i;
    modifying_template = 0;
    for (i = 0; i < ENTRIES_PER_PAGE; i++) {
        void *ptr = (void *)((long)i);
        int row = init_y + 28 * i;
        GUI_CreateLabel(10, row, channel_name[ENTRIES_PER_PAGE * page + i], 0x0000);
        strcpy(input_str[i], (i < 4) ? inp[i] : "");
        GUI_CreateLabel(40, row, input_str[i], 0x0000);
        GUI_CreateButton(100, row - 4, BUTTON_90, template_name(MIX_GetTemplate(i)), 0x0000, templateselect_cb, ptr);
        strcpy(switch_str[i], (i < 4) ? inp[i+4] : "");
        GUI_CreateLabel(240, row, switch_str[i], 0x0000);
    }
    GUI_DrawScreen();
}

void PAGE_MixerEvent()
{
}

int PAGE_MixerCanChange()
{
    return ! modifying_template;
}

static const char *template_name(enum TemplateType template)
{
    switch(template) {
    case MIXERTEMPLATE_NONE :   return "None";
    case MIXERTEMPLATE_SIMPLE:  return "Simple";
    case MIXERTEMPLATE_DR:      return "Dual Rate";
    case MIXERTEMPLATE_EXPO:    return "Expo";
    case MIXERTEMPLATE_COMPLEX: return "Complex";
    default:                    return "Unknown";
    }
}

static void change_template()
{
    switch(cur_template)  {
    case MIXERTEMPLATE_NONE:
        show_none();
        break;
    case MIXERTEMPLATE_SIMPLE:
        show_simple();
        break;
    case MIXERTEMPLATE_DR:
        show_dr();
        break;
    case MIXERTEMPLATE_EXPO:
        show_expo();
        break;
    case MIXERTEMPLATE_COMPLEX:
        show_complex();
        break;
    }
}

static const char *templatetype_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    if(dir > 0) {
        if(cur_template < MIXERTEMPLATE_MAX) {
            if (cur_template == MIXERTEMPLATE_EXPO)
                mixer[1].curve.type = old_curvetype;
            cur_template++;
            change_template();
            return "";
        }
    } else if(dir < 0) {
        if(cur_template) {
            if (cur_template == MIXERTEMPLATE_EXPO)
                mixer[1].curve.type = old_curvetype;
            cur_template--;
            change_template();
            return "";
        }
    }
    return template_name(cur_template);
}

static void templateselect_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    long idx = (long)data;

    cur_template = MIX_GetTemplate(idx);
    modifying_template = 1;
    MIX_GetLimit(idx, &limit);
    channel = idx;
    switch(cur_template)  {
    case MIXERTEMPLATE_NONE:
        MIX_InitMixer(&mixer[0], idx);
        MIX_InitMixer(&mixer[1], idx);
        show_none();
        break;
    case MIXERTEMPLATE_SIMPLE:
        MIX_InitMixer(&mixer[1], idx);
        if(! MIX_GetMixers(idx, mixer, 1)) {
            MIX_InitMixer(mixer, idx);
        }
        show_simple();
        break;
    case MIXERTEMPLATE_DR:
        if(! MIX_GetMixers(idx, mixer, 2)) {
            MIX_InitMixer(&mixer[0], idx);
            MIX_InitMixer(&mixer[1], idx);
        }
        show_dr();
        break;
    default:
        MIX_InitMixer(mixer, idx);
        show_none();
        break;
    }
}

static const char *set_curvename_cb(guiObject_t *obj, int dir, void *data);
void curveselect_cb(guiObject_t *obj, void *data);
const char *set_source_cb(guiObject_t *obj, int dir, void *data);
const char *set_expoval_cb(guiObject_t *obj, int dir, void *data);
static void okcancel_cb(guiObject_t *obj, void *data);

static void show_titlerow()
{
    GUI_CreateLabel(10, 10, channel_name[channel], 0x0000);
    GUI_CreateTextSelect(40, 10, TEXTSELECT_96, 0x0000, NULL, templatetype_cb, (void *)((long)channel));
    GUI_CreateButton(150, 6, BUTTON_90, "Cancel", 0x0000, okcancel_cb, (void *)0);
    GUI_CreateButton(264, 6, BUTTON_45, "Ok", 0x0000, okcancel_cb, (void *)1);
}

static void show_none()
{
    GUI_RemoveAllObjects();
    //Row 0
    show_titlerow();
}
 
static void show_simple()
{
    GUI_RemoveAllObjects();
    //Row 0
    show_titlerow();
    //Row 1
    GUI_CreateLabel(10, 40, "Src:", 0x0000);
    GUI_CreateTextSelect(50, 40, TEXTSELECT_96, 0x0000, NULL, set_source_cb, &mixer[0].src);
    GUI_CreateLabel(170, 40, "Curve:", 0x0000);
    GUI_CreateTextSelect(214, 40, TEXTSELECT_96, 0x0000, curveselect_cb, set_curvename_cb, &show_simple);
    //Row 2
    GUI_CreateLabel(10, 66, "Scale:", 0x0000);
    GUI_CreateTextSelect(50, 66, TEXTSELECT_96, 0x0000, NULL, PAGEMIX_SetNumberCB, &mixer[0].scaler);
    GUI_CreateLabel(170, 66, "Offset:", 0x0000);
    GUI_CreateTextSelect(214, 66, TEXTSELECT_96, 0x0000, NULL, PAGEMIX_SetNumberCB, &mixer[0].offset);
    //Row 4
    GUI_CreateLabel(10, 92, "Max:", 0x0000);
    GUI_CreateTextSelect(50, 92, TEXTSELECT_96, 0x0000, NULL, PAGEMIX_SetNumberCB, &limit.max);
    GUI_CreateLabel(170, 92, "Min:", 0x0000);
    GUI_CreateTextSelect(214, 92, TEXTSELECT_96, 0x0000, NULL, PAGEMIX_SetNumberCB, &limit.min);
    //Row 5
    graph = GUI_CreateXYGraph(10, 118, 300, 112,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              (s16 (*)(s16,  void *))CURVE_Evaluate, NULL, &mixer[0].curve);
}

static void show_dr()
{
    GUI_RemoveAllObjects();
    //Row 0
    show_titlerow();
    //Row 1
    GUI_CreateLabel(10, 40, "Src:", 0x0000);
    GUI_CreateTextSelect(50, 40, TEXTSELECT_96, 0x0000, NULL, set_source_cb, &mixer[0].src);
    GUI_CreateLabel(170, 40, "Curve:", 0x0000);
    GUI_CreateTextSelect(214, 40, TEXTSELECT_96, 0x0000, curveselect_cb, set_curvename_cb, &show_dr);
    //Row 2
    GUI_CreateLabel(10, 66, "Scale:", 0x0000);
    GUI_CreateTextSelect(50, 66, TEXTSELECT_96, 0x0000, NULL, PAGEMIX_SetNumberCB, &mixer[0].scaler);
    GUI_CreateLabel(170, 66, "Offset:", 0x0000);
    GUI_CreateTextSelect(214, 66, TEXTSELECT_96, 0x0000, NULL, PAGEMIX_SetNumberCB, &mixer[0].offset);
    //Row 3
    GUI_CreateLabel(10, 92, "D/R:", 0x0000);
    GUI_CreateTextSelect(50, 92, TEXTSELECT_96, 0x0000, NULL, PAGEMIX_SetNumberCB, &mixer[1].scaler);
    GUI_CreateLabel(170, 92, "Switch:", 0x0000);
    GUI_CreateTextSelect(214, 92, TEXTSELECT_96, 0x0000, NULL, set_source_cb, &mixer[1].sw);
    //Row 4
    GUI_CreateLabel(10, 118, "Max:", 0x0000);
    GUI_CreateTextSelect(50, 118, TEXTSELECT_96, 0x0000, NULL, PAGEMIX_SetNumberCB, &limit.max);
    GUI_CreateLabel(170, 118, "Min:", 0x0000);
    GUI_CreateTextSelect(214, 118, TEXTSELECT_96, 0x0000, NULL, PAGEMIX_SetNumberCB, &limit.min);
    //Row 5
    graph = GUI_CreateXYGraph(10, 144, 300, 86,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              (s16 (*)(s16,  void *))CURVE_Evaluate, NULL, &mixer[0].curve);
}

static void show_expo()
{
    old_curvetype = mixer[1].curve.type;
    mixer[1].curve.type = CURVE_EXPO;
    GUI_RemoveAllObjects();
    //Row 0
    show_titlerow();
    //Row 1
    GUI_CreateLabel(10, 40, "Src:", 0x0000);
    GUI_CreateTextSelect(50, 40, TEXTSELECT_96, 0x0000, NULL, set_source_cb, &mixer[0].src);
    GUI_CreateLabel(170, 40, "Expo:", 0x0000);
    GUI_CreateTextSelect(214, 40, TEXTSELECT_96, 0x0000, curveselect_cb, set_expoval_cb, &show_expo);
    //Row 2
    GUI_CreateLabel(10, 66, "Scale:", 0x0000);
    GUI_CreateTextSelect(50, 66, TEXTSELECT_96, 0x0000, NULL, PAGEMIX_SetNumberCB, &mixer[0].scaler);
    GUI_CreateLabel(170, 66, "Offset:", 0x0000);
    GUI_CreateTextSelect(214, 66, TEXTSELECT_96, 0x0000, NULL, PAGEMIX_SetNumberCB, &mixer[0].offset);
    //Row 3
    GUI_CreateLabel(170, 92, "Switch:", 0x0000);
    GUI_CreateTextSelect(214, 92, TEXTSELECT_96, 0x0000, NULL, set_source_cb, &mixer[1].sw);
    //Row 4
    GUI_CreateLabel(10, 118, "Max:", 0x0000);
    GUI_CreateTextSelect(50, 118, TEXTSELECT_96, 0x0000, NULL, PAGEMIX_SetNumberCB, &limit.max);
    GUI_CreateLabel(170, 118, "Min:", 0x0000);
    GUI_CreateTextSelect(214, 118, TEXTSELECT_96, 0x0000, NULL, PAGEMIX_SetNumberCB, &limit.min);
    //Row 5
    graph = GUI_CreateXYGraph(10, 144, 300, 86,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              (s16 (*)(s16,  void *))CURVE_Evaluate, NULL, &mixer[1].curve);
}

static void show_complex()
{
    GUI_RemoveAllObjects();
    //Row 0
    show_titlerow();
    //Row 1
    GUI_CreateLabel(10, 40, "Src:", 0x0000);
    GUI_CreateTextSelect(50, 40, TEXTSELECT_96, 0x0000, NULL, set_source_cb, &mixer[0].src);
    GUI_CreateLabel(170, 40, "Curve:", 0x0000);
    GUI_CreateTextSelect(214, 40, TEXTSELECT_96, 0x0000, curveselect_cb, set_curvename_cb, &show_simple);
    //Row 2
    GUI_CreateLabel(10, 66, "Scale:", 0x0000);
    GUI_CreateTextSelect(50, 66, TEXTSELECT_96, 0x0000, NULL, PAGEMIX_SetNumberCB, &mixer[0].scaler);
    GUI_CreateLabel(170, 66, "Offset:", 0x0000);
    GUI_CreateTextSelect(214, 66, TEXTSELECT_96, 0x0000, NULL, PAGEMIX_SetNumberCB, &mixer[0].offset);
    //Row 3
    GUI_CreateLabel(10, 92, "Max:", 0x0000);
    GUI_CreateTextSelect(50, 92, TEXTSELECT_96, 0x0000, NULL, PAGEMIX_SetNumberCB, &limit.max);
    GUI_CreateLabel(170, 92, "Min:", 0x0000);
    GUI_CreateTextSelect(214, 92, TEXTSELECT_96, 0x0000, NULL, PAGEMIX_SetNumberCB, &limit.min);
    //Row 4
    GUI_CreateLabel(10, 118, "Switch:", 0x0000);
    GUI_CreateTextSelect(50, 118, TEXTSELECT_96, 0x0000, NULL, set_source_cb, &mixer[0].sw);
    graph = GUI_CreateXYGraph(170, 118, 140, 112,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              (s16 (*)(s16,  void *))CURVE_Evaluate, NULL, &mixer[0].curve);
    //Row 5
    GUI_CreateLabel(10, 144, "Mux:", 0x0000);
    //GUI_CreateTextSelect(50, 144, TEXTSELECT_96, 0x0000, NULL, set_source_cb, &mixer[1].sw);
    //Row 6
    GUI_CreateLabel(10, 170, "Mixers:", 0x0000);
    //GUI_CreateTextSelect(50, 170, TEXTSELECT_96, 0x0000, NULL, set_source_cb, &mixer[1].sw);
    //Row 6
    GUI_CreateLabel(10, 196, "Page:", 0x0000);
    //GUI_CreateTextSelect(50, 196, TEXTSELECT_96, 0x0000, NULL, set_source_cb, &mixer[1].sw);
}

const char *PAGEMIX_SetNumberCB(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    s8 *value = (s8 *)data;
    if (dir > 0) {
        if (*value < 100) {
            if (dir == 1) {
                //short_press
                *value += 1;
            } else {
                //long_press
                *value += 5;
                if (*value > 100)
                    *value = 100;
            }
        }
    } else if (dir < 0) {
        if (*value > -100) {
            if (dir == -1) {
                //short_press
                *value -= 1;
            } else {
                //long_press
                *value -= 5;
                if (*value < -100)
                    *value = -100;
            }
        }
    }
    sprintf(tmpstr, "%d", *value);
    return tmpstr;
}

const char *set_expoval_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    s8 origval = mixer[1].curve.points[0];
    const char *str = PAGEMIX_SetNumberCB(obj, dir, &mixer[1].curve.points[0]);
    if (origval != mixer[1].curve.points[0])
        GUI_Redraw(graph);
    return str;
}

const char *set_source_cb(guiObject_t *obj, int dir, void *data)
{
    (void) obj;
    u8 *source = (u8 *)data;
    if (dir > 0) {
        if (*source < NUM_INPUTS + NUM_CHANNELS)
           *source += 1;
    } else if (dir < 0) {
        if (*source)
            *source -= 1;
    }
    if(! *source) {
        return "None";
    }
    if(*source <= NUM_TX_INPUTS) {
        return tx_input_str[*source - 1];
    }
    if(*source <= NUM_INPUTS) {
        sprintf(tmpstr, "CYC%d", *source - NUM_TX_INPUTS - 1);
        return tmpstr;
    }
    return channel_name[*source - NUM_INPUTS - 1];
}

static const char *set_curvename_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    struct Curve *curve = &mixer[0].curve;
    if(dir > 0 && curve->type < CURVE_MAX) {
        curve->type++;
        GUI_Redraw(graph);
    } else if(dir < 0 && curve->type) {
        curve->type--;
        GUI_Redraw(graph);
    }
    return CURVE_GetName(curve);
}

void curveselect_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    MIXPAGE_EditCurves(&mixer[0].curve, data);   
}

static void okcancel_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    if (data) {
        //Save mixer here
        MIX_SetLimit(channel, &limit);
        MIX_SetTemplate(channel, cur_template);
        switch(cur_template) {
        case MIXERTEMPLATE_NONE:
            mixer[0].src = 0;
        case MIXERTEMPLATE_SIMPLE:
            mixer[0].sw = 0;
            mixer[0].mux = MUX_REPLACE;
        case MIXERTEMPLATE_COMPLEX:
            MIX_SetMixers(mixer, 1);
            break;
        case MIXERTEMPLATE_DR:
            mixer[1].src = mixer[0].src;
            mixer[1].dest = mixer[0].dest;
            mixer[1].curve = mixer[0].curve;
            mixer[1].mux = MUX_REPLACE;
            //Scale the offset by the value of scaler ???
            mixer[1].offset = (s32)mixer[0].offset * mixer[1].scaler / mixer[0].scaler;
            mixer[0].sw = 0;
            mixer[0].mux = MUX_REPLACE;
            MIX_SetMixers(mixer, 2);
            break;
        case MIXERTEMPLATE_EXPO:
            mixer[0].curve.type = CURVE_NONE;
            mixer[1].src = mixer[0].src;
            mixer[1].dest = mixer[0].dest;
            mixer[1].mux = MUX_REPLACE;
            mixer[1].offset = mixer[0].offset;
            mixer[1].scaler = mixer[0].scaler;
            mixer[0].sw = 0;
            mixer[0].mux = MUX_REPLACE;
            MIX_SetMixers(mixer, 2);
            
        }
    }
    GUI_RemoveAllObjects();
    PAGE_MixerInit(0);
}
