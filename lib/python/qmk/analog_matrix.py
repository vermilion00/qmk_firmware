from qmk.constants import CHIBIOS_PROCESSORS, LUFA_PROCESSORS, VUSB_PROCESSORS
from milc import cli

#TODO: Expand this list, need to make sure they follow the same calling structure as well
#BSRR needs to have set priority, be 32 bits, with upper 16 bits being the reset bits
BSRR_PROCESSORS = 'RP2040', 'STM32F042', 'STM32F072', 'STM32F303', 'STM32F401', 'STM32F405', 'STM32F407', 'STM32F411', 'STM32F446', 'STM32G0B1', 'STM32G431', 'STM32G474', 'STM32H723', 'STM32H733', 'STM32L412', 'STM32L422', 'STM32L432', 'STM32L433', 'STM32L442', 'STM32L443', 'AT32F415'
# Above this value, the key modes for special keys start. If for some reason I need more modes, increase this
MODE_NUM = 9
NO_KEY = [0, "X", "none", "None", "NONE"]

INIT_KEYS = {
    'CALIBRATION_KEY': 'analog_matrix.config.calibration_keys',
    'BOOTMAGIC_KEY': 'analog_matrix.config.bootmagic_keys',
    'BOOTLOADER_KEY': 'analog_matrix.config.bootloader_keys',
    # Use normal bootmagic key for mechanical button reset
    # 'BOOTMAGIC_KEY': 'bootmagic.matrix',
    # 'BOOTMAGIC_KEY_RIGHT': 'split.bootmagic.matrix'
}

# _RIGHT keys automatically translate to their left variant
INIT_FUNCTIONS = {
    'BOOTLOADER_KEY': '_bootloader_jump',
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
split_keyboard = False
use_priority_mode = False

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
    global split_keyboard
    if 'analog_matrix' not in info_data:
        return info_data

    if 'split' in info_data and info_data['split'].get('enabled', False):
        split_keyboard = True
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
    am_hardware = info_data['analog_matrix']['hardware']

    if 'adc_pins' in am_hardware:
        adc_pins = len(am_hardware['adc_pins'])
    else:
        cli.log.error("No ADC pins defined!")
        return info_data

    mux_channels = 1 << len(am_hardware.get('mux_pins', ''))
    # If power pins are used instead of multiplexers, use their amount instead
    if mux_channels == 1:
        mux_channels = 1 << len(am_hardware.get('power_pins', ''))
    # Direct pins are functionally converted to a matrix with 1 row
    row_pins = max(len(am_hardware.get('row_pins', [])), 1)
    col_pins = len(am_hardware.get('col_pins', [])) + len(am_hardware.get('direct_pins', []))
    used_rc_positions = []
    matrix_size = [0, 0]
    highest_adc = 0
    highest_mux = 0

    # We only need one layout, hence the break at the end
    for layout_name, layout_data in info_data['layouts'].items():
        mux_to_num = [[255 for _ in range(adc_pins)] for _ in range(mux_channels)]
        num_to_matrix = [-1 for _ in range(len(layout_data['layout']))]
        rc_to_matrix = [[[255, 255] for _ in range(col_pins)] for _ in range(row_pins)]
        key_index = 0
        total_key_num = 0
        used_positions = []
        for key_data in layout_data['layout']:
            #TODO: Change the name from mux to a/m?
            if key_data['mux'][0] > highest_adc: highest_adc = key_data['mux'][0]
            if key_data['mux'][1] > highest_mux: highest_mux = key_data['mux'][1]

            if 'mux' in key_data:
                adc, mux = key_data['mux']
                mux_to_num[mux][adc] = key_index
                num_to_matrix[key_index] = key_data['matrix']
                matrix_size[0] = key_data['matrix'][0] if key_data['matrix'][0] > matrix_size[0] else matrix_size[0]
                matrix_size[1] = key_data['matrix'][1] if key_data['matrix'][1] > matrix_size[1] else matrix_size[1]
                key_index += 1
                total_key_num += 1
                if [adc, mux] not in used_positions:
                    used_positions.append([adc, mux])
                else:
                    cli.log.error(f"Mux combination {[adc, mux]} appears multiple times in the layout!")
            #TODO: Workshop the name a bit -> r/c, and change mux to a/m?
            elif 'rc' in key_data:
                data = key_data['rc']
                if type(data) == list:
                    row, col = data
                else:
                    row, col = [0, data]
                if row > row_pins:
                    cli.log.error(f"R/C row index {row} for key {total_key_num} is too high!")
                if col > col_pins:
                    cli.log.error(f"R/C col index {col} for key {total_key_num} is too high!")
                if [row, col] not in used_rc_positions:
                    used_rc_positions.append([row, col])
                    rc_to_matrix[row][col] = key_data['matrix']
                else:
                    cli.log.error(f"R/C combination {[row, col]} appears multiple times in the layout!")
                matrix_size[0] = key_data['matrix'][0] if key_data['matrix'][0] > matrix_size[0] else matrix_size[0]
                matrix_size[1] = key_data['matrix'][1] if key_data['matrix'][1] > matrix_size[1] else matrix_size[1]

                total_key_num += 1

            else:
                cli.log.error(f"Missing info on key {key_index} in layout {layout_name}!")
                return info_data
        break

    # Trim unnecessary adc, mux and power pins
    info_data['analog_matrix']['hardware']['adc_pins'] = am_hardware['adc_pins'][:highest_adc+1]
    if 'mux_pins' in am_hardware:
        for n in range(1, 6):
            if mux_channels < (1 << n):
                info_data['analog_matrix']['hardware']['mux_pins'] = am_hardware['mux_pins'][:n+1]
                break
    if 'power_pins' in am_hardware:
        info_data['analog_matrix']['hardware']['power_pins'] = am_hardware['power_pins'][:highest_mux+1]

    # Trim the mux_to_nums
    mux_to_num = [row[:highest_adc+1] for row in mux_to_num[:highest_mux+1]]

    # Trim the num_to_matrixes
    num_to_matrix = [i for i in num_to_matrix if i != -1]

    info_data['analog_matrix']['hardware']['mux_to_num'] = mux_to_num
    info_data['analog_matrix']['hardware']['num_to_matrix'] = num_to_matrix
    info_data['analog_matrix']['hardware']['switch_num'] = len(num_to_matrix)
    info_data['analog_matrix']['hardware']['switch_num_right'] = 0
    info_data['analog_matrix']['hardware']['switch_num_l'] = len(num_to_matrix)
    info_data['analog_matrix']['hardware']['total_switch_num'] = len(num_to_matrix)
    info_data['analog_matrix']['hardware']['mux_channels'] = len(mux_to_num)
    info_data['matrix_size'] = {}
    info_data['matrix_size']['rows'] = matrix_size[0] + 1
    info_data['matrix_size']['cols'] = matrix_size[1] + 1
    info_data['matrix_size']['analog_rows'] = max([i[0] for i in num_to_matrix]) + 1
    info_data['matrix_size']['analog_cols'] = max([i[1] for i in num_to_matrix]) + 1
    if used_rc_positions != []:
        info_data['analog_matrix']['hardware']['rc_to_matrix'] = rc_to_matrix
        info_data['analog_matrix']['hardware']['rc_switch_num'] = len([key for row in rc_to_matrix for key in row])

    return info_data


#TODO: Add check for duplicate matrix positions
#MARK: Layout split
def _transform_layout_split(info_data):
    am_hardware = info_data['analog_matrix']['hardware']

    if 'adc_pins' in am_hardware:
        adc_pin_num = len(am_hardware['adc_pins'])
    else:
        cli.log.error("No ADC pins defined!")
        return info_data
    mux_channels = 1 << len(am_hardware.get('mux_pins', ''))
    # If power pins are used instead of multiplexers, use their amount instead
    if mux_channels == 1:
        mux_channels = 1 << len(am_hardware.get('power_pins', ''))
    row_split = am_hardware['row_split']
    #TODO: This needs to be updated for possibly separate pin amounts per half
    row_pins = len(am_hardware.get('row_pins', []))
    col_pins = len(am_hardware.get('col_pins', []))
    used_rc_positions = []
    matrix_size = [0, 0]
    highest_adc = 0
    highest_mux = 0

    for layout_name, layout_data in info_data['layouts'].items():
        mux_to_num_l = [[255 for _ in range(adc_pin_num)] for _ in range(mux_channels)]
        num_to_matrix_l = [-1 for _ in range(len(layout_data['layout']))]
        mux_to_num_r = [[255 for _ in range(adc_pin_num)] for _ in range(mux_channels)]
        num_to_matrix_r = [-1 for _ in range(len(layout_data['layout']))]
        rc_to_matrix_l = [[[255, 255] for _ in range(col_pins)] for _ in range(row_pins)]
        rc_to_matrix_r = [[[255, 255] for _ in range(col_pins)] for _ in range(row_pins)]
        key_index_l = 0
        key_index_r = 0
        total_key_num = 0
        used_positions_l = []
        used_positions_r = []
        for key_data in layout_data['layout']:
            if 'mux' in key_data:
                if key_data['mux'][0] > highest_adc: highest_adc = key_data['mux'][0]
                if key_data['mux'][1] > highest_mux: highest_mux = key_data['mux'][1]

                if key_data['matrix'][0] < row_split:
                    adc, mux = key_data['mux']
                    mux_to_num_l[mux][adc] = key_index_l
                    num_to_matrix_l[key_index_l] = key_data['matrix']
                    key_index_l += 1
                    matrix_size[1] = key_data['matrix'][1] if key_data['matrix'][1] > matrix_size[1] else matrix_size[1]
                    if [adc, mux] not in used_positions_l:
                        used_positions_l.append([adc, mux])
                    else: # Mux combo already used
                        cli.log.error(f"Mux combination {[adc, mux]} appears multiple times in the left half of the layout!")
                else: # Right hand side
                    adc, mux = key_data['mux']
                    mux_to_num_r[mux][adc] = key_index_r
                    num_to_matrix_r[key_index_r] = key_data['matrix']
                    key_index_r += 1
                    matrix_size[0] = key_data['matrix'][0] if key_data['matrix'][0] > matrix_size[0] else matrix_size[0]
                    matrix_size[1] = key_data['matrix'][1] if key_data['matrix'][1] > matrix_size[1] else matrix_size[1]
                    if [adc, mux] not in used_positions_r:
                        used_positions_r.append([adc, mux])
                    else: # Mux combo already used
                        cli.log.error(f"Mux combination {[adc, mux]} appears multiple times in the right half of the layout!")
                total_key_num += 1

            elif 'rc' in key_data:
                if key_data['matrix'][0] < row_split:
                    data = key_data['rc']
                    if type(data) == "<class 'list'>":
                        row, col = data
                    else:
                        row, col = [0, data]
                    if row > row_pins:
                        cli.log.error(f"Row index {row} for key {total_key_num} is too high!")
                    if col > col_pins:
                        cli.log.error(f"Col index {col} for key {total_key_num} is too high!")
                    if [row, col] not in used_rc_positions:
                        used_rc_positions.append([row, col])
                        rc_to_matrix_l[row][col] = key_data['matrix']
                    else:
                        cli.log.error(f"Matrix combination {[row, col]} appears multiple times in the layout!")
                    matrix_size[1] = key_data['matrix'][1] if key_data['matrix'][1] > matrix_size[1] else matrix_size[1]
                else:
                    row, col = key_data['rc']
                    if row > row_pins:
                        cli.log.error(f"Row index {row} for key {total_key_num} is too high!")
                    if col > col_pins:
                        cli.log.error(f"Col index {col} for key {total_key_num} is too high!")
                    if [row, col] not in used_rc_positions:
                        used_rc_positions.append([row, col])
                        rc_to_matrix_r[row][col] = key_data['matrix']
                    else:
                        cli.log.error(f"Matrix combination {[row, col]} appears multiple times in the layout!")
                    matrix_size[0] = key_data['matrix'][0] if key_data['matrix'][0] > matrix_size[0] else matrix_size[0]
                    matrix_size[1] = key_data['matrix'][1] if key_data['matrix'][1] > matrix_size[1] else matrix_size[1]

                total_key_num += 1

            else: # No mux definition in key data
                cli.log.error(f"Missing info on key {key_index_l + key_index_r} in layout {layout_name}!")
                return info_data
        # We only need one layout
        break

    # Trim unnecessary adc, mux and power pins
    for postfix in ['', '_right']:
        if f'adc_pins{postfix}' in am_hardware:
            info_data['analog_matrix']['hardware'][f'adc_pins{postfix}'] = am_hardware[f'adc_pins{postfix}'][:highest_adc+1]
        if f'mux_pins{postfix}' in am_hardware:
            for n in range(1, 6):
                if mux_channels < (1 << n):
                    info_data['analog_matrix']['hardware'][f'mux_pins{postfix}'] = am_hardware[f'mux_pins{postfix}'][:n+1]
                    break
        if f'power_pins{postfix}' in am_hardware:
            info_data['analog_matrix']['hardware'][f'power_pins{postfix}'] = am_hardware[f'power_pins{postfix}'][:highest_mux+1]

    # Trim mux_to_nums
    mux_to_num_l = [row[:highest_adc+1] for row in mux_to_num_l[:highest_mux+1]]
    mux_to_num_r = [row[:highest_adc+1] for row in mux_to_num_r[:highest_mux]]

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
    info_data['matrix_size'] = {}
    info_data['matrix_size']['rows'] = matrix_size[0] + 1
    info_data['matrix_size']['cols'] = matrix_size[1] + 1
    info_data['matrix_size']['analog_rows'] = max([i[0] for i in num_to_matrix_r]) + 1
    info_data['matrix_size']['analog_cols'] = max(max([i[1] for i in num_to_matrix_l]) + 1, max([i[1] for i in num_to_matrix_r]) + 1)
    if used_rc_positions != []:
        info_data['analog_matrix']['hardware']['rc_to_matrix'] = rc_to_matrix_l
        info_data['analog_matrix']['hardware']['rc_to_matrix_right'] = rc_to_matrix_r
        info_data['analog_matrix']['hardware']['rc_switch_num'] = len([key for row in rc_to_matrix_l for key in row])
        info_data['analog_matrix']['hardware']['rc_switch_num_r'] = len([key for row in rc_to_matrix_r for key in row])

    return info_data


#MARK: Get split config
# Generates the row split index and the global to local transformations
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
# Generates a reverse matrix transformation array
def get_matrix_to_mux(info_data, config_h_lines):
    hardware = info_data['analog_matrix']['hardware']
    cols = info_data['matrix_size']['cols']
    rows = info_data['matrix_size']['rows'] // 2 if split_keyboard else info_data['matrix_size']['rows']

    for postfix in ['', '_right'] if split_keyboard else ['']:
        fix = '_R' if postfix == '_right' else ''
        row_split = rows if postfix == '_right' else 0
        mux_to_num = hardware[f'mux_to_num{postfix}']
        num_to_matrix = hardware[f'num_to_matrix{postfix}']
        switch_num = hardware[f'switch_num{postfix}']
        num_to_mux = [[] for _ in range(switch_num)]
        for row_idx, row in enumerate(mux_to_num):
            for col_idx, idx in enumerate(row):
                if idx < 255:
                    num_to_mux[idx] = [col_idx, row_idx]

        config_h_lines.append(generate_define(f'NUM_TO_MUX{fix}', str(num_to_mux).replace('[', '{').replace(']', '}')))
        info_data['analog_matrix']['hardware'][f'num_to_mux{postfix}'] = num_to_mux

        matrix_to_num = [[255 for _ in range(cols)] for _ in range(rows)]

        for idx, pos in enumerate(num_to_matrix):
            matrix_to_num[pos[0] - row_split][pos[1]] = idx

        config_h_lines.append(generate_define(f'MATRIX_TO_NUM{fix}', str(matrix_to_num).replace('[', '{').replace(']', '}')))
        info_data['analog_matrix']['hardware'][f'matrix_to_num{postfix}'] = matrix_to_num

    return info_data


#MARK: Matrix_to_rc
def get_matrix_to_rc(info_data, config_h_lines):
    hardware = info_data['analog_matrix']['hardware']
    used_rc_pos = []
    cols = info_data['matrix_size']['cols']
    if split_keyboard:
        rows = info_data['matrix_size']['rows'] // 2
    else:
        rows = info_data['matrix_size']['rows']
    for postfix in ['', '_right'] if split_keyboard else ['']:
        fix = '_R' if postfix == '_right' else ''
        row_split = rows if postfix == '_right' else 0
        rc_to_matrix = hardware.get(f'rc_to_matrix{postfix}', [])
        if rc_to_matrix == []: return info_data
        matrix_to_rc_num = [[255 for _ in range(cols)] for _ in range(rows)]
        index = 0
        num_to_rc = []
        for row_idx, row in enumerate(rc_to_matrix):
            for pos_idx, pos in enumerate(row):
                matrix_to_rc_num[pos[0] - row_split][pos[1]] = index
                index += 1
                used_rc_pos.append(pos)
                num_to_rc.append([row_idx, pos_idx])
        info_data['analog_matrix']['hardware'][f'matrix_to_rc_num{postfix}'] = matrix_to_rc_num
        info_data['analog_matrix']['hardware'][f'num_to_rc{postfix}'] = num_to_rc
        config_h_lines.append(generate_define(f'NUM_TO_RC{fix}', str(num_to_rc).replace('[', '{').replace(']', '}')))
    info_data['analog_matrix']['hardware']['used_rc_pos'] = used_rc_pos

    return info_data


#TODO: Check if I need to adjust this for the different families
#MARK: Port def
def get_port_def(json, config_h_lines):
    if json['processor'] in BSRR_PROCESSORS:
        json['analog_matrix']['hardware']['use_bsrr'] = True
        config_h_lines.append(generate_define('USE_BSRR'))

    if json['processor'] in CHIBIOS_PROCESSORS:
        json['analog_matrix']['port_def'] = "GPIO"
        return json

    if json['processor'] in LUFA_PROCESSORS + VUSB_PROCESSORS:
        json['analog_matrix']['port_def'] = "PORT"
        return json

    raise Exception("Unknown processor!")


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
        scan_mux_r = num_to_mux_r[matrix_to_num_r[scan_pos[0] - row_split][scan_pos[1]]]
    else:
        scan_mux = num_to_mux[matrix_to_num[scan_pos[0]][scan_pos[1]]]

    if scan_mux != []:
        config_h_lines.append(generate_define('DEBUG_MUX_POSITION', str(scan_mux).replace('[', '{').replace(']', '}')))
    if scan_mux_r != []:
        config_h_lines.append(generate_define('DEBUG_MUX_POSITION_R', str(scan_mux_r).replace('[', '{').replace(']', '}')))



#MARK: Init keys
def transform_init_keys(info_data, config_h_lines):
    global split_keyboard
    use_init_keys = False
    hardware = info_data['analog_matrix']['hardware']
    rc_pos = hardware.get('used_rc_pos', [])
    rows = info_data['matrix_size']['rows'] // 2 if split_keyboard else info_data['matrix_size']['rows']
    max_rows = info_data['matrix_size']['rows']
    max_cols = info_data['matrix_size']['cols']
    # '_right' postfix is used twice to scan _right key implementation as well
    repeated_right = False

    for postfix in ['', '_right', '_right'] if split_keyboard else ['']:
        used_pos = []
        init_key_num = 0
        init_functions = []
        init_keys = []
        fix = '_R' if postfix == '_right' else ''
        matrix_to_num = hardware[f'matrix_to_num{postfix}']
        num_to_mux = hardware[f'num_to_mux{postfix}']
        matrix_to_rc_num = hardware.get(f'matrix_to_rc_num{postfix}', [])
        num_to_rc = hardware.get(f'num_to_rc{postfix}', [])
        rc_init_key_num = 0
        rc_init_keys = []
        rc_init_functions = []
        low_offset = 0
        offset = rows

        # Only apply these offsets the second time we scan right half keys
        if split_keyboard and postfix == '_right':
            offset = rows * 2
            low_offset = rows

        for key in INIT_KEYS:
            if postfix == '_right' and not repeated_right:
                path = INIT_KEYS[key] + postfix
            else:
                path = INIT_KEYS[key]
            if path in info_data:
                key_data = info_data[path]
                # If only a single position is given, put it in a list
                for key_pos in key_data if type(key_data[0]) == list else [key_data]:
                    if key_pos in used_pos:
                        cli.log.error(f"Key {key_pos} has multiple initialization functions! Skipping rest of {path}")
                        break
                    else:
                        used_pos.append(key_pos)

                    # Check if it's a valid position
                    if key_pos[0] >= max_rows or key_pos[1] >= max_cols:
                        print(f"Key position {key_pos} is outside of the matrix!")

                    # Skip keys assigned to the opposite half for now
                    if key_pos[0] < low_offset or key_pos[0] >= offset:
                        continue

                    # Check if the position corresponds to a mechanical key
                    if key_pos in rc_pos:
                        rc = num_to_rc[matrix_to_rc_num[key_pos[0] - low_offset][key_pos[1]]]
                        rc_init_keys.append(rc)
                        rc_init_key_num += 1
                        rc_init_functions.append(INIT_FUNCTIONS[key])

                    else:
                        #TODO: Will this work if a mechanical key adds another row that doesn't exist in matrix_to_num? Or does matrix_to_num take it into account
                        key_mux = num_to_mux[matrix_to_num[key_pos[0] - low_offset][key_pos[1]]]
                        init_keys.append(key_mux)
                        init_key_num += 1
                        init_functions.append(INIT_FUNCTIONS[key])

        if init_key_num > 0:
            config_h_lines.append(generate_define(f'AM_INIT_KEY_NUM{fix}', init_key_num))
            config_h_lines.append(generate_define(f'AM_INIT_KEYS{fix}', str(init_keys).replace('[', '{').replace(']', '}')))
            config_h_lines.append(generate_define(f'AM_INIT_FUNCTIONS{fix}', f'{{ {", ".join(map(str, init_functions))} }}'))
            use_init_keys = True

        if rc_init_key_num > 0:
            config_h_lines.append(generate_define(f'RC_INIT_KEY_NUM{fix}', rc_init_key_num))
            config_h_lines.append(generate_define(f'RC_INIT_KEYS{fix}', str(rc_init_keys).replace('[', '{').replace(']', '}')))
            config_h_lines.append(generate_define(f'RC_INIT_FUNCTIONS{fix}', f'{{ {", ".join(map(str, rc_init_functions))} }}'))
            use_init_keys = True

        repeated_right = True if postfix == '_right' else False

    if use_init_keys:
        config_h_lines.append(generate_define('USE_INIT_KEYS'))
    else:
        config_h_lines.append(generate_define('AM_INIT_KEY_NUM', 0))


#MARK: Profile config
def generate_profile_config(info_data, config_h_lines):
    """Extract the profile configuration"""
    am_profiles = info_data['analog_matrix']['profiles']
    switch_num = info_data['analog_matrix']['hardware']['total_switch_num']
    trigger_heights = []
    release_heights = []
    press_distances = []
    release_distances = []
    profile_config = []
    key_modes = []
    profile_num = 0
    from_bottom = info_data['analog_matrix']['config'].get('distance_from_bottom', False)
    global use_priority_mode

    while True:
        #TODO: I can probably simplify this with a loop
        if f'profile_{profile_num}' in am_profiles:
            profile_data = am_profiles[f'profile_{profile_num}']

            # base_trigger_height is the default value that then gets masked by the trigger_height values if they're not 0
            # Multiply the heights by 100 to get rid of the floating point
            base_trigger_height = int(profile_data.get('base_trigger_height', 0) * 100)
            trigger_height = profile_data.get('trigger_height', [0])
            trigger_height = [int(val * 100) for val in trigger_height]
            if len(trigger_height) == 1:
                trigger_height = [trigger_height[0] if trigger_height[0] > 0 else base_trigger_height for _ in range(switch_num)]
            trigger_heights.append(trigger_height)

            # If base_release_height is defined, it takes priority over offset, as the two values clash
            # If base_release_height isn't defined, the trigger_height at that index + offset becomes the base value
            base_release_height = int(profile_data.get('base_release_height', 0) * 100)
            offset = -int(profile_data.get('release_offset', 0.2) * 100) if not from_bottom else int(profile_data.get('release_offset', 0.2) * 100)
            release_height = profile_data.get('release_height', [0])
            release_height = [int(val * 100) for val in release_height]
            if len(release_height) == 1:
                release_height = [release_height[0] if release_height[0] > 0 else base_release_height for _ in range(switch_num)]
            if base_release_height == 0:
                release_height = [round(trigger_height[idx] + offset, 2) if release_height[idx] == 0 else release_height[idx] for idx in range(switch_num)]
            release_heights.append(release_height)

            base_press_distance = int(profile_data.get('base_press_distance', 0) * 100)
            rt_press_distance = profile_data.get('rt_press_distance', [0])
            rt_press_distance = [int(val * 100) for val in rt_press_distance]
            if len(rt_press_distance) == 1:
                rt_press_distance = [rt_press_distance[0] for _ in range(switch_num)]
            rt_press_distance = [distance if distance > 0 else base_press_distance for distance in rt_press_distance]

            base_release_distance = int(profile_data.get('base_release_distance', 0) * 100)
            offset = int(profile_data.get('rt_release_offset', 0) * 100)
            rt_release_distance = profile_data.get('rt_release_distance', [0])
            rt_release_distance = [int(val * 100) for val in rt_release_distance]
            if len(rt_release_distance) == 1:
                rt_release_distance = [rt_release_distance[0] if rt_release_distance[0] > 0 else base_release_distance for _ in range(switch_num)]
            if base_release_distance == 0:
                rt_release_distance = [round(rt_press_distance[idx] + offset, 2) if rt_release_distance[idx] == 0 else rt_release_distance[idx] for idx in range(switch_num)]

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
            if use_priority_mode:
                profile_config[profile_num].append(1 if profile_data.get('priority_profile', False) else 0)

            #MARK: Key modes
            if 'rapid_trigger_type' in profile_data:
                #TODO: This is kinda useless
                # If a rapid_trigger_type is defined for the profile, set the mode to it for all RT enabled keys
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
                default_mode = 0
                if 'key_modes' not in profile_data:
                    if 'trigger_height' in profile_data:
                        default_mode = 0
                    elif 'rt_press_distance' in profile_data:
                        default_mode = 3
                key_mode = profile_data.get('key_modes', [default_mode])
                if len(key_mode) == 1:
                    key_mode = [key_mode[0] for _ in range(switch_num)]
                key_modes.append(key_mode)

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
    if 'profile_switch_mode' in am_profiles:
        config_h_lines.append(generate_define('PROFILE_SWITCH_MODE', f'{am_profiles['profile_switch_mode'].upper()}_PROFILE'))
    else:
        config_h_lines.append(generate_define('PROFILE_SWITCH_MODE', 'LAST_PROFILE'))

    default_profile = am_profiles.get('default_profile', 0)
    if default_profile >= profile_num:
        print(f"Default profile {default_profile} is outside of the possible range of profiles, since only {profile_num} are defined. Defaulting to profile 0. Keep in mind that profiles are 0-indexed.")
        default_profile = 0

    #TODO: This stuff can definitely be simplified
    # Split up the height configs if necessary
    if split_keyboard:
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
        for index, postfix in enumerate(['', '_R']):
            if [idx for side in split_trigger_heights for profile in side for idx in profile].count(0) != switch_num * profile_num:
                    config_h_lines.append(generate_define(f'TRIGGER_HEIGHT{postfix}', f'{str(split_trigger_heights[index]).replace('[', '{').replace(']', '}')}'))
                    config_h_lines.append(generate_define(f'RELEASE_HEIGHT{postfix}', f'{str(split_release_heights[index]).replace('[', '{').replace(']', '}')}'))

            if [idx for side in split_press_distances for profile in side for idx in profile].count(0) != switch_num * profile_num:
                    config_h_lines.append(generate_define(f'RT_PRESS_DISTANCE{postfix}', f'{str(split_press_distances[index]).replace('[', '{').replace(']', '}')}'))
                    config_h_lines.append(generate_define(f'RT_RELEASE_DISTANCE{postfix}', f'{str(split_release_distances[index]).replace('[', '{').replace(']', '}')}'))

            if [idx for side in split_key_modes for profile in side for idx in profile].count(-1) != switch_num * profile_num:
                config_h_lines.append(generate_define(f'KEY_MODES{postfix}', f'{str(split_key_modes[index]).replace('[', '{').replace(']', '}')}'))

    # Not a split keyboard
    else:
        if [idx for profile in trigger_heights for idx in profile].count(0) != switch_num * profile_num:
            config_h_lines.append(generate_define('TRIGGER_HEIGHT', f'{str(trigger_heights).replace('[', '{').replace(']', '}')}'))
            config_h_lines.append(generate_define('RELEASE_HEIGHT', f'{str(release_heights).replace('[', '{').replace(']', '}')}'))

        if [idx for profile in press_distances for idx in profile].count(0) != switch_num * profile_num:
            config_h_lines.append(generate_define('RT_PRESS_DISTANCE', f'{str(press_distances).replace('[', '{').replace(']', '}')}'))
            config_h_lines.append(generate_define('RT_RELEASE_DISTANCE', f'{str(release_distances).replace('[', '{').replace(']', '}')}'))

        if [idx for profile in key_modes for idx in profile].count(-1) != switch_num * profile_num:
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


def validate_profile_name(profile):
    if profile[:8] == 'profile_' and profile[8:].isnumeric():
        return True
    else:
        return False

#MARK: Validation
def validate_analog_matrix_config(info_data):
    """Validate the analog matrix configuration."""
    am_json = info_data['analog_matrix']
    am_hardware = am_json['hardware']
    switch_num = info_data['analog_matrix']['hardware']['total_switch_num']
    if 'profiles' in am_json:
        am_profiles = am_json['profiles']

    valid = True

    if 'power_pins' in am_hardware:
        if len(am_hardware.get('mux_to_num', '')) != len(am_hardware.get('power_pins', '')) \
            and not am_hardware.get('custom_power_before_scan', False):
            valid = False
            cli.log.error("If custom_power_before_scan isn't enabled, you need to define as many power_pins as mux_channels are used.")
    if 'power_pins_right' in am_hardware:
        if len(am_hardware.get('mux_to_num_right', '')) != len(am_hardware.get('power_pins_right', '')) \
            and not am_hardware.get('custom_power_before_scan', False):
            valid = False
            cli.log.error("If custom_power_before_scan isn't enabled, you need to define as many power_pins as mux_channels are used.")

    if 'profiles' in am_json:
        profile = 0
        while True:
            if f'profile_{profile}' not in am_profiles:
                break
            profile_data = am_profiles[f'profile_{profile}']

            if 'rapid_trigger_type' in profile_data:
                if profile_data['rapid_trigger_type'] != 'constant_rapid_trigger':
                    if 'trigger_height' not in profile_data:
                        valid = False
                        print(f"trigger_height needs to be set if constant rapid trigger isn't used in profile_{profile}")
                    if 'rt_press_distance' not in profile_data and profile_data['rapid_trigger_type'].upper() != 'NONE':
                        valid = False
                        print(f"rt_press_distance needs to be set if rapid trigger is used in profile_{profile}!")
            else:
                if 'trigger_height' not in profile_data and 'rt_press_distance' not in profile_data:
                    valid = False
                    print(f"Either trigger height or rt press distance needs to be set in profile_{profile}!")

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
def validate_height_config(am_json):
    invert_adc = am_json['hardware'].get('invert_adc', False)
    from_bottom = am_json['config'].get('distance_from_bottom', False)
    am_profiles = am_json['profiles']
    valid = True

    if (invert_adc and from_bottom) or (not invert_adc and not from_bottom):
        for profile in range(len(am_profiles)):
            if f'profile_{profile}' in am_profiles:
                profile_data = am_profiles[f'profile_{profile}']
                if 'trigger_height' in profile_data and 'release_height' in profile_data:
                    for idx, i in enumerate(profile_data['trigger_height']):
                        if i < profile_data['release_height'][idx]:
                            valid = False
                            print(f'\nThe trigger height for key {idx} in profile {profile} is higher than the release height when it needs to be lower or equal.\n')

    else:
        for profile in range(len(am_profiles)):
            if f'profile_{profile}' in am_profiles:
                profile_data = am_profiles[f'profile_{profile}']
                if 'trigger_height' in profile_data and 'release_height' in profile_data:
                    for idx, i in enumerate(profile_data['trigger_height']):
                        if i > profile_data['release_height'][idx]:
                            valid = False
                            print(f'\nThe trigger height for key {idx} in profile {profile} is lower than the release height when it needs to be higher or equal.\n')

    return valid


def get_features(info_data):
    global split_keyboard
    for feature, enabled in info_data['features'].items():
        features.update(((feature.lower(), enabled),))

    for name, _ in info_data['analog_matrix'].items():
        objects.append(name.lower())

    if 'split' in info_data and info_data['split'].get('enabled', False):
        split_keyboard = True


#MARK: General config
def generate_analog_matrix_config(info_data, config_h_lines):
    """Generate the config.h lines for analog matrix keyboards."""
    global use_priority_mode
    am_json = info_data['analog_matrix']
    am_hardware = info_data['analog_matrix']['hardware']
    am_config = info_data['analog_matrix']['config']

    # Get all defined features and their status
    #TODO: Check if the features from rules_mk have been combined at this point
    if 'features' in info_data:
        get_features(info_data)

    validate_analog_matrix_config(info_data)

    #TODO: The define in analog_matrix.h is too late, but find a cleaner way to force 12 bit res
    config_h_lines.append('''\n// Force ADC_RESOLUTION to 12 bits\n#undef ADC_RESOLUTION\n#define ADC_RESOLUTION 12''')

    analog_debounce = am_hardware.get('analog_debounce', 0)
    if analog_debounce > 0:
        config_h_lines.append(f"\n#ifndef ANALOG_DEBOUNCE\n#  define ANALOG_DEBOUNCE {analog_debounce}\n#endif // ANALOG_DEBOUNCE\n#undef DEBOUNCE\n#define DEBOUNCE {analog_debounce}")
    else:
        config_h_lines.append(generate_define('ANALOG_DEBOUNCE', 0))
        config_h_lines.append(generate_define('DEBOUNCE', 0))

    # if split_keyboard:
    #     info_data = check_right_side_pins(info_data, config_h_lines)

    #MARK: Mixed matrix def
    if 'rc_to_matrix' in am_hardware:
        config_h_lines.append(generate_define('USE_MIXED_MATRIX'))
        rc_to_matrix = am_hardware['rc_to_matrix']
        config_h_lines.append(generate_define('RC_TO_MATRIX', str(rc_to_matrix).replace('[', '{').replace(']', '}')))
        if split_keyboard:
            rc_to_matrix_r = am_hardware.get('rc_to_matrix_right', rc_to_matrix)
            config_h_lines.append(generate_define('RC_TO_MATRIX_R', str(rc_to_matrix_r).replace('[', '{').replace(']', '}')))

    #TODO: Add this back when debouncing works
    #Hardware stuff
    # if info_data.get('debounce', 0) > 0:
    #     config_h_lines.append(generate_define('USE_DEBOUNCE'))

    info_data = get_port_def(info_data, config_h_lines)

    info_data = check_pins(info_data, config_h_lines)

    # Get the reverse transform arrays
    info_data = get_matrix_to_mux(info_data, config_h_lines)
    info_data = get_matrix_to_rc(info_data, config_h_lines)

    # Transform init key matrix positions to mux combinations
    transform_init_keys(info_data, config_h_lines)

    get_priority_features(info_data, config_h_lines)

    if am_config.get('adc_scan_delay', 0) + am_config.get('mux_select_delay', 0) + am_config.get('power_select_delay', 0) > 0:
        config_h_lines.append(generate_define('AM_USE_DELAY'))

    split_layer_sync = False
    use_special_mode = False
    if 'joystick' in objects or features.get('joystick', True):
        use_special_mode = True
        split_layer_sync = True
        generate_joystick_config(info_data, config_h_lines)

    #TODO: Implement this
    if 'midi' in objects and features.get('midi', True):
        use_special_mode = True
        split_layer_sync = True
        #generate_midi_config(info_data, config_h_lines)

    if use_special_mode:
        config_h_lines.append(generate_define('USE_SPECIAL_MODE'))

    if 'debug_matrix_position' in am_json['config']:
        debug_matrix_position(info_data, config_h_lines)

    switch_num = am_hardware.get('total_switch_num')
    # Only get the heights from the config if no profiles are defined
    if 'profiles' not in am_json:
        #Config stuff
        if 'trigger_height' in am_json['config']:
            trigger_height = am_json['config']['trigger_height']
            if len(trigger_height) == 1:
                height = trigger_height[0]
                trigger_height = [height for _ in range(switch_num)]
            config_h_lines.append(generate_define('TRIGGER_HEIGHT', f'{{{{ {", ".join(map(str, trigger_height))} }}}}'))

            if 'release_height' in am_json['config']:
                release_height = am_json['config']['release_height']
                if len(release_height) == 1:
                    height = release_height[0]
                    release_height = [height for _ in range(switch_num)]
                config_h_lines.append(generate_define('RELEASE_HEIGHT', f'{{{{ {", ".join(map(str, release_height))} }}}}'))
            #If no release height is defined, set it to the trigger height
            else:
                config_h_lines.append(generate_define('RELEASE_HEIGHT', f'{{{{ {", ".join(map(str, trigger_height))} }}}}'))

    else: # Profiles are configured
        generate_profile_config(info_data, config_h_lines)


    if split_keyboard and split_layer_sync == True:
        config_h_lines.append(generate_define('SPLIT_LAYER_SYNC'))

    # Validate trigger_heights separately after setting, in case of a len 1 define
    validate_height_config(am_json)

    return info_data


#MARK: Pins
def check_pins(info_data, config_h_lines):
    # Check ADC Pins
    am_json = info_data['analog_matrix']
    hardware = am_json['hardware']

    #TODO: This is already set in info.py, no need for it here, except it doesn't work without it, why not?
    #TODO: Instead of defining some pins via the auto system, just put them all into the lower loop to consolidate everything into one
    # Check right side pins
    if split_keyboard:
        for pins in ['adc', 'mux', 'power']:
            if f'{pins}_pins' in hardware and f'{pins}_pins_right' not in hardware:
                pins_r = hardware[f'{pins}_pins']
                info_data['analog_matrix']['hardware'][f'{pins}_pins_right'] = pins_r
                config_h_lines.append(generate_define(f'{pins.upper()}_PINS_R', f'{{ {", ".join(map(str, pins_r))} }}'))

    # Generate pin num defines
    for postfix in ['', '_right'] if split_keyboard else ['']:
        fix = '_R' if postfix == '_right' else ''
        for pin in ['adc', 'mux', 'power', 'direct', 'row', 'col']:
            if f'{pin}_pins{postfix}' in hardware:
                num = len(hardware[f'{pin}_pins{postfix}'])
                config_h_lines.append(generate_define(f'{pin.upper()}_PIN_NUM{fix}', num))
            elif split_keyboard and postfix == '_right':
                config_h_lines.append(generate_define(f'{pin.upper()}_PIN_NUM_R', 0))

            # If direct pins are defined, we ignore row and col pins
            if pin == 'direct' and 'direct_pins' in hardware:
                config_h_lines.append(generate_define(f'ROW_PIN_NUM{fix}', 1))
                config_h_lines.append(generate_define(f'COL_PIN_NUM{fix}', len(hardware['direct_pins'])))
                # Don't check row and col pins
                break
                #TODO: Allow separate amounts of pins for L/R?
                # config_h_lines.append(generate_define(f'COL_PIN_NUM{fix}', len(hardware[f'direct_pins{postfix}'])))

    if 'power_pins' in hardware:
        config_h_lines.append(generate_define('POWER_BEFORE_SCAN'))

    if info_data['processor'] == 'RP2040':
        pre = 2
    else:
        pre = 1

    port_def = am_json['port_def']

    if hardware.get('adc_pins') == hardware.get('adc_pins_right'):
        config_h_lines.append(generate_define('EQUAL_ADC_PINS'))

    for type in ['mux', 'power']:
        if f'{type}_pins' not in hardware and f'{type}_pins_right' not in hardware:
            continue

        if hardware.get(f'{type}_pins') == hardware.get(f'{type}_pins_right'):
            config_h_lines.append(generate_define(f'EQUAL_{type.upper()}_PINS'))

        for postfix in ['', '_right']:
            if f'{type}_pins{postfix}' in am_json['hardware']:
                pins = am_json['hardware'][f'{type}_pins{postfix}']
                port = pins[0][:pre]
                offset = int(pins[0][pre:])
                init_offset = offset

                for pin in pins:
                    if pin[:pre] == port and int(pin[pre:]) == offset:
                        offset += 1
                    else:
                        break

                prefix = type.upper()
                config_h_lines.append(generate_define(f'{prefix}_PINS{postfix.upper()}_CONTINUOUS'))
                config_h_lines.append(generate_define(f'{prefix}_PIN{postfix.upper()}_OFFSET', f'{init_offset}'))
                config_h_lines.append(generate_define(f'CONTINUOUS_{prefix}_PORT{postfix.upper()}', f'{port_def}{port}'))

    return info_data



#TODO: Add the other pin stuff into here
#MARK: Right side pins
# def check_right_side_pins(info_data, config_h_lines):
#     am_hardware = info_data['analog_matrix']['hardware']

#     for pins in ['adc_pins', 'mux_pins', 'power_pins']:
#         if pins in am_hardware and f'{pins}_right' not in am_hardware:
#             pins_r = am_hardware[pins]
#             info_data['analog_matrix']['hardware'][f'{pins}_right'] = pins_r
#             config_h_lines.append(generate_define(f'{pins.upper()}_R', f'{{ {", ".join(map(str, pins_r))} }}'))

    # return info_data


#MARK: Joystick config
def generate_joystick_config(info_data, config_h_lines):
    AXIS_INDICES = { "x": 0, "y": 1, "trigger": 2, "z": 2, "rx": 3, "ry": 4, "rz": 5 }
    RESOLUTION_NAMES = { "difference": 0, "lowest": 1, "positive_dominant": 2, "negative_dominant": 3, "cancel": 4 }
    am_joystick = info_data['analog_matrix'].get('joystick', {})

    config_h_lines.append(generate_define(f'{am_joystick.get('layout', 'XBOX').upper()}_LAYOUT'))
    config_h_lines.append(generate_define('JOYSTICK_AXIS_COUNT', am_joystick.get('axes', 6)))
    config_h_lines.append(generate_define('JOYSTICK_BUTTON_COUNT', am_joystick.get('buttons', 16)))
    config_h_lines.append(generate_define('JS_TOP_DEADZONE', am_joystick.get('top_deadzone', 10)))
    config_h_lines.append(generate_define('JS_BOTTOM_DEADZONE', am_joystick.get('bottom_deadzone', 0)))

    method_config = am_joystick.get('resolution_methods', {})
    # Use the resolution_method option as the default for any axis that isn't specified
    resolutions = [[RESOLUTION_NAMES[am_joystick.get('resolution_method', 'difference').lower()]] for _ in range(am_joystick.get('axes', 6))]
    if method_config != {}:
        for axis, method in method_config.items():
            resolutions[AXIS_INDICES[axis]][0] = RESOLUTION_NAMES[method]

    config_h_lines.append(generate_define('AM_JOYSTICK_AXIS_CONFIG', f'{str(resolutions).replace('[', '{').replace(']', '}')}'))


def get_priority_features(info_data, config_h_lines):
    global use_priority_mode
    if 'priority_keys' in info_data['analog_matrix']['config']:
        use_priority_mode = True
        # Transform priority key matrix positions to key indices
        get_priority_keys(info_data, config_h_lines)

    if info_data['analog_matrix']['config'].get('slave_low_priority', False):
        use_priority_mode = True

    if info_data['analog_matrix']['config'].get('dynamic_calibration', False):
        use_priority_mode = True

    if use_priority_mode:
        config_h_lines.append(generate_define("USE_PRIORITY_MODE"))


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
    if split_keyboard:
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
#             num = matrix_to_num[key[0]][key[1]]
#             #TODO: Either flip mux channel and adc channel here or during assignment
#             mux = num_to_mux[num]
#             mux.append(num)
#             # Result is a list of mux channel, adc channel, matrix index
#             priority_muxes.append(mux)
#         else:
#             num = matrix_to_num_r[key[0] - row_split][key[1]]
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


#MARK: Height layout
# def make_height_macro(info_data, config_h_lines):


