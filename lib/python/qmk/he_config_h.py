from qmk.constants import CHIBIOS_PROCESSORS, LUFA_PROCESSORS, VUSB_PROCESSORS
from milc import cli

# If you want to have more profiles, you also need to add the objects to keyboard.jsonschema
#TODO: Rework profile scanning function to loop through all things in profile object
#      and ignore things that don't start in profile_
MAX_PROFILES = 4
# Above this value, the key modes for special keys start. If for some reason I need more modes, increase this
MODE_NUM = 9
NO_KEY = [0, "X", "none","None", "NONE"]

INIT_KEYS = {
    'CALIBRATION_KEY': 'hall_effect.config.calibration_key',
    'CALIBRATION_KEY_RIGHT': 'hall_effect.config.calibration_key_right',
    'BOOTMAGIC_KEY': 'hall_effect.config.bootmagic_key',
    'BOOTMAGIC_KEY_RIGHT': 'hall_effect.config.bootmagic_key_right',
    'BOOTLOADER_KEY': 'hall_effect.config.bootloader_key',
    'BOOTLOADER_KEY_RIGHT': 'hall_effect.config.bootloader_key_right',
    #TODO: Enabling both causes it to skip the first, since you can't have two identical keys
    # Use normal bootmagic key for mechanical button reset
    # 'BOOTMAGIC_KEY': 'bootmagic.matrix',
    # 'BOOTMAGIC_KEY_RIGHT': 'split.bootmagic.matrix'
}

# _RIGHT keys automatically translate to their left variant
#TODO: Reassign bootmagic key to proper function, since it should also reset eeprom in case of issues
INIT_FUNCTIONS = {
    'BOOTLOADER_KEY': 'bootloader_jump',
    'BOOTMAGIC_KEY': 'bootloader_jump',
    'CALIBRATION_KEY': 'calibrate_switches'
}

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

#MARK: Transform
# Called by info.py before the rest of the functions here
def _transform_he(info_data):
    if 'hall_effect' not in info_data:
        return info_data

    he_hardware = info_data['hall_effect']['hardware']

    if 'split' in info_data and info_data['split'].get('enabled', False):
        info_data = _get_split_config(info_data)

        #TODO: Either disallow mux_to_num configs etc or add translations
        # Currently only layout mux definitons are allowed when using split keyboards
        info_data = _transform_layout_split(info_data)

    else: # split not in info_data
        if 'mux_to_num' in he_hardware and 'num_to_matrix' in he_hardware:
            info_data['hall_effect']['hardware']['mux_channels'] = len(he_hardware['mux_to_num'])
            info_data['hall_effect']['hardware']['switch_num'] = len(he_hardware['num_to_matrix'])

        elif 'mux_to_matrix' in he_hardware:
            if'mux_to_num' in he_hardware:
                print("mux_to_matrix and mux_to_num defined, but num_to_matrix is missing. mux_to_num will be overwritten by mux_to_matrix.")
            elif 'num_to_matrix' in he_hardware:
                print("mux_to_matrix and num_to_matrix defined, but mux_to_num is missing. num_to_matrix will be overwritten by mux_to_matrix.")
            info_data = _transform_mtm(info_data)

        # Since checking if 'mux' is set in the layout is hard to do cleanly, just call the function and check there
        else:
            info_data = _transform_layout(info_data)

    return info_data

#MARK: Transform mtm
def _transform_mtm(info_data):
    he_hardware = info_data['hall_effect']['hardware']
    mux_to_matrix = he_hardware['mux_to_matrix']
    used_positions = []
    mux_to_num = []
    mtn_row = []
    num_to_matrix = []
    matrix_index = 0
    for row in mux_to_matrix:
        for key in row:
            # Blank matrix spot at the index
            if key in NO_KEY:
                mtn_row.append(0)
            else:
                matrix_index += 1
                mtn_row.append(matrix_index)
                num_to_matrix.append(key)
                if key not in used_positions:
                    used_positions.append(key)
                else:
                    cli.log.error(f"Matrix position {key} appears multiple times in mux_to_matrix!")
        mux_to_num.append(mtn_row)
        mtn_row = []

    info_data['hall_effect']['hardware']['mux_to_num'] = mux_to_num
    info_data['hall_effect']['hardware']['num_to_matrix'] = num_to_matrix
    info_data['hall_effect']['hardware']['switch_num'] = len(num_to_matrix)
    info_data['hall_effect']['hardware']['mux_channels'] = len(mux_to_num)

    return info_data


#MARK: Transform layout
# Transform HE matrix definition from layout to mux_to_num and num_to_matrix
def _transform_layout(info_data):
    """Transforms the mux matrix defined in the layout into mux_to_num and num_to_matrix"""
    he_hardware = info_data['hall_effect']['hardware']

    if 'adc_pins' in he_hardware:
        adc_pins = len(he_hardware['adc_pins'])
    else:
        cli.log.error("No ADC pins defined!")
        return info_data

    max_channels = 1 << len(he_hardware.get('mux_pins', ''))

    # We only need one layout, hence the break at the end
    for layout_name, layout_data in info_data['layouts'].items():
        mux_to_num = [[0 for _ in range(adc_pins)] for _ in range(max_channels)]
        num_to_matrix = [[0, 0] for _ in range(len(layout_data['layout']))]
        key_index = 0
        used_positions = []
        for key_data in layout_data['layout']:
            if 'mux' in key_data:
                key_index += 1
                adc, mux = key_data['mux']
                mux_to_num[mux][adc] = key_index
                num_to_matrix[key_index-1] = key_data['matrix']
                if [adc, mux] not in used_positions:
                    used_positions.append([adc, mux])
                else:
                    cli.log.error(f"Mux combination {[adc, mux]} appears multiple times in the layout!")
            else:
                cli.log.error(f"Missing mux info on key {key_index} in layout {layout_name}!")
                return info_data
        break

    # Trim the mux_to_nums
    last_row = 0
    for num, row in enumerate(mux_to_num):
        for idx in row:
            if idx != 0:
                last_row = num
    mux_to_num = mux_to_num[:last_row + 1]

    # Trim the num_to_matrixes
    num_to_matrix = [i for i in num_to_matrix if i != -1]

    info_data['hall_effect']['hardware']['mux_to_num'] = mux_to_num
    info_data['hall_effect']['hardware']['num_to_matrix'] = num_to_matrix
    info_data['hall_effect']['hardware']['switch_num'] = len(num_to_matrix)
    info_data['hall_effect']['hardware']['mux_channels'] = len(mux_to_num)

    return info_data


#TODO: Add check for duplicate matrix positions
#MARK: Layout split
def _transform_layout_split(info_data):
    he_hardware = info_data['hall_effect']['hardware']

    if 'adc_pins' in he_hardware:
        adc_pin_num = len(he_hardware['adc_pins'])
    else:
        cli.log.error("No ADC pins defined!")
        return info_data
    mux_channels = 1 << len(he_hardware.get('mux_pins', ''))
    row_split = he_hardware['row_split']

    # We only need one layout, hence the break at the end
    for layout_name, layout_data in info_data['layouts'].items():
        mux_to_num_l = [[0 for _ in range(adc_pin_num)] for _ in range(mux_channels)]
        num_to_matrix_l = [-1 for _ in range(len(layout_data['layout']))]
        mux_to_num_r = [[0 for _ in range(adc_pin_num)] for _ in range(mux_channels)]
        #Oversize this since we don't know how many keys are there per side, then trim after
        num_to_matrix_r = [-1 for _ in range(len(layout_data['layout']))]
        key_index_l = 0
        key_index_r = 0
        used_positions_l = []
        used_positions_r = []
        for key_data in layout_data['layout']:
            if 'mux' in key_data:
                if key_data['matrix'][1] < row_split:
                    key_index_l += 1
                    adc, mux = key_data['mux']
                    mux_to_num_l[mux][adc] = key_index_l
                    num_to_matrix_l[key_index_l-1] = key_data['matrix']
                    if [adc, mux] not in used_positions_l:
                        used_positions_l.append([adc, mux])
                    else: # Mux combo already used
                        cli.log.error(f"Mux combination {[adc, mux]} appears multiple times in the left half of the layout!")
                #TODO: Subtract row_split from right side matrix row since offset is applied during
                #      split transaction
                else: # Right hand side
                    key_index_r += 1
                    adc, mux = key_data['mux']
                    mux_to_num_r[mux][adc] = key_index_r
                    matrix = [key_data['matrix'][0], key_data['matrix'][1] - row_split]
                    num_to_matrix_r[key_index_r-1] = matrix
                    if [adc, mux] not in used_positions_r:
                        used_positions_r.append([mux, adc])
                    else: # Mux combo already used
                        cli.log.error(f"Mux combination {[adc, mux]} appears multiple times in the right half of the layout!")

            else: # No mux definition in key data
                cli.log.error(f"Missing mux info on key {key_index_l + key_index_r} in layout {layout_name}!")
                return info_data
        break

    # Trim the mux_to_nums
    last_row = 0
    for num, row in enumerate(mux_to_num_l):
        for idx in row:
            if idx != 0:
                last_row = num
    mux_to_num_l = mux_to_num_l[:last_row + 1]

    for num, row in enumerate(mux_to_num_r):
        for idx in row:
            if idx != 0:
                last_row = num
    mux_to_num_r = mux_to_num_r[:last_row + 1]

    # Trim the num_to_matrixes
    num_to_matrix_l = [i for i in num_to_matrix_l if i != -1]
    num_to_matrix_r = [i for i in num_to_matrix_r if i != -1]

    #TODO: Do I actually need this many _right defines?
    info_data['hall_effect']['hardware']['mux_to_num'] = mux_to_num_l
    info_data['hall_effect']['hardware']['num_to_matrix'] = num_to_matrix_l
    info_data['hall_effect']['hardware']['mux_to_num_right'] = mux_to_num_r
    info_data['hall_effect']['hardware']['num_to_matrix_right'] = num_to_matrix_r
    info_data['hall_effect']['hardware']['mux_channels'] = len(mux_to_num_l)
    info_data['hall_effect']['hardware']['mux_channels_right'] = len(mux_to_num_r)
    info_data['hall_effect']['hardware']['switch_num'] = len(num_to_matrix_l)
    info_data['hall_effect']['hardware']['switch_num_right'] = len(num_to_matrix_r)
    info_data['hall_effect']['hardware']['mux_channels'] = len(mux_to_num_l)
    info_data['hall_effect']['hardware']['mux_channels_right'] = len(mux_to_num_r)

    return info_data


# Generates the row split index and the global to local transformations
#MARK: Get split config
def _get_split_config(info_data):
    # Get the row index that splits the two keyboard halves
    for layout_name, layout_data in info_data['layouts'].items():
        highest_row = 0
        for key_data in layout_data['layout']:
            if 'matrix' in key_data:
                if key_data['matrix'][0] > highest_row:
                    highest_row = key_data['matrix'][0]
            else:
                cli.log.error("Matrix data not defined!")
        break

    # Rows are doubled up when using split keyboards
    row_split = (highest_row + 1) // 2
    info_data['hall_effect']['hardware']['row_split'] = row_split
    # cli.log.info(f"Row split detected at row {row_split} in {layout_name}")

    # Generate global to local index transformation
    g_to_l_index = []
    l_to_g_index = [[], []]
    l_index_r = 0
    l_index_l = 0
    g_index = 0
    for layout_name, layout_data in info_data['layouts'].items():
        for key_data in layout_data['layout']:
            if 'matrix' in key_data:
                if key_data['matrix'][0] >= row_split:
                    g_to_l_index.append([1, l_index_r])
                    l_to_g_index[1].append(g_index)
                    l_index_r += 1

                else:
                    g_to_l_index.append([0, l_index_l])
                    l_to_g_index[0].append(g_index)
                    l_index_l += 1

                g_index += 1
        break

    info_data['hall_effect']['hardware']['global_to_local_index'] = g_to_l_index
    info_data['hall_effect']['hardware']['local_to_global_index'] = l_to_g_index[0]
    info_data['hall_effect']['hardware']['local_to_global_index_right'] = l_to_g_index[1]

    return info_data


#MARK: Matrix_to_mux
# Generates a reverse matrix transformation array for the initialization keys
def get_matrix_to_mux(info_data, config_h_lines):
    mux_to_num = info_data['hall_effect']['hardware']['mux_to_num']
    num_to_matrix = info_data['hall_effect']['hardware']['num_to_matrix']
    switch_num = info_data['hall_effect']['hardware']['switch_num']
    cols = info_data['matrix_size']['cols']
    # if 'split' in info_data and info_data['split'].get('enabled', False):
    #     #TODO: If scanning left side causes issues try using normal row num
    #     rows = info_data['matrix_size']['rows'] // 2
    # else:
    #     rows = info_data['matrix_size']['rows']

    rows = info_data['matrix_size']['rows']

    num_to_mux = [[] for _ in range(switch_num)]
    for row_idx, row in enumerate(mux_to_num):
        for col_idx, idx in enumerate(row):
            num_to_mux[idx-1] = [row_idx, col_idx]

    config_h_lines.append(generate_define('NUM_TO_MUX', str(num_to_mux).replace('[', '{').replace(']', '}')))
    info_data['hall_effect']['hardware']['num_to_mux'] = num_to_mux

    matrix_to_num = [[0 for _ in range(cols)] for _ in range(rows)]

    for idx, pos in enumerate(num_to_matrix):
        matrix_to_num[pos[0]][pos[1]] = idx + 1

    config_h_lines.append(generate_define('MATRIX_TO_NUM', str(matrix_to_num).replace('[', '{').replace(']', '}')))
    info_data['hall_effect']['hardware']['matrix_to_num'] = matrix_to_num

    if 'split' in info_data and info_data['split'].get('enabled', False):
        mux_to_num = info_data['hall_effect']['hardware']['mux_to_num_right']
        num_to_matrix = info_data['hall_effect']['hardware']['num_to_matrix_right']
        switch_num = info_data['hall_effect']['hardware']['switch_num_right']
        rows = info_data['matrix_size']['rows'] # Use normal row len for offset
        cols = info_data['matrix_size']['cols']
        num_to_mux = [[] for _ in range(switch_num)]

        for row_idx, row in enumerate(mux_to_num):
            for col_idx, idx in enumerate(row):
                num_to_mux[idx-1] = [row_idx, col_idx]

        config_h_lines.append(generate_define('NUM_TO_MUX_R', str(num_to_mux).replace('[', '{').replace(']', '}')))
        info_data['hall_effect']['hardware']['num_to_mux_right'] = num_to_mux

        matrix_to_num = [[0 for _ in range(cols)] for _ in range(rows)]

        for idx, pos in enumerate(num_to_matrix):
            matrix_to_num[pos[0]][pos[1]] = idx + 1

        config_h_lines.append(generate_define('MATRIX_TO_NUM_R', str(matrix_to_num).replace('[', '{').replace(']', '}')))
        info_data['hall_effect']['hardware']['matrix_to_num_right'] = matrix_to_num


    return info_data


#MARK: Port def
def get_port_def(json):
    if json['processor'] in CHIBIOS_PROCESSORS:
        return "GPIO"

    if json['processor'] in LUFA_PROCESSORS + VUSB_PROCESSORS:
        return "PORT"

    raise Exception("Unknown processor!")


# MARK: Mux pins
def check_mux_pins(he_json, port_def, config_h_lines):
    """Check if mux pins are continuous on one port, and set the defines
    """
    for postfix in ['', '_right']:
        if f'mux_pins{postfix}' in he_json['hardware']:
            mux_pins = he_json['hardware'][f'mux_pins{postfix}']
            port = mux_pins[0][:1]
            offset = int(mux_pins[0][1:])
            init_offset = offset

            for pin in mux_pins:
                if pin[:1] == port and int(pin[1:]) == offset:
                    offset += 1
                else:
                    return

            config_h_lines.append(generate_define(f'MUX_PINS{postfix.upper()}_CONTINUOUS'))
            config_h_lines.append(generate_define(f'MUX_PIN{postfix.upper()}_OFFSET', f'{init_offset}'))
            config_h_lines.append(generate_define(f'CONTINUOUS_MUX_PORT{postfix.upper()}', f'{port_def}{port}'))


# MARK: Power pins
def check_power_pins(he_json, port_def, config_h_lines):
    """Check if mux pins are continuous on one port, and set the defines
    """
    for postfix in ['', '_right']:
        if f'power_pins{postfix}' in he_json['hardware']:
            power_pins = he_json['hardware'][f'power_pins{postfix}']
            port = power_pins[0][:1]
            offset = int(power_pins[0][1:])
            init_offset = offset

            for pin in power_pins:
                if pin[:1] == port and int(pin[1:]) == offset:
                    offset += 1
                else:
                    return

            config_h_lines.append(generate_define(f'POWER_PINS{postfix.upper()}_CONTINUOUS'))
            config_h_lines.append(generate_define(f'POWER_PIN{postfix.upper()}_OFFSET', f'{init_offset}'))
            config_h_lines.append(generate_define(f'CONTINUOUS_POWER_PORT{postfix.upper()}', f'{port_def}{port}'))


#TODO: Split init keys by side
#MARK: Init keys
def transform_init_keys(info_data, config_h_lines):
    #TODO: Check if I need to define matrix_to_num etc or if just the keys suffice
    matrix_to_num = info_data['hall_effect']['hardware']['matrix_to_num']
    matrix_to_num_r = info_data['hall_effect']['hardware'].get('matrix_to_num_right', '')
    num_to_mux = info_data['hall_effect']['hardware']['num_to_mux']
    num_to_mux_r = info_data['hall_effect']['hardware'].get('num_to_mux_right', '')
    used_pos= []
    init_functions = []
    init_functions_r = []
    init_keys = []
    init_keys_r = []
    init_key_num = 0
    init_key_num_r = 0

    for key in INIT_KEYS:
        path = INIT_KEYS[key]
        if path in info_data:
            key_pos = info_data[path]
            if key_pos in used_pos:
                cli.log.error(f"Key {key_pos} has multiple initialization functions! Skipping {path}")
                continue
            else:
                used_pos.append(key_pos)
            if len(key) > 6 and key[-6:] == '_RIGHT':
                key = key[:-6]
                key_mux = num_to_mux_r[matrix_to_num_r[key_pos[0]][key_pos[1]] - 1]
                init_keys_r.append(key_mux)
                init_key_num_r += 1
                init_functions_r.append(INIT_FUNCTIONS[key])
            else:
                key_mux = num_to_mux[matrix_to_num[key_pos[0]][key_pos[1]] - 1]
                init_keys.append(key_mux)
                init_key_num += 1
                init_functions.append(INIT_FUNCTIONS[key])
            #TODO: Remove this
            cli.echo(f'Found key {path} with matrix {key_pos}, mux {key_mux} executing function {INIT_FUNCTIONS[key]}()')

    config_h_lines.append(generate_define('HE_INIT_KEY_NUM', init_key_num))
    if len(init_functions) > 0:
        config_h_lines.append(generate_define('HE_INIT_KEYS', str(init_keys).replace('[', '{').replace(']', '}')))
        #TODO: This way I can define the function names as strings and have them be defined as functions
        config_h_lines.append(generate_define('HE_INIT_FUNCTIONS', f'{{ {", ".join(map(str, init_functions))} }}'))

    config_h_lines.append(generate_define('HE_INIT_KEY_NUM_R', init_key_num))
    if len(init_functions_r) > 0:
        config_h_lines.append(generate_define('HE_INIT_KEYS_R', str(init_keys).replace('[', '{').replace(']', '}')))
        #TODO: This way I can define the function names as strings and have them be defined as functions
        config_h_lines.append(generate_define('HE_INIT_FUNCTIONS_R', f'{{ {", ".join(map(str, init_functions))} }}'))

#MARK: Profiles
#TODO: Remove config height options, only use profiles for that
def generate_profile_config(kb_info_json, config_h_lines):
    """Extract the profile configuration"""
    he_profiles = kb_info_json['hall_effect']['profiles']

    switch_num = kb_info_json['hall_effect']['hardware']['switch_num'] + kb_info_json['hall_effect']['hardware'].get('switch_num_right', 0)
    trigger_heights = []
    release_heights = []
    press_distances = []
    release_distances = []
    profile_config = []
    rt_type = []
    rt_num = []
    key_modes = []
    profile_num = 0

    for profile in range(MAX_PROFILES):
        if f'profile_{profile}' in he_profiles:
            profile_num += 1
            profile_data = he_profiles[f'profile_{profile}']
            if 'trigger_height' in profile_data:
                trigger_height = profile_data['trigger_height']
                if len(trigger_height) == 1:
                    trigger_height = [trigger_height[0] for _ in range(switch_num)]
                trigger_heights.append(trigger_height)

                if 'release_height' in profile_data:
                    release_height = profile_data['release_height']
                    if len(release_height) == 1:
                        release_height = [release_height[0] for _ in range(switch_num)]
                    release_heights.append(release_height)
                #If no release height is defined, set it to the trigger height
                else:
                    release_heights.append(trigger_height)
            else:   # No trigger height defined
                trigger_heights.append([0 for _ in range(switch_num)])
                release_heights.append([0 for _ in range(switch_num)])

            if 'rapid_trigger_type' in profile_data:
                rt_types = {
                    'NONE': 0,
                    'RAPID_TRIGGER': 1,
                    'CONTINUOUS_RAPID_TRIGGER': 2,
                    'CONSTANT_RAPID_TRIGGER': 3
                }
                rt_num.append(rt_types[profile_data['rapid_trigger_type'].upper()])
                rt_type.append(profile_data['rapid_trigger_type'].upper())

                if rt_type[profile] != 'NONE':
                    rt_press_distance = profile_data['rt_press_distance']
                    if len(rt_press_distance) == 1:
                        rt_press_distance = [rt_press_distance[0] for _ in range(switch_num)]
                    press_distances.append(rt_press_distance)

                    if 'rt_release_distance' in profile_data:
                        rt_release_distance = profile_data['rt_release_distance']
                        if len(rt_release_distance) == 1:
                            rt_release_distance = [rt_release_distance[0] for _ in range(switch_num)]
                        release_distances.append(rt_release_distance)
                    #If no release distance is defined, set it to the press distance
                    else:
                        release_distances.append(rt_press_distance)
                else:   #RT type = 'NONE'
                    press_distances.append([0 for _ in range(switch_num)])
                    release_distances.append([0 for _ in range(switch_num)])

            else:   #No RT defined
                rt_type.append('NONE')
                rt_num.append(0)
                press_distances.append([0 for _ in range(switch_num)])
                release_distances.append([0 for _ in range(switch_num)])

            # Rest of the profile config
            profile_config.append([])
            if 'layers' in profile_data:
                profile_layers = 0
                for num in profile_data['layers']:
                    profile_layers |= (1 << num)
                profile_config[profile].append(profile_layers)
            else:
                profile_config[profile].append(0)

            if 'rapid_trigger_type' in profile_data:
                profile_config[profile].append(rt_num[profile])
            else:
                profile_config[profile].append(0)

            #TODO: This can be merged with the section directly above
            #MARK: Key modes
            if 'rapid_trigger_type' in profile_data:
                rt_types = {
                    'NONE': 0,
                    'RAPID_TRIGGER': 1,
                    'CONTINUOUS_RAPID_TRIGGER': 2,
                    'CONSTANT_RAPID_TRIGGER': 3
                }
                # If a rapid_trigger_type is defined for the profile, set the mode to it for
                # all keys except the ones with disabled RT or special keys
                if 'key_modes' in profile_data:
                    rt_int = rt_types[profile_data['rapid_trigger_type'].upper()]
                    key_mode = profile_data['key_modes']
                    if len(key_mode) == 1:
                        key_mode = [key_mode[0] for _ in range(switch_num)]
                    modes = []
                    for mode in key_mode:
                        if mode > MODE_NUM or mode == 0:
                            modes.append(mode)
                        else:
                            modes.append(rt_int)
                    key_modes.append(modes)

                else: #RT defined but no key_modes
                    key_modes.append([rt_types[profile_data['rapid_trigger_type'].upper()] for _ in range(switch_num)])
            else:
                key_modes.append(profile_data['key_modes'])

        else: # Profile isn't in the json
            break

    # Check default profile
    if 'switch_mode' in he_profiles:
        config_h_lines.append(generate_define('PROFILE_SWITCH_MODE', f'{he_profiles['switch_mode'].upper()}'))
    else:
        config_h_lines.append(generate_define('PROFILE_SWITCH_MODE', 'LAST'))

    default_profile = he_profiles.get('default_profile', 0)
    profile_num = 1 if profile_num <= 1 else profile_num
    if default_profile >= profile_num:
        print(f"Default profile {default_profile} is outside of the possible range of profiles, since only {profile_num} are defined. Defaulting to profile 0. Keep in mind that profiles are 0-indexed.")
        default_profile = 0

    # Split up the height configs if necessary
    if 'split' in kb_info_json and kb_info_json['split'].get('enabled', False):
        g_to_l_index = kb_info_json['hall_effect']['hardware']['global_to_local_index']
        profile_num = len(trigger_heights)
        split_trigger_heights = [[[] for _ in range(profile_num)], [[] for _ in range(profile_num)]]
        split_release_heights = [[[] for _ in range(profile_num)], [[] for _ in range(profile_num)]]
        split_press_distances = [[[] for _ in range(profile_num)], [[] for _ in range(profile_num)]]
        split_release_distances = [[[] for _ in range(profile_num)], [[] for _ in range(profile_num)]]
        split_key_modes = [[[] for _ in range(profile_num)], [[] for _ in range(profile_num)]]
        for idx, _ in enumerate(trigger_heights[0]):
            for profile in range(profile_num):
                #TODO: Check if I need the local index or just the half
                if g_to_l_index[idx][0] == 0:
                    split_trigger_heights[0][profile].append(trigger_heights[profile][idx])
                    split_release_heights[0][profile].append(release_heights[profile][idx])
                    split_press_distances[0][profile].append(press_distances[profile][idx])
                    split_release_distances[0][profile].append(release_distances[profile][idx])
                    split_key_modes[0][profile].append(key_modes[profile][idx])
                else:
                    split_trigger_heights[1][profile].append(trigger_heights[profile][idx])
                    split_release_heights[1][profile].append(release_heights[profile][idx])
                    split_press_distances[1][profile].append(press_distances[profile][idx])
                    split_release_distances[1][profile].append(release_distances[profile][idx])
                    split_key_modes[1][profile].append(key_modes[profile][idx])

        config_h_lines.append(generate_define('TRIGGER_HEIGHT', f'{str(split_trigger_heights[0]).replace('[', '{').replace(']', '}')}'))
        config_h_lines.append(generate_define('RELEASE_HEIGHT', f'{str(split_release_heights[0]).replace('[', '{').replace(']', '}')}'))
        config_h_lines.append(generate_define('RT_PRESS_DISTANCE', f'{str(split_press_distances[0]).replace('[', '{').replace(']', '}')}'))
        config_h_lines.append(generate_define('RT_RELEASE_DISTANCE', f'{str(split_release_distances[0]).replace('[', '{').replace(']', '}')}'))
        config_h_lines.append(generate_define('TRIGGER_HEIGHT_R', f'{str(split_trigger_heights[1]).replace('[', '{').replace(']', '}')}'))
        config_h_lines.append(generate_define('RELEASE_HEIGHT_R', f'{str(split_release_heights[1]).replace('[', '{').replace(']', '}')}'))
        config_h_lines.append(generate_define('RT_PRESS_DISTANCE_R', f'{str(split_press_distances[1]).replace('[', '{').replace(']', '}')}'))
        config_h_lines.append(generate_define('RT_RELEASE_DISTANCE_R', f'{str(split_release_distances[1]).replace('[', '{').replace(']', '}')}'))
        config_h_lines.append(generate_define('KEY_MODES', f'{str(split_key_modes[0]).replace('[', '{').replace(']', '}')}'))
        config_h_lines.append(generate_define('KEY_MODES_R', f'{str(split_key_modes[1]).replace('[', '{').replace(']', '}')}'))

    else:
        config_h_lines.append(generate_define('TRIGGER_HEIGHT', f'{str(trigger_heights).replace('[', '{').replace(']', '}')}'))
        config_h_lines.append(generate_define('RELEASE_HEIGHT', f'{str(release_heights).replace('[', '{').replace(']', '}')}'))
        config_h_lines.append(generate_define('RT_PRESS_DISTANCE', f'{str(press_distances).replace('[', '{').replace(']', '}')}'))
        config_h_lines.append(generate_define('RT_RELEASE_DISTANCE', f'{str(release_distances).replace('[', '{').replace(']', '}')}'))
        config_h_lines.append(generate_define('KEY_MODES', f'{str(key_modes).replace('[', '{').replace(']', '}')}'))

    # Add the profile config to info_config.h
    config_h_lines.append(generate_define('HE_PROFILE_NUM', profile_num))
    config_h_lines.append(generate_define('HE_DEFAULT_PROFILE', default_profile))
    config_h_lines.append(generate_define('HE_PROFILE_CONFIG', f'{str(profile_config).replace('[', '{').replace(']', '}')}'))

    for type in set(rt_type):
        config_h_lines.append(generate_define(f'USE_{type}'))


#MARK: Mux_to_matrix
def validate_mux_to_matrix(mux_to_matrix):
    mux_to_num_len = 0
    matrix_pos = []
    valid = True
    for row in mux_to_matrix:
        for key in row:
            if key not in NO_KEY:
                mux_to_num_len += 1
                if key in matrix_pos:
                    print(f"Matrix position {key} appears multiple times in mux_to_matrix!")
                    valid = False
                else:
                    matrix_pos.append(key)

    return valid


#MARK: Validation
def validate_hall_effect_config(info_data):
    """Validate the hall effect configuration."""
    he_json = info_data['hall_effect']
    he_hardware = he_json['hardware']
    he_config = he_json['config']
    if 'profiles' in he_json:
        he_profiles = he_json['profiles']

    valid = True

    #TODO: Add validation for _right pins (or leave them off tbh)
    if 'power_pins' in he_hardware:
        if len(he_hardware.get('mux_to_num', '')) != len(he_hardware.get('power_pins', '')) \
            and not he_hardware.get('custom_power_before_scan', False):
            valid = False
            cli.log.error("If custom_power_before_scan isn't enabled, you need to define as many power_pins as you use mux_channels.")
    if 'power_pins_right' in he_hardware:
        if len(he_hardware.get('mux_to_num_right', '')) != len(he_hardware.get('power_pins_right', '')) \
            and not he_hardware.get('custom_power_before_scan', False):
            valid = False
            cli.log.error("If custom_power_before_scan isn't enabled, you need to define as many power_pins as you mux_channels are used.")

    if 'mux_to_num' not in he_hardware:
        valid = False
        print("You have to define a way to translate the ADC/MUX combination to the matrix position, using either the mux parameter in the layout, defining mux_to_matrix, or defining a combination of mux_to_num and num_to_matrix!")

    else:
        mux_to_num_len = 0
        for row in he_hardware['mux_to_num']:
            for i in row:
                if i > 0:
                    mux_to_num_len += 1

        if mux_to_num_len != len(he_hardware['num_to_matrix']):
            valid = False
            print("The amount of keys defined in mux_to_num doesn't equal the amount of keys in num_to_matrix!")

    if 'profiles' in he_json:
        heights = ['trigger_height', 'release_height', 'rt_press_distance', 'rt_release_distance']
        for height in heights:
            if height in he_config and height in he_profiles['profile_0']:
                print(f'{height} is defined in the config as well as the profiles section, heights should be configured in the profiles section if profiles are used.')

        for profile in range(MAX_PROFILES):
            if f'profile_{profile}' not in he_profiles:
                break
            profile_data = he_profiles[f'profile_{profile}']

            if 'rapid_trigger_type' in profile_data:
                if profile_data['rapid_trigger_type'] != 'constant_rapid_trigger':
                    if 'trigger_height' not in profile_data:
                        valid = False
                        print(f"trigger_height needs to be set if constant rapid trigger isn't used in profile_{profile}")
                    if 'rt_press_distance' not in profile_data and profile_data['rapid_trigger_type'].upper() != 'NONE':
                        valid = False
                        print(f"rt_press_distance needs to be set if rapid trigger is used in profile_{profile}")
            else:
                if 'trigger_height' not in profile_data:
                    valid = False
                    print(f"Trigger height needs to be set if constant rapid trigger isn't used in profile_{profile}")

            if 'trigger_height' in profile_data:
                trigger_height_len = len(profile_data['trigger_height'])
                if trigger_height_len != 1 and trigger_height_len != mux_to_num_len:
                    valid = False
                    print(f"The amount of trigger_height values in profile_{profile} needs to be either 1 or equal to the amount of switches in mux_to_num")

            if 'release_height' in profile_data:
                release_height_len = len(profile_data['release_height'])
                if release_height_len != 1 and release_height_len != mux_to_num_len:
                    valid = False
                    print(f"The amount of release_height values in profile_{profile} needs to be either 1 or equal to the amount of switches in mux_to_num")

            if 'rt_press_distance' in profile_data:
                rt_press_len = len(profile_data['rt_press_distance'])
                if rt_press_len != 1 and rt_press_len != mux_to_num_len:
                    valid = False
                    print(f"The amount of rt_press_distance values in profile_{profile} needs to be either 1 or equal to the amount of switches in mux_to_num")

            if 'rt_release_distance' in profile_data:
                rt_release_len = len(profile_data['rt_release_distance'])
                if rt_release_len != 1 and rt_release_len != mux_to_num_len:
                    valid = False
                    print(f"The amount of trigger_height values in profile_{profile} needs to be either 1 or equal to the amount of switches in mux_to_num")

    else: # Profiles not defined
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
# Checks if the heights are defined correctly
def validate_height_config(he_json, invert_adc, from_bottom):
    he_config = he_json['config']
    he_profiles = he_json['profiles']
    valid = True

    if (invert_adc and from_bottom) or (not invert_adc and not from_bottom):
        if 'trigger_height' in he_config and 'release_height' in he_config:
            for idx, i in enumerate(he_config['trigger_height']):
                if i < he_config['release_height'][idx]:
                    valid = False
                    print(f'\nThe trigger height for key {idx} is higher than the release height when it needs to be lower or equal.\n')

        for profile in range(len(he_profiles)):
            if f'profile_{profile}' in he_profiles:
                profile_data = he_profiles[f'profile_{profile}']
                if 'trigger_height' in profile_data and 'release_height' in profile_data:
                    for idx, i in enumerate(profile_data['trigger_height']):
                        if i < profile_data['release_height'][idx]:
                            valid = False
                            print(f'\nThe trigger height for key {idx} in profile {profile} is higher than the release height when it needs to be lower or equal.\n')

    else:
        if 'trigger_height' in he_config and 'release_height' in he_config:
            for idx, i in enumerate(he_config['trigger_height']):
                if i > he_config['release_height'][idx]:
                    valid = False
                    print(f'\nnThe trigger height for key {idx} is lower than the release height when it needs to be higher or equal.\n')

        for profile in range(len(he_profiles)):
            if f'profile_{profile}' in he_profiles:
                profile_data = he_profiles[f'profile_{profile}']
                if 'trigger_height' in profile_data and 'release_height' in profile_data:
                    for idx, i in enumerate(profile_data['trigger_height']):
                        if i > profile_data['release_height'][idx]:
                            valid = False
                            print(f'\nThe trigger height for key {idx} in profile {profile} is lower than the release height when it needs to be higher or equal.\n')

    return valid


#MARK: General config
def generate_hall_effect_config(info_data, config_h_lines):
    """Generate the config.h lines for hall effect keyboards."""
    # validate_hall_effect_config(info_data)

    if 'split' in info_data and info_data['split'].get('enabled', False):
        info_data = check_right_side_pins(info_data, config_h_lines)

    he_json = info_data['hall_effect']
    he_hardware = info_data['hall_effect']['hardware']
    #Hardware stuff
    adc_pin_num = len(he_hardware.get('adc_pins', ''))
    config_h_lines.append(generate_define('ADC_PIN_NUM', adc_pin_num))

    port_def = get_port_def(info_data)

    if 'mux_pins' in he_hardware:
        mux_pin_num = len(he_hardware.get('mux_pins', ''))
        config_h_lines.append(generate_define('MUX_PIN_NUM', mux_pin_num))
        check_mux_pins(info_data['hall_effect'], port_def, config_h_lines)
    else:
        #TODO: Is this necessary?
        config_h_lines.append(generate_define('MUX_PIN_NUM', 0))

    if 'power_pins' in he_hardware:
        power_pin_num = len(he_hardware['power_pins'])
        config_h_lines.append(generate_define('POWER_PIN_NUM', power_pin_num))
        config_h_lines.append(generate_define('POWER_BEFORE_SCAN', 'TRUE'))
        check_power_pins(info_data['hall_effect'], port_def, config_h_lines)

    # This is the total switch num to be used with the trigger_heights
    #TODO: Since the heights are defined in one array, make a transform matrix that transforms
    #      the global index into the index and side, to be used in the translation function
    switch_num = he_hardware.get('switch_num', 0) + he_hardware.get('switch_num_right', 0)

    # Get the reverse transform arrays
    info_data = get_matrix_to_mux(info_data, config_h_lines)

    # Transform init key matrix positions to mux combinations
    transform_init_keys(info_data, config_h_lines)

    #Only get the heights from the config if no profiles are defined
    if 'profiles' not in he_json:
        #Config stuff
        if 'trigger_height' in he_json['config']:
            trigger_height = he_json['config']['trigger_height']
            if len(trigger_height) == 1:
                height = trigger_height[0]
                trigger_height = [height for _ in range(switch_num)]
            config_h_lines.append(generate_define('TRIGGER_HEIGHT', f'{{{{ {", ".join(map(str, trigger_height))} }}}}'))

            if 'release_height' in he_json['config']:
                release_height = he_json['config']['release_height']
                if len(release_height) == 1:
                    height = release_height[0]
                    release_height = [height for _ in range(switch_num)]
                config_h_lines.append(generate_define('RELEASE_HEIGHT', f'{{{{ {", ".join(map(str, release_height))} }}}}'))
            #If no release height is defined, set it to the trigger height
            else:
                config_h_lines.append(generate_define('RELEASE_HEIGHT', f'{{{{ {", ".join(map(str, trigger_height))} }}}}'))

        if 'rapid_trigger_type' in he_json['config']:
            rt_type = he_json['config']['rapid_trigger_type'].upper()
            config_h_lines.append(generate_define(f'USE_{rt_type}'))

            rt_press_distance = he_json['config']['rt_press_distance']
            if len(rt_press_distance) == 1:
                distance = rt_press_distance[0]
                rt_press_distance = [distance for _ in range(switch_num)]
            config_h_lines.append(generate_define('RT_PRESS_DISTANCE', f'{{{{ {", ".join(map(str, rt_press_distance))} }}}}'))

            if 'rt_release_distance' in he_json['config']:
                rt_release_distance = he_json['config']['rt_release_distance']
                if len(rt_release_distance) == 1:
                    distance = rt_release_distance[0]
                    rt_release_distance = [distance for _ in range(switch_num)]
                config_h_lines.append(generate_define('RT_RELEASE_DISTANCE', f'{{{{ {", ".join(map(str, rt_release_distance))} }}}}'))
            #If no release distance is defined, set it to the press distance
            else:
                config_h_lines.append(generate_define('RT_RELEASE_DISTANCE', f'{{{{ {", ".join(map(str, rt_press_distance))} }}}}'))

    # Validate trigger_heights separately after setting, in case of a len 1 define
    invert_adc = he_hardware.get('invert_adc', False)
    from_bottom = he_json['config'].get('distance_from_bottom', False)

    # validate_height_config(he_json, invert_adc, from_bottom)

    return info_data



#MARK: Right side pins
def check_right_side_pins(info_data, config_h_lines):
    he_hardware = info_data['hall_effect']['hardware']

    for pins in ['adc_pins', 'mux_pins', 'power_pins']:
        if pins in he_hardware and f'{pins}_right' not in he_hardware:
            pins_r = he_hardware[pins]
            info_data['hall_effect']['hardware'][f'{pins}_right'] = pins_r
            config_h_lines.append(generate_define(f'{pins.upper()}_R', f'{{ {", ".join(map(str, pins_r))} }}'))

    return info_data
