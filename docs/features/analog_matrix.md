# Analog Matrix

While the analog matrix feature is functional, it still needs a lot of testing and, while every STM32F4 and higher chip should work (in theory), only the ones in the table below have been tested. <br>
It is designed to read the output of hall effect sensors connected to multiplexers or the chip directly, with each combination of analog pin and multiplexer channel corresponding to exactly zero or one sensors, meaning it's not an actual matrix hardware-wise.
This feature hasn't been tested together with other features that require the usage of an ADC, but they likely won't work together.

To activate this feature, simply configure the analog_matrix object in your info.json, it will set the relevant make rules automatically.

This is a list of tested microcontrollers:

| Microcontroller |       Working      |      EEPROM     |
|-----------------|--------------------|-----------------|
| STM32F446       | :heavy_check_mark: | :x:<sup>1</sup> |
| STM32F411       | :heavy_check_mark: | :heavy_check_mark:<sup>2</sup> |
| RP2040<sup>3</sup> | :heavy_check_mark: | :heavy_check_mark: |
| Any AVR chip    | :x:                | :x:             |

1: Persistent storage works only with an external chip <br>
2: EEPROM/Flash is reset when flashing firmware, can be fixed by using a different bootloader (e.g. tinyuf2) or an external chip <br>
3: While the RP2040 works without issues, it only has four usable ADC channels per chip. <br>

## What works?

This feature is in a beta state. This means that, while it is fully functional, configuration is more clunky than I'd like, and not everything is guaranteed to be bug-free (or even tested properly.) What I can say is that my personal split analog keyboard has seen a lot of use over the last ~3 months, both for work and video games (Valorant mainly, Immortal rank), so I can attest that the main functions all work perfectly fine (on my setup at least).

Working features are:
* Calibration
    * Save calibration data to persistent storage
    * Automatic adjusting of calibration data during use (as an optional mode)
* Split communication
    * Split profile and calibration synchronisation
* Automatic and manual profile switching
* Rapid trigger
* Analog joystick mode
    * Axes only work on the main half for split keyboards
    * While the keyboard is detected as a gamepad on windows, many games don't automatically pick up inputs. I've found success in using steam input to bind the buttons as a workaround.
* Debug modes
    * Options to output the scan values for a single key or the entire matrix to the console
    * Options to disable output while debugging
* Priority keys
    * Select certain matrix positions to be scanned more often than others
    * Not very useful without high polling rate support, which is a low priority at the moment
* Custom scanning routine
    * If your matrix doesn't conform to the standards layed out in the first section, you can use a custom scanning implementation and still make use of all other features
        * E.g. selectively powering sensors and using diodes, layering multiplexers
    * Overwriting the standard analog initialization routine is also possible
* Mixed matrix
    * Both mechanical and analog keys can be used together
    * Can choose between debouncing only the mechanical keys, or all keys
    * Mechanical keys as bootmagic coming soon
* Predefined filter options + debouncing
    * Choose between several predefined filter strengths, or use your own filter
    * Standard timer-based debouncing is disabled by default, but can be enabled
* and more

## What doesn't work?

* GUI interface
    * VIAL support is in the early stages of development, with a high priority.
* Joystick axes on the slave half
* Controlling the sensor power via gpio pins<sup>1</sup>
* High USB polling rates
    * Currently low priority
* Assigning a color to a profile
    * You can instead assign a color to a layer and let the profile switch automatically when that layer is active
* DKS (multiple actions assigned to one key, triggering depending on the distance)

1: An implementation is available, but hasn't been tested.

## Current TO-DO list

Roughly in descending order of priority:
* Mechanical keys as bootmagic
    * Also allow multiple keys to do one function
* DKS
* GUI (VIAL)
* Joystick axes on slave half
* Velocity-sensitive MIDI keys
* Higher USB polling rates (> 1kHz)
* Controlling sensor power via GPIO*
    * Support for two different modes exists but hasn't been fully tested yet
    * Supported modes are:
        * A single pin controlling a FET, to cut off power to all sensors in case of a hibernation mode
        * One power pin controlling power to all sensors on one mux channel, to toggle every time the mux channel changes
            * This way, only the sensors being scanned will be powered
        * User defined callback functions also exist, to allow for a custom powering logic
* Assigning a color to a profile

<!--MARK: Calibration -->
# Calibration

If you've just finished building your keyboard, or some keys have stopped actuating correctly, you might need to calibrate it. To start calibration, you can either use the AM_CLBR keycode, or define a calibration key and hold that during startup (more info in the config section.) If the keyboard doesn't yet have any calibration values saved, it will automatically enter calibration mode.

While calibration mode is active, you'll need to press every single key on the keyboard down fully at least once. Once a valid top and bottom value has been read for every key, the keyboard will exit calibration mode automatically. How these values are saved depends on the configuration:

## Persistent Storage

By default, the calibration values can be saved to some form of persistent storage, and recalled during startup. This requires you to either:
* Use a chip with eeprom emulation support in QMK, or
* Use and configure an external flash/eeprom storage chip
It is heavily recommended to use one of these two options, as external flash chips are available for low prices at e.g. aliexpress, and being able to save calibration values will make the experience a lot better. Even low storage values like 512k are way more than enough.

::: warning  
The stm32-dfu bootloader will always reset the internal flash while flashing firmware, so any chips using it will not be able to remember the values after a firmware change. To circumvent this, you can use another bootloader (like tinyuf2), or an external storage chip.  
:::

This requires a proper configuration of the EEPROM feature to work, visit the [EEPROM](../feature_eeprom.md) and [EEPROM driver](../drivers/eeprom.md) or [Flash driver](../drivers/flash.md) pages for more information.

When using an external storage chip, don't forget to configure the used communication peripheral (I2C or SPI) as well.

## Hardcode values

::: tip  
It is highly recommended to use a form of persistent storage. Many chips support saving to the internal flash, and external storage chips can be bought for very cheap.  
This option is only supposed to be used as a last resort, or for a testing environment.  
:::

To enable this option, set it in keyboard.json:

```json
"analog_matrix": {
    "config": {
        "no_eeprom": true
    }
}
```
While less convenient than the other option, this is a workaround for chips that don't have EFL support in QMK (e.g. STM32F446). In this mode, when calibration is finished, the keyboard will print the values to the console, so make sure you have a way of reading it (e.g. QMK CLI or QMK Toolbox), and that your tool is listening when the keyboard is done calibrating. The values are then hardcoded in keyboard.json:

```json
"analog_matrix": {
    "config": {
        "no_eeprom": true,
        "top_values": [632, 701, 682, 643],
        "bottom_values": [239, 290, 276, 254]
    }
}
```

For split keyboards, the left and right half are separated:
```json
"top_values": [632, 701, 682, 643],
"bottom_values": [239, 290, 276, 254],
"top_values_right": [642, 688, 643, 687],
"bottom_values_right": [242, 291, 286, 253]
```


# AVR configuration

The analog matrix feature doesn't work on AVR chips. Support may be added in the future, but ARM will be preferred generally anyway, due to firmware size limitations and better specs.

<!--MARK: Configuration -->
# ARM configuration

Enable the ADC peripheral:

## halconf.h

```c
#define HAL_USE_ADC TRUE
```

## mcuconf.h

If you wish to use a specific ADC (provided the chosen chip has multiple ADCs supported by ChibiOS), you can set that here:

```c
#undef STM32_ADC_USE_ADC1
#define STM32_ADC_USE_ADC1 TRUE
```

You may also need to configure the DMA settings, particularily when using an ADC other than the default. 
```c
// In mcuconf.h:
#undef STM32_ADC_ADC1_DMA_STREAM
#define STM32_ADC_ADC1_DMA_STREAM           STM32_DMA_STREAM_ID(2, 4) // DMA 2 Stream 4
#define STM32_ADC_ADC1_DMA_PRIORITY         2
#define STM32_ADC_ADC1_DMA_IRQ_PRIORITY     6
```

You can find these values in the reference manual of your chip.


# RP2040 configuration

While this chip works without issues, the fact that it has only 4 usable ADC channels means that, when using 16 channel multiplexers, you'd be limited to 64 keys.
You'd either have to use 32 channel multiplexers, which are a lot more expensive than 16 channels, or make a split keyboard, to get access to more keys than that.

## halconf.h

```c
#define HAL_USE_ADC TRUE
```

## mcuconf.h

```c
#undef RP_ADC_USE_ADC1
#define RP_ADC_USE_ADC1 TRUE
```

<!--MARK: Gen config -->
# General configuration

## info.json/keyboard.json

This is where the bulk of the configuration lives. Since a GUI is still work in progress, the heights, rapid trigger settings, profiles and additional features are all configured here.

<!--MARK: Hardware -->
### Hardware

This section contains the details of how everything is connected to the microcontroller. If you're making a split keyboard, it is recommended to use the same pins for both halves. While it is possible to change them, some optimizations might not work.

```json
{
    "analog_matrix": {
        "hardware": {
            "mux_pins": ["B12", "B13", "B14", "B15"],
            "adc_pins": ["A0", "A1", "A2"],
            "travel_distance": 4.0,
            "smoothing": 60,
            "deadzone": 80,
            "analog_debounce": 5
        }
    },
    "layouts": {
        "LAYOUT": {
            "layout": [
                {"matrix": [0, 0], "x": 0, "y": 0, "mux": [0, 0]},
                {"matrix": [0, 1], "x": 1, "y": 0, "mux": [0, 1]},
                {"matrix": [0, 2], "x": 2, "y": 0, "mux": [1, 0]},
                {"matrix": [0, 3], "x": 3, "y": 0, "mux": [1, 1]},
                ...
            ]
        }
    }
}
```

```json
"mux_pins": ["B12", "B13", "B14", "B15"]
```
This is an array of the pins used to switch the multiplexer channels. The leftmost pin toggles the least significant channel select bit, usually called S0. The channel select pins for all multiplexers should be wired up together, so that all multiplexers change the channel together. Switching channels independently isn't supported.

```json
"adc_pins": ["A0", "A1", "A2"]
```
This is an array of the analog pins used by the chip. The output of each multiplexer should be connected to one of these. In case you're handwiring, check the reference manual of your chip to make sure that these pins have an ADC peripheral connected to them.

```json
"travel_distance": 4.0
```
This setting sets the switch travel distance that the firmware expects. Defaults to 4 mm. If it is wrong, then heights will be incorrectly converted. The travel distance of a switch can usually be found in its product description or datasheet.

```json
"smoothing": 60
```
Controls how sensitive the ADC is. A lower value means that smaller changes can be picked up, but less resistance to noise. If this value is larger than a distance setting, it will win, e.g. if the smoothing value is too high, it can cause small movements to be registered late. If keys are pressed/released accidentally, try increasing this value. If the keys seem to activate later than expected, try to decrease it.


```json
"deadzone": 80
```
Sets a deadzone at the top and bottom range of the switch travel. A smaller value shrinks the deadzone. If you have issues with the switch occasionally not counting as released, increase this value.
You can have different deadzones for top and bottom by using top_deadzone and bottom_deadzone instead. If the switch is inside the top deadzone, it will always count as released, regardless of height settings. Similarily, a switch inside the bottom deadzone will always count as pressed


```json
"invert_adc": false
```
The analog matrix logic assumes that the ADC value of a released switch is higher than the value of a pressed switch, which should be the default for most of-the-shelf keyboards. If this is not the case, set this to true.


```json
"analog_debounce": 5
```
While it's disabled by default, you can enable timer-based debouncing routines by setting this parameter. Keep in mind that the normal "debounce" parameter doesn't work for analog keys, as that is reserved for debouncing the mechanical keys in a mixed matrix configuration.


#### Layout
```json
"layouts": {
    "LAYOUT": {
        "layout": [
            {"matrix": [0, 0], "x": 0, "y": 0, "mux": [0, 0]},
            {"matrix": [0, 1], "x": 1, "y": 0, "mux": [0, 1]},
            {"matrix": [0, 2], "x": 2, "y": 0, "mux": [1, 0]},
            {"matrix": [0, 3], "x": 3, "y": 0, "mux": [1, 1]},
            ...
        ]
    }
}
```

The controller needs to know what ADC pin and multiplexer channel combination corresponds to what key. This is done by adding a "mux" parameter to each key in the layout definition. This mux parameter has the ADC pin as the first index, and the mux channel as the second index. Both of these are 0-indexed.
Assume our hardware config looks like this:

```json
"adc_pins": ["A0", "A1", "A2"],
"mux_pins": ["B12", "B13", "B14", "B15"],
"no_mux_optimization": false
```
This means that a key with the mux combination [1, 5] is read on pin A1, if pins B12 and B14 are activated, and pins B13 and B15 are deactivated. The output of this keys' sensor is wired to channel 5 of the multiplexer that is connected to pin A1 on the microcontroller.

When choosing the layout, it is best to use the lowest multiplexer channel numbers first, as the feature scans the matrix by looping over every ADC pin and multiplexer channel combination, starting from 0. Any unused combinations are skipped automatically, so having an uneven amount of keys connected to the multiplexers isn't a problem.
It is best to distribute the channels evenly across the multiplexers. If you use e.g. 64 keys distributed among 6 multiplexers, the optimal arrangement would be using channels 0 through 10 on four multiplexers, and channels 0 through 9 on the remaining two. This way, the firmware recognizes that the highest channel number in use is channel 10, and skip reading the remaining 5 (assuming 16 channel multiplexers are used).

If possible, the multiplexer channel select pins should be selected to all be on the same port, arranged consecutively in ascending order (e.g. B12, B13, B14, B15 or C10, C11, C12). <br> This allows the firmware to set the output via direct register access, making it more performant. In case this causes issues, you can also set the "no_mux_optimization" flag to true in the same object to skip this optimization. If the pins aren't arranged in this fashion, the optimization is skipped automatically.
The performance impact of the optimization is low, however.

Currently, it's only possible to connect the sensors to a multiplexer or to an ADC pin directly. Chaining multiplexers or using diodes to connect multiple sensor outputs to one mux channel is not supported, and will likely never be. If you wish to use such an arrangement with this feature, then you'd need to make a custom matrix scanning routine <!--TODO: Add anchor link here-->, which would still allow you to use the rest of the features.

<!--MARK: Config -->
### Config

This section contains information about specific subfeatures and profile settings.

```json
{
    "analog_matrix": {
        "config": {
            "bootloader_key": [0, 0],
            "calibration_key": [1, 0]
        }
    }
}
```

```json
"bootloader_key": [0, 0]
```
Similar to the bootmagic configuration, this configures a matrix position that you can hold during startup to force the keyboard into the bootloader. Unlike the bootmagic feature, this will not reset the EEPROM of the keyboard, so calibration values saved to it won't be erased. Should be used instead of the bootmagic feature, as mixed matrices aren't allowed at the moment, and bootmagic only works on normal matrix keys.
For split keyboards, you can define a separate key for the right half by setting "bootloader_key_right".

```json
"bootmagic_key": [0, 1]
```
Similar to the above option, but also resets the EEPROM, meaning all data stored there will be lost.
For split keyboards, you can define a separate key for the right half by setting "bootmagic_key_right".

```json
"calibration_key": [1, 0]
```
Similar to the above options, this key causes the keyboard to enter calibration mode when held during startup.
For split keyboards, you can define a separate key for the right half by setting "calibration_key_right".
Currently, only one half can be calibrated at a time. To calibrate the other half, use it as the host and start calibration on it.

If the initialization keys aren't registered properly during startup, you can try increasing the startup delay:
```json
"startup_delay": 20
```
This the amount of time, in milliseconds, that the MCU waits before reading the initialization keys, as scanning them too early causes them to not be registered. Defaults to 20.

If matrix scanning doesn't work correctly, you can also try adding various delays to this section:
```json
"mux_select_delay": 500,
"adc_scan_delay": 500
```
Like the startup delay, this is an extremely short, arbitrary amount of time (one asm("nop") instruction to be specific). The mux select delay waits every time the multiplexer channel was changed, while the adc scan delay waits after every adc scan. The time value is dependent on the processing speed. Both default to 0.

<!--MARK: Profiles -->
### Profiles

This section contains all information about trigger heights, rapid trigger, etc. The json is currently the only way to configure them, as VIAL integration is still a ways off. 
To switch between profiles, you can use the layer parameter to automatically switch the profile based on the active layer, or you can use the AM_AP("profile") keycode by placing it in your keymap and replacing "profile" with the profile you want to switch to. Keep in mind that profiles are 0-indexed, and that manually switching the profile disables automatic profile switching (if configured), which can then be reactivated by pressing that same key again, or using the AM_LOCK keycode.

```json
{
    "analog_matrix": {
        "profiles": {
            "profile_switch_mode": "last",
            "default_profile": 0,
            "profile_0": {
                "layers": [0, 1, 2],
                "trigger_height": [
                    1.5, 1.0,
                    2.0, 2.5
                ],
                "release_height": [1.0],
            },
            "profile_1": {
                "layers": [3],
                "rapid_trigger_type": "constant_rapid_trigger",
                "rt_press_distance": [
                    0.3, 0.3,
                    0.2, 0.5
                ],
                "rt_release_distance": [
                    0.4, 0.5,
                    0.2, 0.1
                ]
            }
        }
    }
}
```
The analog matrix feature allows you to assign a profile to a layer. This means that when a layer is switched, it will also activate the lowest index profile that is assigned to the highest active layer. If no profile is assigned, then this parameter decides which layer should be activated:

```json
"profile_switch_mode": "last"
```

|     Switch Mode   |        Description                                                                                             |
|-------------------|----------------------------------------------------------------------------------------------------------------|
| "default"         | Activates the profile set by "default_profile". If this parameter isn't set, it defaults to 0.                 |
| "last"            | Keeps the currently active profile.                                                                            |
| "manual"          | Ignores layer assignment. If this mode is used, you need to use the AM_AP(profile) keycode to switch profiles. |

You can use profile switch keycodes at any point, even with the switch mode set to "default" or "last". This will lock automatic profile switching. To unlock it again, you can either use the AM_LOCK keycode to toggle the lock state, or use the same AM_AP(profile) keycode again to unlock auto profile switching.


```json
"default_profile": 0
```
Sets the profile that is used at startup or when a layer with no assigned profile is active, assuming that profile_switch_mode is set to default.
Defaults to 0 if not set, or if set to a profile that isn't configured.

<!--MARK: Profile objects -->
### Profile objects

A profile object contains all information relevant to that profile, and looks roughly like this:

```json
"profile_0": {
    "layers": [0, 1, 2],
    "trigger_height": [
        1.5, 1.0,
        2.0, 2.5
    ],
    "release_height": [1.0],
}
```

You can use up to 32 profiles, as long as they all follow the "profile_n" naming pattern, where n is an integer starting at 0 and incremented by one for each profile used. Every profile can have its own height, rapid trigger, layer and priority settings for different scenarios.

```json
"layers": [0, 1, 2]
```

As long as profile_switch_mode isn't set to "manual", this profile will be automatically activated when switching to one of these layers. If multiple profiles contain a layer, the profile with the lowest index will be activated. If you don't wish to switch to a specific profile automatically, you can ignore this setting, and use the AM_AP(profile) keycode to switch manually.


#### Height configuration

The order of these keys is identical to the order in the layout, so index 0 sets the mode for the first key in the layout definition. This holds true for split keyboards as well. The same logic is applied to all key-specific arrays further down, like height and rapid trigger distance.

```json
"key_modes": [
    0, 0, 0, 0, 0, 0,              0, 0, 0, 0, 0, 0,
    0, 0, 0, 3, 0, 0,              0, 0, 0, 0, 0, 0,
    0, 3, 3, 3, 3, 0,              0, 3, 0, 0, 0, 0,
    0, 3, 0, 0, 0, 0,              0, 0, 0, 0, 0, 0,
            0, 0,                          0, 0,
                0, 3, 0,        0, 0, 0,
                    0, 0,        0
]
```
::: tip  
All arrays can be formatted however you want, with as many spaces, tabs and newlines as you wish to use. This is helpful for arrays where each position controls a key, e.g. the key mode array and all height/distance arrays.  
:::

The key mode controls the rapid trigger setting for each key:
| Mode name                | key mode    | Description                                                                                                               |
|--------------------------|-------------|---------------------------------------------------------------------------------------------------------------------------|
| none                     | 0           | No rapid trigger. Activation is handled via a simple height check against the trigger and release heights. Default value. |
| rapid_trigger            | 1           | Rapid trigger activates below the trigger height, and deactivates above the release height.                               |
| continuous_rapid_trigger | 2           | Rapid trigger that activates below the trigger height, but only deactivates when the key is fully released.               |
| constant_rapid_trigger   | 3           | Rapid trigger is always active. Functionally the same as using key mode 1 or 2 with very low heights.                     | 

If you want to use the same key mode for every key in this profile, you can just set only one value in the array:

```json
"key_modes": [ 3 ]
```
The same goes for all key-specific arrays.

You can achieve the same thing by using the rapid_trigger_type parameter, e.g.:

```json
"rapid_trigger_type": "constant_rapid_trigger"
```
This will use the constant rapid trigger mode as the default for all keys in that profile. If used in conjunction with the key_modes array in a profile, it will overwrite all key modes outside of 0.


```json
"trigger_height": [ 1.0, 1.5, 2.0, 0.3 ]
```
This is an array of floats where every index corresponds to one key. These floats are scaled to integers according to the calibration values during initialization, and no scanning logic uses the actual floats, so don't worry about performance here. By default, the height is counted from the top, so a height of 1.0 means that pressing the key down by 1 millimeter will activate it. If you want to set the heights to be counted from the bottom, you can set "distance_from_bottom": true in the config section (not the profile section).  
If you wish to use the same height for all keys in the profile, you can just set one value:

```json
"trigger_height": [ 1.0 ]
```
::: warning  
Keep in mind that the output of the sensor isn't directly proportional to the travel distance of the switch, and that it depends on a lot of factors, e.g. the switch used, the sensor used, the thickness of the PCB, etc. This means that the distance values are likely not going to be accurate, so go by feel instead of measurements when configuring your profiles.  
:::

It is also possible to set a default height value for the profile, and overwrite this for specific keys only:
```json
"base_trigger_height": 1.0,
"trigger_height": [ 0, 0.3, 0.5, 0 ]
```
The base trigger height is first used for all keys, then the trigger height overwrites the value for all keys that don't have a value of 0. The resulting height config would be [ 1.0, 0.3, 0.5, 1.0 ].

Only necessary when the profile uses a key mode other than "constant_rapid_trigger" (3) on at least one of the keys.

```json
"release_height": [ 0.7 ]
```
Similar to the trigger_height, this sets the height above which a key is counted as released. Has to be set to a lower value than the trigger height for that key.

It is also possible to set a default release height value for the profile. Instead of using the trigger height for that key, it will instead use the base release height.
```json
"base_release_height": 1.0,
"release_height": [ 0, 0.3, 0.5, 0 ]
```
You also have the option of setting an offset. If the release height isn't defined for the profile, the trigger height will instead be used with the offset subtracted:
```json
"trigger_height": [ 1.0, 1.5, 2.0, 0.3 ],
"release_offset": 0.2
```
This option would result in a release height config of [ 0.8, 1.3, 1.8, 0.1 ]. Keep in mind that the offset needs to be positive, so that the resulting release height is lower than the trigger height for the same key, as it is counted from the top by default.

```json
"rt_press_distance": [ 0.7, 0.3, 0.5, 1.5 ]
```
Set the distance that keys need to travel down in order to count as being activated. Only necessary when the profile uses a rapid trigger mode on at least one of the keys.

```json
"rt_release_distance": [ 0.7 ]
```
Sets the distance that keys need to travel up in order to count as being released. If it's not defined, it will be set to be equal to the press distance. Unlike the release height, this doesn't need to be lower than the press distance, as fully released keys are always counted as being deactivated.

The same base options and offset options can be used with the rapid trigger values. In this case, the offset can also be negative; an offset of -0.3 with an rt_press_distance of 0.8 results in an rt_release_distance of 0.5 millimeters.

<!--MARK: Keycodes -->
# Keycodes

Profile-related keycodes:
| Keycode               | Alias          | Description                                                                  |
|-----------------------|----------------|------------------------------------------------------------------------------|
| AM_AP(profile)        | AM_AP(profile) | Manually switches the profile to the one selected and disables automatic profile switching. If pressed again with the same profile active, it will re-activate automatic switching instead. |
| AM_LOCK_PROFILE       | AM_LOCK        | Toggles automatic layer switching.                                           |
| AM_PRINT_CALIBRATION  | AM_PRNT        | Prints the current calibration values to the console. Only prints the master half on split keyboards. |
| AM_PRINT_PROFILE      | AM_PRPR        | Prints the currently active profile to the console.                          |
| AM_CALIBRATE          | AM_CLBR        | Starts calibration of the keyboard. On split keyboards, only starts calibration of the master half. |

<!--MARK: Functions -->
# Functions

| Function             | Description                        |
|----------------------|------------------------------------|
| void set_active_profile(uint8_t profile) | Activates the profile passed as the parameter. |
| uint8_t get_active_profile(void) | Returns the currently active profile. |
| void toggle_profile_lock(void) | Toggles automatic profile switching on and off. |
| void set_profile_lock(bool value) | Sets the automatic profile switching state to the passed parameter. |
| void calibrate_switches(void) | Starts calibration immediately. |

You can use these functions from your 'keyboard'.c or keymap.c file by including the analog_matrix header at the top of the file:
```c
#include "analog_matrix.h"
```


<!--MARK: Split kb -->
# Split keyboards

The analog matrix feature works just fine on split keyboards, but there are some things to keep in mind:

<!--
By default, QMK loads the same firmware into both halves and assigns the side during initialization. As an analog matrix requires a lot of additional information, this feature gives you the option to use the -s flag during flashing to reduce the firmware size by compiling the firmware specifically for one side.

To flash the left half, you use
"qmk flash -kb path/to/your/keyboard -km keymap_name -s left"

Accepted options are l/left/r/right. Using this flag means that only the configuration for that specific half is compiled, which can reduce firmware size by a decent bit. For example, my personal keyboard is a split 61 key dactyl manuform with 2 profiles. Using the flag gives me a firmware size of ~80kb, while not using it takes me to ~100kb (25% more.) I still have the option of using either side as master, the only changes are a slightly longer compilation time, as the firmware needs to be recompiled for each half.  
Flashing without the parameter also works fine, but could cause space problems on chips with less flash and larger configs. To speed up compilation time, you can use the -j flag to compile multiple files in parallel (e.g. -j 10).

The -s flag works by defining SIDE_LEFT or SIDE_RIGHT respectively and then forcing recompilation. If any of your features require this already, you can use this by doing
```c
#ifdef SIDE_LEFT // To check for the right half, use SIDE_RIGHT
//Do something here
#endif
```
This will run that code only if the -s left flag is set.

In case of issues with your configuration, try using the -s flag when flashing the sides, as assigning the sides at initialization is generally added later for new features.
-->

Joystick axes currently don't work on the slave half. The slave half will need to be flashed when enabling/disabling the analog joystick feature, or else it won't connect.

Debug options to print to the console don't work on the slave half, but having them enabled can still cause a (often major) performance hit.
When changing "debug_scan_no_input", you need to flash both halves, as it applies to each half separately.

If no_eeprom is set, the master half will wait for the slave half to finish calibrating before printing the values for both halves.
When using the calibration_key to start calibration at init, only the half with the pressed down key will start calibration.

If you wish to use different multiplexer pins for each half, the optimizations might not work. In case of issues, you can manually disable these optimizations by setting
```json
"no_mux_optimization": true
```
to true inside the hardware object in the json, or by defining
```c
#define NO_MUX_OPTIMIZATION
```
inside of your keyboards config.h. This will cause a minor performance penalty.


<!--MARK: Filtering -->
# Filtering

By default, the feature implements the following predefined filters, to reduce noise in the ADC readings:
```c
#if defined STRONG_ADC_FILTER // 1/2 new, 1/2 old
#   define ADC_FILTER(value, index) value = (value >> 1) + (key_config[index].scan_value >> 1)
#elif defined WEAK_ADC_FILTER // 7/8 new, 1/8 old
#   define ADC_FILTER(value, index) value = ((value * 7) >> 3) + (key_config[index].scan_value >> 3)
#elif defined NO_ADC_FILTER  // The filter is disabled
#   define ADC_FILTER(value, index)
#else // Medium filter, default  3/4 new, 1/4 old
#   define ADC_FILTER(value, index) value = ((value * 3) >> 2) + (key_config[index].scan_value >> 2)
#endif
```
You can select the predefined filter by defining one of the following:
```c
// In config.h
#define STRONG_ADC_FILTER
// or
#define WEAK_ADC_FILTER
// or, to disable the filter entirely
#define NO_ADC_FILTER
```
You can use a custom filter implementation by defining it in config.h:
```c
// In config.h
#define ADC_FILTER(value, index) value -= (value - key_config[index].scan_value) >> 2
// #define ADC_FILTER(value, index) value = ((value * 2) / 3) + (key_config[index].scan_value / 3)
```
The filter gets called like this:
```c
// In analog_matrix.c
ADC_FILTER(adc_value, matrix_index);
```
This means that any filter implementation needs to assign the adc value, which you can access via the 'value' parameter. To access the previously scanned value of that switch, you can use key_config[index].scan_value.

As the filter works on absolute values, it is recommended to use the debug_matrix_position feature to see what the expected travel range of a switch is, and base your filter strength on that. The higher the range, the stronger the filter should be.

If you wish to use your own filter, keep in mind that this filter will run once for every single key in every single scan, meaning that performance should be prioritised over steady readings, within reason. A simple filter will hardly cause a performance penalty, but a more complex filter might. Furthermore, it is advised to use primarily simple operations like adding, subtracting and bitshifting. If possible, multiply and divide by powers of 2 (2, 4, 8 etc), as those operations can be done efficiently via bitshifts (multiplying by 4 equals << 2, dividing by 8 equals >> 3 etc).

## Debouncing 

Timer-based debouncing (the same stuff used for mechanical keys) is disabled by default, but can be enabled by setting the "analog_debounce" parameter:
```json
"analog_matrix": {
    "hardware": {
        "analog_debounce": 5
    }
},
"debounce_type": "asym_eager_defer_pk"
```
This will use a debounce time of 5 milliseconds.


<!--TODO: Add adjustment section here -->


<!--MARK: Mixed matrix-->
# Mixed Matrix

If you wish, you can use normal (mechanical) keys together with your analog keys, e.g. to use the press action on an encoder. <br>
You can either connect them to GPIO 'direct pin' style (one leg to a GPIO pin, the other leg to ground), or together in a matrix configuration using diodes. However, the pin definition will be different, and the layout macro needs extra information to work correctly.

## Pin configuration

If you wish to wire your mechanical keys together in a matrix configuration, you need to define both the row and column pins inside the "hardware" object:
```json
"analog_matrix": {
    "hardware": {
        "row_pins": ["B11", "B12"],
        "col_pins": ["B13", "B14", "B15"]
    }
},
"diode_direction": "COL2ROW"
```
By default, the diode orientation is column to row. If you wish to change it, you need to set the top level parameter "diode_direction" to "ROW2COL". This value is dependent on the orientation of your diodes. If the mechanical keys aren't working, try changing this parameter.

If you only need a few mechanical keys, it is easier to use a direct pin configuration:
```json
"direct_pins": ["C12", "C13", "C14"]
```
This skips the need for diodes and simplifies the wiring, as you only need to connect one leg of the switch to ground and the other to one of the defined pins.
You can only use either direct_pins or both row_pins and col_pins, as either option constitutes a full definition. If both options are defined, direct_pins wins.<br>
::: tip  
Don't forget that, while the direct_pins definition for a normal QMK matrix is a 2-dimensional array, this version only uses one dimension, as the "rc" parameter in the layout macro makes the second dimension obsolete (and the rc parameter is needed anyway)  
:::

## Layout configuration

Similar to the mux parameter for analog keys, all mechanical keys need an added "rc" parameter to work correctly. Since the resulting keymap is a combination of mechanical and analog keys that don't fit into an electrical matrix together, the rc parameter maps the pin index(es) to a matrix position. The contents of the parameter depend on the pin definition used. If row_pins and col_pins is defined, functions identically to the mux parameter:
```json
"layouts": {
    "LAYOUT": {
        "layout": [
            {"matrix": [0, 0], "x": 0, "y": 0, "mux": [0, 0]},
            {"matrix": [0, 1], "x": 1, "y": 0, "mux": [0, 1]},
            {"matrix": [0, 2], "x": 2, "y": 0, "rc":  [0, 0]},
            {"matrix": [0, 3], "x": 3, "y": 0, "rc":  [1, 0]},
            ...
        ]
    }
}
```
The rc parameter is an array of two indices, with the first one corresponding to the pin index in the row array, and the second one belonging to the col pin index.

If the direct_pin parameter is used instead, it will be a single integer, also corresponding to the direct pin index in the array:
```json
{"matrix": [0, 4], "x": 3, "y": 0, "rc": 0},
{"matrix": [0, 5], "x": 3, "y": 0, "rc": 1}
```
To enable debouncing, you use the standard QMK configuration:
```json
"debounce": 5,
"build": {
    "debounce_type": "asym_eager_defer_pk"
}
```
This will enable debouncing with a time of 5 milliseconds for the mechanical keys only. If these parameters aren't set, they default to 5 milliseconds and sym_defer_g respectively. If you wish to disable debouncing for the mechanical keys, you can set "debounce" to 0. <br>
If you instead wish to use timer-based debouncing for both mechanical and analog keys, keep in mind that the time will be set by "analog_debounce" for both switch types instead. It is not possible to use different debouncing algorithms per switch type.


<!--TODO: Put this into its own page -->
<!--MARK: Custom scanning -->
# Custom Scanning

The default matrix scanning routine works like this:
1. Select multiplexer channel
    * The channels are counted up from 0. Any channels higher than the highest used channel are skipped automatically.
2. Loop through all used ADC pins
3. Check if a switch is connected to the multiplexer channel and ADC pin combination
4. If yes, measure the voltage and check if the switch state has changed (evaluation)
5. Select the next multiplexer channel and start anew

The scanning function returns true if the matrix state has changed, and false if it hasn't. The way this scanning routine is set up means that you can only use zero or one layers of multiplexers to connect your sensors to the microcontroller, i.e. the every multiplexer output pin has to be connected to an ADC pin directly. Connecting the output of one multiplexer to the input of another isn't supported natively, nor is using diodes to connect multiple sensors to one multiplexer channel, then selectively powering them.  
However, if your matrix is layed out differently from the standard described above, you can still make use of this feature by replacing the default scanning routine with your own implementation. This would allow you to still make use of the calibration (including saving data), height translation, switch state evaluation, and all other features that don't directly change how the scanning routine works (i.e. basically any feature except dynamic calibration and priority keys will still work, and support for those can be implemented in your own scanning routine as well).

## Setup

To use your own scanning or initialization routine, you need to add a new file placed in your keyboard folder, and tell QMK to compile it as well. Assuming that file is called matrix.c: <br>
In rules.mk:
```
SRC += matrix.c
```
Importantly, you should NOT add CUSTOM_MATRIX = yes or CUSTOM_MATRIX = lite to your rules.mk.
A barebones implementation will look something like this:
```c
// In matrix.c
#include "analog_matrix.h" // To make use of analog matrix functions
// The function definition has to look like this
uint8_t analog_matrix_scan(void) {
    bool matrix_has_changed = false;
    // Handle the matrix scanning here

    return matrix_has_changed;
}
```
This is all that's necessary to overwrite the scanning routine with your own. For it to work correctly, you need to keep a few things in mind:
The evaluation function translates the ADC scan value of a switch into its press/release state, and returns true if the switch state has changed, NOT if it is pressed. <br> The evaluation function also handles stuff like joystick axes automatically (if those are enabled). <br>

A simple scanning routine can look something like this:
```c
uint8_t analog_matrix_scan(void) {
    bool matrix_has_changed = false;
    uint16_t adc_value;
    uint8_t index;
    
    // Loop through all used multiplexer channels. mux_channel_num contains the highest used mux channel (of that half for split keyboards)
    for(uint8_t mux_channel = 0; mux_channel < mux_channel_num; mux_channel++) {
        set_mux_channel(mux_channel);
        // If you need it, you can set a short delay here to let the multiplexer output settle before scanning.
        delay_ns(MUX_SELECT_DELAY); 
        // Loop through all used ADC pins. adc_pin_num contains the amount of pins (of that half for split keyboards)
        for(uint8_t adc_channel = 0; adc_channel < adc_pin_num; adc_channel++) {
            // mux_to_num contains the matrix index for all possible intersections of mux channels and adc pins. If the index at an intersection is 255, it means it is unused.
            index = mux_to_num[mux_channel][adc_channel];
            // An index of 0 means that the intersection is unused, we can continue with the next combination
            if (index == 255) continue;

            // Scan the selected channel combination. adc_pin_mux contains the converted ADC and ADC channel combination of all used ADC pins.
            adc_value = adc_read(adc_pin_mux[adc_channel]);
            // Filter the scan value using the algorithms described in the Filtering section
            ADC_FILTER(adc_value, index);
            // This checks if the value has changed enough compared to the previous value to warrant a full evaluation.
            // If the change is smaller than ADC_SMOOTHING (60 by default, 12 bit ADC), we continue with the next switch.
            if ((adc_value < (key_config[index].scan_value + ADC_SMOOTHING)) && (adc_value > (key_config[index].scan_value - ADC_SMOOTHING))) continue;
            // Check if key is pressed/released, returns true if the switch state has changed
            if(evaluate_value(index, adc_value)) {
                matrix_has_changed = true;
                // Toggle the state of the matrix position. The matrix array doesn't reset between scans. The matrix position of each switch is saved to the key_config[index].
                matrix[key_config[index].row] ^= 1 << key_config[index].col;
            }
            // Update the previous scan value with the current one
            key_config[index].scan_value = adc_value;
        }
    }

    // On split keyboards, we need to run matrix_post_scan to get the slave matrix state
    // This define is automatically set if a split keyboard is used, otherwise this code won't be included
    #ifdef SPLIT_KEYBOARD
    matrix_has_changed |= matrix_post_scan();
    #endif

    // Unlike the default QMK scanning function, this one returns true if the matrix state has changed
    return matrix_has_changed;
}
```

Let's assume we're using five switches. Two of those are connected to channels 0 and 1 of multiplexer number 0, the other three are connected to channels 0, 1 and 2 of mux 1.
That means that our mux_to_num array has the form {{1, 2}, {3, 4}, {255, 5}}. As no switch is connected to multiplexer 0 channel 2, the index will be 255, meaning that it won't be scanned.<br>
The mux_to_num array is filled automatically with the information taken from the LAYOUT object in info.json, and doesn't have to be manually set. If it isn't correct, double check that the mux parameters in the LAYOUT definition are set correctly.

::: warning  
The special debugging modes described in "Other Features->Debugging" are called directly during scanning, replacing the scanning routine will make them unusable.  
:::

To be able to print information to the console during development, you can include the "debug.h" header, allowing you to use the dprint and dprintf functions. Make sure that you have enabled the QMK debugging mode, or else they won't print anything. To print even with debugging disabled, you can use the "print.h" header and the print and printf functions instead, but keep in mind that these will cause a major performance penalty even with debugging disabled.

If you need to initialize additional things, you can do so by defining a matrix_init_kb() function:
```c
void matrix_init_kb(void) {
    // Initialize additional stuff without overwriting basic functionality
}
```
This function will be called at the end of the default initialization function. 
If you instead wish to overwrite it completely, you can do so similarily to the scanning function:
```c
void analog_matrix_init(void) {
    // Setting this definition will completely overwrite the original function, so be VERY careful
}
```
This function assigns the configuration to the correct half on split keyboards, sets the pin modes for the used multiplexer and ADC pins, reads the switch calibration data from EEPROM, calls the calibration function if the data is invalid, scans the init keys if they're used, translates the configured heights into the equivalent ADC readings, and calls the mixed_matrix_init function, if enabled.  
Overwriting this function can break all functionality VERY easily, in many more ways than the scanning function, so it is recommended to do so only if you fully understand how the analog matrix feature works. Otherwise, it is best to stick to adding stuff via matrix_init_kb().


<!--MARK: Features -->
# Other Features

## Dynamic Calibration

This feature will automatically update the calibration values of a switch based on several parameters:
```json
"dynamic_calibration": true,
"dynamic_calibration_factor": 0.1,
"dynamic_calibration_delta": 5,
"recalibrated_switches": 5
```
The basic functionality is enabled by setting "dynamic_calibration" to true. This will simply monitor the switch boundaries and update them if they've been exceeded.  
If the boundaries have changed, the height values for that switch will be updated automatically according to the new bounds.  
You can also set a factor that is applied to the calibration values at initialization. This means that the bounds are moved slightly closer together (a factor of 0.1 will move the top value down and the bottom value up by 10% of the travel range).  
The new values for a switch are only saved, if the bounds have been moved due to a switch activation. To make sure that the flash isn't being written to too often, you have several options. The new boundary value needs to be higher or lower than the old calibration value by at least "dynamic_calibration_delta", and the values are only saved to storage while a layer is being changed, if the amount of switches with new values is equal or higher than "recalibrated_switches". If you don't wish to save the new values to storage automatically, you can set "recalibrated_switches" to a higher number than the amount of switches you have. This will disable the check completely.  

If the no_eeprom option is used, this check is also disabled automatically.

While the boundary check is very efficient, you nevertheless have the option of setting a profile to be a priority, by setting
```json
"priority_profile": true
```
inside the profile_n object. This will disable the boundary checks on that profile.


## Debugging

There are several settings you can use to help you debug issues with the analog matrix. These print out the status to the console, so make sure debugging and the console feature is enabled, and that you have access to the console (QMK CLI or QMK Toolbox for example).

::: warning  
On split keyboards, printing the scan values only works for the master half.  
:::

```json
{
    "analog_matrix": {
        "config": {
            "debug_mux_value": [0, 0],
            "debug_matrix_value": [0, 0],
            "debug_scan_values": false,
            "debug_calibration": false,
            "debug_scan_no_input": false
        }
    }
}
```
"debug_mux_value" allows you to monitor a specific ADC pin/mux channel combination. Setting this will print out the scan value of that key during each scan cycle. <br>
"debug_matrix_value" allows you to monitor a specific matrix position. Setting this will print out the scan value of that key during each scan cycle. <br>
"debug_scan_values" will print out the scan values for each switch during the matrix scan. Each value is prefixed by the mux combination. <br>
"debug_calibration" will print out the scan values for each switch during calibration only. Each value is prefixed by the mux combination. <br>
"debug_scan_no_input" will stop the keyboard from registering keypresses. Enabling this can be helpful to monitor the scan values without triggering random actions on your computer.

::: warning  
Enabling any of the print options will cause a performance hit, even if the console isn't enabled on your end. If you suddenly have performance issues after debugging, try checking if you disabled all debug modes, and flash the firmware again. <br>
If you have a split keyboard and the slave half suddenly updates slowly, make sure that all debug options are disabled, then flash the firmware again.
On split keyboards, enabling debug_scan_no_input on one half will not disable inputs on the other half.  
:::

## Priority keys

You can force the firmware to scan some keys more often, thereby increasing the scan rate for situations in which only some keys are important. This is done by setting these options:


```json
{
    "analog_matrix": {
        "config": {
            "priority_keys": [[0, 0], [0, 1], [1, 0], [2, 4]],
            "priority_level": 5,
            "slave_low_priority": true
        }
    }
}
```

To activate the feature, you also need to assign profiles as priority profiles. This is done by setting the priority_profile flag in the profile_* object:
```json
{
    "analog_matrix": {
        "profiles": {
            "profile_0": {
                "priority_profile": true
            }
        }
    }
}
```
The priority keys are only prioritized while a profile with this flag enabled is active, else all keys are scanned equally as often.

```json
"piority_keys": [[0, 0], [0, 1], [1, 0], [2, 4]]
```
Each index in the priority_keys array corresponds to one important matrix position \[row/col\]. These are the keys that will be scanned more often. 

```json
"piority_level": 5
```
A higher priority level will cause unimportant keys to be scanned less often, specifically only once every priority_level scans, while the important keys are scanned every scan. Defaults to 5.

```json
"slave_low_priority": true
```
On split keyboards, this causes the slave synchronisation to happen only when the full matrix is scanned. Can boost the scanrate by a lot.
This is automatically defined when no priority keys appear on the slave matrix half.

On my setup (split STM32F446 with 61 keys total, RGB underglow and slave half trackball) I usually get a scanrate of ~3850, but with 6 important keys that are all on the master half, and a priority level of 5, I get a scanrate of >13000.

This feature is not very useful currently, as USB polling rates above 1k aren't supported, but this is work in progress.

<!--MARK: Joystick -->
## Analog Joystick

::: warning  
This feature currently doesn't work that well. Most games don't recognize the keyboard as a controller, or only the movement axes work. <br>
One workaround is to use Steam input to emulate a proper controller, this will recognize the buttons and axes. For non-steam games, you can still try to add them to Steam to use Steam input. <br>
On split keyboards, assigning joystick axes to the slave half doesn't work. You can still use it for joystick buttons.  
:::

The analog joystick feature can be enabled by simply configuring it in the keyboard.json. The buttons and axes follow the xbox naming convention by default, but the alias can be changed via the "layout" parameter.

```json
{
    "analog_matrix": {
        "joystick": {
            "axes": 6,
            "buttons": 16,
            "deadzone": 80,
            "layout": "XBOX",
            "resolution_methods": {
                "x": "difference",
                "y": "lowest",
                "trigger": "difference",
                "rx": "cancel",
                "ry": "positive_dominant"
            }
        }
    }
}
```
The axis buttons are split up by component. This means that for each axis (X, Y, Triggers, RX, RY, RZ) there are two buttons, one for the positive component, and one for the negative. When both buttons are released, the axis value is 0.
If the negative axis value is bottomed out, the axis component value -X will be 127, and the axis value will be -127. If both buttons are pressed simultaneously, the output is decided by the resolution method.

```json
"deadzone": 120
```
This sets a deadzone to the top and bottom of the travel range, separate from the normal switch deadzone. If you wish to use different values for top and bottom, you can instead use top_deadzone and bottom_deadzone.

```json
"layout": "XBOX"
```
This imports a list of aliases for the joystick buttons and axes, to make them easier. All buttons are prefixed by JS_. Using the xbox layout means that you can use keycodes like JS_A and JS_RT. Accepted values are xbox, playstation and nintendo. Defaults to xbox.

```json
"resolution_method": "difference"
```
If two competing axis buttons are pressed simultaneously, this options controls how the conflict is resolved.
For example, if the buttons that control the positive and negative X axis components are pressed together.

| Resolution                        | Description                                                                                                                 |
|-----------------------------------|-----------------------------------------------------------------------------------------------------------------------------|
| Difference                        | The resulting output is the difference between the absolute values of each axis component. Default value.                   |
| Lowest                            | The output is controlled by the component that is pressed down further.                                                     |
| Cancel                            | If both buttons are not completely released, the output will be 0.                                                          |
| Positive Dominant                 | The positive component wins out against the negative. The negative component only counts if the positive is fully released. |
| Negative Dominant                 | The negative component wins out against the positive. The positive component only counts if the negative is fully released. |

Using this option will set the method for all axes. If you wish to set different methods for specific axes, you can specify them like this:

```json
"resolution_methods": {
    "x": "difference",
    "y": "lowest",
    "trigger": "difference",
    "rx": "cancel",
    "ry": "positive_dominant"
}
```
The "resolution_method" parameter only applies to axes that aren't specifically defined in the "resolution_methods" object. To specify the axes, you can use the following names: x, y, trigger/z, rx, ry, rz.

::: warning  
The trigger buttons are two components of the z axis, with LB/L2 corresponding to the negative component of the Z axis and RB/R2 corresponding to the positive component.
I currently don't know if they work together.  
:::

```json
"axes": 6,
"buttons": 16
```
These options control the amount of axes and buttons that the descriptor will use. Trying to use a keycode for an axis that is outside of the range of the defined axis amount will crash the keyboard, while trying to use a button that is outside of that range will result in the button simply not working.
Defaults to 6 axes and 16 buttons.

::: warning  
The axis amount needs to be 1 higher than the actual amount of axes you're trying to use. If you plan on emulating a standard gamepad controller with two analog sticks and two triggers, this means that while you're only using 5 axes, the keyboard expects 6. <br>
Also, the joystick feature accepts a maximum of 6 axes, meaning that the RZ axis cannot be used at this moment.  
:::

### Keycodes
All joystick keycodes are prefixed by JS_. You can use the layout parameter to change the button names to your preferred system, XBox naming convention is used by default.

| Button    | XBox    | Playstation   | Nintendo |
|-----------|---------|---------------|----------|
| JS_0      | JS_A    | JS_X, JS_CRSS | JS_B     |
| JS_1      | JS_B    | JS_O, JS_CRCL | JS_A     |
| JS_2      | JS_X    | JS_SQRE       | JS_Y     |
| JS_3      | JS_Y    | JS_TNGL       | JS_X     |
| JS_4      | JS_LB   | JS_L1         | JS_L     |
| JS_5      | JS_RB   | JS_R1         | JS_R     |
| JS_6      | JS_BACK | JS_SHRE       | JS_MINS  |
| JS_7      | JS_STRT | JS_OPTN       | JS_PLUS  |
| JS_8      | JS_LS   | JS_L3         | JS_LS    |
| JS_9      | JS_RS   | JS_R3         | JS_RS    |
| JS_10     | JS_DPU  | JS_DPU        | JS_DPU   |
| JS_11     | JS_DPD  | JS_DPD        | JS_DPD   |
| JS_12     | JS_DPL  | JS_DPL        | JS_DPL   |
| JS_13     | JS_DPR  | JS_DPR        | JS_DPR   |

There are more buttons defined, up to JS_31. These can be used, but don't have an alias, as they don't appear in default gamepad layouts.

Each logical axis (X, Y, Z/Triggers, RX, RY, RZ) has a range from -127 to 127. This feature divides the axes up into two components, each being controlled by one keycode. The keycode name is comprised of:
* The prefix JS_
* L or R, for the Left or Right axis
* N or P, for the negative or positive component
* The axis designation (X, Y etc)

| Axis component | XBox       | Playstation | Nintendo |
|----------------|------------|-------------|----------|
| JS_LPX         | :o:        | :o:         | :o:      |
| JS_LNX         | :o:        | :o:         | :o:      |
| JS_LPY         | :o:        | :o:         | :o:      |
| JS_LNY         | :o:        | :o:         | :o:      |
| JS_LPZ         | JS_RT      | JS_R2       | JS_ZR    |
| JS_LNZ         | JS_LT      | JS_L2       | JS_ZL    |
| JS_RPX         | :o:        | :o:         | :o:      |
| JS_RNX         | :o:        | :o:         | :o:      |
| JS_RPY         | :o:        | :o:         | :o:      |
| JS_RNY         | :o:        | :o:         | :o:      |
| JS_RPZ<sup>1</sup> | :o:    | :o:         | :o:      |
| JS_RNZ<sup>1</sup> | :o:    | :o:         | :o:      |

1: The right Z axis does not corrently work, due to a bug with the firmware, where one more axis than necessary needs to be defined, while the joystick feature limits the amount of joystick axes to 6. As standard gamepads only use 5 axes anyway, fixing this is a low priority at the moment.


## Mixed matrix

While an actual mixed matrix isn't supported yet, it's possible to fake it by using pull-up/down resistors. <br>
If you're not using the invert_adc option:
1. One leg of the mechanical switch should be connected to +3.3V/+5V via a pull-up resistor, and to an input pin on a multiplexer (or directly to an ADC pin, if you're not using multiplexers)
2. The other leg should be connected to ground
    * The resistance value doesn't need to be exact, ~50-100k is fine
3. Calibrate and configure like a regular switch
    * Keep in mind that rapid trigger won't work on that switch for obvious reasons

If the invert_adc option is used, instead of connecting the multiplexer input to the leg connected to the positive voltage source, connect it to the ground leg. <br>
While this isn't very useful for most situations, it can be used to be able to use the encoder click action until actual mixed matrix support is implemented.

<!--MARK: Debug guide -->
# Debugging guide

#### Keyboard doesn't work after flashing

1. If you have a bootloader or bootmagic key configured, this key might be being triggered during startup, causing the keyboard to boot into the bootloader instead. You can go into the Device settings of your computer to check for bootloaders. Try disabling the bootloader/bootmagic keys to see if they're the cause.

2. Another reason could be a conflict with another feature using the ADC. This feature very likely doesn't support this, but it hasn't been tested.

3. The ADC channel or multiplexer pins could be assigned to another function as well, causing issues.

4. Calibration mode might be triggered at startup, see next section.

#### Keyboard starts calibration by itself

1. If this happens every time the keyboard starts up, this likely means that either the calibration_key is registering a press during startup, or the calibration values aren't available. Try removing the calibration_key parameter, if bound. If that doesn't help: <br>
If the no_eeprom flag is set, the keyboard expects to find these parameters:
```json
"analog_matrix": {
    "config": {
        "no_eeprom": true,
        "top_values": [632, 701, 682, 643],
        "bottom_values": [239, 290, 276, 254]
    }
}
```
The length of the array needs to be equal to the amount of switches in your layout. On split keyboards, each half has its own array:
```json
"top_values": [632, 701, 682, 643],
"bottom_values": [239, 290, 276, 254],
"top_values_right": [642, 688, 643, 687],
"bottom_values_right": [242, 291, 286, 253]
```
Check that the array names are correct, and that the flag is set properly.

If the no_eeprom flag isn't set (values are saved to storage), check if your chip has at least one of these:
* An external EEPROM/Flash chip, correctly wired up and configured
* Support for embedded flash emulation

Chips without flash emulation support will not save their values to storage, meaning that they are lost when the keyboard loses power.

2. If this only happens after flashing firmware, you're likely using a chip with the stm32-dfu bootloader, which always erases the whole flash.  
To circumvent this, you can use an external storage chip, or change your bootloader (if available, tinyuf2 is a good option).  
Another reason this could happen is that you're using the bootmagic function to go into the bootloader. The bootmagic function resets the EEPROM, so if your calibration values are saved there, they'll be lost. Use the bootloader_key instead.

If your keyboard has a visible simple LED somewhere, you can use it to get visual feedback about the calibration state. The LED will be on while calibration is in progress, and turn off once it's finished and the new values have been saved. You can enable this like so:

config.h
```c
// This doesn't work with smart LEDs like WS2812 etc
#define LED_PIN B2 // This is the pin that the LED is connected to. It has to be a simple LED connected to a gpio pin.
```
If the LED works by pulling the connected GPIO pin low, you can add this define to invert the logic:
```c
#define LED_INVERTED
```
This will pull the line low instead of high when the LED should turn on. If your LED still doesn't work, then it's likely not a simple LED.
::: warning  
Using this while something is connected to the same line as the LED can cause problems.  
:::


#### Split keyboard half doesn't work

If your master half has lost connection to the slave half after flashing, the likely cause is a mismatch in the enabled features. If one of the features needs to send data to the other half, it needs to be enabled and configured on both halves. Flashing the slave half with the same firmware usually fixes this.
This includes adding another profile, joystick, MIDI, and priority keys (depending on the configuration).
If you have QMKs debug mode enabled, you will see messages saying that "split synchronisation failed", or similar, in the console.

#### Some keys don't work

This can have many causes. The exact symptom can help with the diagnosis:
For normal keyboards:

* Pressing a key causes many keys to activate:
    * Check your mux pin configuration.
    * A mux channel is missing its connection to the sensor.
* Keys are not activating the correct keycode:
    * Make sure the layout macro definition is correct, and remember that it's 0-indexed.
* One or more keys in different sections don't work:
    * The sensor connection to the multiplexer could be severed.
    * The ground connection of the sensor could be missing.
    * The 5V/3.3V connection of the sensor could be missing.
    * Use debug options to read the value of one sensor at a time:
        * 0 -> Missing 5V/3.3V connection
        * 4095 -> Missing ground connection
        * Reasonable value but doesn't change, or changes together with another key -> missing sensor connection to mux
* An entire section of keys close together doesn't work:
    * This likely means that the connection between multiplexer output and ADC pin is severed.

<!-- 
TODO: Add this when it's more fleshed out
MARK: Design tips
# Keyboard design tips

* Keep the analog traces short
* Use a small decoupling capacitor (~100nF) between the ADC pin and ground
* Low pass filter at each sensor?
* Tie unused multiplexer channels to ground
* Select the ADC Vref voltage appropriately for your sensor/orientation/switch combo
-->

# Additional Resources

If you want an implementation example, you can look up [my current keyboard.](https://www.github.com/vermilion00/qmk_firmware/tree/kb/keyboards/0/he)
There you can see an actual, working implementation.

If you have any questions, feel free to contact me at vermilion00.github@gmail.com.

<!--
TODO:
* Make analog_matrix the main page, then have hardware and software config as two separate subpages
* Do I need to specifically set ADC_1 and/or DMA info in mcuconf? Probably only when the default one isn't used
* Show function calls
* Add example code for functions
* Draw a schematic for an example keyboard, and base my explanations on that
* Test the power feature, then add it to the docs
* Improve and test dynamic calibration, then add to docs
* Include information about endpoints etc, for MIDI and joystick features, in the mcu list
* Add options to the config options doc and the data driven doc
* Remove personal references and email before upstreaming

-->
