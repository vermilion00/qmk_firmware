from qmk.constants import CHIBIOS_PROCESSORS, LUFA_PROCESSORS, VUSB_PROCESSORS
from milc import cli

# If you want to have more profiles, you also need to add the objects to keyboard.jsonschema
MAX_PROFILES = 4

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


#MARK: Profiles
#TODO: Remove config height options, only use profiles for that
def generate_profile_config(he_profiles, config_h_lines, switch_num):
    """Extract the profile configuration"""
    if 'switch_mode' in he_profiles:
        config_h_lines.append(generate_define('PROFILE_SWITCH_MODE', f'{he_profiles['switch_mode'].upper()}'))

    trigger_heights = []
    release_heights = []
    press_distances = []
    release_distances = []
    profile_config = []
    rt_type = []
    rt_index = []
    profile_num = 0
    for profile in range(MAX_PROFILES):
        if f'profile_{profile}' in he_profiles:
            profile_num += 1
            if 'trigger_height' in he_profiles[f'profile_{profile}']:
                trigger_height = he_profiles[f'profile_{profile}']['trigger_height']
                if len(trigger_height) == 1:
                    trigger_height = [trigger_height[0] for num in range(switch_num)]
                trigger_heights.append(trigger_height)

                if 'release_height' in he_profiles[f'profile_{profile}']:
                    release_height = he_profiles[f'profile_{profile}']['release_height']
                    if len(release_height) == 1:
                        release_height = [release_height[0] for num in range(switch_num)]
                    release_heights.append(release_height)
                #If no release height is defined, set it to the trigger height
                else:
                    release_heights.append(trigger_height)
            else:   # No trigger height defined
                #TODO: Check if uneven arrays cause issues in c
                trigger_heights.append([])
                release_heights.append([])

            if 'rapid_trigger_type' in he_profiles[f'profile_{profile}']:
                rt_types = {
                    'NONE': 0,
                    'RAPID_TRIGGER': 1,
                    'CONTINUOUS_RAPID_TRIGGER': 2,
                    'CONSTANT_RAPID_TRIGGER': 3
                }
                rt_index.append(rt_types[he_profiles[f'profile_{profile}']['rapid_trigger_type'].upper()])
                rt_type.append(he_profiles[f'profile_{profile}']['rapid_trigger_type'].upper())

                if rt_type[profile] != 'NONE':
                    rt_press_distance = he_profiles[f'profile_{profile}']['rt_press_distance']
                    if len(rt_press_distance) == 1:
                        rt_press_distance = [rt_press_distance[0] for num in range(switch_num)]
                    press_distances.append(rt_press_distance)

                    if 'rt_release_distance' in he_profiles[f'profile_{profile}']:
                        rt_release_distance = he_profiles[f'profile_{profile}']['rt_release_distance']
                        if len(rt_release_distance) == 1:
                            rt_release_distance = [rt_release_distance[0] for num in range(switch_num)]
                        release_distances.append(rt_release_distance)
                    #If no release distance is defined, set it to the press distance
                    else:
                        release_distances.append(rt_press_distance)
                else:   #RT type = 'NONE'
                    press_distances.append([])
                    release_distances.append([])

            else:   #No RT defined
                rt_type.append('NONE')
                rt_index.append(0)
                press_distances.append([])
                release_distances.append([])

            #TODO: What is this?
            profile_config.append([])
            # Rest of the profile config
            if 'layers' in he_profiles[f'profile_{profile}']:
                profile_layer = 0
                profile_config[profile].append(sum(set(he_profiles[f'profile_{profile}']['layers'])))
            else:
                profile_config[profile].append(0)

            if 'rapid_trigger_type' in he_profiles[f'profile_{profile}']:
                profile_config[profile].append(rt_index[profile])
            else:
                profile_config[profile].append(0)

            #TODO: When switching to per-key rt modes, change this accordingly
            #      Prob just allow setting 2 and 3 in the array
            if 'rapid_trigger_keys' in he_profiles[f'profile_{profile}']:
                rapid_trigger_keys = he_profiles[f'profile_{profile}']['rapid_trigger_keys']
                rt_mask = []
                rest = switch_num
                if len(rapid_trigger_keys) == 1:
                    if rapid_trigger_keys[0] == 1:
                        while rest > 0:
                            if rest // 16 > 1:
                                rt_mask.append(65535)
                                rest -= 16
                            else:
                                rt_mask.append((1 << rest) - 1)
                                break
                    # If all keys are set to 0, you can leave them blank
                else:
                    while True:
                        # Divide the list into 16 bit slices
                        row = rapid_trigger_keys[:16]
                        value = 0
                        for bit in row:
                            value = (value << 1) | bit
                        rt_mask.append(value)
                        if len(row) < 16:
                            break
                profile_config[profile].append(rt_mask)
            else:
                profile_config[profile].append([])

        else: # Profile isn't in the json
            break

    # Add the profile config to info_config.h
    config_h_lines.append(generate_define('HE_PROFILE_NUM', 1 if profile_num < 1 else profile_num))
    #TODO: Add checks to this
    config_h_lines.append(generate_define('TRIGGER_HEIGHT', f'{str(trigger_heights).replace('[', '{').replace(']', '}')}'))
    config_h_lines.append(generate_define('RELEASE_HEIGHT', f'{str(release_heights).replace('[', '{').replace(']', '}')}'))
    config_h_lines.append(generate_define('RT_PRESS_DISTANCE', f'{str(press_distances).replace('[', '{').replace(']', '}')}'))
    config_h_lines.append(generate_define('RT_RELEASE_DISTANCE', f'{str(release_distances).replace('[', '{').replace(']', '}')}'))
    config_h_lines.append(generate_define('HE_PROFILE_CONFIG', f'{str(profile_config).replace('[', '{').replace(']', '}')}'))

    rt_set = set(rt_type)
    for type in rt_set:
        config_h_lines.append(generate_define(f'USE_{type}'))


#MARK: Validation
#TODO: Validate config against layout macro
#TODO: Outsource the profile validation?
def validate_hall_effect_config(he_json):
    """Validate the hall effect configuration."""
    he_hardware = he_json['hardware']
    he_config = he_json['config']
    if 'profiles' in he_json:
        he_profiles = he_json['profiles']

    valid = True
    if 'mux_to_num' not in he_hardware:
        valid = False
        print("mux_to_num needs to be configured")
    else:
        mux_to_num_len = 0
        for row in he_hardware['mux_to_num']:
            for i in row:
                if i > 0:
                    mux_to_num_len += 1

    if 'num_to_matrix' not in he_hardware:
        valid = False
        print("mux_to_num needs to be configured")

    if mux_to_num_len != len(he_hardware['num_to_matrix']):
        valid = False
        print("The amount of keys defined in mux_to_num doesn't equal the amount of keys in num_to_matrix")

    if 'profiles' in he_json:
        heights = ['trigger_height', 'release_height', 'rt_press_distance', 'rt_release_distance']
        for height in heights:
            if height in he_config and height in he_profiles['profile_0']:
                print(f'{height} is defined in the config as well as the profiles section, heights should be configured in the profiles section.')

        for profile in range(MAX_PROFILES):
            if f'profile_{profile}' not in he_profiles:
                break

            if 'rapid_trigger_type' in he_profiles[f'profile_{profile}']:
                if he_profiles[f'profile_{profile}']['rapid_trigger_type'] != 'constant_rapid_trigger':
                    if 'trigger_height' not in he_profiles[f'profile_{profile}']:
                        valid = False
                        print(f"trigger_height needs to be set if constant rapid trigger isn't used in profile_{profile}")
                    if 'rt_press_distance' not in he_profiles[f'profile_{profile}'] and he_profiles[f'profile_{profile}']['rapid_trigger_type'].upper() != 'NONE':
                        valid = False
                        print(f"rt_press_distance needs to be set if rapid trigger is used in profile_{profile}")
            else:
                if 'trigger_height' not in he_profiles[f'profile_{profile}']:
                    valid = False
                    print(f"Trigger height needs to be set if constant rapid trigger isn't used in profile_{profile}")

            if 'trigger_height' in he_profiles[f'profile_{profile}']:
                trigger_height_len = len(he_profiles[f'profile_{profile}']['trigger_height'])
                if trigger_height_len != 1 and trigger_height_len != mux_to_num_len:
                    valid = False
                    print(f"The amount of trigger_height values in profile_{profile} needs to be either 1 or equal to the amount of switches in mux_to_num")

            if 'release_height' in he_profiles[f'profile_{profile}']:
                release_height_len = len(he_profiles[f'profile_{profile}']['release_height'])
                if release_height_len != 1 and release_height_len != mux_to_num_len:
                    valid = False
                    print(f"The amount of release_height values in profile_{profile} needs to be either 1 or equal to the amount of switches in mux_to_num")

            if 'rt_press_distance' in he_profiles[f'profile_{profile}']:
                rt_press_len = len(he_profiles[f'profile_{profile}']['rt_press_distance'])
                if rt_press_len != 1 and rt_press_len != mux_to_num_len:
                    valid = False
                    print(f"The amount of rt_press_distance values in profile_{profile} needs to be either 1 or equal to the amount of switches in mux_to_num")

            if 'rt_release_distance' in he_profiles[f'profile_{profile}']:
                rt_release_len = len(he_profiles[f'profile_{profile}']['rt_release_distance'])
                if rt_release_len != 1 and rt_release_len != mux_to_num_len:
                    valid = False
                    print(f"The amount of trigger_height values in profile_{profile} needs to be either 1 or equal to the amount of switches in mux_to_num")

    else:
        if 'rapid_trigger_type' in he_config:
            if he_config['rapid_trigger_type'] != 'constant_rapid_trigger':
                if 'trigger_height' not in he_config and 'trigger_height' not in he_profiles['profiles']:
                    valid = False
                    print("trigger_height needs to be set if constant rapid trigger isn't used")
                if 'rt_press_distance' not in he_config and he_config['rapid_trigger_type'].upper() != 'NONE':
                    valid = False
                    print("rt_press_distance needs to be set if rapid trigger is used")
        else:
            if 'trigger_height' not in he_config:
                valid = False
                print("Trigger height needs to be set if constant rapid trigger isn't used")

        if 'trigger_height' in he_config:
            trigger_height_len = len(he_config['trigger_height'])
            if trigger_height_len != 1 and trigger_height_len != mux_to_num_len:
                valid = False
                print("The amount of trigger_height values needs to be either 1 or equal to the amount of switches in mux_to_num")

        if 'release_height' in he_config:
            release_height_len = len(he_config['release_height'])
            if release_height_len != 1 and release_height_len != mux_to_num_len:
                valid = False
                print("The amount of release_height values needs to be either 1 or equal to the amount of switches in mux_to_num")

        if 'rt_press_distance' in he_config:
            rt_press_len = len(he_config['rt_press_distance'])
            if rt_press_len != 1 and rt_press_len != mux_to_num_len:
                valid = False
                print("The amount of rt_press_distance values needs to be either 1 or equal to the amount of switches in mux_to_num")

        if 'rt_release_distance' in he_config:
            rt_release_len = len(he_config['rt_release_distance'])
            if rt_release_len != 1 and rt_release_len != mux_to_num_len:
                valid = False
                print("The amount of trigger_height values needs to be either 1 or equal to the amount of switches in mux_to_num")

    return valid


#MARK: Height validation
def validate_height_config(he_json, invert_adc, from_bottom):
    he_config = he_json['config']
    he_profiles = he_json['profiles']
    if (invert_adc and from_bottom) or (not invert_adc and not from_bottom):
        if 'trigger_height' in he_config and 'release_height' in he_config:
            for idx, i in enumerate(he_config['trigger_height']):
                if i < he_config['release_height'][idx]:
                    print(f'\nThe trigger height for key {idx} is higher than the release height when it needs to be lower or equal.\n')

        for profile in range(MAX_PROFILES):
            if 'trigger_height' in he_profiles[f'profile_{profile}'] and 'release_height' in he_profiles['profile_{profile}']:
                for idx, i in enumerate(he_config['trigger_height']):
                    if i < he_profiles[f'profile_{profile}']['release_height'][idx]:
                        print(f'\nThe trigger height for key {idx} in profile {profile} is higher than the release height when it needs to be lower or equal.\n')

    else:
        if 'trigger_height' in he_config and 'release_height' in he_config:
            for idx, i in enumerate(he_config['trigger_height']):
                if i > he_config['release_height'][idx]:
                    print(f'\nnThe trigger height for key {idx} is lower than the release height when it needs to be higher or equal.\n')

        for profile in range(MAX_PROFILES):
            if 'trigger_height' in he_profiles[f'profile_{profile}'] and 'release_height' in he_profiles['profile_{profile}']:
                for idx, i in enumerate(he_config['trigger_height']):
                    if i > he_profiles[f'profile_{profile}']['release_height'][idx]:
                        print(f'\nThe trigger height for key {idx} in profile {profile} is lower than the release height when it needs to be higher or equal.\n')



#MARK: General config
def generate_hall_effect_config(hall_effect_json, config_h_lines):
    """Generate the config.h lines for hall effect keyboards."""
    validate_hall_effect_config(hall_effect_json)

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

    #Only get the heights from the config if no profiles are defined
    if 'profiles' in hall_effect_json:
        return switch_num

    #Config stuff
    if 'trigger_height' in hall_effect_json['config']:
        trigger_height = hall_effect_json['config']['trigger_height']
        if len(trigger_height) == 1:
            height = trigger_height[0]
            trigger_height = [height for num in range(switch_num)]
        config_h_lines.append(generate_define('TRIGGER_HEIGHT', f'{{{{ {", ".join(map(str, trigger_height))} }}}}'))

        if 'release_height' in hall_effect_json['config']:
            release_height = hall_effect_json['config']['release_height']
            if len(release_height) == 1:
                height = release_height[0]
                release_height = [height for num in range(switch_num)]
            config_h_lines.append(generate_define('RELEASE_HEIGHT', f'{{{{ {", ".join(map(str, release_height))} }}}}'))
        #If no release height is defined, set it to the trigger height
        else:
            config_h_lines.append(generate_define('RELEASE_HEIGHT', f'{{{{ {", ".join(map(str, trigger_height))} }}}}'))

    if 'rapid_trigger_type' in hall_effect_json['config']:
        rt_type = hall_effect_json['config']['rapid_trigger_type'].upper()
        config_h_lines.append(generate_define(f'USE_{rt_type}'))

        rt_press_distance = hall_effect_json['config']['rt_press_distance']
        if len(rt_press_distance) == 1:
            distance = rt_press_distance[0]
            rt_press_distance = [distance for num in range(switch_num)]
        config_h_lines.append(generate_define('RT_PRESS_DISTANCE', f'{{{{ {", ".join(map(str, rt_press_distance))} }}}}'))

        if 'rt_release_distance' in hall_effect_json['config']:
            rt_release_distance = hall_effect_json['config']['rt_release_distance']
            if len(rt_release_distance) == 1:
                distance = rt_release_distance[0]
                rt_release_distance = [distance for num in range(switch_num)]
            config_h_lines.append(generate_define('RT_RELEASE_DISTANCE', f'{{{{ {", ".join(map(str, rt_release_distance))} }}}}'))
        #If no release distance is defined, set it to the press distance
        else:
            config_h_lines.append(generate_define('RT_RELEASE_DISTANCE', f'{{{{ {", ".join(map(str, rt_press_distance))} }}}}'))

    # Validate trigger_heights separately after setting, in case of a len 1 define
    if 'invert_adc' in hall_effect_json['hardware']:
        invert_adc = hall_effect_json['hardware']['invert_adc']
    else:
        invert_adc = False
    if 'distance_from_bottom' in hall_effect_json['config']:
        from_bottom = hall_effect_json['config']['distance_from_bottom']
    else:
        from_bottom = False

    validate_height_config(hall_effect_json, invert_adc, from_bottom)

    return switch_num
