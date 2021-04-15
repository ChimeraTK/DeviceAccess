#!/bin/bash
#
# Renames xdma%d_c2h_%d to xdma/slot%d/c2h%d
# Renames xdma%d_h2c_%d to xdma/slot%d/h2c%d
# Renames xdma%d_user   to xdma/slot%d/user
# etc.
#
# Based on Leon Woestenberg <leon@sidebranch.com>'s script
# Adapted by Patrick Huesmann <patrick.huesmann@desy.de>

card_number_fallback() {
    echo $1 | \
    /bin/sed 's:xdma\([0-9][0-9]*\)_events_\([0-9][0-9]*\):xdma/card\1/events\2:' | \
    /bin/sed 's:xdma\([0-9][0-9]*\)_\([ch]2[ch]\)_\([0-9]*\):xdma/card\1/\2\3:' | \
    /bin/sed 's:xdma\([0-9][0-9]*\)_bypass_\([ch]2[ch]\)_\([0-9]*\):xdma/card\1/bypass_\2\3:' | \
    /bin/sed 's:xdma\([0-9][0-9]*\)_\([a-z]*\):xdma/card\1/\2:'
    exit 0
}

# Get PCI bus address (w/o device function) from device path
PCIADDR=$(echo $2 | sed -n -E 's#/devices/pci.*([0-9]{4}:[0-9]{2}:[0-9]{2})\.[0-9]/xdma/.*#\1#p')
if [ -z "${PCIADDR}" ]; then
    # PCI address not found, fall back to card number
    card_number_fallback
fi

# Get physical slot for PCI bus address
SLOTPATH='/sys/bus/pci/slots'
for SLOTDIR in ${SLOTPATH}/*; do
    if [ "$(cat $SLOTDIR/address)" = "${PCIADDR}" ]; then
        SLOTNR=$(echo $SLOTDIR | sed -n -E 's#'${SLOTPATH}'/([0-9]+)#\1#p')
        break
    fi
done
if [ -z "${SLOTNR}" ]; then
    # Slot number not found, fall back to card number
    card_number_fallback
fi

echo $1 | \
/bin/sed -E 's:xdma[0-9]+_events_([0-9]+):xdma/slot'${SLOTNR}'/events\1:' | \
/bin/sed -E 's:xdma[0-9]+_([ch]2[ch])_([0-9]+):xdma/slot'${SLOTNR}'/\1\2:' | \
/bin/sed -E 's:xdma[0-9]+_bypass_([ch]2[ch])_([0-9]+):xdma/slot'${SLOTNR}'/bypass_\1\2:' | \
/bin/sed -E 's:xdma[0-9]+_([a-z]*):xdma/slot'${SLOTNR}'/\1:'
