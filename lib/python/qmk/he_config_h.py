from qmk.constants import CHIBIOS_PROCESSORS, LUFA_PROCESSORS, VUSB_PROCESSORS
from milc import cli

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

#MARK: Port def
def get_port_def(json):
    if json['processor'] in CHIBIOS_PROCESSORS:
        return "GPIO"

    if json['processor'] in LUFA_PROCESSORS + VUSB_PROCESSORS:
        return "PORT"

    raise Exception("Unknown processor!")


# MARK: Mux pins
def check_mux_pins(he_json, port_def, config_h_lines, postfix=""):
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
            raise Exception("The amount of trigger_height values needs to be either 1 or equal to the amount of switches in mux_to_num")

    if 'release_height' in he_json['config']:
        release_height_len = len(he_json['config']['release_height'])
        if release_height_len != 1 and release_height_len != mux_to_num_len:
            valid = False
            raise Exception("The amount of release_height values needs to be either 1 or equal to the amount of switches in mux_to_num")

    if 'rt_press_distance' in he_json['config']:
        rt_press_len = len(he_json['config']['rt_press_distance'])
        if rt_press_len != 1 and rt_press_len != mux_to_num_len:
            valid = False
            raise Exception("The amount of rt_press_distance values needs to be either 1 or equal to the amount of switches in mux_to_num")

    if 'rt_release_distance' in he_json['config']:
        rt_release_len = len(he_json['config']['rt_release_distance'])
        if rt_release_len != 1 and rt_release_len != mux_to_num_len:
            valid = False
            raise Exception("The amount of trigger_height values needs to be either 1 or equal to the amount of switches in mux_to_num")

    return valid


#MARK: Height validation
def validate_height_config(he_config, configs, invert_adc, from_bottom):
    #TODO: Simplify this
    if (invert_adc and from_bottom) or (not invert_adc and not from_bottom):
        if 'trigger_height' in configs and 'release_height' in configs:
            for idx, i in enumerate(he_config['trigger_height']):
                if i < he_config['release_height'][idx]:
                    raise Exception(f'The trigger height for key {idx} is higher than the release height when it needs to be lower.')
    else:
        if 'trigger_height' in configs and 'release_height' in configs:
            for idx, i in enumerate(he_config['trigger_height']):
                if i > he_config['release_height'][idx]:
                    raise Exception(f'The trigger height for key {idx} is lower than the release height when it needs to be higher.')

    #The press distance can actually be higher than the release distance, no need to check


#MARK: Extraction
def generate_hall_effect_config(hall_effect_json, config_h_lines):
    """Generate the config.h lines for hall effect keyboards."""
    validate_hall_effect_config(hall_effect_json)
    configs = []

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
        configs.append('trigger_height')
        if len(trigger_height) == 1:
            height = trigger_height[0]
            trigger_height = [height for num in range(switch_num)]
        config_h_lines.append(generate_define('TRIGGER_HEIGHT', f'{{ {", ".join(map(str, trigger_height))} }}'))

        if 'release_height' in hall_effect_json['config']:
            release_height = hall_effect_json['config']['release_height']
            configs.append('release_height')
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
        # configs.append('press_distance')

        rt_press_distance = hall_effect_json['config']['rt_press_distance']
        if len(rt_press_distance) == 1:
            distance = rt_press_distance[0]
            rt_press_distance = [distance for num in range(switch_num)]
        config_h_lines.append(generate_define('RT_PRESS_DISTANCE', f'{{ {", ".join(map(str, rt_press_distance))} }}'))

        if 'rt_release_distance' in hall_effect_json['config']:
            rt_release_distance = hall_effect_json['config']['rt_release_distance']
            # configs.append('release_distance')
            if len(rt_release_distance) == 1:
                distance = rt_release_distance[0]
                rt_release_distance = [distance for num in range(switch_num)]
            config_h_lines.append(generate_define('RT_RELEASE_DISTANCE', f'{{ {", ".join(map(str, rt_release_distance))} }}'))
        #If no release distance is defined, set it to the press distance
        else:
            config_h_lines.append(generate_define('RT_RELEASE_DISTANCE', f'{{ {", ".join(map(str, rt_press_distance))} }}'))

    # Validate trigger_heights separately after setting, in case of a len 1 define
    if 'invert_adc' in hall_effect_json['hardware']:
        invert_adc = hall_effect_json['hardware']['invert_adc']
    else:
        invert_adc = False
    if 'distance_from_bottom' in hall_effect_json['config']:
        from_bottom = hall_effect_json['config']['distance_from_bottom']
    else:
        from_bottom = False

    validate_height_config(hall_effect_json['config'], configs, invert_adc, from_bottom)
