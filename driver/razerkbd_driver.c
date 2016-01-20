/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Should you need to contact me, the author, you can do so either by
 * e-mail - mail your message to <pez2001@voyagerproject.de>, or by paper mail:
 * Tim Theede, Am See 22, 24790 Schuelldorf, Germany
 * Terry Cain <terrys-home.co.uk>
 */


#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb/input.h>
#include <linux/hid.h>

#include "razerkbd_driver.h"
#include "razercommon.h"


/*
 * Version Information
 */
#define DRIVER_VERSION "0.2"
#define DRIVER_AUTHOR "Tim Theede <pez2001@voyagerproject.de>"
#define DRIVER_DESC "USB HID Razer BlackWidow Chroma"
#define DRIVER_LICENSE "GPL v2"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE(DRIVER_LICENSE);

/*

    TODO:

        restore store rgb profile (helpful for event-animations etc)
        #coloritup update

    future todos:

        read keystroke stats etc.

*/

/**
 * Send report to the keyboard
 */
int razer_send_report(struct usb_device *usb_dev,void const *data) {
    return razer_send_control_msg(usb_dev, data, 0x02, RAZER_BLACKWIDOW_CHROMA_WAIT_MIN_US, RAZER_BLACKWIDOW_CHROMA_WAIT_MAX_US);
}

/**
 * Set pulsate effect on the keyboard
 *
 * Supported by:
 *   Razer BlackWidow Ultimate 2013
 *
 * Still not working
 */
int razer_set_pulsate_mode(struct usb_device *usb_dev)
{
    int retval = 0;
    struct razer_report report;
    razer_prepare_report(&report);
    report.parameter_bytes_num = 0x03;
    report.command = 0x02;

    report.sub_command = 0x01;
    report.command_parameters[0] = 0x04;
    report.command_parameters[1] = 0x02;
    report.crc = razer_calculate_crc(&report);
    retval = razer_send_report(usb_dev, &report);

    return retval;
}

/**
 * Get the devices serial number
 *
 * Makes a request like normal, this must change a variable in the mouse as then we
 * tell it give us data (same request for get_battery in the mouse driver) and it 
 * gives us a report.
 *
 * Supported Devices:
 *   Razer Chroma
 *   Razer BlackWidow Ultimate 2013*
 * 
 * *Untested but should work
 */
void razer_get_serial(struct usb_device *usb_dev, unsigned char* serial_string)
{
    struct razer_report response_report;
    struct razer_report request_report;
    int retval;
    int i;

    razer_prepare_report(&request_report);

    request_report.parameter_bytes_num = 0x16;
    request_report.reserved2 = 0x00;
    request_report.command = 0x82;
    request_report.sub_command = 0x00;
    request_report.command_parameters[0] = 0x00;
    request_report.crc = razer_calculate_crc(&request_report);


    retval = razer_get_usb_response(usb_dev, 0x01, &request_report, 0x01, &response_report, RAZER_BLACKWIDOW_CHROMA_WAIT_MIN_US, RAZER_BLACKWIDOW_CHROMA_WAIT_MAX_US);

    if(retval == 0)
    {
        if(response_report.report_start_marker == 0x02 && response_report.reserved2 == 0x00 && response_report.command == 0x82)
        {
            unsigned char* pointer = &response_report.sub_command;
            for(i = 0; i < 20; ++i)
            {
                serial_string[i] = *pointer;
                ++pointer;
            }
        } else
        {
            print_erroneous_report(&response_report, "razerkbd", "Invalid Report Type");
        }
    } else
    {
      print_erroneous_report(&response_report, "razerkbd", "Invalid Report Length");
    }
}

/**
 * Set game mode on the keyboard
 *
 * Supported by:
 *   Razer BlackWidow Chroma
 *   Razer BlackWidow Ultimate 2013
 */
int razer_set_game_mode(struct usb_device *usb_dev, unsigned char enable)
{
    int retval = 0;
    if(enable > 1)
    {
        printk(KERN_WARNING "razerkbd: Cannot set game mode to %d. Only 1 or 0 allowed.", enable);
    } else
    {
        struct razer_report report;
        razer_prepare_report(&report);
        report.parameter_bytes_num = 0x03;
        report.command = 0x00;
        report.sub_command = 0x01;
        report.command_parameters[0] = 0x08;
        report.command_parameters[1] = enable;
        report.crc = razer_calculate_crc(&report);
        retval = razer_send_report(usb_dev, &report);
    }
    return retval;
}

/**
 * Set the wave effect on the keyboard
 *
 * Supported by:
 *   Razer BlackWidow Chroma
 */
int razer_set_wave_mode(struct usb_device *usb_dev, unsigned char direction)
{
    int retval;
    struct razer_report report;
    razer_prepare_report(&report);
    report.parameter_bytes_num = 0x02;
    report.command = RAZER_BLACKWIDOW_CHROMA_CHANGE_EFFECT;   /* Change effect command ID */
    report.sub_command = RAZER_BLACKWIDOW_CHROMA_EFFECT_WAVE; /* Wave mode ID */
    report.command_parameters[0] = direction;                 /* Direction 2=Left / 1=Right */
    report.crc = razer_calculate_crc(&report);
    retval = razer_send_report(usb_dev, &report);
    return retval;
}

/**
 * Set no effect on the keyboard
 *
 * Supported by:
 *   Razer BlackWidow Chroma
 */
int razer_set_none_mode(struct usb_device *usb_dev)
{
    int retval;
    struct razer_report report;
    razer_prepare_report(&report);
    report.parameter_bytes_num = 0x01;
    report.command = RAZER_BLACKWIDOW_CHROMA_CHANGE_EFFECT; /*change effect command id*/
    report.sub_command = 0x00;/*none mode id*/
    //report.command_parameters[0] = 0x01; /*profile index? active ?*/
    report.crc = razer_calculate_crc(&report);
    retval = razer_send_report(usb_dev, &report);
    return retval;
}

/**
 * Set reactive effect on the keyboard
 *
 * The speed must be within 01-03
 *
 * 1 Short, 2 Medium, 3 Long
 *
 * Supported by:
 *   Razer BlackWidow Chroma
 */
int razer_set_reactive_mode(struct usb_device *usb_dev, struct razer_rgb *color, unsigned char speed)
{
    int retval = 0;
    if(speed > 0 && speed < 4)
    {
        struct razer_report report;
        razer_prepare_report(&report);
        report.parameter_bytes_num = 0x05;
        report.command = RAZER_BLACKWIDOW_CHROMA_CHANGE_EFFECT; /*change effect command id*/
        report.sub_command = RAZER_BLACKWIDOW_CHROMA_EFFECT_REACTIVE;/*reactive mode id*/
        report.command_parameters[0] = speed;/*identified by Oleg Finkelshteyn*/
        report.command_parameters[1] = color->r; /*rgb color definition*/
        report.command_parameters[2] = color->g;
        report.command_parameters[3] = color->b;
        report.crc = razer_calculate_crc(&report);
        retval = razer_send_report(usb_dev, &report);
    } else
    {
        printk(KERN_WARNING "razerkbd: Reactive mode, Speed must be within 1-3. Got: %d", speed);
    }
    return retval;
}

/**
 * Set breath effect on the keyboard
 *
 * Breathing types
 * 1: Only 1 Colour
 * 2: 2 Colours
 * 3: Random
 *
 * Supported by:
 *   Razer BlackWidow Chroma
 *
 */
int razer_set_breath_mode(struct usb_device *usb_dev, unsigned char breathing_type, struct razer_rgb *color1, struct razer_rgb *color2)
{
    int retval;
    struct razer_report report;
    razer_prepare_report(&report);
    report.parameter_bytes_num = 0x08;
    report.command = RAZER_BLACKWIDOW_CHROMA_CHANGE_EFFECT;
    report.sub_command = RAZER_BLACKWIDOW_CHROMA_EFFECT_BREATH;

    report.command_parameters[0] = breathing_type;

    if(breathing_type == 1 || breathing_type == 2)
    {
        // Colour 1
        report.command_parameters[1] = color1->r;
        report.command_parameters[2] = color1->g;
        report.command_parameters[3] = color1->b;
    }

    if(breathing_type == 2)
    {
        // Colour 2
        report.command_parameters[4] = color2->r;
        report.command_parameters[5] = color2->g;
        report.command_parameters[6] = color2->b;
    }

    //printk(KERN_WARNING "razerkbd: Breath C1: %02x%02x%02x, C2: %02x%02x%02x", color1->r, color1->g, color1->b, color2->r, color2->g, color2->b);

    report.crc = razer_calculate_crc(&report);
    retval = razer_send_report(usb_dev, &report);
    return retval;
}

/**
 * Set spectrum effect on the keyboard
 *
 * Supported by:
 *   Razer BlackWidow Chroma
 */
int razer_set_spectrum_mode(struct usb_device *usb_dev)
{
    int retval;
    struct razer_report report;
    razer_prepare_report(&report);
    report.parameter_bytes_num = 0x01;
    report.command = RAZER_BLACKWIDOW_CHROMA_CHANGE_EFFECT; /*change effect command id*/
    report.sub_command = RAZER_BLACKWIDOW_CHROMA_EFFECT_SPECTRUM;/*spectrum mode id*/
    report.crc = razer_calculate_crc(&report);
    retval = razer_send_report(usb_dev, &report);
    return retval;
}

/**
 * Set custom effect on the keyboard
 *
 * Supported by:
 *   Razer BlackWidow Chroma
 */
int razer_set_custom_mode(struct usb_device *usb_dev)
{
    int retval;
    struct razer_report report;
    razer_prepare_report(&report);
    report.parameter_bytes_num = 0x02;
    report.command = RAZER_BLACKWIDOW_CHROMA_CHANGE_EFFECT; /*change effect command id*/
    report.sub_command = RAZER_BLACKWIDOW_CHROMA_EFFECT_CUSTOM;/*custom mode id*/
    report.command_parameters[0] = 0x01; /*profile index? active ?*/
    report.crc = razer_calculate_crc(&report);
    retval = razer_send_report(usb_dev, &report);
    return retval;
}

/**
 * Set static effect on the keyboard
 *
 * Supported by:
 *   Razer BlackWidow Chroma
 */
int razer_set_static_mode(struct usb_device *usb_dev, struct razer_rgb *color)
{
    int retval;
    struct razer_report report;
    razer_prepare_report(&report);
    report.parameter_bytes_num = 0x04;
    report.command = RAZER_BLACKWIDOW_CHROMA_CHANGE_EFFECT; /*change effect command id*/
    report.sub_command = RAZER_BLACKWIDOW_CHROMA_EFFECT_STATIC;/*static mode id*/
    report.command_parameters[0] = color->r; /*rgb color definition*/
    report.command_parameters[1] = color->g;
    report.command_parameters[2] = color->b;
    report.crc = razer_calculate_crc(&report);
    retval = razer_send_report(usb_dev, &report);
    return retval;
}

/**
 * Set static effect on the keyboard
 *
 * Supported by:
 *   Razer BlackWidow Ultimate 2013
 */
int razer_set_static_mode_blackwidow_ultimate(struct usb_device *usb_dev)
{
    int retval = 0;
    struct razer_report report;
    razer_prepare_report(&report);
    report.parameter_bytes_num = 0x03;
    report.command = 0x02;
    report.sub_command = 0x01;
    report.command_parameters[0] = 0x04;
    report.command_parameters[1] = 0x00;
    report.crc = razer_calculate_crc(&report);
    retval = razer_send_report(usb_dev, &report);
    return retval;
}

/**
 * Clear row on the keyboard
 *
 * Clears a row's colour on the keyboard. Rows range from 0-5, 0 being the top row with escape
 * and 5 being the last row with the spacebar
 *
 * Supported by:
 *   Razer BlackWidow Chroma
 */
int razer_temp_clear_row(struct usb_device *usb_dev, unsigned char row_index)
{
    int retval;
    struct razer_report report;
    razer_prepare_report(&report);
    report.parameter_bytes_num = 0x02;
    report.command = RAZER_BLACKWIDOW_CHROMA_CHANGE_EFFECT; /*change effect command id*/
    report.sub_command = RAZER_BLACKWIDOW_CHROMA_EFFECT_CLEAR_ROW;/*clear_row mode id*/
    report.command_parameters[0] = row_index; /*line number starting from top*/
    report.crc = razer_calculate_crc(&report);
    retval = razer_send_report(usb_dev, &report);
    return retval;
}

/**
 * Set row colour on the keyboard
 *
 * This sets the colour of a row on the keyboard. Takes in an array of RGB bytes.
 * The mappings below are correct for the BlackWidow Chroma. The BlackWidow Ultimate 2016
 * contains LEDs under the spacebar and the FN key so there will be changes once I get the
 * hardware.
 *
 * Row 0:
 *  0      Unused
 *  1      ESC
 *  2      Unused
 *  3-14   F1-F12
 *  15-17  PrtScr, ScrLk, Pause
 *  18-19  Unused
 *  20     Razer Logo
 *  21     Unused
 *
 * Row 1:
 *  0-21   M1 -> NP Minus
 *
 * Row 2:
 *  0-13   M2 -> Right Square Bracket ]
 *  14 Unused
 *  15-21 Delete -> NP Plus
 *
 * Row 3:
 *  0-14   M3 -> Return
 *  15-17  Unused
 *  18-20  NP4 -> NP6
 *
 * Row 4:
 *  0-12   M4 -> Forward Slash /
 *  13     Unused
 *  14     Right Shift
 *  15     Unused
 *  16     Up Arrow Key
 *  17     Unused
 *  18-21  NP1 -> NP Enter
 *
 * Row 5:
 *  0-3    M5 -> Alt
 *  4-10   Unused
 *  11     Alt GR
 *  12     Unused
 *  13-17  Context Menu Key -> Right Arrow Key
 *  18     Unused
 *  19-20  NP0 -> NP.
 *  21     Unused
 *
 * Supported by:
 *   Razer BlackWidow Chroma
 */
int razer_set_key_row(struct usb_device *usb_dev, unsigned char row_index, unsigned char *row_cols) //struct razer_row_rgb *row_cols)
{
    int retval;
    struct razer_report report;
    razer_prepare_report(&report);
    report.parameter_bytes_num = RAZER_BLACKWIDOW_CHROMA_ROW_LEN * 3 + 4;
    report.command = 0x0B; /*set keys command id*/
    report.sub_command = 0xFF;/*set keys mode id*/
    report.command_parameters[0] = row_index; /*row number*/
    report.command_parameters[1] = 0x00; /*unknown always 0*/
    report.command_parameters[2] = RAZER_BLACKWIDOW_CHROMA_ROW_LEN - 1; /*number of keys in row always 21*/
    memcpy(&report.command_parameters[3], row_cols, (report.command_parameters[2]+1)*3);
    report.crc = razer_calculate_crc(&report);
    retval = razer_send_report(usb_dev, &report);
    return retval;
}

/**
 * Reset the keyboard
 *
 * Supported by:
 *   Razer BlackWidow Chroma
 *   Razer BlackWidow Ultimate 2013
 */
int razer_reset(struct usb_device *usb_dev)
{
    int retval;
    struct razer_report report;
    razer_prepare_report(&report);
    report.parameter_bytes_num = 0x03;
    report.command = 0x00; /*reset command id*/
    report.sub_command = 0x01;/*unknown*/
    report.command_parameters[0] = 0x08;/*unknown*/
    report.command_parameters[1] = 0x00;/*unknown*/
    report.crc = razer_calculate_crc(&report);
    retval = razer_send_report(usb_dev, &report);
    return retval;
}

/**
 * Set the keyboard brightness
 *
 * Supported by:
 *   Razer BlackWidow Chroma
 *   Razer BlackWidow Ultimate 2013
 */
int razer_set_brightness(struct usb_device *usb_dev, unsigned char brightness)
{
    int retval;
    struct razer_report report;
    razer_prepare_report(&report);
    report.parameter_bytes_num = 0x03;
    report.command = 0x03; /*set brightness command id*/
    report.sub_command = 0x01;/*unknown*/

    if(usb_dev->descriptor.idProduct == USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2013 || 
       usb_dev->descriptor.idProduct == USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_STEALTH_2016)
    {
        report.command_parameters[0] = 0x04;/*unknown (not speed)*/
    } else if(usb_dev->descriptor.idProduct == USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA)
    {
        report.command_parameters[0] = 0x05;/*unknown (not speed)*/
    } else {
        printk(KERN_WARNING "razerkbd: Unknown product ID '%d'", usb_dev->descriptor.idProduct);
    }

    report.command_parameters[1] = brightness;
    report.crc = razer_calculate_crc(&report);
    retval = razer_send_report(usb_dev, &report);
    return retval;
}

/**
 * Enable keyboard macro keys
 *
 * The keycodes for the macro keys are 191-195 for M1-M5.
 * Been tested on the Chroma and the Ultimate
 *
 * Supported by:
 *   Razer BlackWidow Chroma
 *   Razer BlackWidow Ultimate 2013
 */
int razer_activate_macro_keys(struct usb_device *usb_dev)
{
    int retval;
    struct razer_report report;
    razer_prepare_report(&report);
    report.parameter_bytes_num = 0x02;
    report.command = 0x04; /*reset command id*/
    report.reserved2 = 0x00;
    report.sub_command = 0x02;/*unknown*/
    report.command_parameters[0] = 0x00;/*unknown*/
    report.crc = razer_calculate_crc(&report);
    retval = razer_send_report(usb_dev, &report);
    return retval;
}

//TESTS
/**
 * Not sure what this does.
 */
int razer_test(struct usb_device *usb_dev)
{
    int retval;
    struct razer_report report;
    razer_prepare_report(&report);
    report.parameter_bytes_num = 0x02;
    report.command = 0x00; /*init command id ?*/
    report.sub_command = 0x04;/*unknown*/
    report.command_parameters[0] = 8;/*unknown*/
    report.command_parameters[1] = 0;/*unknown*/
    report.crc = razer_calculate_crc(&report);
    retval = razer_send_report(usb_dev, &report);
    return retval;

}

/**
 * Change the effect on the keyboard
 *
 * Currently contains unknown commands
 *
 * Supported by:
 *   Razer BlackWidow Chroma
 */
void razer_change_effect(struct usb_device *usb_dev, uint effect_id)
{
    struct razer_report report;
    razer_prepare_report(&report);

    switch(effect_id)
    {
        case RAZER_BLACKWIDOW_CHROMA_EFFECT_UNKNOWN3:
            printk(KERN_ALERT "setting mode to: Unknown3\n");//most likely stats profile change    
            report.parameter_bytes_num = 0x03;
            report.command = 0x01; /*reset command id*/
            report.sub_command = 0x05;/*unknown*/
            report.command_parameters[0] = 0xFF;/*unknown*/
            report.command_parameters[0] = 0x01;/*unknown*/
            report.crc = razer_calculate_crc(&report);
            razer_send_report(usb_dev, &report);
            break;
        case RAZER_BLACKWIDOW_CHROMA_EFFECT_UNKNOWN:
            printk(KERN_ALERT "setting mode to: Unknown\n");//most likely stats profile change    
            report.parameter_bytes_num = 0x03;
            report.command = 0x02; /*reset command id*/
            report.sub_command = 0x00;/*unknown*/
            report.command_parameters[0] = 0x07;/*unknown*/
            report.command_parameters[1] = 0x00;/*unknown*/
            report.crc = razer_calculate_crc(&report);
            razer_send_report(usb_dev, &report);
            break;
        case RAZER_BLACKWIDOW_CHROMA_EFFECT_UNKNOWN2:
            printk(KERN_ALERT "setting mode to: Unknown2\n");    
            report.parameter_bytes_num = 0x03;
            report.command = 0x00; /*reset command id*/
            report.sub_command = 0x00;/*unknown*/
            report.command_parameters[0] = 0x07;/*unknown*/
            report.command_parameters[1] = 0x00;/*unknown*/
            report.crc = razer_calculate_crc(&report);
            razer_send_report(usb_dev, &report);
            break;
        case RAZER_BLACKWIDOW_CHROMA_EFFECT_UNKNOWN4:
            printk(KERN_ALERT "setting mode to: Unknown4\n");    
            report.parameter_bytes_num = 0x03;
            report.command = 0x00; /*reset command id*/
            report.sub_command = 0x01;/*unknown*/
            report.command_parameters[0] = 0x05;/*unknown*/
            report.command_parameters[1] = 0x01;/*unknown*/
            report.crc = razer_calculate_crc(&report);
            razer_send_report(usb_dev, &report);
            break;
            //1,8,0 
    }
}

/**
 * Get raw event from the keyboard
 *
 * Useful if the keyboard's 2 keyboard devices are binded then keypress's can be
 * monitored and used.
 */
static int razer_raw_event(struct hid_device *hdev,
    struct hid_report *report, u8 *data, int size)
{
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
    //struct razer_kbd_device *widow = hid_get_drvdata(hdev);

    if (intf->cur_altsetting->desc.bInterfaceProtocol != USB_INTERFACE_PROTOCOL_MOUSE)
        return 0;

    return 0;
}

/**
 * Write device file "test"
 *
 * Does nothing
 */
static ssize_t razer_attr_write_test(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    razer_test(usb_dev);
    return count;
}

/**
 * Write device file "reset"
 *
 * Resets the keyboard whenever anything is written
 */
static ssize_t razer_attr_write_reset(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    razer_reset(usb_dev);
    return count;
}

/**
 * Write device file "macro_keys"
 *
 * Enables the macro keys whenever the file is written to
 */
static ssize_t razer_attr_write_macro_keys(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    razer_activate_macro_keys(usb_dev);
    return count;
}

/**
 * Write device file "mode_game"
 *
 * When 1 is written (as a character, 0x31) Game mode will be enabled, if 0 is written (0x30)
 * then game mode will be disabled
 *
 * The reason the keyboard appears as 2 keyboard devices is that one of those devices is used by
 * game mode as that keyboard device is missing a super key. A hacky and over-the-top way to disable
 * the super key if you ask me.
 */
static ssize_t razer_attr_write_mode_game(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    int temp = simple_strtoul(buf, NULL, 10);
    razer_set_game_mode(usb_dev, temp);
    return count;
}

/**
 * Write device file "mode_pulsate"
 *
 * The brightness oscillates between fully on and fully off generating a pulsing effect
 */
static ssize_t razer_attr_write_mode_pulsate(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    razer_set_pulsate_mode(usb_dev);
    return count;
}

/**
 * Write device file "mode_wave"
 *
 * When 1 is written (as a character, 0x31) the wave effect is displayed moving left across the keyboard
 * if 2 is written (0x32) then the wave effect goes right
 */
static ssize_t razer_attr_write_mode_wave(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    int temp = simple_strtoul(buf, NULL, 10);
    razer_set_wave_mode(usb_dev, temp);
    return count;
}
/**
 * Write device file "mode_spectrum"
 *
 * Specrum effect mode is activated whenever the file is written to
 */
static ssize_t razer_attr_write_mode_spectrum(struct device *dev, struct device_attribute *attr,
               const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    razer_set_spectrum_mode(usb_dev);
    return count;
}

/**
 * Write device file "mode_none"
 *
 * No keyboard effect is activated whenever this file is written to
 */
static ssize_t razer_attr_write_mode_none(struct device *dev, struct device_attribute *attr,
               const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    razer_set_none_mode(usb_dev);
    return count;
}

/**
 * Write device file "set_brightness"
 *
 * Sets the brightness to the ASCII number written to this file.
 */
static ssize_t razer_attr_write_set_brightness(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    int temp = simple_strtoul(buf, NULL, 10);
    razer_set_brightness(usb_dev, (unsigned char)temp);
    return count;
}

/**
 * Write device file "mode_reactive"
 *
 * Sets reactive mode when this file is written to. A speed byte and 3 RGB bytes should be written
 */
static ssize_t razer_attr_write_mode_reactive(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    if(count == 4)
    {
        unsigned char speed = (unsigned char)buf[0];
        razer_set_reactive_mode(usb_dev, (struct razer_rgb*)&buf[1], speed);
    }
    return count;
}

/**
 * Write device file "mode_breath"
 */
static ssize_t razer_attr_write_mode_breath(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);

    const char *alt_buf[6] = { 0 };

    if(count == 3)
    {
        // Single colour mode
        razer_set_breath_mode(usb_dev, 0x01, (struct razer_rgb*)&buf[0], (struct razer_rgb*)&alt_buf[3]);
    } else if(count == 6)
    {
        // Dual colour mode
        razer_set_breath_mode(usb_dev, 0x02, (struct razer_rgb*)&buf[0], (struct razer_rgb*)&buf[3]);
    } else
    {
        // "Random" colour mode
        razer_set_breath_mode(usb_dev, 0x03, (struct razer_rgb*)&alt_buf[0], (struct razer_rgb*)&alt_buf[3]);
    }
    return count;
}

/**
 * Write device file "mode_custom"
 *
 * Sets the keyboard to custom mode whenever the file is written to
 */
static ssize_t razer_attr_write_mode_custom(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    razer_reset(usb_dev);
    razer_set_custom_mode(usb_dev);
    return count;
}

/**
 * Write device file "mode_static"
 *
 * Set the keyboard to static mode when 3 RGB bytes are written
 */
static ssize_t razer_attr_write_mode_static(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);

    if(usb_dev->descriptor.idProduct == USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2013 ||
       usb_dev->descriptor.idProduct == USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_STEALTH_2016)
    {
        // Set BlackWidow Ultimate to static colour
        razer_set_static_mode_blackwidow_ultimate(usb_dev);

    } else if(usb_dev->descriptor.idProduct == USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA)
    {
        // Set BlackWidow Chroma to static colour
        if(count == 3)
        {
            razer_set_static_mode(usb_dev, (struct razer_rgb*)&buf[0]);
        }

    } else
    {
        printk(KERN_WARNING "razerkbd: Cannot set static mode for this device");
    }
    return count;
}

/**
 * Write device file "temp_clear_row"
 *
 * Clears a row when an ASCII number is written
 */
static ssize_t razer_attr_write_temp_clear_row(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    int temp = simple_strtoul(buf, NULL, 10);
    razer_temp_clear_row(usb_dev, temp);
    return count;
}

/**
 * Write device file "set_key_row"
 *
 * Writes the colour rows on the keyboard. Takes in all the colours for the keyboard
 */
static ssize_t razer_attr_write_set_key_row(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    size_t offset = 0;
    unsigned char row_index;
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    size_t buf_size = RAZER_BLACKWIDOW_CHROMA_ROW_LEN * 3 + 1;

    while(offset < count) {
        if((count-offset) < buf_size) {
            printk(KERN_ALERT "Wrong Amount of RGB data provided: %d of %d\n",(int)(count-offset), (int)buf_size);
            return -EINVAL;
        }
        row_index = (unsigned char)buf[offset];
        razer_set_key_row(usb_dev, row_index, (unsigned char*)&buf[offset + 1]);
        offset += buf_size;
    }
    return count;
}

/**
 * Read device file "device_type"
 *
 * Returns friendly string of device type
 */
static ssize_t razer_attr_read_device_type(struct device *dev, struct device_attribute *attr,
                char *buf)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    //struct razer_kbd_device *widow = usb_get_intfdata(intf);
    struct usb_device *usb_dev = interface_to_usbdev(intf);

    int write_count = 0;
    if(usb_dev->descriptor.idProduct == USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA)
    {
        write_count = sprintf(buf, "Razer BlackWidow Chroma\n");
    } else if(usb_dev->descriptor.idProduct == USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA_TE)
    {
        write_count = sprintf(buf, "Razer BlackWidow Chroma Tournament Edition\n");
    } else if(usb_dev->descriptor.idProduct == USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2013)
    {
        write_count = sprintf(buf, "Razer BlackWidow Ultimate 2013\n");
    } else if(usb_dev->descriptor.idProduct == USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_STEALTH_2016)
    {
        write_count = sprintf(buf, "Razer BlackWidow Ultimate Stealth 2016\n");
    } else
    {
        write_count = sprintf(buf, "Unknown Device\n");
    }
    return write_count;
}

/**
 * Read device file "get_serial"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_get_serial(struct device *dev, struct device_attribute *attr, char *buf)
{
	char serial_string[100] = ""; // Cant be longer than this as report length is 90
	
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);

    razer_get_serial(usb_dev, &serial_string[0]);
    return sprintf(buf, "%s\n", &serial_string[0]);
}


/**
 * Set up the device driver files
 *
 * Read only is 0444
 * Write only is 0220
 * Read and write is 0664
 */
static DEVICE_ATTR(device_type,    0444, razer_attr_read_device_type, NULL);
static DEVICE_ATTR(get_serial,     0444, razer_attr_read_get_serial,  NULL);
static DEVICE_ATTR(mode_game,      0220, NULL, razer_attr_write_mode_game);
static DEVICE_ATTR(mode_pulsate,   0220, NULL, razer_attr_write_mode_pulsate);
static DEVICE_ATTR(mode_wave,      0220, NULL, razer_attr_write_mode_wave);
static DEVICE_ATTR(mode_spectrum,  0220, NULL, razer_attr_write_mode_spectrum);
static DEVICE_ATTR(mode_none,      0220, NULL, razer_attr_write_mode_none);
static DEVICE_ATTR(mode_reactive,  0220, NULL, razer_attr_write_mode_reactive);
static DEVICE_ATTR(mode_breath,    0220, NULL, razer_attr_write_mode_breath);
static DEVICE_ATTR(mode_custom,    0220, NULL, razer_attr_write_mode_custom);
static DEVICE_ATTR(mode_static,    0220, NULL, razer_attr_write_mode_static);
static DEVICE_ATTR(temp_clear_row, 0220, NULL, razer_attr_write_temp_clear_row);
static DEVICE_ATTR(set_key_row,    0220, NULL, razer_attr_write_set_key_row);
static DEVICE_ATTR(reset,          0220, NULL, razer_attr_write_reset);
static DEVICE_ATTR(macro_keys,     0220, NULL, razer_attr_write_macro_keys);
static DEVICE_ATTR(set_brightness, 0220, NULL, razer_attr_write_set_brightness);
static DEVICE_ATTR(test,           0220, NULL, razer_attr_write_test);


/**
 * Probe method is ran whenever a device is binded to the driver
 *
 * TODO remove goto's
 */
static int razer_kbd_probe(struct hid_device *hdev,
             const struct hid_device_id *id)
{
    int retval;
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    struct razer_kbd_device *dev = NULL;

    dev = kzalloc(sizeof(struct razer_kbd_device), GFP_KERNEL);
    if(dev == NULL) {
        dev_err(&intf->dev, "out of memory\n");
        retval = -ENOMEM;
        goto exit;
    }
    
    if(usb_dev->descriptor.idProduct == USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2013 ||
        usb_dev->descriptor.idProduct == USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_STEALTH_2016)
    {
        retval = device_create_file(&hdev->dev, &dev_attr_mode_pulsate);
            if (retval)
                goto exit_free;
    } else
    {
        retval = device_create_file(&hdev->dev, &dev_attr_mode_wave);
        if (retval)
            goto exit_free;
        retval = device_create_file(&hdev->dev, &dev_attr_mode_spectrum);
        if (retval)
            goto exit_free;
        retval = device_create_file(&hdev->dev, &dev_attr_mode_none);
        if (retval)
            goto exit_free;
        retval = device_create_file(&hdev->dev, &dev_attr_mode_reactive);
        if (retval)
            goto exit_free;
        retval = device_create_file(&hdev->dev, &dev_attr_mode_breath);
        if (retval)
            goto exit_free;
        retval = device_create_file(&hdev->dev, &dev_attr_mode_custom);
        if (retval)
            goto exit_free;
        retval = device_create_file(&hdev->dev, &dev_attr_temp_clear_row);
        if (retval)
            goto exit_free;
        retval = device_create_file(&hdev->dev, &dev_attr_set_key_row);
        if (retval)
            goto exit_free;
    }

    retval = device_create_file(&hdev->dev, &dev_attr_get_serial);
    if (retval)
        goto exit_free;
    retval = device_create_file(&hdev->dev, &dev_attr_device_type);
    if (retval)
        goto exit_free;
    retval = device_create_file(&hdev->dev, &dev_attr_mode_game);
    if (retval)
        goto exit_free;
    retval = device_create_file(&hdev->dev, &dev_attr_mode_static);
    if (retval)
        goto exit_free;
    retval = device_create_file(&hdev->dev, &dev_attr_reset);
    if (retval)
        goto exit_free;
    retval = device_create_file(&hdev->dev, &dev_attr_macro_keys);
    if (retval)
        goto exit_free;
    retval = device_create_file(&hdev->dev, &dev_attr_set_brightness);
    if (retval)
        goto exit_free;
    retval = device_create_file(&hdev->dev, &dev_attr_test);
    if (retval)
        goto exit_free;

    hid_set_drvdata(hdev, dev);


    retval = hid_parse(hdev);
    if(retval)    {
        hid_err(hdev, "parse failed\n");
       goto exit_free;
    }
    retval = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
    if (retval) {
        hid_err(hdev, "hw start failed\n");
       goto exit_free;
    }


    razer_reset(usb_dev);
    usb_disable_autosuspend(usb_dev);
    //razer_activate_macro_keys(usb_dev);
    //msleep(3000);
    return 0;
exit:
    return retval;
exit_free:
    kfree(dev);
    return retval;
}

/**
 * Unbind function
 */
static void razer_kbd_disconnect(struct hid_device *hdev)
{
    struct razer_kbd_device *dev;
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);

    dev = hid_get_drvdata(hdev);

    if(usb_dev->descriptor.idProduct == USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2013 ||
        usb_dev->descriptor.idProduct == USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_STEALTH_2016)
    {
        device_remove_file(&hdev->dev, &dev_attr_mode_pulsate);
    } else
    {
        device_remove_file(&hdev->dev, &dev_attr_mode_wave);
        device_remove_file(&hdev->dev, &dev_attr_mode_spectrum);
        device_remove_file(&hdev->dev, &dev_attr_mode_none);
        device_remove_file(&hdev->dev, &dev_attr_mode_reactive);
        device_remove_file(&hdev->dev, &dev_attr_mode_breath);
        device_remove_file(&hdev->dev, &dev_attr_mode_custom);
        device_remove_file(&hdev->dev, &dev_attr_temp_clear_row);
        device_remove_file(&hdev->dev, &dev_attr_set_key_row);
    }

    device_remove_file(&hdev->dev, &dev_attr_mode_game);
    device_remove_file(&hdev->dev, &dev_attr_get_serial);
    device_remove_file(&hdev->dev, &dev_attr_mode_static);
    device_remove_file(&hdev->dev, &dev_attr_reset);
    device_remove_file(&hdev->dev, &dev_attr_macro_keys);
    device_remove_file(&hdev->dev, &dev_attr_set_brightness);
    device_remove_file(&hdev->dev, &dev_attr_test);
    device_remove_file(&hdev->dev, &dev_attr_device_type);

    hid_hw_stop(hdev);
    kfree(dev);
    dev_info(&intf->dev, "Razer Device disconnected\n");
}

/**
 * Device ID mapping table
 */
static const struct hid_device_id razer_devices[] = {
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA_TE) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2013) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_STEALTH_2016) },
    { }
};

MODULE_DEVICE_TABLE(hid, razer_devices);

/**
 * Describes the contents of the driver
 */
static struct hid_driver razer_kbd_driver = {
    .name =        "razerkbd",
    .id_table =    razer_devices,
    .probe =    razer_kbd_probe,
    .remove =    razer_kbd_disconnect,
    .raw_event = razer_raw_event
};

module_hid_driver(razer_kbd_driver);


