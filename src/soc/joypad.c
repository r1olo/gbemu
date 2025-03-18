#include "soc/soc.h"

uint8_t
jp_read(jp_t *jp)
{
    /* whole jp logic ahead */
    uint8_t higher_nibble = 0xC | (jp->action << 1) | jp->dpad;
    uint8_t lower_nibble = 0xF;
    if (jp->action)
        lower_nibble &= ~(jp->start << 3) & ~(jp->select << 2) &
            ~(jp->b << 1) & !jp->a;
    if (jp->dpad)
        lower_nibble &= ~(jp->down << 3) & ~(jp->up << 2) &
            ~(jp->left << 1) & !jp->right;

    return (higher_nibble << 8) | lower_nibble;
}

void
jp_write(jp_t *jp, uint8_t val)
{
    /* just these two are set */
    jp->action = JP_ACTION(val);
    jp->dpad = JP_DPAD(val);
}

void
jp_cycle(jp_t *jp)
{
    /* get the current status of the joypad for comparison (buttons only) */
    uint8_t cur = jp_read(jp);

    /* if any of the buttons are pressed, check whether we need to fire the
     * interrupt */
    if (jp->start_pressed) {
        jp->start = true;
        if (JP_START_DOWN(cur) && jp->action)
            soc_interrupt(jp->soc, INT_JP);
    } else {
        jp->start = false;
    }

    if (jp->select_pressed) {
        jp->select = true;
        if (JP_SELECT_UP(cur) && jp->action)
            soc_interrupt(jp->soc, INT_JP);
    } else {
        jp->select = false;
    }

    if (jp->a_pressed) {
        jp->a = true;
        if (JP_A_RIGHT(cur) && jp->action)
            soc_interrupt(jp->soc, INT_JP);
    } else {
        jp->a = false;
    }

    if (jp->b_pressed) {
        jp->b = true;
        if (JP_B_LEFT(cur) && jp->action)
            soc_interrupt(jp->soc, INT_JP);
    } else {
        jp->b = false;
    }

    if (jp->down_pressed) {
        jp->down = true;
        if (JP_START_DOWN(cur) && jp->dpad)
            soc_interrupt(jp->soc, INT_JP);
    } else {
        jp->down = false;
    }

    if (jp->up_pressed) {
        jp->up = true;
        if (JP_SELECT_UP(cur) && jp->dpad)
            soc_interrupt(jp->soc, INT_JP);
    } else {
        jp->up = false;
    }

    if (jp->left_pressed) {
        jp->left = true;
        if (JP_B_LEFT(cur) && jp->dpad)
            soc_interrupt(jp->soc, INT_JP);
    } else {
        jp->left = false;
    }

    if (jp->right_pressed) {
        jp->right = true;
        if (JP_A_RIGHT(cur) && jp->dpad)
            soc_interrupt(jp->soc, INT_JP);
    } else {
        jp->right = false;
    }
}

void
jp_init(jp_t *jp, soc_t *soc)
{
    /* set up soc */
    jp->soc = soc;

    /* initially none of the buttons are up */
    jp->start = jp->select = jp->a = jp->b = false;
    jp->down = jp->up = jp->left = jp->right = false;

    /* also not pressed currently */
    jp->start_pressed = jp->select_pressed = false;
    jp->a_pressed = jp->b_pressed = false;
    jp->down_pressed = jp->up_pressed = false;
    jp->left_pressed = jp->right_pressed = false;
}
