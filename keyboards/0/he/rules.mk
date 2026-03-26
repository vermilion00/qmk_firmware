POINTING_DEVICE_DRIVER = pmw3360

KEYBOARD_SHARED_EP = yes
#MOUSE_SHARED_EP = yes

USE_FPU = yes

#External flash settings
EEPROM_DRIVER = wear_leveling
WEAR_LEVELING_DRIVER = spi_flash
FLASH_DRIVER = spi

#This doesn't seem to help with the joystick issue(s)
USE_PROCESS_STACKSIZE = 0x2000
