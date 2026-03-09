# POINTING_DEVICE_DRIVER = pmw3360

KEYBOARD_SHARED_EP = yes
#MOUSE_SHARED_EP = yes

USE_FPU = yes

# EEPROM_DRIVER = wear_leveling
# WEAR_LEVELING_DRIVER = legacy


# EEPROM_DRIVER = wear_leveling
# WEAR_LEVELING_DRIVER = embedded_flash

EEPROM_DRIVER = wear_leveling
WEAR_LEVELING_DRIVER = spi_flash
FLASH_DRIVER = spi
