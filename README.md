# atenvc080
------
**atenvc080** is an unofficial macOS command line tool to control **ATEN VC080 HDMI EDID emulator**.

Invoke `atenvc080 -?` for some help.

e.g. to program VC080’s SET 2 with EDID in file edid.bin, use a command such as:
`atenvc080 -d /dev/cu.usbserial-* -s 2 -w edid.bin`

**atenvc080** can’t be used to edit EDID files, you may use the free [**AW EDID Editor**](https://www.analogway.com/fr/produits/software-et-outils/aw-edid-editor/) instead.

While not tested, **atenvc080** is believed to also handle **ATEN VC060 DVI EDID emulator** with no or little modification. Your feedback is welcome.

When looking at the RS232 jack 3.5 mm 4 contacts connector pointing upwards (that is, the cable at the bottom), the contacts are, from top to bottom: RxD, RTS, TxD, GND.

xvi;
