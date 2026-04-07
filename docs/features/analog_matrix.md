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

1: Persistent storage only works with an external chip <br>
2: EEPROM/Flash is reset when flashing firmware, can be fixed by using a different bootloader (e.g. tinyuf2) or an external chip <br>
3: While the RP2040 works without issues, it only has four usable ADC channels per chip, and the ADC has worse performance than STMs offerings <br>

## What works?

This feature is in a beta state. This means that, while it is fully functional, configuration is more clunky than I'd like, and not everything is guaranteed to be bug-free (or even tested properly.) What I can say is that my personal split analog keyboard has seen a lot of use over the last ~3 months, both for work and video games (Valorant mainly, Immortal rank), so I can attest that the main functions all work perfectly fine (on my setup at least).

Working features are:
* [Adjustable heights](analog_matrix#height-configuration)
* [Rapid trigger](analog_matrix#height-configuration)
    * Standard, Continuous and Constant modes are available
* Deadzones
    * Deadzones can be adjusted separately for top and bottom
    * Also for the left and right half on [split keyboards](analog_matrix#split-keyboards)
    * The top deadzone can be [calibrated](analog_matrix#deadzone-calibration)
* [Calibration](analog_matrix#calibration)
    * Save calibration data to persistent storage
    * Automatic adjusting of calibration data during use (as an optional mode)
    * The top deadzone can be [calibrated](analog_matrix#deadzone-calibration)
* [Split communication](analog_matrix#split-keyboards)
    * Split profile and calibration synchronisation
    * Assign different deadzones, smoothing values, and filter strengths on master and slave
        * This can be used to remedy less stable scan values on the slave
* [Automatic](analog_matrix#profile-objects) and manual profile switching
* [Analog joystick](analog_matrix#analog-joystick) mode
    * While the keyboard is detected as a gamepad on windows, many games don't automatically pick up inputs. I've found success in using steam input to bind the buttons and axes as a workaround.
* [SOCD / Snap Tap / Input Cleaner](analog_matrix#SOCD)*
    * This functionality is available as a separate module in QMK
    * A height-based priority can't be set, however.
* [Priority keys](analog_matrix#priority-keys)
    * Select certain matrix positions to be scanned more often than others
    * Not very useful without high polling rate support, which is a low priority at the moment
* [Custom scanning](analog_matrix#custom-scanning) routine
    * If your matrix doesn't conform to the standards layed out in the first section, you can use a custom scanning implementation and still make use of all other features
        * E.g. selectively powering sensors and using diodes, layering multiplexers*
        * Selectively powering sensors is likely possible by default but hasn't been tested
    * Both [full](analog_matrix#full-replacement) and [lite](analog_matrix#lite-implementation) implementations are possible
    * Overwriting the standard analog [initialization](analog_matrix#custom-initialization) routine is also possible
* [Mixed matrix](analog_matrix#mixed-matrix)
    * Both mechanical and analog keys can be used together
    * Choose between debouncing only the mechanical keys, or all keys
    * Mechanical and analog keys with bootmagic function
* Predefined [filter](analog_matrix#filtering) options + [debouncing](analog_matrix#debouncing)
    * Choose between several predefined filter strengths, or use your own filter
    * Standard timer-based debouncing is disabled by default, but can be enabled
* [Debug modes](analog_matrix#debugging)
    * Options to output the scan values for a single key or the entire matrix to the console
    * Options to disable output while debugging
* and more

### What doesn't work?

* GUI configurator
    * VIAL can be used with this branch to change the keymap and QMK settings only, no analog configuration
    * Full VIAL support is in the early stages of development
* Controlling the sensor power via gpio pins<sup>1</sup>
* High USB polling rates
    * Currently low priority
* Assigning a color to a profile
    * You can instead assign a color to a layer and let the profile switch automatically when that layer is active instead
* DKS (multiple actions assigned to one key, triggering depending on the distance)

1: An implementation is available, but hasn't been tested.

### Current TO-DO list

Roughly in descending order of priority:
* GUI confugurator (VIAL)
* Velocity-sensitive MIDI keys
* DKS
* Higher USB polling rates (> 1kHz)
* Controlling sensor power via GPIO*
    * Support for two different modes exists but hasn't been fully tested yet
    * Supported modes are:
        * A single pin controlling a transistor, to cut off power to all sensors in case of a hibernation mode, to reduce power usage
        * One power pin controlling power to all sensors on one mux channel, to toggle every time the mux channel changes
            * This way, only the sensors being scanned will be powered
        * User defined callback functions also exist, to allow for a custom powering logic
* Assigning a color to a profile


<!--MARK: Calibration -->
## Calibration

If you've just finished building your keyboard, or some keys have stopped actuating correctly, you might need to calibrate it. To start calibration, you can either use the AM_CLBR keycode, or define a [calibration key](analog_matrix#config) and hold that during startup. If the keyboard can't access any calibration values, it will automatically enter calibration mode during startup.

While calibration mode is active, you'll need to press every single key on the keyboard down fully at least once. Once a valid top and bottom value has been read for every key, the keyboard will exit calibration mode automatically. If the calibration doesn't finish after pressing every key down, it might mean that the keyboard is expecting a larger difference between the top and bottom values than your sensors are providing. In that case, you can use the following define to lower the threshold:
```c
#define CAL_THRESHOLD (5 * ADC_TOP_DEADZONE)
```
This is the minimum absolute difference in value between top and bottom, for a switch to be seen as calibrated. As the calibration only stops once all keys are counted as calibrated, setting this value too high will cause the calibration to never finish, while setting this value to low will cancel calibration prematurely. Keep in mind that the ADC is configured to use a 12 bit resolution.  

### Deadzone calibration

The top deadzone of each switch can be calibrated. The resulting values are saved in the same manner as the calibration values. To start calibrating the top deadzone, place the AM_CLTP (AM_CALIBRATE_TOP) keycode in your keymap and use it. After activating it, you need to release all keys. The calibration will start two seconds after the keycode is activated, and take almost no time. Make sure not to press any keys in this time frame, or the deadzone values will be way off. After the calibration is done, the keyboard automatically returns to the normal state, and the new deadzones are used immediately.  
If you have a form of persistent storage set up, the values will be saved automatically. Keep in mind that any action that wipes the EEPROM will also remove these values, at which point the fallback "adc_deadzone" is used.  
If the no_eeprom option is used instead, the values will be printed to the console, similar to the regular switch calibration. Make sure to have your console open to be able to save the new values. On split keyboards, similar to the regular calibration, the master will pull the calibrated values from the slave and print them out together.

How the calibration values are saved depends on the configuration:

### Persistent Storage

By default, the calibration values can be saved to some form of persistent storage, and recalled during startup. This requires you to either:
* Use a chip with eeprom emulation support in QMK, or
* Use and configure an external flash/eeprom storage chip
It is heavily recommended to use one of these two options, as external flash chips are available for low prices at e.g. aliexpress, and being able to save calibration values will make the experience a lot better. Even low storage values like 512k are way more than enough.

::: warning  
The stm32-dfu bootloader will always reset the internal flash while flashing firmware, so any chips using it will not be able to remember the values after a firmware change. To circumvent this, you can use another bootloader (like tinyuf2), or an external storage chip.  
:::

This requires a proper configuration of the EEPROM feature to work, visit the [EEPROM](../feature_eeprom) and [EEPROM driver](../drivers/eeprom) or [Flash driver](../drivers/flash) pages for more information.

When using an external storage chip, don't forget to configure the used communication peripheral (I2C or SPI) as well.

### Hardcode values

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


## AVR configuration

The analog matrix feature doesn't work on AVR chips. Support may be added in the future, but ARM will be preferred generally anyway, due to firmware size limitations and better specs.

<!--MARK: Configuration -->
## ARM configuration

Enable the ADC peripheral:

### halconf.h

```c
#define HAL_USE_ADC TRUE
```

### mcuconf.h

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


## RP2040 configuration

While this chip works without issues, the fact that it has only 4 usable ADC channels means that, when using 16 channel multiplexers, you'd be limited to 64 keys.
You'd either have to use 32 channel multiplexers, which are a lot more expensive than 16 channels, or make a split keyboard, to get access to more keys than that.  
Another (minor) problem is that the ADC performs poorly compared to STMs offerings, with a fairly slow sample rate of 500ksps, and a lower accuracy. This means that stronger filtering and/or debouncing might need to be enabled to use a sensitive configuration.

### halconf.h

```c
#define HAL_USE_ADC TRUE
```

### mcuconf.h

```c
#undef RP_ADC_USE_ADC1
#define RP_ADC_USE_ADC1 TRUE
```

<!--MARK: Gen config -->
## General configuration

### info.json/keyboard.json

This is where the bulk of the configuration lives. Since a GUI is still work in progress, the heights, rapid trigger settings, profiles and additional features are all configured here.

<!--MARK: Hardware -->
#### Hardware

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
This setting sets the switch travel distance that the firmware expects. Defaults to 4 mm. If it is wrong, then heights will be incorrectly converted. The travel distance of a switch can usually be found in its product description or datasheet, and is different from the spring length of the switch.

```json
"smoothing": 20
```
Controls how sensitive the ADC is. A lower value means that smaller changes can be picked up, but less resistance to noise. If this value is larger than a distance setting, it will win, e.g. if the smoothing value is too high, it can cause small movements to be registered later than expected. If keys are pressed/released accidentally, try increasing this value. If the keys seem to activate later than expected, try to decrease it. A general starting point for this value is (top_value - bottom_value) / (2 * travel_distance).
If the change in value compared to the last evaluated scan is smaller than this value, the switch evaluation is skipped.

Deadzones are split into two parts. While both are supposed to keep the keyboard from registering a press/release where there isn't supposed to be one, they target different causes: The "adc_deadzone" is intended to eliminate the possibility of false presses/releases caused by noise in the ADC readings, while the "deadzone" is intended to give users with a heavy touch room to rest their hand on the switch without accidentally registering a press/release.  
Until the deadzone is [calibrated](analog_matrix#deadzone-calibration), this distinction isn't important, as they stack additively: An ADC deadzone of 60 and a deadzone of 20 will behave identically if the values are flipped. This changes when the top deadzone is calibrated, as that will take care of the ADC deadzone, allowing the user to finetune the sensitivity without worrying about accidentally setting the deadzone low enough to register presses due to ADC inaccuracies.

```json
"adc_deadzone": 60
"deadzone": 80
```
You can have different deadzones for top and bottom by using top_deadzone, bottom_deadzone, adc_top_deadzone and adc_bottom_deadzone instead. If the switch is inside the top deadzone, it will always count as released, regardless of height settings. Conversely, a switch inside the bottom deadzone will always count as pressed. Both the "adc_deadzone" and "deadzone" values together need to be in the range from 0 to 255.

```json
"invert_adc": false
```
The analog matrix logic assumes that the ADC value of a released switch is higher than the value of a pressed switch, which should be the default for most of-the-shelf keyboards. If this is not the case, set this to true.

```json
"analog_debounce": 5
```
While it's disabled by default, you can enable timer-based debouncing routines by setting this parameter. Keep in mind that the normal "debounce" parameter doesn't work for analog keys, as it is reserved for debouncing the mechanical keys in a mixed matrix configuration.


##### Layout
```json
"layouts": {
    "LAYOUT": {
        "layout": [
            {"matrix": [0, 0], "x": 0, "y": 0, "mux": [0, 0]},
            {"matrix": [0, 1], "x": 1, "y": 0, "mux": [0, 1]},
            {"matrix": [0, 2], "x": 2, "y": 0, "mux": [1, 0]},
            {"matrix": [0, 3], "x": 3, "y": 0, "mux": [1, 1]}
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

If possible, the multiplexer channel select pins should be selected to all be on the same port, arranged consecutively in ascending order (e.g. B12, B13, B14, B15 or C10, C11, C12). This allows the firmware to set the output via direct register access, making it more performant. In case this causes issues, you can also set the "no_mux_optimization" flag to true in the same object to skip this optimization. If the pins aren't arranged in this fashion, the optimization is skipped automatically.
The performance impact of the optimization is low, however.

Currently, it's only possible to connect the sensors to a multiplexer or to an ADC pin directly. Chaining multiplexers or using diodes to connect multiple sensor outputs to one mux channel is not supported, and will likely never be. If you wish to use such an arrangement with this feature, then you'd need to make a custom matrix scanning routine <!--TODO: Add anchor link here-->, which would still allow you to use the rest of the features.

<!--MARK: Config -->
#### Config

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
You can set certain keys to be scanned during startup. If the matrix position is detected as pressed, it will run the associated function immediately. Keep in mind that due to the different matrix definition compared to default QMK, the bootmagic feature should remain disabled. Instead, use the following parameters to set the keys:

```json
"bootloader_keys": [0, 0]
```
Similar to the bootmagic configuration, this configures a matrix position that you can hold during startup to force the keyboard into the bootloader. Unlike the bootmagic feature, this will not reset the EEPROM of the keyboard, so calibration values saved to it won't be erased. Should be used instead of the bootmagic feature, as mixed matrices aren't allowed at the moment, and bootmagic only works on normal matrix keys.  
For split keyboards, you can also define separate keys for the right half by setting "bootloader_keys_right".

```json
"bootmagic_keys": [0, 1]
```
Similar to the above option, but also resets the EEPROM, meaning all data stored there will be lost.
For split keyboards, you can define a separate key for the right half by setting "bootmagic_keys_right".

```json
"calibration_keys": [1, 0]
```
Similar to the above options, this key causes the keyboard to enter calibration mode when held during startup.
For split keyboards, you can define a separate key for the right half by setting "calibration_keys_right". Only the half with the pressed down calibration key will start calibration. <br>

If more than one key (per keyboard half) should be bound to the same function, you can set it as an array of matrix positions:
```json
"calibration_keys": [[1, 0], [2, 4], [3, 3], [0, 5], [10, 3]]
```
This will cause calibration to start if any one of the above keys are detected as pressed during startup.
If you're using a mixed matrix, you can also use the matrix positions of any mechanical key. On split keyboards, right half calibration keys can also be defined in these arrays by simply using the correct position. Keep in mind that the right half sits below the left half in the matrix.  

If the initialization keys aren't registered properly during startup, you can try increasing the startup delay:
```json
"startup_delay": 20
```
This the amount of time, in milliseconds, that the MCU waits before reading the initialization keys, as scanning them too early causes them to not be registered. Defaults to 20.
If that doesn't help, you can also set the following define:
```c
#define INIT_THRESHOLD (4 * ADC_BOTTOM_DEADZONE)
```
The init keys are only counted as pressed if the read value falls below bottom_value + INIT_THRESHOLD, so increasing this might help when keys aren't detected, while decreasing it helps if keys are detected too easily. 

If general matrix scanning doesn't work correctly, you can also try adding various delays to this section:
```json
"mux_select_delay": 500,
"adc_scan_delay": 500
```
Like the startup delay, this is an extremely short, arbitrary amount of time (one asm("nop") instruction to be specific). The mux select delay waits every time the multiplexer channel was changed, while the adc scan delay waits after every adc scan. The time value is dependent on the processing speed. Both default to 0.

<!--MARK: Profiles -->
#### Profiles

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
#### Profile objects

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


##### Height configuration

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

If no release_height, no base_release_height and no release_offset is defined, the release height will be higher than the trigger height by 0.2 millimeters (default offset is 0.2).

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
## Keycodes

Profile-related keycodes:
| Keycode               | Alias          | Description                                                                  |
|-----------------------|----------------|------------------------------------------------------------------------------|
| AM_AP(profile)        | AM_AP(profile) | Manually switches the profile to the one selected and disables automatic profile switching. If pressed again with the same profile active, it will re-activate automatic switching instead. |
| AM_LOCK_PROFILE       | AM_LOCK        | Toggles automatic layer switching.                                           |
| AM_PRINT_CALIBRATION  | AM_PRNT        | Prints the current calibration values to the console. Only prints the master half on split keyboards. |
| AM_PRINT_PROFILE      | AM_PRPR        | Prints the currently active profile to the console.                          |
| AM_CALIBRATE          | AM_CLBR        | Starts calibration of the keyboard. On split keyboards, only starts calibration of the master half. |

<!--MARK: Functions -->
## Functions

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


<!--MARK: Filtering -->
## Filtering

The feature implements predefined filters, with selectable strength values from 0 (disabled) to 4 (maximum filtering)
You can select the predefined filter by defining one of the following:
```c
// In config.h
#define FILTER_STRENGTH 3
```
You can use a custom filter implementation by defining it in config.h:
```c
// In config.h
#define ADC_FILTER(value, index) (value - (value - key_config[index].scan_value) >> 2)
// #define ADC_FILTER(value, index) ((value * 2) / 3) + (key_config[index].scan_value / 3)
```
The filter gets called like this:
```c
// In analog_matrix.c
adc_value = ADC_FILTER(adc_value, matrix_index); // adc_value is uint16_t
```
This means that any filter implementation needs to assign the adc value, which you can access via the 'value' parameter. To access the previously scanned value of that switch, you can use key_config[index].scan_value.

As the filter works on absolute values, it is recommended to use the debug_matrix_position feature to see what the expected travel range of a switch is, and base your filter strength on that. The higher the range, the stronger the filter should be.

If you wish to use your own filter, keep in mind that this filter will run once for every single key in every single scan, meaning that performance should be prioritised over steady readings, within reason. A simple filter will hardly cause a performance penalty, but a more complex filter might. Furthermore, it is advised to use primarily simple operations like adding, subtracting and bitshifting. If possible, multiply and divide by powers of 2 (2, 4, 8 etc), as those operations can be done efficiently via bitshifts (multiplying by 4 equals << 2, dividing by 8 equals >> 3 etc).

On split keyboards, it is possible to use different filter strenghts per half:
```c
// In config.h
// These default to FILTER_STRENGTH
#define RIGHT_FILTER_STRENGTH 3
#define SLAVE_FILTER_STRENGTH 3
```
This is helpful in the case that the slave power supply is less stable. Using a higher filter strength can smooth out the resulting jitters, but will cause a minor performance penalty. You can use either the _RIGHT option or the _SLAVE option, but the _RIGHT option is generally preferred and wins out over the SLAVE option if both are defined.


## Debouncing

[Timer-based debouncing](../feature_debounce_type) (the same algorithm used for mechanical keys) is disabled by default, but can be enabled by setting the "analog_debounce" parameter:
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


<!--MARK: Split kb -->
## Split keyboards

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

On split keyboards, it is generally recommended to use one half as the constant master, as the slave values are generally less stable. Both halves should be calibrated with the constant master half plugged in. If you're going to use the keyboard for gaming, the movement keys should all be on the master half.
To mitigate this, you can set a multiplier for the deadzones and smoothing values, as well as a separate filter strength, to use on the right half / slave half:
```c
#define RIGHT_MULTIPLIER 1.5 
#define RIGHT_FILTER_STRENGTH 4 // Same as FILTER_STRENGTH by default (3)
#define SLAVE_MULTIPLIER 1.5 
#define SLAVE_FILTER_STRENGTH 4 // Same as FILTER_STRENGTH by default (3)
```
This will multiply the deadzone and smoothing values by the defined amount, if the keyboard half is detected as the right half.
As this is only checked once during initialization, don't be afraid to use floating values.
The SLAVE version works the same, but is used when the keyboard is detected as the slave half. If both are defined, only the RIGHT version is used.
Using a separate filter strength on the slave will cause a minor performance hit.

The multipliers only affect the [ADC deadzone](analog_matrix#hardware), not the user deadzone. Using the top deadzone calibration feature will remove the multiplier from the top deadzone, as it's assumed to not be necessary anymore.

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


<!--MARK: Mixed matrix-->
## Mixed Matrix

If you wish, you can use normal (mechanical) keys together with your analog keys, e.g. to use the press action on an encoder. <br>
You can either connect them to GPIO 'direct pin' style (one leg to a GPIO pin, the other leg to ground), or together in a matrix configuration using diodes. However, the pin definition will be different, and the layout macro needs extra information to work correctly.

### Pin configuration

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

If you only need a few mechanical keys, it is easier to use a direct pin configuration, also defined in the hardware object:
```json
"direct_pins": ["C12", "C13", "C14"]
```
This skips the need for diodes and simplifies the wiring, as you only need to connect one leg of the switch to ground and the other to one of the defined pins.
You can only use either direct_pins or both row_pins and col_pins, as either option constitutes a full definition. If both options are defined, direct_pins wins.<br>
::: tip  
Don't forget that, while the direct_pins definition for a normal QMK matrix is a 2-dimensional array, this version only uses one dimension, as the "rc" parameter in the layout macro makes the second dimension obsolete (and the rc parameter is needed anyway)  
:::   
If you wish to use a mechanical key as an init key, you can simply set it in the same way as an analog key. Mechanical matrix locations are detected and converted automatically, no additional configuration required.


### Layout configuration

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
## Custom Scanning

The default matrix scanning routine works like this:
1. Scan mechanical keys (if configured)
2. Select multiplexer channel
    * The channels are counted up from 0. Any channels higher than the highest used channel are skipped automatically.
3. Loop through all used ADC pins
4. Check if a switch is connected to the multiplexer channel and ADC pin combination
5. If yes, measure the voltage and check if the switch state has changed (evaluation)
6. Select the next multiplexer channel and start anew
7. Debounce the matrix state once all keys have been scanned (if enabled)
8. Synchronise the slave matrix state to the master (for split keyboards only)
Only after all of this does QMK process the keypresses, and only if the matrix state has changed.

The scanning function returns true if the matrix state has changed, and false if it hasn't. The way this scanning routine is set up means that you can only use zero or one layers of multiplexers to connect your sensors to the microcontroller, i.e. the every multiplexer output pin has to be connected to an ADC pin directly. Connecting the output of one multiplexer to the input of another isn't supported natively, nor is using diodes to connect multiple sensors to one multiplexer channel, then selectively powering them.  
However, if your matrix is layed out differently from the standard described above, you can still make use of this feature by replacing the default scanning routine with your own implementation. This would allow you to still make use of the calibration (including saving data), height translation, switch state evaluation, and all other features that don't directly change how the scanning routine works (i.e. basically any feature except dynamic calibration and priority keys will still work, and support for those can be implemented in your own scanning routine as well).

There are two different ways to implement a custom scanning routine, depending on your needs: a "lite" implementation that keeps the debouncing, split synchronisation, and mechanical key scanning in place; and the full replacement, which gives you full control.

### Setup

To use your own scanning or initialization routine, you need to add a new file placed in your keyboard folder, and tell QMK to compile it as well. Assuming that file is called matrix.c: <br>
In rules.mk:
```make
SRC += matrix.c
```
Importantly, you should NOT add CUSTOM_MATRIX = yes or CUSTOM_MATRIX = lite to your rules.mk.

#### General information

An analog switch is represented with the analog_key_t struct in the firmware. The definition of this struct looks like this:
```c
typedef struct analog_key_t {
    uint8_t pressed;
    uint8_t mode[AM_PROFILE_NUM];
    #if defined USE_CONTINUOUS_RAPID_TRIGGER
    uint8_t rt_active;
    #endif
    #if defined USE_TRIGGER_HEIGHT
    // The switch is counted as pressed below this value
    uint16_t trigger_value[AM_PROFILE_NUM];
    // The switch is counted as released above this value
    uint16_t release_value[AM_PROFILE_NUM];
    #endif
    #if defined USE_RT_DISTANCE
    // The switch is counted as pressed when the rapid trigger crosses this threshold
    // As the switch is traveling downward, this value is constantly updated. The release distance is simply checked against this value to determine if the switch should be released
    uint16_t rt_threshold;
    // The distance that the switch is required to travel downwards before it's registered as pressed
    uint16_t rt_press_value[AM_PROFILE_NUM];
    // The distance that the switch is required to travel upwards before it's registered as released
    uint16_t rt_release_value[AM_PROFILE_NUM];
    #endif
    // This saves the last scanned value
    uint16_t scan_value;
    uint16_t bottom_value;
    uint16_t top_value;
    // These are the matrix position coordinates
    uint8_t row;
    uint8_t col;

    // Can be defined in config.h to add extra parameters
    AM_USER_PARAMS
} analog_key_t;
```
For most custom scanning implementations, only the row, col, and scan_value fields should be of importance, as the predefined functions can handle the rest. 
If you need to save additional information to the switch, you can define AM_USER_PARAMS in your config.h:
```c
// In config.h
#define AM_USER_PARAMS \
uint8_t mux_channel; \
uint8_t adc_channel; \
uint16_t activations[AM_PROFILE_NUM];
```
This will allow you to access these fields in your custom routines without modifying the actual struct.
If you need to save data that will be different on each profile, you can define it as an array with AM_PROFILE_NUM elements. AM_PROFILE_NUM is automatically defined to be equal to the profiles configured in keyboard.json.
To be able to use linebreaks in your definition, you need to put a backslash ('\') at the end of the line.

If you need to save information to these fields only once before actual scanning starts, you can do so by using the [matrix_init_kb()](analog_matrix#custom-initialization) function.

All switches (on this half, if using a split keyboard) are configured in a one-dimensional array:
```c
analog_key_t key_config[SWITCH_NUM];
```
The index of each key corresponds to its index in the layout macro (from top to bottom). For split keyboards, each half has its own key_config array that only contains information about the switches on that half.
The total amount of switches (in that half) is stored in the switch_num variable.

<!--TODO: Expand this info-->



<!--MARK: Custom lite-->

#### Lite Implementation

To make use of the "lite" implementation, you also need to add the following parameter to the hardware object:
```json
"analog_matrix": {
    "hardware": {
        "custom_matrix_lite": true
    }
}
```
or set the define in config.h:
```c
#define CUSTOM_MATRIX_LITE
```
This would replace points 2 through 6 in the section above with a single function call:

```c
#include "analog_matrix.h"

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool matrix_has_changed = false;

    return matrix_has_changed;
}
```
`current_matrix[MATRIX_ROWS]` is an array with a length equal to the amount of rows in the layout macro (not to be confused with the amount of multiplexer channels, ADC pins, etc). Every index represents the state of an entire row as a bitmask. Let's assume that you have a 4 row, 3 column macropad. In this configuration, matrix_row_t is equivalent to a uint8_t, as this can be used to store the states of up to 8 switches in a single row. Every pressed switch is represented by a 1 in this configuration, read from right to left. If you were to press matrix positions [0, 2], [3, 1] and [1, 2], our current_matrix[] should look like this after scanning:
```c
0b00000100
0b00000100
0b00000000
0b00000010
```
matrix_row_t will automatically scale with the amount of columns needed for the matrix, up to a maximum of 32.

To read the ADC value of an already selected switch, you can use this function:
```c
uint16_t adc_value = adc_read(adc_pin_mux[adc_pin]);
// You may also want to filter the value
ADC_FILTER(adc_value, index);
```
To find out if the switch is pressed or released according to its height and rapid trigger configuration, call this function:
```c
// It's important to note that this function returns true if the switch state has changed, NOT when it's pressed
bool changed = evaluate_value(switch_index, adc_value);
```
To update the matrix state, you can use the following statements:
```c
// This will set the switch state to 'released' -> 0
current_matrix[row] &= ~(1 << col);
// This will set the switch state to 'pressed' -> 1
current_matrix[row] |= (1 << col);
// This will toggle the switch state (pressed becomes released and vice versa)
current_matrix[row] ^= (1 << col);
```

After scanning all analog keys in the matrix, don't forget to return matrix_has_changed (and don't forget to set it to true if the matrix state has changed).
Since we're only doing a lite implementation, things like debouncing, split synchronisation and mechanical matrix scanning is still handled for you (if required).


<!--MARK: Custom full-->
#### Full Replacement

If you instead need full control over the scanning routine, use a full replacement instead, by setting these values:
```json
"analog_matrix": {
    "hardware": {
        "custom_matrix_full": true
    }
}
```
or set the define in config.h:
```c
#define CUSTOM_MATRIX_FULL
``` 
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
The evaluation function translates the ADC scan value of a switch into its press/release state, and returns true if the switch state has changed, NOT if it is pressed. <br> The evaluation function also handles stuff like joystick axes automatically (if used). <br>

A simple scanning routine can look something like this:
```c
uint8_t analog_matrix_scan(void) {
    bool matrix_has_changed = false;
    
    // Loop through all used multiplexer channels. mux_channel_num contains the highest used mux channel (of that half for split keyboards)
    for(uint8_t mux_channel = 0; mux_channel < mux_channel_num; mux_channel++) {
        // If a multiplexer delay is set, it will be called inside set_mux_channel
        set_mux_channel(mux_channel);
        // Loop through all used ADC pins. adc_pin_num contains the amount of pins (of that half for split keyboards)
        for(uint8_t adc_channel = 0; adc_channel < adc_pin_num; adc_channel++) {
            // mux_to_num contains the matrix index for all possible intersections of mux channels and adc pins. If the index at an intersection is 255, it means it is unused.
            const uint8_t index = mux_to_num[mux_channel][adc_channel];
            // An index of 0 means that the intersection is unused, we can continue with the next combination
            if (index == 255) continue;

            // Scan the selected channel combination. adc_pin_mux contains the converted ADC and ADC channel combination of all used ADC pins.
            uint16_t adc_value = adc_read(adc_pin_mux[adc_channel]);
            // Filter the scan value using the algorithms described in the Filtering section
            adc_value = ADC_FILTER(adc_value, index);
            // This checks if the value has changed enough compared to the previous value to warrant a full evaluation.
            // If the change is smaller than ADC_SMOOTHING (40 by default, 12 bit ADC), we continue with the next switch.
            if ((adc_value < (key_config[index].scan_value + ADC_SMOOTHING)) && (adc_value > (key_config[index].scan_value - ADC_SMOOTHING))) continue;

            // Update the previous scan value with the current one
            key_config[index].scan_value = adc_value;
            // Check if key is pressed/released, returns true if the switch state has changed
            if(evaluate_value(index, adc_value)) {
                matrix_has_changed = true;
                // Toggle the state of the matrix position. The matrix array doesn't reset between scans. The matrix position of each switch is saved to the key_config[index].
                matrix[key_config[index].row] ^= 1 << key_config[index].col;
            }
        }
    }


    //Important: Only one of the next two code sections should be used!
    // On split keyboards, we need to run matrix_post_scan to get the slave matrix state
    // This define is automatically set if a split keyboard is used, otherwise this code won't be included
    // 1.
    #ifdef SPLIT_KEYBOARD
    matrix_has_changed |= matrix_post_scan();
    #endif

    // Most of the time, time-based debouncing isn't necessary on analog keyboards. If you still wish to make use of it, use code block 2 instead:
    // 2.
    #ifdef SPLIT_KEYBOARD
    matrix_has_changed = debounce(raw_matrix + thisHand, matrix + thisHand, MATRIX_ROWS_PER_HAND, matrix_has_changed) | matrix_post_scan();
    #else
    matrix_has_changed = debounce(raw_matrix, matrix, MATRIX_ROWS_PER_HAND, changed);
    #endif

    return matrix_has_changed;
}
```

Let's assume we're using five switches. Two of those are connected to channels 0 and 1 of multiplexer number 0, the other three are connected to channels 0, 1 and 2 of mux 1.
That means that our mux_to_num array has the form {{1, 2}, {3, 4}, {255, 5}}. As no switch is connected to multiplexer 0 channel 2, the index will be 255, and will therefore be skipped.<br>
The mux_to_num array is filled automatically with the information taken from the LAYOUT object in info.json, and doesn't have to be manually set. If it isn't correct, double check that the mux parameters in the LAYOUT definition are set correctly.

::: warning  
The special debugging modes described in "Other Features->[Debugging](analog_matrix#debugging)" are called directly during scanning, replacing the scanning routine will make them unusable.  
:::

To be able to print information to the console during development, you can include the "debug.h" header, allowing you to use the dprint and dprintf functions. Make sure that you have enabled the QMK debugging mode, or else they won't print anything. To print even with debugging disabled, you can use the "print.h" header and the print and printf functions instead, but keep in mind that these will cause a major performance penalty even with debugging disabled.

#### Custom Initialization

If you need to initialize additional things, you can do so by defining a matrix_init_kb() function:
```c
void matrix_init_kb(void) {
    // Initialize additional stuff without overwriting basic functionality
}
```
This function will be called at the end of the default initialization function. If the [`#define USER_PARAMS` option](analog_matrix#general-information) is used, it should be initialized here (if necessary).  
If you instead wish to overwrite it completely, you can do so similarily to the scanning function:
```c
void analog_matrix_init(void) {
    // Setting this definition will completely overwrite the original function, so be VERY careful
}
```
This function assigns the configuration to the correct half on split keyboards, sets the pin modes for the used multiplexer and ADC pins, reads the switch calibration data from EEPROM, calls the calibration function if the data is invalid, scans the init keys if they're used, translates the configured heights into the equivalent ADC readings, and calls the mixed_matrix_init function, if enabled.  
Overwriting this function can break all functionality VERY easily, in many more ways than the scanning function, so it is recommended to do so only if you fully understand how the analog matrix feature works. Otherwise, it is best to stick to adding stuff via matrix_init_kb().


<!--MARK: Features -->
## Other Features

<!--MARK: SOCD-->
### SOCD

While SOCD isn't implemented as part of the analog matrix feature specifically, it is available as a separate module.  
a. To install it manually, you need to download them from [here](https://github.com/getreuer/qmk-modules), and put the socd_cleaner folder into the modules/getreuer directory (or into a subfolder).  
b. To install it through the terminal, navigate to the qmk_firmware directory on your Computer, then run the following two commands: 
```
git submodule add https://github.com/getreuer/qmk-modules.git modules/getreuer
git submodule update --init --recursive
```
This will install all modules in that directory through the git submodule system.

To use it in a keyboard, you need to enable the module in its keyboard.json through the top level "modules" object. The specified path is relative to the qmk_firmware/modules directory, and needs to point to the <module>.c file. Assuming that the module is installed in the modules/getreuer/socd_cleaner directory, the path will look like this:
```json
{
    "modules": ["getreuer/socd_cleaner"]
}

```
If you want to use more than one module, put all of the paths into the array, and separate them by a comma (,).

The key pairs are configured inside your keymap.c file:
```c
#include "modules/getreuer/socd_cleaner/socd_cleaner.h"

socd_cleaner_t socd_opposing_pairs[] = {
	{{KC_A, KC_D}, <RESOLUTION_METHOD>},
};
```

The <RESOLUTION_METHOD> option defines the conflict resolution method. The available options are:
```c
  // Last input priority with reactivation. You most likely want this option.
  SOCD_CLEANER_LAST,
  // Neutral resolution. When both keys are pressed, they cancel.
  SOCD_CLEANER_NEUTRAL,
  // Key 0 always wins.
  SOCD_CLEANER_0_WINS,
  // Key 1 always wins.
  SOCD_CLEANER_1_WINS
```

Many thanks to [@getreuer](https://getreuer.info/posts) for this module!

<!--MARK: Dynamic Cali-->
### Dynamic Calibration

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
The new values for a switch are only saved, if the bounds have been moved due to a switch activation. To make sure that the flash isn't being written to too often, you have several options. The new boundary value needs to be higher or lower than the old calibration value by at least "dynamic_calibration_delta", and the values are only saved to storage while a layer is being changed, if the amount of switches with new values is equal or higher than "recalibrated_switches". By default, the new values are not saved to storage automatically. To enable this, set the "recalibrated_switches" parameter to a value that is less than the amount of switches (per half).

If the no_eeprom option is used, this check is also disabled automatically.

While the boundary check is very efficient, you nevertheless have the option of setting a profile to be a priority, by setting
```json
"priority_profile": true
```
inside the profile_n object. This will disable the boundary checks on that profile.

<!--MARK: Debugging-->
### Debugging

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

<!--MARK: Prio Keys-->
### Priority keys

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
### Analog Joystick

::: warning  
Most games don't recognize the buttons natively. <br>
One workaround is to use Steam Input to emulate a proper controller, this will recognize the buttons and axes. For non-steam games, you can still add them to Steam to use Steam Input. <br>
:::

The analog joystick feature can be enabled by simply configuring it in the keyboard.json. The buttons and axes follow the xbox naming convention by default, but the alias can be changed via the "layout" parameter.

```json
{
    "analog_matrix": {
        "joystick": {
            "axes": 6,
            "buttons": 16,
            "top_deadzone": 20,
            "bottom_deadzone": 0,
            "layout": "XBOX",
            "resolution_methods": {
                "x": "difference",
                "y": "lowest",
                "trigger": "difference",
                "rx": "cancel",
                "ry": "positive_dominant",
                "rz": "negative_dominant"
            }
        }
    }
}
```
The axis keys are split up by component. This means that for each axis (X, Y, Triggers, RX, RY, RZ) there are two keycodes, one for the positive component, and one for the negative. When both buttons are released, the axis value is 0.
If the negative axis value is bottomed out, the axis component value -X will be 127, and the axis value will be -127. If both buttons are pressed simultaneously, the output is decided by the resolution method.

```json
"top_deadzone": 50
```
This sets a deadzone to the top of the travel range, separate from the normal switch deadzone. While the switch is inside this deadzone, the axis is fully released. Defaults to 20. 

```json
"bottom_deadzone": 50
```
This sets a deadzone to the bottom of the travel range. While the switch is inside this deadzone, the axis is fully pressed. Defaults to 0.

Keep in mind that these deadzones stack with the normal switch deadzones.

```json
"layout": "XBOX"
```
This imports a list of aliases for the joystick buttons and axes, to make them easier. All buttons are prefixed by JS_. For example, using the xbox layout means that you can use keycodes like JS_A and JS_RT, while playstation can use JS_SQRE. Accepted values are xbox, playstation and nintendo. Defaults to xbox.

```json
"resolution_method": "difference"
```
If two competing axis buttons are pressed simultaneously, this options controls how the conflict is resolved.
For example, if the buttons that control the positive and negative X axis components are pressed together.

| Resolution method                 | Description                                                                                                                 |
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
Any axis that isn't named in the "resolution_methods" object will default to "resolution_method", which in turn defaults to "difference".

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

#### Keycodes
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
* The axis direction (X, Y, Z)
For the stick axis components, you can alternatively use the following aliases:
* L or R, for the Left or Right stick
* S for Stick
* L, R, U, D for Left, Right, Up, Down

| Axis component | XBox       | Playstation | Nintendo |
|----------------|------------|-------------|----------|
| JS_LPX         | JS_LSR     | JS_LSR      | JS_LSR   |
| JS_LNX         | JS_LSL     | JS_LSL      | JS_LSL   |
| JS_LPY         | JS_LSU     | JS_LSU      | JS_LSU   |
| JS_LNY         | JS_LSD     | JS_LSD      | JS_LSD   |
| JS_LPZ         | JS_RT      | JS_R2       | JS_ZR    |
| JS_LNZ         | JS_LT      | JS_L2       | JS_ZL    |
| JS_RPX         | JS_RSR     | JS_RSR      | JS_RSR   |
| JS_RNX         | JS_RSL     | JS_RSL      | JS_RSL   |
| JS_RPY         | JS_RSU     | JS_RSU      | JS_RSU   |
| JS_RNY         | JS_RSD     | JS_RSD      | JS_RSD   |
| JS_RPZ         | :o:        | :o:         | :o:      |
| JS_RNZ         | :o:        | :o:         | :o:      |

If the joystick feature is disabled, the button and axis keycodes will simply not work. There is no need to remove them from your keymap.


<!--MARK: Debug guide -->
## Debugging guide

#### Keyboard doesn't work after flashing

1. If you have a bootloader or bootmagic key configured, this key might be being triggered during startup, causing the keyboard to boot into the bootloader instead. Try disabling the bootloader/bootmagic keys to see if they're the cause.
    * To see if this is the case, you can look in your Device Manager. The bootloader will carry a name like "STM32 BOOTLOADER" or "RPI-RP2".
2. Another reason could be a conflict with another feature using the ADC. This feature very likely doesn't support this, but it hasn't been tested.

3. The ADC channel or multiplexer pins could be assigned to another function as well, causing issues.

4. Calibration mode might be triggered at startup, see next section.

#### Keyboard starts calibration by itself

1. If this happens every time the keyboard starts up, this likely means that either the calibration_key is registering a press during startup, or the calibration values aren't available. Try removing the calibration_keys parameter, if bound. If that doesn't help: <br>
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

You could also see if defining a default offset fixes this:
```c
#define DEFAULT_OFFSET 800
```
This will set the bottom value of a switch to be equal to (top value - DEFAULT_OFFSET) instead of starting calibration, if a key has invalid calibration data. If the calibration stops happening during keyboard startup after setting this value, it guarantees that the cause is a wrong calibration configuration.

If the no_eeprom flag isn't set (values are saved to storage), check if your chip has at least one of these:
* An external EEPROM/Flash chip, correctly wired up and configured, or
* Support for embedded flash emulation

As the storage configuration can be finicky, a misconfigured storage configuration is a likely reason for this issue.

2. If this only happens after flashing firmware, you're likely using a chip with the stm32-dfu bootloader, which always erases the whole internal flash.  
To circumvent this, you can use an external storage chip, or change your bootloader (if available, tinyuf2 is a good option).  
Another reason this could happen is that you're using the bootmagic function to go into the bootloader. The bootmagic function resets the EEPROM, so if your calibration values are saved there, they'll be lost. Use the bootloader_key instead.

3. If a calibration key is set to an invalid value, it will falsely trigger the associated function. Double check all your matrix positions, and keep in mind that the matrix positions defined in the [layout object](analog_matrix#layout) might not match up with the physical key location you see on the keyboard. For split keyboards, you should also keep in mind that the right half sits underneath the left half in the matrix definition. This means that a split keyboard with 5 rows and 6 columns per half will have a matrix size of 10 rows and 6 columns, instead of the expected 5 rows and 12 columns.

4. The difference between the top and bottom values that your sensor reads might be lower than the threshold that the keyboard expects for a valid value. In that case, you can use the following define to lower the threshold:
```c
#define CAL_THRESHOLD (5 * ADC_TOP_DEADZONE)
```
Keep in mind that the ADC is configured to use a 12 bit resolution.  

If your keyboard has a visible simple LED somewhere, you can use it to get visual feedback about the calibration state. The LED will be on while calibration is in progress, and turn off once it's finished and the new values have been saved. You can enable this like so:

In config.h:
```c
// This doesn't work with smart LEDs like WS2812 etc
#define LED_PIN B2 // This is the pin that the LED is connected to. It has to be a simple LED connected to a gpio pin.
```
If the LED works by pulling the connected GPIO pin low, you can add this define to invert the logic:
```c
#define LED_INVERTED
```
This will pull the line low, instead of high, when the LED should turn on. If your LED still doesn't work, then it's likely not a simple LED.
::: warning  
Using this while something is connected to the same line as the LED can cause problems.  
:::


#### Split keyboard half doesn't work

If your master half has lost connection to the slave half after flashing, the likely cause is a mismatch in the enabled features. If one of the features needs to send data to the other half, it needs to be enabled and configured on both halves. Flashing the slave half with the same firmware usually fixes this.
This includes adding another profile, joystick, MIDI, and priority keys (depending on the configuration).
If you have QMKs debug mode enabled, you will see messages saying "split synchronisation failed", or similar, in the console.

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
* One or more keys activate randomly on sensitive settings:
    * Increase the [filter strength](analog_matrix#filtering), or enable [debouncing](analog_matrix#debouncing)


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

## Additional Resources

If you want an implementation example, you can look up [my current keyboard.](https://www.github.com/vermilion00/qmk_firmware/tree/analog_matrix/keyboards/0/he)
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
