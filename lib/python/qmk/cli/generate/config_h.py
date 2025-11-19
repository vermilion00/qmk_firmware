"""Used by the make system to generate info_config.h from info.json.
"""
from pathlib import Path
from dotty_dict import dotty

from argcomplete.completers import FilesCompleter
from milc import cli

from qmk.info import info_json
from qmk.json_schema import json_load
from qmk.keyboard import keyboard_completer, keyboard_folder
from qmk.commands import dump_lines, parse_configurator_json
from qmk.path import normpath, FileType
from qmk.constants import GPL2_HEADER_C_LIKE, GENERATED_HEADER_C_LIKE, CHIBIOS_PROCESSORS, LUFA_PROCESSORS, VUSB_PROCESSORS


def generate_define(define, value=None):
    is_keymap = cli.args.filename
    value = f' {value}' if value is not None else ''
    if is_keymap:
        return f"""
#undef {define}
#define {define}{value}"""
    return f"""
#ifndef {define}
#    define {define}{value}
#endif // {define}"""


def direct_pins(direct_pins, postfix):
    """Return the config.h lines that set the direct pins.
    """
    rows = []

    for row in direct_pins:
        cols = ','.join(map(str, [col or 'NO_PIN' for col in row]))
        rows.append('{' + cols + '}')

    return generate_define(f'DIRECT_PINS{postfix}', f'{{ {", ".join(rows)} }}')


def pin_array(define, pins, postfix):
    """Return the config.h lines that set a pin array.
    """
    pin_array = ', '.join(map(str, [pin or 'NO_PIN' for pin in pins]))

    return generate_define(f'{define}_PINS{postfix}', f'{{ {pin_array} }}')


def matrix_pins(matrix_pins, postfix=''):
    """Add the matrix config to the config.h.
    """
    pins = []

    if 'direct' in matrix_pins:
        pins.append(direct_pins(matrix_pins['direct'], postfix))

    if 'cols' in matrix_pins:
        pins.append(pin_array('MATRIX_COL', matrix_pins['cols'], postfix))

    if 'rows' in matrix_pins:
        pins.append(pin_array('MATRIX_ROW', matrix_pins['rows'], postfix))

    if 'he_mux_pins' in matrix_pins:
        pins.append(pin_array('HE_MUX_PINS', matrix_pins['he_mux_pins'], postfix))

    if 'he_adc_pins' in matrix_pins:
        pins.append(pin_array('HE_ADC_PINS', matrix_pins['he_adc_pins'], postfix))

    return '\n'.join(pins)


def generate_matrix_size(kb_info_json, config_h_lines):
    """Add the matrix size to the config.h.
    """
    if 'matrix_size' in kb_info_json:
        config_h_lines.append(generate_define('MATRIX_COLS', kb_info_json['matrix_size']['cols']))
        config_h_lines.append(generate_define('MATRIX_ROWS', kb_info_json['matrix_size']['rows']))

def generate_config_items(kb_info_json, config_h_lines):
    """Iterate through the info_config map to generate basic config values.
    """
    info_config_map = json_load(Path('data/mappings/info_config.hjson'))

    for config_key, info_dict in info_config_map.items():
        info_key = info_dict['info_key']
        key_type = info_dict.get('value_type', 'raw')
        to_c = info_dict.get('to_c', True)

        if not to_c:
            continue

        try:
            config_value = kb_info_json[info_key]
        except KeyError:
            continue

        if key_type.startswith('array.array'):
            config_h_lines.append(generate_define(config_key, f'{{ {", ".join(["{" + ",".join(list(map(str, x))) + "}" for x in config_value])} }}'))
        elif key_type.startswith('array'):
            config_h_lines.append(generate_define(config_key, f'{{ {", ".join(map(str, config_value))} }}'))
        elif key_type == 'bool':
            config_h_lines.append(generate_define(config_key, 'true' if config_value else 'false'))
        elif key_type == 'flag':
            if config_value:
                config_h_lines.append(generate_define(config_key))
        elif key_type == 'mapping':
            for key, value in config_value.items():
                config_h_lines.append(generate_define(key, value))
        elif key_type == 'str':
            escaped_str = config_value.replace('\\', '\\\\').replace('"', '\\"')
            config_h_lines.append(generate_define(config_key, f'"{escaped_str}"'))
        elif key_type == 'bcd_version':
            (major, minor, revision) = config_value.split('.')
            config_h_lines.append(generate_define(config_key, f'0x{major.zfill(2)}{minor}{revision}'))
        else:
            config_h_lines.append(generate_define(config_key, config_value))


def generate_encoder_config(encoder_json, config_h_lines, postfix=''):
    """Generate the config.h lines for encoders."""
    a_pads = []
    b_pads = []
    resolutions = []
    for encoder in encoder_json.get("rotary", []):
        a_pads.append(encoder["pin_a"])
        b_pads.append(encoder["pin_b"])
        resolutions.append(encoder.get("resolution", None))

    config_h_lines.append(generate_define(f'ENCODER_A_PINS{postfix}', f'{{ {", ".join(a_pads)} }}'))
    config_h_lines.append(generate_define(f'ENCODER_B_PINS{postfix}', f'{{ {", ".join(b_pads)} }}'))

    if None in resolutions:
        cli.log.debug(f"Unable to generate ENCODER_RESOLUTION{postfix} configuration")
    elif len(resolutions) == 0:
        cli.log.debug(f"Skipping ENCODER_RESOLUTION{postfix} configuration")
    elif len(set(resolutions)) == 1:
        config_h_lines.append(generate_define(f'ENCODER_RESOLUTION{postfix}', resolutions[0]))
    else:
        config_h_lines.append(generate_define(f'ENCODER_RESOLUTIONS{postfix}', f'{{ {", ".join(map(str,resolutions))} }}'))


def generate_split_config(kb_info_json, config_h_lines):
    """Generate the config.h lines for split boards."""
    if 'handedness' in kb_info_json['split']:
        # TODO: change SPLIT_HAND_MATRIX_GRID to require brackets
        handedness = kb_info_json['split']['handedness']
        if 'matrix_grid' in handedness:
            config_h_lines.append(generate_define('SPLIT_HAND_MATRIX_GRID', ', '.join(handedness['matrix_grid'])))

    if 'protocol' in kb_info_json['split'].get('transport', {}):
        if kb_info_json['split']['transport']['protocol'] == 'i2c':
            config_h_lines.append(generate_define('USE_I2C'))

    if 'right' in kb_info_json['split'].get('matrix_pins', {}):
        config_h_lines.append(matrix_pins(kb_info_json['split']['matrix_pins']['right'], '_RIGHT'))

    if 'right' in kb_info_json['split'].get('encoder', {}):
        generate_encoder_config(kb_info_json['split']['encoder']['right'], config_h_lines, '_RIGHT')


def generate_led_animations_config(feature, led_feature_json, config_h_lines, enable_prefix, animation_prefix):
    if 'animation' in led_feature_json.get('default', {}):
        config_h_lines.append(generate_define(f'{feature.upper()}_DEFAULT_MODE', f'{animation_prefix}{led_feature_json["default"]["animation"].upper()}'))

    for animation in led_feature_json.get('animations', {}):
        if led_feature_json['animations'][animation]:
            config_h_lines.append(generate_define(f'{enable_prefix}{animation.upper()}'))


def get_port_def(json):
    if json['processor'] in CHIBIOS_PROCESSORS:
        return "GPIO"

    if json['processor'] in LUFA_PROCESSORS + VUSB_PROCESSORS:
        return "PORT"

    raise Exception("Unknown processor!")


# MARK: Mux pins
def check_continuous_mux_pins(he_json, port_def, config_h_lines, postfix=""):
    """Check if mux pins are continuously on one port, and set the defines
    """
    if 'mux_pins' in he_json['hardware']:
        mux_pins = he_json['hardware']['mux_pins']
        port = mux_pins[0][:1]
        offset = int(mux_pins[0][1:])
        init_offset = offset
        for pin in mux_pins:
            if pin[:1] == port and int(pin[1:]) == offset:
                offset += 1
            else:
                return
        config_h_lines.append(generate_define('MUX_PINS_CONTINUOUS'))
        config_h_lines.append(generate_define('MUX_PIN_OFFSET', f'{init_offset}'))
        config_h_lines.append(generate_define('CONTINUOUS_MUX_PORT', f'{port_def}{port}'))


#MARK: Validation
#TODO: Validate config against layout macro
def validate_hall_effect_config(he_json):
    """Validate the hall effect configuration."""
    valid = True
    if 'mux_to_num' not in he_json['hardware']:
        valid = False
        raise Exception("mux_to_num needs to be configured")
    else:
        mux_to_num_len = 0
        for row in he_json['hardware']['mux_to_num']:
            for i in row:
                if i > 0:
                    mux_to_num_len += 1

    if 'num_to_matrix' not in he_json['hardware']:
        valid = False
        raise Exception("mux_to_num needs to be configured")

    if mux_to_num_len != len(he_json['hardware']['num_to_matrix']):
        valid = False
        raise Exception("The amount of keys defined in mux_to_num doesn't equal the amount of keys in num_to_matrix")

    if 'rapid_trigger_type' in he_json['config']:
        if he_json['config']['rapid_trigger_type'] != 'constant_rapid_trigger':
            if 'trigger_height' not in he_json['config']:
                valid = False
                raise Exception("trigger_height needs to be set if constant rapid trigger isn't used")
            if 'rt_press_distance' not in he_json['config']:
                valid = False
                raise Exception("rt_press_distance needs to be set if rapid trigger is used")
    else:
        if 'trigger_height' not in he_json['config']:
            valid = False
            raise Exception("Trigger height needs to be set if constant rapid trigger isn't used")

    if 'trigger_height' in he_json['config']:
        trigger_height_len = len(he_json['config']['trigger_height'])
        if trigger_height_len != 1 and trigger_height_len != mux_to_num_len:
            valid = False
            raise Exception("The amount of trigger_height values needs to be either 1 or equal to the amount of values in mux_to_num")

    if 'release_height' in he_json['config']:
        release_height_len = len(he_json['config']['release_height'])
        if release_height_len != 1 and release_height_len != mux_to_num_len:
            valid = False
            raise Exception("The amount of release_height values needs to be either 1 or equal to the amount of values in mux_to_num")

    if 'rt_press_distance' in he_json['config']:
        rt_press_len = len(he_json['config']['rt_press_distance'])
        if rt_press_len != 1 and rt_press_len != mux_to_num_len:
            valid = False
            raise Exception("The amount of rt_press_distance values needs to be either 1 or equal to the amount of values in mux_to_num")

    if 'rt_release_distance' in he_json['config']:
        rt_release_len = len(he_json['config']['rt_release_distance'])
        if rt_release_len != 1 and rt_release_len != mux_to_num_len:
            valid = False
            raise Exception("The amount of trigger_height values needs to be either 1 or equal to the amount of values in mux_to_num")


        # Used mux channels now derived from mux_to_num
        # if 'used_mux_channels' in he_json['config']:
        #     used_mux_channels = he_json['config']['used_mux_channels']
        #     if 'mux_pins' not in he_json['hardware']:
        #         raise Exception("Mux channels are defined but no mux pins are!")
        #     else:
        #         if used_mux_channels > (1 << len(he_json['mux_pins'])):
        #             raise Exception("The amount of mux channels is higher than the possible amount, set by mux_pins")

        #     if used_mux_channels != len(he_json['hardware']['mux_to_num']):
        #         raise Exception("The amount of mux channels needs to be equal to the amount of rows in mux_to_num")

        #TODO: Add checks to make sure the trigger heights are lower than or equal to the release heights

    return valid


#MARK: Extraction
def generate_hall_effect_config(hall_effect_json, config_h_lines):
    """Generate the config.h lines for hall effect keyboards."""
    validate_hall_effect_config(hall_effect_json)

    #TODO: Add config for correct us delay function, maybe in gpio func?

    #Hardware stuff
    adc_pin_num = len(hall_effect_json['hardware']['adc_pins'])
    config_h_lines.append(generate_define('ADC_PIN_NUM', adc_pin_num))
    switch_num = len(hall_effect_json['hardware']['num_to_matrix'])
    config_h_lines.append(generate_define('SWITCH_NUM', switch_num))
    mux_channels = len(hall_effect_json['hardware']['mux_to_num'])
    config_h_lines.append(generate_define('MUX_CHANNELS', mux_channels))
    if 'mux_pins' in hall_effect_json['hardware']:
        mux_pin_num = len(hall_effect_json['hardware']['mux_pins'])
        config_h_lines.append(generate_define('MUX_PIN_NUM', mux_pin_num))
    if 'power_pins' in hall_effect_json['hardware']:
        power_pin_num = len(hall_effect_json['hardware']['power_pins'])
        config_h_lines.append(generate_define('POWER_PIN_NUM', power_pin_num))

    #Config stuff
    if 'trigger_height' in hall_effect_json['config']:
        trigger_height = hall_effect_json['config']['trigger_height']
        if len(trigger_height) == 1:
            height = trigger_height[0]
            trigger_height = [height for num in range(switch_num)]
        config_h_lines.append(generate_define('TRIGGER_HEIGHT', f'{{ {", ".join(map(str, trigger_height))} }}'))

        if 'release_height' in hall_effect_json['config']:
            release_height = hall_effect_json['config']['release_height']
            if len(release_height) == 1:
                height = release_height[0]
                release_height = [height for num in range(switch_num)]
            config_h_lines.append(generate_define('RELEASE_HEIGHT', f'{{ {", ".join(map(str, release_height))} }}'))
        #If no release height is defined, set it to the trigger height
        else:
            config_h_lines.append(generate_define('RELEASE_HEIGHT', f'{{ {", ".join(map(str, trigger_height))} }}'))

    if 'rapid_trigger_type' in hall_effect_json['config']:
        rt_type = hall_effect_json['config']['rapid_trigger_type'].upper()
        config_h_lines.append(generate_define('RAPID_TRIGGER_TYPE', f'{rt_type}'))

        rt_press_distance = hall_effect_json['config']['rt_press_distance']
        if len(rt_press_distance) == 1:
            distance = rt_press_distance[0]
            rt_press_distance = [distance for num in range(switch_num)]
        config_h_lines.append(generate_define('RT_PRESS_DISTANCE', f'{{ {", ".join(map(str, rt_press_distance))} }}'))

        if 'rt_release_distance' in hall_effect_json['config']:
            rt_release_distance = hall_effect_json['config']['rt_release_distance']
            if len(rt_release_distance) == 1:
                distance = rt_release_distance[0]
                rt_release_distance = [distance for num in range(switch_num)]
            config_h_lines.append(generate_define('RT_RELEASE_DISTANCE', f'{{ {", ".join(map(str, rt_release_distance))} }}'))
        #If no release distance is defined, set it to the press distance
        else:
            config_h_lines.append(generate_define('RT_RELEASE_DISTANCE', f'{{ {", ".join(map(str, rt_press_distance))} }}'))




@cli.argument('filename', nargs='?', arg_only=True, type=FileType('r'), completer=FilesCompleter('.json'), help='A configurator export JSON to be compiled and flashed or a pre-compiled binary firmware file (bin/hex) to be flashed.')
@cli.argument('-o', '--output', arg_only=True, type=normpath, help='File to write to')
@cli.argument('-q', '--quiet', arg_only=True, action='store_true', help="Quiet mode, only output error messages")
@cli.argument('-kb', '--keyboard', arg_only=True, type=keyboard_folder, completer=keyboard_completer, help='Keyboard to generate config.h for.')
@cli.subcommand('Used by the make system to generate info_config.h from info.json', hidden=True)
def generate_config_h(cli):
    """Generates the info_config.h file.
    """
    # Determine our keyboard/keymap
    if cli.args.filename:
        user_keymap = parse_configurator_json(cli.args.filename)
        kb_info_json = dotty(user_keymap.get('config', {}))
    elif cli.args.keyboard:
        kb_info_json = dotty(info_json(cli.args.keyboard))
    else:
        cli.log.error('You must supply a configurator export or `--keyboard`.')
        cli.subcommands['generate-config-h'].print_help()
        return False

    # Build the info_config.h file.
    config_h_lines = [GPL2_HEADER_C_LIKE, GENERATED_HEADER_C_LIKE, '#pragma once']

    generate_config_items(kb_info_json, config_h_lines)

    generate_matrix_size(kb_info_json, config_h_lines)

    #MARK: Main function
    if 'hall_effect' in kb_info_json:
        generate_hall_effect_config(kb_info_json['hall_effect'], config_h_lines)
        port_def = get_port_def(kb_info_json)
        check_continuous_mux_pins(kb_info_json['hall_effect'], port_def, config_h_lines)

    if 'matrix_pins' in kb_info_json:
        config_h_lines.append(matrix_pins(kb_info_json['matrix_pins']))

    if 'encoder' in kb_info_json:
        generate_encoder_config(kb_info_json['encoder'], config_h_lines)

    if 'split' in kb_info_json:
        generate_split_config(kb_info_json, config_h_lines)

    if 'led_matrix' in kb_info_json:
        generate_led_animations_config('led_matrix', kb_info_json['led_matrix'], config_h_lines, 'ENABLE_LED_MATRIX_', 'LED_MATRIX_')

    if 'rgb_matrix' in kb_info_json:
        generate_led_animations_config('rgb_matrix', kb_info_json['rgb_matrix'], config_h_lines, 'ENABLE_RGB_MATRIX_', 'RGB_MATRIX_')

    if 'rgblight' in kb_info_json:
        generate_led_animations_config('rgblight', kb_info_json['rgblight'], config_h_lines, 'RGBLIGHT_EFFECT_', 'RGBLIGHT_MODE_')

    # Show the results
    dump_lines(cli.args.output, config_h_lines, cli.args.quiet)
