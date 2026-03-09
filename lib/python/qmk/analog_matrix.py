from qmk.constants import CHIBIOS_PROCESSORS, LUFA_PROCESSORS, VUSB_PROCESSORS
from milc import cli

#TODO: Expand this list, need to make sure they follow the same calling structure as well
#BSRR needs to have set priority, be 32 bits, with upper 16 bits being the reset bits
BSRR_PROCESSORS = 'RP2040', 'STM32F042', 'STM32F072', 'STM32F303', 'STM32F401', 'STM32F405', 'STM32F407', 'STM32F411', 'STM32F446', 'STM32G0B1', 'STM32G431', 'STM32G474', 'STM32H723', 'STM32H733', 'STM32L412', 'STM32L422', 'STM32L432', 'STM32L433', 'STM32L442', 'STM32L443', 'AT32F415'
# Above this value, the key modes for special keys start. If for some reason I need more modes, increase this
MODE_NUM = 9
NO_KEY = [0, "X", "none", "None", "NONE"]

INIT_KEYS = {
    'CALIBRATION_KEY': 'analog_matrix.config.calibration_key',
    'CALIBRATION_KEY_RIGHT': 'analog_matrix.config.calibration_key_right',
    'BOOTMAGIC_KEY': 'analog_matrix.config.bootmagic_key',
    'BOOTMAGIC_KEY_RIGHT': 'analog_matrix.config.bootmagic_key_right',
    'BOOTLOADER_KEY': 'analog_matrix.config.bootloader_key',
    'BOOTLOADER_KEY_RIGHT': 'analog_matrix.config.bootloader_key_right',
    # Use normal bootmagic key for mechanical button reset
    # 'BOOTMAGIC_KEY': 'bootmagic.matrix',
    # 'BOOTMAGIC_KEY_RIGHT': 'split.bootmagic.matrix'
}

# _RIGHT keys automatically translate to their left variant
INIT_FUNCTIONS = {
    'BOOTLOADER_KEY': 'bootloader_jump',
    'BOOTMAGIC_KEY': '_bootmagic',
    'CALIBRATION_KEY': 'calibrate_switches'
}

#TODO: Do I need the joystick type here?
RT_TYPES = {
    'NONE': 0,
    'RAPID_TRIGGER': 1,
    'CONTINUOUS_RAPID_TRIGGER': 2,
    'CONSTANT_RAPID_TRIGGER': 3,
    'JOYSTICK': 4
}

RT_NAMES = {
   -1: 'INVALID',
    0: 'NONE',
    1: 'RAPID_TRIGGER',
    2: 'CONTINUOUS_RAPID_TRIGGER',
    3: 'CONSTANT_RAPID_TRIGGER',
    4: 'JOYSTICK'
}

# Will be populated with all features in lowercase and their status, taken from json{'features'}
features = {}
# Will be populated with all analog_matrix feature objects in lowercase
objects = []

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
def _transform_am(info_data):
    if 'analog_matrix' not in info_data:
        return info_data

    # he_hardware = info_data['analog_matrix']['hardware']

    if 'split' in info_data and info_data['split'].get('enabled', False):
        info_data = _get_split_config(info_data)

        #TODO: Either disallow mux_to_num configs etc or add translations
        # Currently only layout mux definitons are allowed when using split keyboards
        info_data = _transform_layout_split(info_data)

    else: # split not in info_data
        info_data = _transform_layout(info_data)

    return info_data


#MARK: Transform layout
# Transform analog matrix definition from layout to mux_to_num and num_to_matrix
def _transform_layout(info_data):
    """Transforms the mux matrix defined in the layout into mux_to_num and num_to_matrix"""
    he_hardware = info_data['analog_matrix']['hardware']

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
                #TODO: When mixed matrices are a thing, remove this
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

    info_data['analog_matrix']['hardware']['mux_to_num'] = mux_to_num
    info_data['analog_matrix']['hardware']['num_to_matrix'] = num_to_matrix
    info_data['analog_matrix']['hardware']['switch_num'] = len(num_to_matrix)
    info_data['analog_matrix']['hardware']['switch_num_right'] = 0
    info_data['analog_matrix']['hardware']['switch_num_l'] = len(num_to_matrix)
    info_data['analog_matrix']['hardware']['total_switch_num'] = len(num_to_matrix)
    info_data['analog_matrix']['hardware']['mux_channels'] = len(mux_to_num)

    return info_data


#TODO: Add check for duplicate matrix positions
#MARK: Layout split
def _transform_layout_split(info_data):
    he_hardware = info_data['analog_matrix']['hardware']

    if 'adc_pins' in he_hardware:
        adc_pin_num = len(he_hardware['adc_pins'])
    else:
        cli.log.error("No ADC pins defined!")
        return info_data
    mux_channels = 1 << len(he_hardware.get('mux_pins', ''))
    row_split = he_hardware['row_split']

    for layout_name, layout_data in info_data['layouts'].items():
        mux_to_num_l = [[0 for _ in range(adc_pin_num)] for _ in range(mux_channels)]
        num_to_matrix_l = [-1 for _ in range(len(layout_data['layout']))]
        mux_to_num_r = [[0 for _ in range(adc_pin_num)] for _ in range(mux_channels)]
        #Oversize this since we don't know how many keys there are per side, then trim after
        num_to_matrix_r = [-1 for _ in range(len(layout_data['layout']))]
        key_index_l = 0
        key_index_r = 0
        used_positions_l = []
        used_positions_r = []
        for key_data in layout_data['layout']:
            if 'mux' in key_data:
                if key_data['matrix'][0] < row_split:
                    key_index_l += 1
                    adc, mux = key_data['mux']
                    mux_to_num_l[mux][adc] = key_index_l
                    num_to_matrix_l[key_index_l-1] = key_data['matrix']
                    if [adc, mux] not in used_positions_l:
                        used_positions_l.append([adc, mux])
                    else: # Mux combo already used
                        cli.log.error(f"Mux combination {[adc, mux]} appears multiple times in the left half of the layout!")
                else: # Right hand side
                    key_index_r += 1
                    adc, mux = key_data['mux']
                    mux_to_num_r[mux][adc] = key_index_r
                    matrix = [key_data['matrix'][0], key_data['matrix'][1]]
                    #TODO: This works, but maybe find a way without this warning?
                    num_to_matrix_r[key_index_r-1] = matrix
                    if [adc, mux] not in used_positions_r:
                        used_positions_r.append([adc, mux])
                    else: # Mux combo already used
                        cli.log.error(f"Mux combination {[adc, mux]} appears multiple times in the right half of the layout!")

            else: # No mux definition in key data
                cli.log.error(f"Missing mux info on key {key_index_l + key_index_r} in layout {layout_name}!")
                return info_data
        # We only need one layout
        break

    # Trim mux_to_nums
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

    info_data['analog_matrix']['hardware']['mux_to_num'] = mux_to_num_l
    info_data['analog_matrix']['hardware']['num_to_matrix'] = num_to_matrix_l
    info_data['analog_matrix']['hardware']['mux_to_num_right'] = mux_to_num_r
    info_data['analog_matrix']['hardware']['num_to_matrix_right'] = num_to_matrix_r
    info_data['analog_matrix']['hardware']['mux_channels'] = len(mux_to_num_l)
    info_data['analog_matrix']['hardware']['mux_channels_right'] = len(mux_to_num_r)
    info_data['analog_matrix']['hardware']['switch_num'] = len(num_to_matrix_l)
    info_data['analog_matrix']['hardware']['switch_num_l'] = len(num_to_matrix_l)
    info_data['analog_matrix']['hardware']['switch_num_right'] = len(num_to_matrix_r)
    info_data['analog_matrix']['hardware']['total_switch_num'] = len(num_to_matrix_l) + len(num_to_matrix_r)
    info_data['analog_matrix']['hardware']['mux_channels'] = len(mux_to_num_l)
    info_data['analog_matrix']['hardware']['mux_channels_right'] = len(mux_to_num_r)

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
    info_data['analog_matrix']['hardware']['row_split'] = row_split

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

    info_data['analog_matrix']['hardware']['global_to_local_index'] = g_to_l_index
    info_data['analog_matrix']['hardware']['local_to_global_index'] = l_to_g_index[0]
    info_data['analog_matrix']['hardware']['local_to_global_index_right'] = l_to_g_index[1]

    return info_data


#MARK: Matrix_to_mux
# Generates a reverse matrix transformation array for the initialization keys
def get_matrix_to_mux(info_data, config_h_lines):
    mux_to_num = info_data['analog_matrix']['hardware']['mux_to_num']
    num_to_matrix = info_data['analog_matrix']['hardware']['num_to_matrix']
    switch_num = info_data['analog_matrix']['hardware']['switch_num']
    cols = info_data['matrix_size']['cols']
    if 'split' in info_data and info_data['split'].get('enabled', False):
        rows = info_data['matrix_size']['rows'] // 2
    else:
        rows = info_data['matrix_size']['rows']

    num_to_mux = [[] for _ in range(switch_num)]
    for row_idx, row in enumerate(mux_to_num):
        for col_idx, idx in enumerate(row):
            num_to_mux[idx-1] = [col_idx, row_idx]

    config_h_lines.append(generate_define('NUM_TO_MUX', str(num_to_mux).replace('[', '{').replace(']', '}')))
    info_data['analog_matrix']['hardware']['num_to_mux'] = num_to_mux

    matrix_to_num = [[0 for _ in range(cols)] for _ in range(rows)]

    for idx, pos in enumerate(num_to_matrix):
        matrix_to_num[pos[0]][pos[1]] = idx + 1

    config_h_lines.append(generate_define('MATRIX_TO_NUM', str(matrix_to_num).replace('[', '{').replace(']', '}')))
    info_data['analog_matrix']['hardware']['matrix_to_num'] = matrix_to_num

    if 'split' in info_data and info_data['split'].get('enabled', False):
        mux_to_num = info_data['analog_matrix']['hardware']['mux_to_num_right']
        num_to_matrix = info_data['analog_matrix']['hardware']['num_to_matrix_right']
        switch_num = info_data['analog_matrix']['hardware']['switch_num_right']
        rows = info_data['matrix_size']['rows'] // 2
        cols = info_data['matrix_size']['cols']
        row_split = info_data['analog_matrix']['hardware'].get('row_split', 0)
        num_to_mux = [[] for _ in range(switch_num)]

        for row_idx, row in enumerate(mux_to_num):
            for col_idx, idx in enumerate(row):
                num_to_mux[idx-1] = [col_idx, row_idx]

        config_h_lines.append(generate_define('NUM_TO_MUX_R', str(num_to_mux).replace('[', '{').replace(']', '}')))
        info_data['analog_matrix']['hardware']['num_to_mux_right'] = num_to_mux

        matrix_to_num = [[0 for _ in range(cols)] for _ in range(rows)]

        for idx, pos in enumerate(num_to_matrix):
            matrix_to_num[pos[0] - row_split][pos[1]] = idx + 1

        config_h_lines.append(generate_define('MATRIX_TO_NUM_R', str(matrix_to_num).replace('[', '{').replace(']', '}')))
        info_data['analog_matrix']['hardware']['matrix_to_num_right'] = matrix_to_num


    return info_data


#TODO: Check if I need to adjust this for the different families
#MARK: Port def
def get_port_def(json):
    if json['processor'] in BSRR_PROCESSORS:
        json['analog_matrix']['hardware']['use_bsrr'] = True

    if json['processor'] in CHIBIOS_PROCESSORS:
        json['analog_matrix']['port_def'] = "GPIO"
        return json

    if json['processor'] in LUFA_PROCESSORS + VUSB_PROCESSORS:
        json['analog_matrix']['port_def'] = "PORT"
        return json

    raise Exception("Unknown processor!")


#TODO: Check if want to allow different pins per half, remove if not
#MARK: ADC pins
def check_adc_pins(he_json, config_h_lines):
    hardware = he_json['hardware']

    if hardware.get('adc_pins', 0) == hardware.get('adc_pins_right', 1):
        config_h_lines.append(generate_define('EQUAL_ADC_PINS'))


# MARK: Mux pins
def check_mux_pins(json, config_h_lines):
    """Check if mux pins are continuous on one port, and set the defines
    """
    #TODO: Implement the optimization for RP2040
    # Currently, no register access optimizations are avaliable for the RP2040
    # if json['processor'] == 'RP2040':
    #     return

    he_json = json['analog_matrix']
    port_def = he_json['port_def']
    hardware = he_json['hardware']

    if json['processor'] == 'RP2040':
        prefix = 2
    else:
        prefix = 1

    if hardware.get('mux_pins', ['A0']) == hardware.get('mux_pins_right', ['A1']):
        config_h_lines.append(generate_define('EQUAL_MUX_PINS'))

    for postfix in ['', '_right']:
        if f'mux_pins{postfix}' in he_json['hardware']:
            mux_pins = he_json['hardware'][f'mux_pins{postfix}']
            port = mux_pins[0][:prefix]
            offset = int(mux_pins[0][prefix:])
            init_offset = offset

            for pin in mux_pins:
                if pin[:prefix] == port and int(pin[prefix:]) == offset:
                    offset += 1
                else:
                    return

            config_h_lines.append(generate_define(f'MUX_PINS{postfix.upper()}_CONTINUOUS'))
            config_h_lines.append(generate_define(f'MUX_PIN{postfix.upper()}_OFFSET', f'{init_offset}'))
            config_h_lines.append(generate_define(f'CONTINUOUS_MUX_PORT{postfix.upper()}', f'{port_def}{port}'))


# MARK: Power pins
def check_power_pins(he_json, config_h_lines):
    """Check if mux pins are continuous on one port, and set the defines
    """
    port_def = he_json['port_def']
    hardware = he_json['hardware']

    if hardware.get('power_pins', 0) == hardware.get('power_pins_right', 1):
        config_h_lines.append(generate_define('EQUAL_POWER_PINS'))

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


#MARK: Debug matrix
def debug_matrix_position(info_data, config_h_lines):
    matrix_to_num = info_data['analog_matrix']['hardware']['matrix_to_num']
    matrix_to_num_r = info_data['analog_matrix']['hardware'].get('matrix_to_num_right', '')
    num_to_mux = info_data['analog_matrix']['hardware']['num_to_mux']
    num_to_mux_r = info_data['analog_matrix']['hardware'].get('num_to_mux_right', '')
    rows = info_data['matrix_size']['rows']
    row_split = info_data['analog_matrix']['hardware'].get('row_split', rows)
    scan_mux = []
    scan_mux_r = []
    scan_pos = info_data['analog_matrix']['config']['debug_matrix_position']
    if scan_pos[0] >= row_split:
        scan_mux_r = num_to_mux_r[matrix_to_num_r[scan_pos[0] - row_split][scan_pos[1]] - 1]
    else:
        scan_mux = num_to_mux[matrix_to_num[scan_pos[0]][scan_pos[1]] - 1]

    if scan_mux != []:
        config_h_lines.append(generate_define('DEBUG_MUX_POSITION', str(scan_mux).replace('[', '{').replace(']', '}')))
    if scan_mux_r != []:
        config_h_lines.append(generate_define('DEBUG_MUX_POSITION_R', str(scan_mux_r).replace('[', '{').replace(']', '}')))



#MARK: Init keys
def transform_init_keys(info_data, config_h_lines):
    matrix_to_num = info_data['analog_matrix']['hardware']['matrix_to_num']
    matrix_to_num_r = info_data['analog_matrix']['hardware'].get('matrix_to_num_right', '')
    num_to_mux = info_data['analog_matrix']['hardware']['num_to_mux']
    num_to_mux_r = info_data['analog_matrix']['hardware'].get('num_to_mux_right', '')
    row_split = info_data['analog_matrix']['hardware'].get('row_split', 0)
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
                key_mux = num_to_mux_r[matrix_to_num_r[key_pos[0] - row_split][key_pos[1]] - 1]
                init_keys_r.append(key_mux)
                init_key_num_r += 1
                init_functions_r.append(INIT_FUNCTIONS[key])
            else:
                key_mux = num_to_mux[matrix_to_num[key_pos[0]][key_pos[1]] - 1]
                init_keys.append(key_mux)
                init_key_num += 1
                init_functions.append(INIT_FUNCTIONS[key])

    config_h_lines.append(generate_define('AM_INIT_KEY_NUM', init_key_num))
    if len(init_functions) > 0:
        config_h_lines.append(generate_define('AM_INIT_KEYS', str(init_keys).replace('[', '{').replace(']', '}')))
        config_h_lines.append(generate_define('AM_INIT_FUNCTIONS', f'{{ {", ".join(map(str, init_functions))} }}'))

    config_h_lines.append(generate_define('AM_INIT_KEY_NUM_R', init_key_num_r))
    if len(init_functions_r) > 0:
        config_h_lines.append(generate_define('AM_INIT_KEYS_R', str(init_keys_r).replace('[', '{').replace(']', '}')))
        config_h_lines.append(generate_define('AM_INIT_FUNCTIONS_R', f'{{ {", ".join(map(str, init_functions_r))} }}'))


#MARK: Profile config
def generate_profile_config(info_data, config_h_lines):
    """Extract the profile configuration"""
    he_profiles = info_data['analog_matrix']['profiles']
    switch_num = info_data['analog_matrix']['hardware']['total_switch_num']
    trigger_heights = []
    release_heights = []
    press_distances = []
    release_distances = []
    profile_config = []
    key_modes = []
    profile_num = 0

    while True:
        if f'profile_{profile_num}' in he_profiles:
            profile_data = he_profiles[f'profile_{profile_num}']

            trigger_height = profile_data.get('trigger_height', [0])
            if len(trigger_height) == 1:
                trigger_height = [trigger_height[0] for _ in range(switch_num)]
            trigger_heights.append(trigger_height)

            release_height = profile_data.get('release_height', [0])
            if release_height == [0]:
                release_height = trigger_height
            elif len(release_height) == 1:
                release_height = [release_height[0] for _ in range(switch_num)]
            release_heights.append(release_height)

            rt_press_distance = profile_data.get('rt_press_distance', [0])
            if len(rt_press_distance) == 1:
                rt_press_distance = [rt_press_distance[0] for _ in range(switch_num)]

            rt_release_distance = profile_data.get('rt_release_distance', [0])
            if rt_release_distance == [0]:
                rt_release_distance = rt_press_distance
            elif len(rt_release_distance) == 1:
                rt_release_distance = [rt_release_distance[0] for _ in range(switch_num)]

            press_distances.append(rt_press_distance)
            release_distances.append(rt_release_distance)

            # Rest of the profile config
            profile_config.append([])
            if 'layers' in profile_data:
                profile_layers = 0
                for num in profile_data['layers']:
                    profile_layers |= (1 << num)
                profile_config[profile_num].append(profile_layers)
            else:
                profile_config[profile_num].append(0)

            # Priority profile data
            #TODO: Update this stuff for priority_muxes if needed
            if 'priority_keys' in info_data['analog_matrix']['config'] or info_data['analog_matrix']['config'].get('slave_low_priority', False):
                profile_config[profile_num].append(1 if profile_data.get('priority_profile', False) else 0)

            #MARK: Key modes
            if 'rapid_trigger_type' in profile_data:
                # If a rapid_trigger_type is defined for the profile, set the mode to it for
                # all RT enabled keys
                if 'key_modes' in profile_data:
                    rt_int = RT_TYPES[profile_data['rapid_trigger_type'].upper()]
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
                    key_modes.append([RT_TYPES[profile_data['rapid_trigger_type'].upper()] for _ in range(switch_num)])
            else:
                #TODO: Test this part
                key_mode = profile_data.get('key_modes', [-1])
                if len(key_mode) == 1:
                    key_mode = [key_mode[0] for _ in range(switch_num)]
                key_modes.append(key_mode)
                # key_mode = profile_data.get('key_modes', [0])
                # if len(key_mode) == 1:
                #     key_mode = [key_mode[0] for _ in range(switch_num)]
                # key_modes.append(key_mode)

            profile_num += 1

        else: # Profile isn't in the json
            break

    # Set the mode definitions
    used_modes = []
    for profile_modes in key_modes:
        for mode in profile_modes:
            if RT_NAMES[mode] not in used_modes:
                used_modes.append(RT_NAMES[mode])

    # Check default profile
    if 'profile_switch_mode' in he_profiles:
        config_h_lines.append(generate_define('PROFILE_SWITCH_MODE', f'{he_profiles['profile_switch_mode'].upper()}_PROFILE'))
    else:
        config_h_lines.append(generate_define('PROFILE_SWITCH_MODE', 'LAST_PROFILE'))

    default_profile = he_profiles.get('default_profile', 0)
    if default_profile >= profile_num:
        print(f"Default profile {default_profile} is outside of the possible range of profiles, since only {profile_num} are defined. Defaulting to profile 0. Keep in mind that profiles are 0-indexed.")
        default_profile = 0

    #TODO: This stuff can definitely be simplified
    # Split up the height configs if necessary
    if 'split' in info_data and info_data['split'].get('enabled', False):
        g_to_l_index = info_data['analog_matrix']['hardware']['global_to_local_index']
        profile_num = len(trigger_heights)
        split_trigger_heights = [[[] for _ in range(profile_num)], [[] for _ in range(profile_num)]]
        split_release_heights = [[[] for _ in range(profile_num)], [[] for _ in range(profile_num)]]
        split_press_distances = [[[] for _ in range(profile_num)], [[] for _ in range(profile_num)]]
        split_release_distances = [[[] for _ in range(profile_num)], [[] for _ in range(profile_num)]]
        split_key_modes = [[[] for _ in range(profile_num)], [[] for _ in range(profile_num)]]
        for idx, _ in enumerate(trigger_heights[0]):
            for profile in range(profile_num):
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

        # Only define these if they're actually configured in the json
        if [idx for side in split_trigger_heights for profile in side for idx in profile].count(0) != switch_num * profile_num:
            config_h_lines.append(generate_define('TRIGGER_HEIGHT', f'{str(split_trigger_heights[0]).replace('[', '{').replace(']', '}')}'))
            config_h_lines.append(generate_define('TRIGGER_HEIGHT_R', f'{str(split_trigger_heights[1]).replace('[', '{').replace(']', '}')}'))
            config_h_lines.append(generate_define('RELEASE_HEIGHT', f'{str(split_release_heights[0]).replace('[', '{').replace(']', '}')}'))
            config_h_lines.append(generate_define('RELEASE_HEIGHT_R', f'{str(split_release_heights[1]).replace('[', '{').replace(']', '}')}'))

        if [idx for side in split_press_distances for profile in side for idx in profile].count(0) != switch_num * profile_num:
            config_h_lines.append(generate_define('RT_PRESS_DISTANCE', f'{str(split_press_distances[0]).replace('[', '{').replace(']', '}')}'))
            config_h_lines.append(generate_define('RT_PRESS_DISTANCE_R', f'{str(split_press_distances[1]).replace('[', '{').replace(']', '}')}'))
            config_h_lines.append(generate_define('RT_RELEASE_DISTANCE', f'{str(split_release_distances[0]).replace('[', '{').replace(']', '}')}'))
            config_h_lines.append(generate_define('RT_RELEASE_DISTANCE_R', f'{str(split_release_distances[1]).replace('[', '{').replace(']', '}')}'))

        if [idx for side in split_key_modes for profile in side for idx in profile].count(-1) != switch_num * profile_num:
            config_h_lines.append(generate_define('KEY_MODES', f'{str(split_key_modes[0]).replace('[', '{').replace(']', '}')}'))
            config_h_lines.append(generate_define('KEY_MODES_R', f'{str(split_key_modes[1]).replace('[', '{').replace(']', '}')}'))

    # Not a split keyboard
    else:
        if [idx for profile in trigger_heights for idx in profile].count(0) != switch_num * profile_num:
            config_h_lines.append(generate_define('TRIGGER_HEIGHT', f'{str(trigger_heights).replace('[', '{').replace(']', '}')}'))
            config_h_lines.append(generate_define('RELEASE_HEIGHT', f'{str(release_heights).replace('[', '{').replace(']', '}')}'))

        if [idx for profile in trigger_heights for idx in profile].count(0) != switch_num * profile_num:
            config_h_lines.append(generate_define('RT_PRESS_DISTANCE', f'{str(press_distances).replace('[', '{').replace(']', '}')}'))
            config_h_lines.append(generate_define('RT_RELEASE_DISTANCE', f'{str(release_distances).replace('[', '{').replace(']', '}')}'))

        if [idx for profile in trigger_heights for idx in profile].count(-1) != switch_num * profile_num:
            config_h_lines.append(generate_define('KEY_MODES', f'{str(key_modes).replace('[', '{').replace(']', '}')}'))

    # Add the profile config to info_config.h
    config_h_lines.append(generate_define('AM_PROFILE_NUM', profile_num))
    config_h_lines.append(generate_define('AM_DEFAULT_PROFILE', default_profile))
    config_h_lines.append(generate_define('AM_PROFILE_CONFIG', f'{str(profile_config).replace('[', '{').replace(']', '}')}'))

    use_height = False
    use_distance = False
    for mode in set(used_modes):
        config_h_lines.append(generate_define(f'USE_{mode}'))
        if mode in ['NONE', 'RAPID_TRIGGER', 'CONTINUOUS_RAPID_TRIGGER']:
            use_height = True
        if mode in ['RAPID_TRIGGER', 'CONTINUOUS_RAPID_TRIGGER', 'CONSTANT_RAPID_TRIGGER']:
            use_distance = True

    if use_height == True:
        config_h_lines.append(generate_define('USE_TRIGGER_HEIGHT'))
    if use_distance == True:
        config_h_lines.append(generate_define('USE_RT_DISTANCE'))

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


def valid_profile_name(profile):
    if profile[:8] == 'profile_' and profile[8:].isnumeric():
        return True
    else:
        return False

#MARK: Validation
def validate_analog_matrix_config(info_data):
    """Validate the analog matrix configuration."""
    he_json = info_data['analog_matrix']
    he_hardware = he_json['hardware']
    switch_num = info_data['analog_matrix']['hardware']['total_switch_num']
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

    if 'profiles' in he_json:
        profile = 0
        while True:
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
                        print(f"rt_press_distance needs to be set if rapid trigger is used in profile_{profile}!")
            else:
                if 'trigger_height' not in profile_data:
                    valid = False
                    print(f"Trigger height needs to be set if constant rapid trigger isn't used in profile_{profile}!")

            for config in ['trigger_height', 'release_height', 'rt_press_distance', 'rt_release_distance', 'key_modes']:
                num = len(profile_data.get(config, []))
                if num > 0 and num != 1 and num != switch_num:
                    valid = False
                    print(f"The amount of {config} values in profile_{profile} needs to be either 1 or equal to the amount of switches!")

            profile += 1

    else: # Profiles not defined
        valid = False
        print("At least one profile needs to be defined!")

    return valid


#MARK: Height validation
# Checks if the heights are defined correctly
def validate_height_config(he_json, invert_adc, from_bottom):
    he_profiles = he_json['profiles']
    valid = True

    if (invert_adc and from_bottom) or (not invert_adc and not from_bottom):
        for profile in range(len(he_profiles)):
            if f'profile_{profile}' in he_profiles:
                profile_data = he_profiles[f'profile_{profile}']
                if 'trigger_height' in profile_data and 'release_height' in profile_data:
                    for idx, i in enumerate(profile_data['trigger_height']):
                        if i < profile_data['release_height'][idx]:
                            valid = False
                            print(f'\nThe trigger height for key {idx} in profile {profile} is higher than the release height when it needs to be lower or equal.\n')

    else:
        for profile in range(len(he_profiles)):
            if f'profile_{profile}' in he_profiles:
                profile_data = he_profiles[f'profile_{profile}']
                if 'trigger_height' in profile_data and 'release_height' in profile_data:
                    for idx, i in enumerate(profile_data['trigger_height']):
                        if i > profile_data['release_height'][idx]:
                            valid = False
                            print(f'\nThe trigger height for key {idx} in profile {profile} is lower than the release height when it needs to be higher or equal.\n')

    return valid


def get_features(info_data):
    for feature, enabled in info_data['features'].items():
        features.update(((feature.lower(), enabled),))

    for name, _ in info_data['analog_matrix'].items():
        objects.append(name.lower())


#MARK: General config
def generate_analog_matrix_config(info_data, config_h_lines):
    """Generate the config.h lines for analog matrix keyboards."""
    # Get all defined features and their status
    #TODO: Check if the features from rules_mk have been combined at this point
    if 'features' in info_data:
        get_features(info_data)

    validate_analog_matrix_config(info_data)

    if 'split' in info_data and info_data['split'].get('enabled', False):
        info_data = check_right_side_pins(info_data, config_h_lines)

    he_json = info_data['analog_matrix']
    he_hardware = info_data['analog_matrix']['hardware']

    #Hardware stuff
    # if info_data.get('debounce', 0) > 0:
    #     config_h_lines.append(generate_define('USE_DEBOUNCE'))

    adc_pin_num = len(he_hardware.get('adc_pins', ''))
    config_h_lines.append(generate_define('ADC_PIN_NUM', adc_pin_num))

    info_data = get_port_def(info_data)
    if info_data['analog_matrix']['hardware'].get('use_bsrr', False):
        config_h_lines.append(generate_define('USE_BSRR'))

    check_adc_pins(info_data['analog_matrix'], config_h_lines)

    if 'mux_pins' in he_hardware:
        mux_pin_num = len(he_hardware.get('mux_pins', ''))
        config_h_lines.append(generate_define('MUX_PIN_NUM', mux_pin_num))
        check_mux_pins(info_data, config_h_lines)
    else:
        #TODO: Is this necessary?
        config_h_lines.append(generate_define('MUX_PIN_NUM', 0))

    if 'power_pins' in he_hardware:
        power_pin_num = len(he_hardware['power_pins'])
        config_h_lines.append(generate_define('POWER_PIN_NUM', power_pin_num))
        config_h_lines.append(generate_define('POWER_BEFORE_SCAN'))
        check_power_pins(info_data['analog_matrix'], config_h_lines)

    # This is the total switch num to be used with the trigger_heights
    switch_num = he_hardware.get('total_switch_num')

    # Get the reverse transform arrays
    info_data = get_matrix_to_mux(info_data, config_h_lines)

    # Transform init key matrix positions to mux combinations
    transform_init_keys(info_data, config_h_lines)

    # Transform priority key matrix positions to key indices
    get_priority_keys(info_data, config_h_lines)

    split_layer_sync = False
    if 'joystick' in objects and features.get('joystick', True):
        split_layer_sync = True
        generate_joystick_config(info_data, config_h_lines)

    #TODO: Implement this
    if 'midi' in objects and features.get('midi', True):
        split_layer_sync = True
        #generate_midi_config(info_data, config_h_lines)

    if 'debug_matrix_position' in he_json['config']:
        debug_matrix_position(info_data, config_h_lines)

    # Only get the heights from the config if no profiles are defined
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

    # Validate trigger_heights separately after setting, in case of a len 1 define
    invert_adc = he_hardware.get('invert_adc', False)
    from_bottom = he_json['config'].get('distance_from_bottom', False)

    if 'split' in info_data and info_data['split'].get('enabled', False) and split_layer_sync == True:
        config_h_lines.append(generate_define('SPLIT_LAYER_SYNC'))

    validate_height_config(he_json, invert_adc, from_bottom)

    return info_data


#MARK: Right side pins
def check_right_side_pins(info_data, config_h_lines):
    he_hardware = info_data['analog_matrix']['hardware']

    for pins in ['adc_pins', 'mux_pins', 'power_pins']:
        if pins in he_hardware and f'{pins}_right' not in he_hardware:
            pins_r = he_hardware[pins]
            info_data['analog_matrix']['hardware'][f'{pins}_right'] = pins_r
            config_h_lines.append(generate_define(f'{pins.upper()}_R', f'{{ {", ".join(map(str, pins_r))} }}'))

    return info_data


#MARK: Joystick config
def generate_joystick_config(info_data, config_h_lines):
    AXIS_INDICES = {
        "x": 0, "y": 1, "trigger": 2, "z": 2, "rx": 3, "ry": 4, "rz": 5
    }
    RESOLUTION_NAMES = {
        "difference": 0, "lowest": 1, "positive_dominant": 2, "negative_dominant": 3, "cancel": 4
    }
    am_joystick = info_data['analog_matrix']['joystick']
    layout = am_joystick.get('layout', 'XBOX')
    config_h_lines.append(generate_define(f'{layout.upper()}_LAYOUT'))
    axis_count = am_joystick.get('axes', 6)
    config_h_lines.append(generate_define('JOYSTICK_AXIS_COUNT', axis_count))
    #TODO: Update this with the standard amount of joystick buttons
    config_h_lines.append(generate_define('JOYSTICK_BUTTON_COUNT', am_joystick.get('buttons', 16)))
    if 'top_deadzone' in am_joystick:
        config_h_lines.append(generate_define('JS_TOP_DEADZONE', am_joystick.get('top_deadzone')))
        if 'bottom_deadzone' in am_joystick:
            config_h_lines.append(generate_define('JS_BOTTOM_DEADZONE', am_joystick.get('bottom_deadzone')))
    else:
        config_h_lines.append(generate_define('JS_DEADZONE', am_joystick.get('deadzone', 15)))

    method_config = am_joystick.get('resolution_methods', {})
    # Use the resolution_method option as the default for any axis that isn't specified
    resolutions = [[RESOLUTION_NAMES[am_joystick.get('resolution_method', 'difference').lower()]] for _ in range(axis_count)]
    if method_config != {}:
        for axis, method in method_config.items():
            resolutions[AXIS_INDICES[axis]][0] = RESOLUTION_NAMES[method]

    config_h_lines.append(generate_define('AM_JOYSTICK_AXIS_CONFIG', f'{str(resolutions).replace('[', '{').replace(']', '}')}'))


#MARK: Priority keys
# def get_priority_keys(info_data, config_h_lines):
#     if 'priority_keys' not in info_data['analog_matrix']['config']:
#         return info_data

#     priority_keys = info_data['analog_matrix']['config']['priority_keys']
#     matrix_to_num = info_data['analog_matrix']['hardware']['matrix_to_num']
#     num_to_mux = info_data['analog_matrix']['hardware']['num_to_mux']
#     rows = info_data['matrix_size']['rows']
#     row_split = info_data['analog_matrix']['hardware'].get('row_split', rows)
#     priority_muxes = []
#     priority_muxes_r = []
#     if 'split' in info_data and info_data['split'].get('enabled', False):
#         matrix_to_num_r = info_data['analog_matrix']['hardware']['matrix_to_num_right']
#         num_to_mux_r = info_data['analog_matrix']['hardware']['num_to_mux_right']

#     # Convert all matrix positions in the array into mux combos
#     for key in priority_keys:
#         if key[0] < row_split:
#             num = matrix_to_num[key[0]][key[1]] - 1
#             #TODO: Either flip mux channel and adc channel here or during assignment
#             mux = num_to_mux[num]
#             mux.append(num)
#             # Result is a list of mux channel, adc channel, matrix index
#             priority_muxes.append(mux)
#         else:
#             num = matrix_to_num_r[key[0] - row_split][key[1]] - 1
#             mux = num_to_mux_r[num]
#             mux.append(num)
#             priority_muxes_r.append(mux)

#     # Sort the array by mux channels in ascending order, so that it will have to be set less often
#     priority_muxes.sort()
#     priority_muxes_r.sort()

#     config_h_lines.append(generate_define("PRIORITY_MUXES", f'{str(priority_muxes).replace('[', '{').replace(']', '}')}'))
#     config_h_lines.append(generate_define("PRIORITY_MUX_NUM", len(priority_muxes)))

#     if priority_muxes_r != []:
#         config_h_lines.append(generate_define("PRIORITY_MUXES_R", f'{str(priority_muxes_r).replace('[', '{').replace(']', '}')}'))
#         config_h_lines.append(generate_define("PRIORITY_MUX_NUM_R", len(priority_muxes_r)))


#MARK: Priority idx
def get_priority_keys(info_data, config_h_lines):
    if 'priority_keys' not in info_data['analog_matrix']['config']:
        return info_data

    switch_num = info_data['analog_matrix']['hardware']['switch_num']
    switch_num_r = info_data['analog_matrix']['hardware'].get('switch_num_r', 0)
    priority_keys = info_data['analog_matrix']['config']['priority_keys']
    matrix_to_num = info_data['analog_matrix']['hardware']['matrix_to_num']
    rows = info_data['matrix_size']['rows']
    row_split = info_data['analog_matrix']['hardware'].get('row_split', rows)
    priority_idx = [0 for _ in range(switch_num)]
    priority_idx_r = [0 for _ in range(switch_num_r)]
    if 'split' in info_data and info_data['split'].get('enabled', False):
        matrix_to_num_r = info_data['analog_matrix']['hardware']['matrix_to_num_right']

    for key in priority_keys:
        if key[0] < row_split:
            priority_idx[matrix_to_num[key[0]][key[1]]] = 1
        else:
            priority_idx_r[matrix_to_num_r[key[0] - row_split][key[1]]] = 1

    config_h_lines.append(generate_define("PRIORITY_INDICES", f'{str(priority_idx).replace('[', '{').replace(']', '}')}'))
    config_h_lines.append(generate_define("PRIORITY_INDEX_NUM", priority_idx.count(1)))

    if row_split != rows:
        config_h_lines.append(generate_define("PRIORITY_INDICES_R", f'{str(priority_idx_r).replace('[', '{').replace(']', '}')}'))
        config_h_lines.append(generate_define("PRIORITY_INDEX_NUM_R", priority_idx_r.count(1)))
        # Assume the slave is the half with no priority keys
        if priority_idx.count(1) == 0 or priority_idx_r.count(1) == 0:
            config_h_lines.append(generate_define("SLAVE_LOW_PRIORITY"))


#MARK: Height layout
# def make_height_macro(info_data, config_h_lines):


