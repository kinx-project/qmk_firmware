MCU_FAMILY = MIMXRT1062
MCU_SERIES = MIMXRT1062
MCU_LDSCRIPT = MIMXRT1062
MCU_STARTUP = MIMXRT1062
BOARD = PJRC_TEENSY_4_1
MCU  = cortex-m4
ARMV = 7

# TODO: this seems to have no effect here. putting it in stapelberg/rules.mk works instead.
# # Console for debug
# CONSOLE_ENABLE = yes

# Debounce eagerly (report change immediately), keep per-key timers. We can use
# this because the kinT does not have to deal with noise.
DEBOUNCE_TYPE = sym_eager_pk
